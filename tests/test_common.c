#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "langcfg.h"
#include "plugins_ini.h"
#include "sfo.h"

#define ES 3
#define ES_LA 20
#define PT_BR 17
#define EN_US 1
#define US_GAME "UP0001-CUSA00001_00-X"
#define EU_GAME "EP0001-CUSA00001_00-X"
#define JP_GAME "JP0001-CUSA00001_00-X"

static langcfg cfg;

static int32_t resolve(const char *ini, const char *title, int32_t sys, const char *content_id) {
    langcfg_init(&cfg);
    langcfg_parse(&cfg, ini);
    return langcfg_resolve(&cfg, title, sys, content_id);
}

static void test_codes(void) {
    assert(lang_from_code("es-419") == ES_LA);
    assert(lang_from_code("17") == PT_BR);
    assert(lang_from_code("99") == LANG_INVALID);
    assert(lang_from_code("klingon") == LANG_INVALID);
    assert(strcmp(lang_code(ES), "es-ES") == 0);
}

static void test_resolve(void) {
    // No config: automatic swap for Europe/Americas, system language elsewhere.
    assert(resolve("", "CUSA1", ES, US_GAME) == ES_LA);
    assert(resolve("", "CUSA1", ES_LA, EU_GAME) == ES);
    assert(resolve("", "CUSA1", ES, EU_GAME) == ES);
    assert(resolve("", "CUSA1", ES, JP_GAME) == ES);
    assert(resolve("", "CUSA1", ES, "") == ES);
    assert(resolve("", "CUSA1", EN_US, EU_GAME) == EN_US);

    const char *ini = "[default]\r\namericas = system ; off\r\njapan=en-US\r\nother=klingon\r\n";
    assert(resolve(ini, "CUSA1", ES, US_GAME) == ES);
    assert(resolve(ini, "CUSA1", ES, JP_GAME) == EN_US);
    assert(resolve(ini, "CUSA1", ES_LA, EU_GAME) == ES);
    assert(resolve(ini, "CUSA1", ES, "") == ES);

    // A game's own choice beats its region, regardless of section order.
    ini = "[CUSA1]\nlang=pt-BR\n[CUSA2]\nlang=system\n[default]\neurope=es-419\n";
    assert(resolve(ini, "CUSA1", ES, EU_GAME) == PT_BR);
    assert(resolve(ini, "CUSA2", ES, EU_GAME) == ES);
    assert(resolve(ini, "CUSA3", ES, EU_GAME) == ES_LA);
}

static void test_format_round_trip(void) {
    langcfg_init(&cfg);
    cfg.regions[REGION_JAPAN] = EN_US;
    cfg.regions[REGION_ASIA] = CHOICE_SYSTEM;
    assert(langcfg_set(&cfg, "CUSA1", ES_LA) == 0);
    assert(langcfg_set(&cfg, "CUSA2", CHOICE_SYSTEM) == 0);
    assert(langcfg_set(&cfg, "CUSA4", ES) == 0);
    assert(langcfg_set(&cfg, "CUSA4", CHOICE_DEFAULT) == 0);
    assert(langcfg_set(&cfg, "CUSA5", CHOICE_DEFAULT) == 0);

    char text[1024], small[10];
    assert(langcfg_format(&cfg, text, sizeof text) > 0);
    assert(strstr(text, "CUSA4") == NULL);
    assert(langcfg_format(&cfg, small, sizeof small) == -1);

    langcfg_init(&cfg);
    langcfg_parse(&cfg, text);
    assert(cfg.regions[REGION_EUROPE] == CHOICE_DEFAULT);
    assert(cfg.regions[REGION_JAPAN] == EN_US);
    assert(cfg.regions[REGION_ASIA] == CHOICE_SYSTEM);
    assert(langcfg_get(&cfg, "CUSA1") == ES_LA);
    assert(langcfg_get(&cfg, "CUSA2") == CHOICE_SYSTEM);
    assert(langcfg_get(&cfg, "CUSA5") == CHOICE_DEFAULT);
}

static void test_plugins_ini(void) {
    const char *e = "/data/GoldHEN/plugins/game_language.prx";
    char out[512], want[512];

    assert(plugins_ini_add("", e, out, sizeof out) == 1);
    snprintf(want, sizeof want, "[default]\n%s\n", e);
    assert(strcmp(out, want) == 0);

    assert(plugins_ini_add("; c\r\n[default]\r\n/a.prx\r\n[CUSA1]\r\n/b.prx\r\n", e, out, sizeof out) == 1);
    snprintf(want, sizeof want, "; c\r\n[default]\r\n%s\n/a.prx\r\n[CUSA1]\r\n/b.prx\r\n", e);
    assert(strcmp(out, want) == 0);

    assert(plugins_ini_add("[CUSA1]\n/b.prx", e, out, sizeof out) == 1);
    snprintf(want, sizeof want, "[CUSA1]\n/b.prx\n\n[default]\n%s\n", e);
    assert(strcmp(out, want) == 0);

    assert(plugins_ini_add("[default]", e, out, sizeof out) == 1);
    snprintf(want, sizeof want, "[default]\n%s\n", e);
    assert(strcmp(out, want) == 0);

    snprintf(want, sizeof want, "[default]\n  %s \r\n", e);
    assert(plugins_ini_add(want, e, out, sizeof out) == 0);
    assert(plugins_ini_add("", e, out, 8) == -1);
}

static void test_sfo(void) {
    // Header (20 bytes) + 2 entries (32) + keys + data.
    unsigned char sfo[128] = {0};
    const char keys[] = "TITLE\0CONTENT_ID";
    const unsigned keys_at = 52, data_at = 72;
    memcpy(sfo, "\0PSF\x01\x01\0\0", 8);
    sfo[8] = keys_at, sfo[12] = data_at, sfo[16] = 2;
    sfo[20 + 0] = 0, sfo[20 + 4] = 6, sfo[20 + 12] = 0;   // TITLE -> "Hola!"
    sfo[36 + 0] = 6, sfo[36 + 4] = 10, sfo[36 + 12] = 8;  // CONTENT_ID -> "UP0001-XX"
    memcpy(sfo + keys_at, keys, sizeof keys);
    memcpy(sfo + data_at, "Hola!\0\0\0UP0001-XX", 18);

    char out[32];
    assert(sfo_get_string(sfo, sizeof sfo, "TITLE", out, sizeof out) && strcmp(out, "Hola!") == 0);
    assert(sfo_get_string(sfo, sizeof sfo, "CONTENT_ID", out, sizeof out) && strcmp(out, "UP0001-XX") == 0);
    assert(sfo_get_string(sfo, sizeof sfo, "CONTENT_ID", out, 3) && strcmp(out, "UP") == 0);
    assert(!sfo_get_string(sfo, sizeof sfo, "TITLE_03", out, sizeof out));
    assert(!sfo_get_string(sfo, 30, "TITLE", out, sizeof out));
}

int main(void) {
    test_codes();
    test_resolve();
    test_format_round_trip();
    test_plugins_ini();
    test_sfo();
    puts("common: all tests passed");
    return 0;
}
