#include "sfo.h"

#include <stdint.h>
#include <string.h>

static uint32_t le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t le16(const unsigned char *p) { return (uint16_t)(p[0] | p[1] << 8); }

int sfo_get_string(const unsigned char *sfo, size_t size, const char *key, char *out, size_t cap) {
    if (cap == 0 || size < 20 || memcmp(sfo, "\0PSF", 4) != 0) return 0;
    uint32_t keys = le32(sfo + 8), data = le32(sfo + 12), count = le32(sfo + 16);
    size_t key_len = strlen(key);

    for (uint32_t i = 0; i < count && 20 + (size_t)(i + 1) * 16 <= size; i++) {
        const unsigned char *entry = sfo + 20 + (size_t)i * 16;
        size_t key_at = (size_t)keys + le16(entry), value_at = (size_t)data + le32(entry + 12);
        uint32_t len = le32(entry + 4);
        if (key_at + key_len >= size || value_at + len > size) continue;
        if (memcmp(sfo + key_at, key, key_len + 1) != 0) continue;

        size_t n = 0;
        while (n + 1 < cap && n < len && sfo[value_at + n]) out[n] = (char)sfo[value_at + n], n++;
        out[n] = '\0';
        return n > 0;
    }
    return 0;
}
