#include "langcfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Index = value of sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG).
static const char *const LANG_CODES[LANG_COUNT] = {
    "ja",      "en-US",   "fr-FR", "es-ES", "de", "it", "nl", "pt-PT", "ru",    "ko",
    "zh-Hant", "zh-Hans", "fi",    "sv",    "da", "no", "pl", "pt-BR", "en-GB", "tr",
    "es-419",  "ar",      "fr-CA", "cs",    "hu", "el", "ro", "th",    "vi",    "id",
};

static const char *const REGION_KEYS[REGION_COUNT] = {"europe", "americas", "japan", "asia", "other"};
static const char *const REGION_PREFIXES[REGION_OTHER] = {"EP", "UP", "JP", "HP"};

// {Europe variant, Americas variant}
static const int32_t SIBLINGS[][2] = {
    {3, 20}, // es-ES / es-419
    {7, 17}, // pt-PT / pt-BR
    {2, 22}, // fr-FR / fr-CA
};

int32_t lang_from_code(const char *code) {
    for (int32_t i = 0; i < LANG_COUNT; i++)
        if (strcmp(code, LANG_CODES[i]) == 0) return i;
    return LANG_INVALID;
}

const char *lang_code(int32_t lang) {
    return lang >= 0 && lang < LANG_COUNT ? LANG_CODES[lang] : "?";
}

int region_of(const char *content_id) {
    for (int r = 0; r < REGION_OTHER; r++)
        if (strncmp(content_id, REGION_PREFIXES[r], 2) == 0) return r;
    return REGION_OTHER;
}

void langcfg_init(langcfg *cfg) {
    for (int r = 0; r < REGION_COUNT; r++) cfg->regions[r] = CHOICE_AUTO;
}

static int32_t choice_from_value(const char *value) {
    if (strcmp(value, "auto") == 0) return CHOICE_AUTO;
    if (strcmp(value, "system") == 0) return CHOICE_SYSTEM;
    return lang_from_code(value);
}

static const char *choice_value(int32_t choice) {
    return choice == CHOICE_AUTO ? "auto" : choice == CHOICE_SYSTEM ? "system" : lang_code(choice);
}

static int is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }

static char *trim(char *s) {
    while (is_space(*s)) s++;
    char *end = s + strlen(s);
    while (end > s && is_space(end[-1])) *--end = '\0';
    return s;
}

// Reads "key=value" lines; anything else (comments, section headers) is ignored.
void langcfg_parse(langcfg *cfg, const char *text) {
    while (*text) {
        size_t len = strcspn(text, "\n");
        char buf[256];
        size_t n = len < sizeof buf - 1 ? len : sizeof buf - 1;
        memcpy(buf, text, n);
        buf[n] = '\0';
        text += len + (text[len] == '\n');

        buf[strcspn(buf, ";#")] = '\0';
        char *eq = strchr(buf, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(buf), *value = trim(eq + 1);
        int32_t choice = choice_from_value(value);
        if (choice == LANG_INVALID && strcmp(value, "auto") != 0) continue; // unknown language code
        for (int r = 0; r < REGION_COUNT; r++)
            if (strcmp(key, REGION_KEYS[r]) == 0) cfg->regions[r] = choice;
    }
}

int langcfg_format(const langcfg *cfg, char *out, size_t cap) {
    size_t n = 0;
    for (int r = -1; r < REGION_COUNT; r++) {
        int w = r < 0 ? snprintf(out, cap, "; Game Language settings: auto, system or a language code.\n")
                      : snprintf(out + n, cap - n, "%s=%s\n", REGION_KEYS[r], choice_value(cfg->regions[r]));
        if (w < 0 || (size_t)w >= cap - n) return -1;
        n += (size_t)w;
    }
    return (int)n;
}

int32_t langcfg_resolve(const langcfg *cfg, int region, int32_t sys_lang) {
    int32_t choice = cfg->regions[region];
    if (choice >= 0) return choice;
    if (choice == CHOICE_SYSTEM || (region != REGION_EUROPE && region != REGION_AMERICAS)) return sys_lang;
    for (size_t i = 0; i < sizeof SIBLINGS / sizeof SIBLINGS[0]; i++)
        if (SIBLINGS[i][0] == sys_lang || SIBLINGS[i][1] == sys_lang) return SIBLINGS[i][region == REGION_AMERICAS];
    return sys_lang;
}
