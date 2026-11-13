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
static const char *const REGION_PREFIXES[REGION_COUNT] = {"EP", "UP", "JP", "HP", ""};

// {Europe variant, Americas variant}
static const int32_t SIBLINGS[][2] = {
    {3, 20}, // es-ES / es-419
    {7, 17}, // pt-PT / pt-BR
    {2, 22}, // fr-FR / fr-CA
};

int32_t lang_from_code(const char *code) {
    if (code[0] >= '0' && code[0] <= '9') {
        int n = atoi(code);
        return n < LANG_COUNT ? n : LANG_INVALID;
    }
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

int32_t lang_region_sibling(int32_t sys_lang, int region) {
    if (region != REGION_EUROPE && region != REGION_AMERICAS) return sys_lang;
    for (size_t i = 0; i < sizeof SIBLINGS / sizeof SIBLINGS[0]; i++)
        if (SIBLINGS[i][0] == sys_lang || SIBLINGS[i][1] == sys_lang) return SIBLINGS[i][region == REGION_AMERICAS];
    return sys_lang;
}

void langcfg_init(langcfg *cfg) {
    for (int r = 0; r < REGION_COUNT; r++) cfg->regions[r] = CHOICE_DEFAULT;
    cfg->count = 0;
}

static langcfg_title *find(const langcfg *cfg, const char *title_id) {
    for (int i = 0; i < cfg->count; i++)
        if (strcmp(cfg->titles[i].title_id, title_id) == 0) return (langcfg_title *)&cfg->titles[i];
    return NULL;
}

static langcfg_title *find_or_add(langcfg *cfg, const char *title_id) {
    langcfg_title *t = find(cfg, title_id);
    if (t || cfg->count == LANGCFG_MAX_TITLES || strlen(title_id) >= sizeof t->title_id) return t;
    t = &cfg->titles[cfg->count++];
    strcpy(t->title_id, title_id);
    t->choice = CHOICE_DEFAULT;
    return t;
}

static int32_t choice_from_value(const char *value) {
    if (strcmp(value, "system") == 0) return CHOICE_SYSTEM;
    if (strcmp(value, "auto") == 0) return CHOICE_DEFAULT;
    return lang_from_code(value);
}

static const char *choice_value(int32_t choice) {
    return choice == CHOICE_SYSTEM ? "system" : choice == CHOICE_DEFAULT ? "auto" : lang_code(choice);
}

static int is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }

static char *trim(char *s) {
    while (is_space(*s)) s++;
    char *end = s + strlen(s);
    while (end > s && is_space(end[-1])) *--end = '\0';
    return s;
}

void langcfg_parse(langcfg *cfg, const char *text) {
    langcfg_title *section = NULL;
    int in_default = 0;

    while (*text) {
        size_t len = strcspn(text, "\n");
        char buf[256];
        size_t n = len < sizeof buf - 1 ? len : sizeof buf - 1;
        memcpy(buf, text, n);
        buf[n] = '\0';
        text += len + (text[len] == '\n');

        buf[strcspn(buf, ";#")] = '\0';
        char *line = trim(buf);

        if (line[0] == '[') {
            char *close = strchr(line, ']');
            if (close) *close = '\0';
            char *name = trim(line + 1);
            in_default = close && strcmp(name, "default") == 0;
            section = close && !in_default ? find_or_add(cfg, name) : NULL;
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(line), *value = trim(eq + 1);
        int32_t choice = choice_from_value(value);
        if (choice == LANG_INVALID && strcmp(value, "auto") != 0) continue; // unknown language code

        if (section && strcmp(key, "lang") == 0) section->choice = choice;
        for (int r = 0; in_default && r < REGION_COUNT; r++)
            if (strcmp(key, REGION_KEYS[r]) == 0) cfg->regions[r] = choice;
    }
}

int langcfg_format(const langcfg *cfg, char *out, size_t cap) {
    size_t n = 0;
#define EMIT(...)                                                    \
    do {                                                             \
        int w = snprintf(out + n, cap - n, __VA_ARGS__);             \
        if (w < 0 || (size_t)w >= cap - n) return -1;                \
        n += (size_t)w;                                              \
    } while (0)

    if (cap == 0) return -1;
    EMIT("; Game Language settings, written by the Game Language app.\n[default]\n");
    for (int r = 0; r < REGION_COUNT; r++) EMIT("%s=%s\n", REGION_KEYS[r], choice_value(cfg->regions[r]));
    for (int i = 0; i < cfg->count; i++)
        if (cfg->titles[i].choice != CHOICE_DEFAULT)
            EMIT("\n[%s]\nlang=%s\n", cfg->titles[i].title_id, choice_value(cfg->titles[i].choice));
#undef EMIT
    return (int)n;
}

int32_t langcfg_get(const langcfg *cfg, const char *title_id) {
    const langcfg_title *t = find(cfg, title_id);
    return t ? t->choice : CHOICE_DEFAULT;
}

int langcfg_set(langcfg *cfg, const char *title_id, int32_t choice) {
    langcfg_title *t = choice == CHOICE_DEFAULT ? find(cfg, title_id) : find_or_add(cfg, title_id);
    if (t) t->choice = choice;
    return t || choice == CHOICE_DEFAULT ? 0 : -1;
}

int32_t langcfg_resolve_region(const langcfg *cfg, int region, int32_t sys_lang) {
    int32_t choice = cfg->regions[region];
    if (choice >= 0) return choice;
    return choice == CHOICE_SYSTEM ? sys_lang : lang_region_sibling(sys_lang, region);
}

int32_t langcfg_resolve(const langcfg *cfg, const char *title_id, int32_t sys_lang, const char *content_id) {
    int32_t choice = langcfg_get(cfg, title_id);
    if (choice >= 0) return choice;
    if (choice == CHOICE_SYSTEM) return sys_lang;
    return langcfg_resolve_region(cfg, region_of(content_id), sys_lang);
}
