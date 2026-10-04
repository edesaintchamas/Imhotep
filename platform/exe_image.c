/* Imhotep - Pharaoh.exe's sections, for the initial game state.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/exe_image.h"

#include <stdio.h>
#include <stdlib.h>

#include "platform/game_files.h"

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

int exe_image_load(const char *game, game_state *st)
{
    char path[1200];
    size_t len;
    snprintf(path, sizeof(path), "%s/Pharaoh.exe", game);
    uint8_t *d = read_file(path, &len);
    if (!d || len < 0x40) {
        free(d);
        return -1;
    }
    uint32_t pe = le32(d + 0x3C);
    if (pe + 0x18 > len || le32(d + pe) != 0x00004550) { /* "PE\0\0" */
        free(d);
        return -1;
    }
    unsigned nsec = d[pe + 6] | d[pe + 7] << 8;
    unsigned opt = d[pe + 20] | d[pe + 21] << 8;
    uint32_t base = le32(d + pe + 24 + 28);
    size_t sec = pe + 24 + opt;
    for (unsigned i = 0; i < nsec && sec + 40 * (i + 1) <= len; i++) {
        const uint8_t *h = d + sec + 40 * i;
        uint32_t vsize = le32(h + 8), va = le32(h + 12), rsize = le32(h + 16), raw = le32(h + 20);
        uint32_t n = rsize < vsize || !vsize ? rsize : vsize;
        if ((size_t) raw + n <= len) {
            state_load_section(st, base + va, d + raw, n);
        }
    }
    free(d);
    return 0;
}
