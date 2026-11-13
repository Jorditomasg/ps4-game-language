#include <stdbool.h> // orbis/SystemService.h uses bool without including it
#include <orbis/Pad.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "gfx.h"
#include "langcfg.h"
#include "plugins_ini.h"

#define FONT_REGULAR "/app0/assets/fonts/NotoSans-Regular.ttf"
#define FONT_SEMIBOLD "/app0/assets/fonts/NotoSans-SemiBold.ttf"
#define PRX_SRC "/app0/assets/game_language.prx"
#define PRX_DST "/data/GoldHEN/plugins/game_language.prx"
#define PLUGINS_INI "/data/GoldHEN/plugins.ini"
#define CONFIG "/data/GoldHEN/game_language.ini"

#define ROW_TOP 230
#define ROW_H 90

#define COLOR_BG 0x0E1626
#define COLOR_ROW 0x1E3A6B
#define COLOR_ACCENT 0xF2C14E
#define COLOR_SYSTEM 0x6FE3B4
#define COLOR_TEXT 0xFFFFFF
#define COLOR_DIM 0x8FA3C7

enum {
    T_SYSTEM_PREFIX, T_AUTO, T_SYSTEM_LANG, T_SYSTEM_TAG, T_HELP, T_PLUGIN_OK, T_PLUGIN_INSTALLED, T_PLUGIN_FAIL,
    T_PACKAGE_BROKEN, T_SAVED, T_SAVE_FAIL,
    T_REGION, // + region
    T_COUNT = T_REGION + REGION_COUNT
};

static const char *const TEXT[2][T_COUNT] = {
    {"System language: ", "Automatic", "System language", "(system)",
     "D-pad: choose region and language     Square: automatic     Circle: exit", "Plugin ready.",
     "Plugin installed. Restart any game that is open.", "Could not write to /data/GoldHEN. Is GoldHEN running?",
     "The package is incomplete: reinstall it.", "Saved. It applies the next time a game starts.",
     "Could not save " CONFIG ".",
     "European games", "American games", "Japanese games", "Asian games", "Games from other regions"},
    {"Idioma del sistema: ", "Automático", "Idioma del sistema", "(sistema)",
     "Cruceta: elegir región e idioma     Cuadrado: automático     Círculo: salir", "Plugin listo.",
     "Plugin instalado. Reinicia los juegos que tengas abiertos.",
     "No se pudo escribir en /data/GoldHEN. ¿Está GoldHEN activo?", "El paquete está incompleto: reinstálalo.",
     "Guardado. Se aplica la próxima vez que abras un juego.", "No se pudo guardar " CONFIG ".",
     "Juegos de Europa", "Juegos de América", "Juegos de Japón", "Juegos de Asia", "Juegos de otras regiones"},
};

// Index = PS4 language id. The font has no CJK, Arabic or Thai glyphs, so those use English names.
static const char *const LANG_NAMES[LANG_COUNT] = {
    "Japanese",         "English (US)",       "Français (France)",   "Español (España)",
    "Deutsch",          "Italiano",           "Nederlands",          "Português (Portugal)",
    "Русский",          "Korean",             "Chinese (Traditional)", "Chinese (Simplified)",
    "Suomi",            "Svenska",            "Dansk",               "Norsk",
    "Polski",           "Português (Brasil)", "English (UK)",        "Türkçe",
    "Español (Latinoamérica)", "Arabic",      "Français (Canada)",   "Čeština",
    "Magyar",           "Ελληνικά",           "Română",              "Thai",
    "Tiếng Việt",       "Bahasa Indonesia",
};

// Selector order: Automatic, System language, then languages alphabetically so variants sit together.
static const int32_t OPTIONS[] = {CHOICE_AUTO, CHOICE_SYSTEM, 29, 23, 14, 4, 18, 1, 3, 20, 22, 2, 5, 24, 6, 15,
                                  16, 17, 7, 26, 12, 13, 28, 19, 25, 8, 21, 11, 10, 0, 9, 27};
#define OPTION_COUNT ((int)(sizeof OPTIONS / sizeof OPTIONS[0]))

static langcfg g_cfg;
static int g_cursor, g_ui, g_status;
static int32_t g_sys_lang;
static font g_title_font, g_text_font, g_small_font;

static const char *t(int id) { return TEXT[g_ui][id]; }

static int install_plugin(void) {
    size_t prx_size, old_size = 0, ini_size = 0;
    char *prx = read_file(PRX_SRC, &prx_size);
    if (!prx) return T_PACKAGE_BROKEN;
    char *old = read_file(PRX_DST, &old_size);
    int same = old && old_size == prx_size && memcmp(old, prx, prx_size) == 0;
    free(old);

    sceKernelMkdir("/data/GoldHEN", 0777);
    sceKernelMkdir("/data/GoldHEN/plugins", 0777);
    int failed = !same && write_file(PRX_DST, prx, prx_size) != 0;
    free(prx);
    if (failed) return T_PLUGIN_FAIL;

    char *ini = read_file(PLUGINS_INI, &ini_size);
    size_t cap = ini_size + sizeof PRX_DST + 32;
    char *updated = malloc(cap);
    int added = updated ? plugins_ini_add(ini ? ini : "", PRX_DST, updated, cap) : -1;
    if (added == 1) {
        // Keep the user's previous plugins.ini next to it before replacing it.
        if (ini && write_file(PLUGINS_INI ".bak", ini, ini_size) != 0) added = -1;
        else if (write_file(PLUGINS_INI, updated, strlen(updated)) != 0) added = -1;
    }
    free(ini);
    free(updated);
    if (added < 0) return T_PLUGIN_FAIL;
    return same && added == 0 ? T_PLUGIN_OK : T_PLUGIN_INSTALLED;
}

