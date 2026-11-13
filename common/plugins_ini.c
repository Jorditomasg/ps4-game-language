#include "plugins_ini.h"

#include <stdio.h>
#include <string.h>

static int line_is(const char *line, size_t len, const char *s) {
    while (len && (*line == ' ' || *line == '\t')) line++, len--;
    while (len && (line[len - 1] == ' ' || line[len - 1] == '\t' || line[len - 1] == '\r')) len--;
    return len == strlen(s) && strncmp(line, s, len) == 0;
}

static int same_ignoring_case(const char *a, const char *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char x = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i], y = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
        if (x != y) return 0;
    }
    return 1;
}

static int is_blank(char c) { return c == ' ' || c == '\t' || c == '\r'; }

int ini_get_bool(const char *ini, const char *key) {
    size_t key_len = strlen(key);
    for (const char *p = ini; *p;) {
        const char *b = p, *e = p + strcspn(p, "\n");
        p = e + (*e == '\n');
        while (b < e && is_blank(*b)) b++;
        while (e > b && is_blank(e[-1])) e--;
        const char *eq = memchr(b, '=', (size_t)(e - b)), *key_end = eq;
        if (!eq) continue;
        while (key_end > b && is_blank(key_end[-1])) key_end--;
        if ((size_t)(key_end - b) != key_len || !same_ignoring_case(b, key, key_len)) continue;
        const char *v = eq + 1;
        while (v < e && is_blank(*v)) v++;
        return (e - v == 1 && *v == '1') || (e - v == 4 && same_ignoring_case(v, "true", 4));
    }
    return -1;
}

int plugins_ini_add(const char *ini, const char *entry, char *out, size_t cap) {
    const char *insert_at = NULL;
    for (const char *p = ini; *p;) {
        size_t len = strcspn(p, "\n");
        if (line_is(p, len, entry)) return 0;
        const char *next = p + len + (p[len] == '\n');
        if (!insert_at && line_is(p, len, "[default]")) insert_at = next;
        p = next;
    }

    size_t head = insert_at ? (size_t)(insert_at - ini) : strlen(ini);
    const char *newline = head && ini[head - 1] != '\n' ? "\n" : "";
    const char *section = insert_at ? "" : head ? "\n[default]\n" : "[default]\n";
    int w = snprintf(out, cap, "%.*s%s%s%s\n%s", (int)head, ini, newline, section, entry, ini + head);
    return w < 0 || (size_t)w >= cap ? -1 : 1;
}
