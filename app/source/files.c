#include "files.h"

#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    char *data = NULL;
    long len;
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0 &&
        (data = malloc((size_t)len + 1)) && fread(data, 1, (size_t)len, f) == (size_t)len) {
        data[len] = '\0';
        if (size) *size = (size_t)len;
    } else {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

int write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    int ok = fwrite(data, 1, size, f) == size;
    return fclose(f) == 0 && ok ? 0 : -1;
}
