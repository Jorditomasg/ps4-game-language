#pragma once

#include <stdint.h>

typedef struct {
    char id[16];
    char title[128];
    char content_id[48];
} game;

// Lists installed games sorted by title, using the localized title for sys_lang when there is one.
// Returns the count, or -1 if GoldHEN refused to lift the app sandbox.
int games_scan(game *out, int max, int32_t sys_lang, const char *skip_id);
