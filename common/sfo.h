#pragma once

#include <stddef.h>

// Copies the string value of `key` from a param.sfo image into `out`. Returns 1 if found and non-empty.
int sfo_get_string(const unsigned char *sfo, size_t size, const char *key, char *out, size_t cap);
