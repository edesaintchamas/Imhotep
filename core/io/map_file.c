/* Imhotep - scenario map (.map) container reader. Format: docs/formats/map.md.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "io/map_file.h"

#include <stdlib.h>
#include <string.h>

#include "io/pkware.h"

/* @bin 0x0055BCF0 compressed-chunk reader: stored size 0x80000000 = raw bytes follow, 0 = the
 * chunk is zero-filled (no data), else PKWARE data (0x0055C000 -> explode 0x0055CBD0). Chunks
 * larger than 1,200,000 bytes are refused. */
#define UNCOMPRESSED_PREFIX 0x80000000u
#define MAX_COMPRESSED_CHUNK 1200000u

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

void map_file_free(map_file *m)
{
    for (int i = 0; i < MAP_FILE_MAX_CHUNKS; i++) {
        free(m->chunk[i]);
        m->chunk[i] = NULL;
    }
}

int map_file_read(const uint8_t *data, size_t len, map_file *m)
{
    memset(m, 0, sizeof(*m));
    /* @bin 0x004D4AE0 magic "MAPS" compared with the string at 0x005E17CC */
    if (len < 12 || memcmp(data, "MAPS", 4) != 0) {
        return -1;
    }
    return chunk_file_read(data, len, m);
}

int chunk_file_read(const uint8_t *data, size_t len, map_file *m)
{
    memset(m, 0, sizeof(*m));
    if (len < 12) {
        return -1;
    }
    m->version = u32(data + 4);
    m->count = u32(data + 8);
    /* @bin 0x004D0E10 the file's own schema (20-byte records from offset 12) drives the read */
    if (m->count > MAP_FILE_MAX_CHUNKS || len < 12 + 20 * (size_t) m->count) {
        return -1;
    }
    size_t pos = 0;
    for (uint32_t i = 0; i < m->count; i++) {
        const uint8_t *rec = data + 12 + 20 * i;
        uint32_t compressed = u32(rec), elem = u32(rec + 8), num = u32(rec + 12);
        uint64_t size = (uint64_t) elem * num;
        if (size > 64u * 1024 * 1024) {
            goto fail;
        }
        m->size[i] = (uint32_t) size;
        m->chunk[i] = malloc(size ? size : 1);
        if (!m->chunk[i]) {
            goto fail;
        }
        if (compressed) {
            /* compressed chunk: u32 stored size, then PKWARE data (map.md §1, §3) */
            if (pos + 4 > len) {
                goto fail;
            }
            uint32_t stored = u32(data + pos);
            pos += 4;
            if (size > MAX_COMPRESSED_CHUNK) {
                goto fail;
            }
            if (stored == 0) {
                memset(m->chunk[i], 0, size);
            } else if (stored == UNCOMPRESSED_PREFIX) {
                if (pos + size > len) {
                    goto fail;
                }
                memcpy(m->chunk[i], data + pos, size);
                pos += size;
            } else {
                if (pos + stored > len || pkware_explode(data + pos, stored, m->chunk[i], size) != 0) {
                    goto fail;
                }
                pos += stored;
            }
        } else {
            if (pos + size > len) {
                goto fail;
            }
            memcpy(m->chunk[i], data + pos, size);
            pos += size;
        }
    }
    if (pos != len) {
        goto fail;
    }
    return 0;
fail:
    map_file_free(m);
    return -1;
}
