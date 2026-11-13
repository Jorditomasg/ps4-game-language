#pragma once

#include <stdint.h>

#include "stb_truetype.h"

#define SCREEN_W 1920
#define SCREEN_H 1080

typedef struct {
    stbtt_fontinfo info;
    unsigned char *data;
    float scale;
    int ascent;
} font;

int font_load(font *f, const char *path, float pixel_height);

int gfx_init(void);
void gfx_present(void);
void gfx_rect(int x, int y, int w, int h, uint32_t rgb);
void gfx_circle(int cx, int cy, int r, uint32_t rgb);
// Draws UTF-8 text with its top-left corner at (x, y), cut at max_w pixels (0 = no limit). Returns the width.
int gfx_text(const font *f, int x, int y, uint32_t rgb, const char *utf8, int max_w);
int gfx_text_width(const font *f, const char *utf8);
