#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "langcfg.h"
#include "plugins_ini.h"

#define ES 3
#define ES_LA 20
#define PT_BR 17
#define EN_US 1

static langcfg cfg;

static int32_t resolve(const char *ini, const char *content_id, int32_t sys) {
    langcfg_init(&cfg);
    langcfg_parse(&cfg, ini);
    return langcfg_resolve(&cfg, region_of(content_id), sys);
}

static void test_codes(void) {
    assert(lang_from_code("es-419") == ES_LA);
    assert(lang_from_code("klingon") == LANG_INVALID);
    assert(strcmp(lang_code(ES), "es-ES") == 0);
    assert(region_of("EP0001-CUSA1_00-X") == REGION_EUROPE);
    assert(region_of("UP0001-CUSA1_00-X") == REGION_AMERICAS);
    assert(region_of("JP0001-CUSA1_00-X") == REGION_JAPAN);
    assert(region_of("HP0001-CUSA1_00-X") == REGION_ASIA);
    assert(region_of("") == REGION_OTHER);
}

static void test_resolve(void) {
    // No config: automatic swap for Europe/Americas, system language elsewhere.
    assert(resolve("", "UP0001", ES) == ES_LA);
    assert(resolve("", "EP0001", ES_LA) == ES);
    assert(resolve("", "EP0001", ES) == ES);
    assert(resolve("", "UP0001", PT_BR) == PT_BR);
    assert(resolve("", "JP0001", ES) == ES);
    assert(resolve("", "EP0001", EN_US) == EN_US);

    const char *ini = "[default]\r\namericas = system ; off\r\njapan=en-US\r\nother=klingon\r\n";
    assert(resolve(ini, "UP0001", ES) == ES);
    assert(resolve(ini, "JP0001", ES) == EN_US);
    assert(resolve(ini, "EP0001", ES_LA) == ES);
    assert(resolve(ini, "", ES) == ES);
}

static void test_format_round_trip(void) {
    langcfg_init(&cfg);
    cfg.regions[REGION_AMERICAS] = ES;
    cfg.regions[REGION_JAPAN] = EN_US;
    cfg.regions[REGION_ASIA] = CHOICE_SYSTEM;

    char text[512], small[10];
    assert(langcfg_format(&cfg, text, sizeof text) > 0);
    assert(langcfg_format(&cfg, small, sizeof small) == -1);

    langcfg_init(&cfg);
    langcfg_parse(&cfg, text);
    assert(cfg.regions[REGION_EUROPE] == CHOICE_AUTO);
    assert(cfg.regions[REGION_AMERICAS] == ES);
    assert(cfg.regions[REGION_JAPAN] == EN_US);
    assert(cfg.regions[REGION_ASIA] == CHOICE_SYSTEM);
    assert(cfg.regions[REGION_OTHER] == CHOICE_AUTO);
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

int main(void) {
    test_codes();
    test_resolve();
    test_format_round_trip();
    test_plugins_ini();
    puts("common: all tests passed");
    return 0;
}
