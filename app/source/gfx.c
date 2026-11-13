#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x) ((void)0)
#include "gfx.h"

#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"

#define FB_SIZE 0x800000 // 1920x1080x4 rounded up to the 2 MiB alignment
#define PIXEL_FORMAT_A8R8G8B8_SRGB 0x80000000
#define TILING_LINEAR 1
#define FLIP_VSYNC 1

static int g_video = -1, g_cur;
static void *g_fb[2];
static uint32_t *g_back;
static OrbisKernelEqueue g_flip_queue;

int font_load(font *f, const char *path, float pixel_height) {
    f->data = (unsigned char *)read_file(path, NULL);
    if (!f->data || !stbtt_InitFont(&f->info, f->data, stbtt_GetFontOffsetForIndex(f->data, 0))) return -1;
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&f->info, &ascent, &descent, &gap);
    f->scale = stbtt_ScaleForPixelHeight(&f->info, pixel_height);
    f->ascent = (int)(ascent * f->scale + 0.5f);
    return 0;
}

int gfx_init(void) {
    g_video = sceVideoOutOpen(0xFF, 0, 0, NULL); // 0xFF: system user
    if (g_video < 0) return -1;
    for (int i = 0; i < 2; i++) {
        off_t phys;
        // Memory type and protection flags as in the OpenOrbis graphics sample.
        if (sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), FB_SIZE, 0x200000, 3, &phys)) return -2;
        if (sceKernelMapDirectMemory(&g_fb[i], FB_SIZE, 0x33, 0, phys, 0x200000)) return -3;
        memset(g_fb[i], 0, FB_SIZE);
    }
    OrbisVideoOutBufferAttribute attr;
    memset(&attr, 0, sizeof attr);
    sceVideoOutSetBufferAttribute(&attr, PIXEL_FORMAT_A8R8G8B8_SRGB, TILING_LINEAR, 0, SCREEN_W, SCREEN_H, SCREEN_W);
    if (sceVideoOutRegisterBuffers(g_video, 0, g_fb, 2, &attr) < 0) return -4;
    sceKernelCreateEqueue(&g_flip_queue, "flip");
    sceVideoOutAddFlipEvent(g_flip_queue, g_video, NULL);
    g_back = malloc((size_t)SCREEN_W * SCREEN_H * 4);
    return g_back ? 0 : -5;
}

void gfx_present(void) {
    memcpy(g_fb[g_cur], g_back, (size_t)SCREEN_W * SCREEN_H * 4);
    sceVideoOutSubmitFlip(g_video, g_cur, FLIP_VSYNC, 0);
    OrbisKernelEvent ev;
    int count;
    OrbisKernelUseconds timeout = 100000;
    sceKernelWaitEqueue(g_flip_queue, &ev, 1, &count, &timeout);
    g_cur ^= 1;
}

void gfx_rect(int x, int y, int w, int h, uint32_t rgb) {
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCREEN_W ? SCREEN_W : x + w, y1 = y + h > SCREEN_H ? SCREEN_H : y + h;
    for (int py = y0; py < y1; py++)
        for (int px = x0; px < x1; px++) g_back[py * SCREEN_W + px] = 0xFF000000 | rgb;
}

static uint32_t mix(uint32_t dst, uint32_t src, uint32_t alpha) {
    uint32_t out = 0xFF000000;
    for (int shift = 0; shift <= 16; shift += 8) {
        uint32_t d = dst >> shift & 0xFF, s = src >> shift & 0xFF;
        out |= (s * alpha + d * (255 - alpha)) / 255 << shift;
    }
    return out;
}

static void blit(const unsigned char *alpha, int w, int h, int x, int y, uint32_t rgb) {
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int i = 0; i < w; i++) {
            int px = x + i;
            unsigned char a = alpha[j * w + i];
            if (a && px >= 0 && px < SCREEN_W) g_back[py * SCREEN_W + px] = mix(g_back[py * SCREEN_W + px], rgb, a);
        }
    }
}

static uint32_t utf8_next(const char **s) {
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t cp;
    int extra;
    if (p[0] < 0x80) cp = p[0], extra = 0;
    else if ((p[0] & 0xE0) == 0xC0) cp = p[0] & 0x1F, extra = 1;
    else if ((p[0] & 0xF0) == 0xE0) cp = p[0] & 0x0F, extra = 2;
    else if ((p[0] & 0xF8) == 0xF0) cp = p[0] & 0x07, extra = 3;
    else return *s += 1, '?';
    for (int i = 1; i <= extra; i++) {
        if ((p[i] & 0xC0) != 0x80) return *s += i, '?';
        cp = cp << 6 | (p[i] & 0x3F);
    }
    *s += extra + 1;
    return cp;
}

static int text_run(const font *f, int x, int y, uint32_t rgb, const char *s, int max_w, int draw) {
    static unsigned char glyph[192 * 192];
    float pen = 0;
    int prev = 0;
    while (*s) {
        int g = stbtt_FindGlyphIndex(&f->info, (int)utf8_next(&s));
        if (!g) continue; // not in the font (e.g. CJK): skip it
        if (prev) pen += f->scale * stbtt_GetGlyphKernAdvance(&f->info, prev, g);
        int advance, lsb;
        stbtt_GetGlyphHMetrics(&f->info, g, &advance, &lsb);
        if (max_w > 0 && pen + advance * f->scale > max_w) break;
        if (draw) {
            int x0, y0, x1, y1;
            float shift = pen - (int)pen;
            stbtt_GetGlyphBitmapBoxSubpixel(&f->info, g, f->scale, f->scale, shift, 0, &x0, &y0, &x1, &y1);
            int w = x1 - x0, h = y1 - y0;
            if (w > 0 && h > 0 && w * h <= (int)sizeof glyph) {
                stbtt_MakeGlyphBitmapSubpixel(&f->info, glyph, w, h, w, f->scale, f->scale, shift, 0, g);
                blit(glyph, w, h, x + (int)pen + x0, y + f->ascent + y0, rgb);
            }
        }
        pen += advance * f->scale;
        prev = g;
    }
    return (int)(pen + 0.5f);
}

int gfx_text(const font *f, int x, int y, uint32_t rgb, const char *utf8, int max_w) {
    return text_run(f, x, y, rgb, utf8, max_w, 1);
}

int gfx_text_width(const font *f, const char *utf8) { return text_run(f, 0, 0, 0, utf8, 0, 0); }
