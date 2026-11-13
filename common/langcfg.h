#pragma once

#include <stddef.h>
#include <stdint.h>

#define LANG_COUNT 30
#define LANG_INVALID (-1)

// A region's choice is one of these, or a language id (0..LANG_COUNT-1).
#define CHOICE_AUTO (-1)   // swap es-ES/es-419, pt-PT/pt-BR, fr-FR/fr-CA to the region's variant
#define CHOICE_SYSTEM (-2) // the console's system language

// Region of a game, from the first two letters of its Content ID.
enum { REGION_EUROPE, REGION_AMERICAS, REGION_JAPAN, REGION_ASIA, REGION_OTHER, REGION_COUNT };

typedef struct {
    int32_t regions[REGION_COUNT];
} langcfg;

int32_t lang_from_code(const char *code);
const char *lang_code(int32_t lang);
int region_of(const char *content_id);

void langcfg_init(langcfg *cfg);
void langcfg_parse(langcfg *cfg, const char *text);
int langcfg_format(const langcfg *cfg, char *out, size_t cap);
int32_t langcfg_resolve(const langcfg *cfg, int region, int32_t sys_lang);
