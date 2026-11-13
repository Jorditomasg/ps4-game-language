#include "plugins_ini.h"

#include <stdio.h>
#include <string.h>

static int line_is(const char *line, size_t len, const char *s) {
    while (len && (*line == ' ' || *line == '\t')) line++, len--;
    while (len && (line[len - 1] == ' ' || line[len - 1] == '\t' || line[len - 1] == '\r')) len--;
    return len == strlen(s) && strncmp(line, s, len) == 0;
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
