#include "games.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "files.h"
#include "sfo.h"

// GoldHEN SDK commands (syscall 500), as in GoldHEN_Plugins_SDK (MIT).
#define GOLDHEN_SYSCALL 500
#define GOLDHEN_CMD_JAILBREAK 2
#define GOLDHEN_CMD_UNJAILBREAK 3

struct jailbreak_backup {
    uint32_t cr_uid, cr_ruid, cr_rgid, cr_groups;
    uint64_t cr_paid;
    uint64_t cr_caps[2];
    void *cr_prison;
    void *fd_cdir, *fd_jdir, *fd_rdir;
};

int goldhen_syscall(int num, ...);
__asm__(".att_syntax prefix\n"
        ".globl goldhen_syscall\n"
        "goldhen_syscall:\n"
        "  movq $0, %rax\n"
        "  movq %rcx, %r10\n"
        "  syscall\n"
        "  jb goldhen_syscall_err\n"
        "  retq\n"
        "goldhen_syscall_err:\n"
        "  pushq %rax\n"
        "  callq __error\n"
        "  popq %rcx\n"
        "  movl %ecx, 0(%rax)\n"
        "  movq $0xFFFFFFFFFFFFFFFF, %rax\n"
        "  movq $0xFFFFFFFFFFFFFFFF, %rdx\n"
        "  retq\n");

// Internal drive first, then extended storage.
static const char *const ROOTS[] = {"", "/mnt/ext0", "/mnt/ext1"};

static int is_title_id(const char *s) {
    if (strlen(s) != 9) return 0;
    for (int i = 0; i < 9; i++)
        if (i < 4 ? (s[i] < 'A' || s[i] > 'Z') : (s[i] < '0' || s[i] > '9')) return 0;
    return 1;
}

static char *load_sfo(const char *root, const char *id, size_t *size) {
    static const char *const META[] = {
        "/user/appmeta/%s/param.sfo",
        "/user/appmeta/external/%s/param.sfo",
        "/system_data/priv/appmeta/%s/param.sfo",
        "/system_data/priv/appmeta/external/%s/param.sfo",
    };
    char path[160];
    for (size_t i = 0; i < sizeof META / sizeof META[0]; i++) {
        snprintf(path, sizeof path, META[i], id);
        char *sfo = read_file(path, size);
        if (sfo) return sfo;
    }
    snprintf(path, sizeof path, "%s/user/app/%s/sce_sys/param.sfo", root, id);
    return read_file(path, size);
}

static int add_game(game *g, const char *root, const char *id, int32_t sys_lang) {
    size_t size;
    char *sfo = load_sfo(root, id, &size);
    if (!sfo) return 0;
    const unsigned char *b = (const unsigned char *)sfo;
    char category[8], key[16];
    snprintf(key, sizeof key, "TITLE_%02d", (int)sys_lang);
    int ok = sfo_get_string(b, size, "CATEGORY", category, sizeof category) && strncmp(category, "gd", 2) == 0;
    if (ok) {
        snprintf(g->id, sizeof g->id, "%s", id);
        if (!sfo_get_string(b, size, key, g->title, sizeof g->title) &&
            !sfo_get_string(b, size, "TITLE", g->title, sizeof g->title))
            snprintf(g->title, sizeof g->title, "%s", id);
        if (!sfo_get_string(b, size, "CONTENT_ID", g->content_id, sizeof g->content_id)) g->content_id[0] = '\0';
    }
    free(sfo);
    return ok;
}

static int by_title(const void *a, const void *b) {
    return strcasecmp(((const game *)a)->title, ((const game *)b)->title);
}

int games_scan(game *out, int max, int32_t sys_lang, const char *skip_id) {
    struct jailbreak_backup jb;
    memset(&jb, 0, sizeof jb);
    if (goldhen_syscall(GOLDHEN_SYSCALL, GOLDHEN_CMD_JAILBREAK, &jb) != 0) return -1;

    int count = 0;
    for (size_t r = 0; r < sizeof ROOTS / sizeof ROOTS[0]; r++) {
        char dir_path[64];
        snprintf(dir_path, sizeof dir_path, "%s/user/app", ROOTS[r]);
        DIR *dir = opendir(dir_path);
        if (!dir) continue;
        struct dirent *e;
        while ((e = readdir(dir)) && count < max) {
            if (!is_title_id(e->d_name) || strcmp(e->d_name, skip_id) == 0) continue;
            int seen = 0;
            for (int i = 0; i < count && !seen; i++) seen = strcmp(out[i].id, e->d_name) == 0;
            if (!seen) count += add_game(&out[count], ROOTS[r], e->d_name, sys_lang);
        }
        closedir(dir);
    }

    goldhen_syscall(GOLDHEN_SYSCALL, GOLDHEN_CMD_UNJAILBREAK, &jb);
    qsort(out, (size_t)count, sizeof *out, by_title);
    return count;
}
