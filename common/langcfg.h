#pragma once

#include <stddef.h>
#include <stdint.h>

#define LANG_COUNT 30
#define LANG_INVALID (-1)

// A choice is one of these, or a language id (0..LANG_COUNT-1).
#define CHOICE_DEFAULT (-1) // region: automatic swap; game: follow its region
#define CHOICE_SYSTEM (-2)  // the console's system language

// Region of a game, from the first two letters of its Content ID.
enum { REGION_EUROPE, REGION_AMERICAS, REGION_JAPAN, REGION_ASIA, REGION_OTHER, REGION_COUNT };

#define LANGCFG_MAX_TITLES 1024

typedef struct {
    char title_id[16];
    int32_t choice;
} langcfg_title;

typedef struct {
    int32_t regions[REGION_COUNT];
    int count;
    langcfg_title titles[LANGCFG_MAX_TITLES];
} langcfg;

int32_t lang_from_code(const char *code);
const char *lang_code(int32_t lang);
int region_of(const char *content_id);
int32_t lang_region_sibling(int32_t sys_lang, int region);

void langcfg_init(langcfg *cfg);
void langcfg_parse(langcfg *cfg, const char *text);
int langcfg_format(const langcfg *cfg, char *out, size_t cap);
int32_t langcfg_get(const langcfg *cfg, const char *title_id);
int langcfg_set(langcfg *cfg, const char *title_id, int32_t choice);
// Language for the region alone (ignores per-game choices).
int32_t langcfg_resolve_region(const langcfg *cfg, int region, int32_t sys_lang);
int32_t langcfg_resolve(const langcfg *cfg, const char *title_id, int32_t sys_lang, const char *content_id);
