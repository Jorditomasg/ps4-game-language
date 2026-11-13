#pragma once

#include <stddef.h>

// Writes `ini` with `entry` added under [default] into `out`.
// Returns 1 if added, 0 if `entry` was already listed (out untouched), -1 if `out` is too small.
int plugins_ini_add(const char *ini, const char *entry, char *out, size_t cap);

// Value of a "key=1/true" flag (key matched case-insensitively): 1 if on, 0 if off, -1 if the key is missing.
int ini_get_bool(const char *ini, const char *key);
