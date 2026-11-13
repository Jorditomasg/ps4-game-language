// Game Language plugin: reports a per-game system language (e.g. es-419 instead of es-ES).

#include <Common.h>
#include <stdbool.h> // orbis/SystemService.h uses bool without including it
#include <orbis/SystemService.h>

#include "langcfg.h"

#define attr_public __attribute__((visibility("default")))
#define attr_module_hidden __attribute__((weak)) __attribute__((visibility("hidden")))

#define CONFIG_PATH "/data/GoldHEN/game_language.ini"

attr_public const char *g_pluginName = "game_language";
attr_public const char *g_pluginDesc = "Reports a per-game system language.";
attr_public const char *g_pluginAuth = "Jorditomasg";
attr_public uint32_t g_pluginVersion = 0x00000100; // 1.00

static int32_t g_lang;
static int g_hooked;
static langcfg g_cfg;
static char g_text[4096];

HOOK_INIT(sceSystemServiceParamGetInt);

static int32_t sceSystemServiceParamGetInt_hook(int32_t id, int32_t *value) {
    int32_t ret = HOOK_CONTINUE(sceSystemServiceParamGetInt, int32_t (*)(int32_t, int32_t *), id, value);
    if (ret == 0 && value && id == ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG) *value = g_lang;
    return ret;
}

static void notify(const char *text) {
    OrbisNotificationRequest req;
    memset(&req, 0, sizeof req);
    req.type = NotificationRequest;
    req.useIconImageUri = 1;
    req.targetId = -1;
    strncpy(req.message, text, sizeof req.message - 1);
    strncpy(req.iconUri, "cxml://psnotification/tex_icon_system", sizeof req.iconUri - 1);
    sceKernelSendNotificationRequest(0, &req, sizeof req, 0);
}

int32_t attr_public plugin_load(int32_t argc, const char *argv[]) {
    struct proc_info info;
    int32_t sys_lang;
    if (sys_sdk_proc_info(&info) != 0 ||
        sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG, &sys_lang) != 0) {
        klog("[game_language] could not read process info or system language\n");
        return -1;
    }

    langcfg_init(&g_cfg);
    FILE *f = fopen(CONFIG_PATH, "r");
    if (f) {
        size_t n = fread(g_text, 1, sizeof g_text - 1, f);
        g_text[n] = '\0';
        fclose(f);
        langcfg_parse(&g_cfg, g_text);
    }

    g_lang = langcfg_resolve(&g_cfg, region_of(info.contentid), sys_lang);
    klog("[game_language] %s (%s): %s -> %s\n", info.titleid, info.contentid, lang_code(sys_lang),
         lang_code(g_lang));
    if (g_lang == sys_lang) return 0;

    HOOK32(sceSystemServiceParamGetInt);
    g_hooked = 1;

    char msg[128];
    snprintf(msg, sizeof msg, "Game Language %s: %s -> %s", info.titleid, lang_code(sys_lang), lang_code(g_lang));
    notify(msg);
    return 0;
}

int32_t attr_public plugin_unload(int32_t argc, const char *argv[]) {
    if (g_hooked) UNHOOK(sceSystemServiceParamGetInt);
    return 0;
}

int32_t attr_module_hidden module_start(int64_t argc, const void *args) { return 0; }

int32_t attr_module_hidden module_stop(int64_t argc, const void *args) { return 0; }