static int save_config(void) {
    char text[512];
    int n = langcfg_format(&g_cfg, text, sizeof text);
    return n >= 0 && write_file(CONFIG, text, (size_t)n) == 0 ? T_SAVED : T_SAVE_FAIL;
}

static void draw_row(int region, int y) {
    int selected = region == g_cursor;
    if (selected) {
        gfx_rect(100, y, 1720, ROW_H - 10, COLOR_ROW);
        gfx_rect(100, y, 6, ROW_H - 10, COLOR_ACCENT);
    }
    gfx_text(&g_text_font, 130, y + 18, COLOR_TEXT, t(T_REGION + region), 0);

    int32_t choice = g_cfg.regions[region], lang = langcfg_resolve(&g_cfg, region, g_sys_lang);
    int is_system = lang == g_sys_lang;
    char label[160];
    if (choice == CHOICE_SYSTEM) snprintf(label, sizeof label, "%s", t(T_SYSTEM_LANG));
    else if (choice == CHOICE_AUTO) snprintf(label, sizeof label, "%s  ·  %s", t(T_AUTO), LANG_NAMES[lang]);
    else snprintf(label, sizeof label, "%s", LANG_NAMES[lang]);
    if (is_system && choice != CHOICE_SYSTEM) {
        size_t used = strlen(label);
        snprintf(label + used, sizeof label - used, " %s", t(T_SYSTEM_TAG));
    }

    char shown[200];
    snprintf(shown, sizeof shown, selected ? "‹  %s  ›" : "%s", label);
    uint32_t color = is_system ? COLOR_SYSTEM : selected ? COLOR_ACCENT : COLOR_DIM;
    gfx_text(&g_text_font, 1790 - gfx_text_width(&g_text_font, shown), y + 18, color, shown, 0);
}

static void draw(void) {
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    int title_w = gfx_text(&g_title_font, 110, 48, COLOR_TEXT, "Game Language", 0);
    gfx_text(&g_small_font, 110 + title_w + 24, 82, COLOR_DIM, "v" APP_VERSION, 0);
    int x = 114 + gfx_text(&g_small_font, 114, 140, COLOR_DIM, t(T_SYSTEM_PREFIX), 0);
    gfx_text(&g_small_font, x, 140, COLOR_SYSTEM, LANG_NAMES[g_sys_lang], 0);

    for (int r = 0; r < REGION_COUNT; r++) draw_row(r, ROW_TOP + r * ROW_H);

    gfx_rect(100, 948, 1720, 2, COLOR_ROW);
    gfx_text(&g_small_font, 110, 966, COLOR_ACCENT, t(g_status), 0);
    gfx_text(&g_small_font, 110, 1010, COLOR_DIM, t(T_HELP), 0);
    gfx_present();
}

static void change_option(int delta, int reset) {
    int option = 0;
    while (!reset && option < OPTION_COUNT - 1 && OPTIONS[option] != g_cfg.regions[g_cursor]) option++;
    option = reset ? 0 : ((option + delta) % OPTION_COUNT + OPTION_COUNT) % OPTION_COUNT;
    g_cfg.regions[g_cursor] = OPTIONS[option];
    g_status = save_config();
}

static void quit(void) {
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
    sceSystemServiceLoadExec("exit", NULL);
}

int main(void) {
    if (sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG, &g_sys_lang) != 0 || g_sys_lang < 0 ||
        g_sys_lang >= LANG_COUNT)
        g_sys_lang = 1;
    g_ui = g_sys_lang == 3 || g_sys_lang == 20; // Spanish UI for es-ES and es-419, English otherwise

    if (font_load(&g_title_font, FONT_SEMIBOLD, 64) || font_load(&g_text_font, FONT_REGULAR, 34) ||
        font_load(&g_small_font, FONT_REGULAR, 26))
        return 1;
    g_status = install_plugin();

    langcfg_init(&g_cfg);
    char *text = read_file(CONFIG, NULL);
    if (text) langcfg_parse(&g_cfg, text);
    free(text);

    if (gfx_init() != 0) return 1;
    int user = 0;
    sceUserServiceInitialize(NULL);
    sceUserServiceGetInitialUser(&user);
    scePadInit();
    int pad = scePadOpen(user, 0, 0, NULL);

    uint32_t previous = 0;
    uint64_t repeat_at = 0;
    draw();
    for (;;) {
        OrbisPadData data;
        memset(&data, 0, sizeof data);
        uint32_t buttons = pad >= 0 && scePadReadState(pad, &data) == 0 && data.connected ? data.buttons : 0;
        uint32_t pressed = buttons & ~previous;
        uint32_t dirs = buttons & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN | ORBIS_PAD_BUTTON_LEFT |
                                   ORBIS_PAD_BUTTON_RIGHT);
        uint64_t now = sceKernelGetProcessTime();
        if (pressed & dirs) repeat_at = now + 400000;
        else if (dirs && now > repeat_at) pressed |= dirs, repeat_at = now + 110000;
        previous = buttons;

        if (!pressed) {
            sceKernelUsleep(16000);
            continue;
        }
        if (pressed & ORBIS_PAD_BUTTON_CIRCLE) quit();
        if ((pressed & ORBIS_PAD_BUTTON_UP) && g_cursor > 0) g_cursor--;
        if ((pressed & ORBIS_PAD_BUTTON_DOWN) && g_cursor < REGION_COUNT - 1) g_cursor++;
        if (pressed & ORBIS_PAD_BUTTON_LEFT) change_option(-1, 0);
        if (pressed & (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_CROSS)) change_option(1, 0);
        if (pressed & ORBIS_PAD_BUTTON_SQUARE) change_option(0, 1);
        draw();
    }
}
