#pragma once

#include <stddef.h>

// Reads a whole file, NUL-terminated. Caller frees. NULL if missing.
char *read_file(const char *path, size_t *size);
int write_file(const char *path, const void *data, size_t size);
