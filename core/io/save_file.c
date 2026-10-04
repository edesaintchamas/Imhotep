/* Imhotep - saved-game reader: file chunks -> the reference Pharaoh.exe's version-160 layout.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "io/save_file.h"

#include <stdlib.h>
#include <string.h>

#include "io/map_file.h"

#define MIN_VERSION 115          /* @bin 0x004D0E10 file version < 0x73: the load fails */
#define DEFAULTS_VERSION 154     /* @bin 0x004D41E0 below 0x9A, stop before chunk 175 (0x004D3C40) */
#define SHORT_SCHEMA_CHUNKS 175

void save_file_free(save_file *s)
{
    for (int i = 0; i < SAVE_SCHEMA_RECORDS; i++) {
        free(s->chunk[i]);
        s->chunk[i] = NULL;
    }
}

/* @bin 0x004D0E10, a chunk whose file element size or count differs from the exe's. `in` holds the
 * file's fcount elements of fsize bytes. The game moves elements with memmove (0x00562880) in
 * descending order, through a temporary buffer when the file data is larger than the variable,
 * in place otherwise; both orders are reproduced, including the case fsize > tsize where each
 * element's tail lands on the start of the next one. Bytes the game would write past the end of
 * the variable (into the next global) are dropped; bytes it would read past the end of the file
 * data (uninitialised heap) are taken as 0 and are always zeroed afterwards. */
static void migrate(uint8_t *out, uint32_t tsize, uint32_t tcount, const uint8_t *in, uint32_t fsize,
                    uint32_t fcount)
{
    size_t ttotal = (size_t) tsize * tcount, ftotal = (size_t) fsize * fcount;
    size_t moved = (size_t) tsize * (tcount > fcount ? tcount : fcount);
    size_t span = (moved > ttotal ? moved : ttotal) + fsize + ftotal; /* every write the game makes */
    uint8_t *buf = calloc(span, 1);
    if (ttotal < ftotal) {
        /* temporary copy of the file data, zero beyond it */
        uint8_t *tmp = calloc((size_t) fsize * (tcount > fcount ? tcount : fcount) + 1, 1);
        memcpy(tmp, in, ftotal);
        for (uint32_t k = tcount; k-- > 0;) {
            memmove(buf + (size_t) k * tsize, tmp + (size_t) k * fsize, fsize);
            if (fsize < tsize) {
                memset(buf + (size_t) k * tsize + fsize, 0, tsize - fsize);
            }
        }
        free(tmp);
    } else {
        memcpy(buf, in, ftotal);              /* read into the zeroed variable */
        for (uint32_t k = fcount; k-- > 0;) {
            memmove(buf + (size_t) k * tsize, buf + (size_t) k * fsize, fsize);
            if (fsize < tsize) {
                memset(buf + (size_t) k * tsize + fsize, 0, tsize - fsize);
            }
        }
    }
    if (fcount < tcount) {                    /* no converter: elements beyond the file's count */
        memset(buf + (size_t) fcount * tsize, 0, (size_t) (tcount - fcount) * tsize);
    }
    memcpy(out, buf, ttotal);
    free(buf);
}

int save_file_read(const uint8_t *data, size_t len, save_file *s)
{
    memset(s, 0, sizeof(*s));
    map_file m;
    if (chunk_file_read(data, len, &m) != 0) {
        return SAVE_ERR_FORMAT;
    }
    int rc = SAVE_OK;
    s->version = m.version;
    s->file_chunks = m.count;
    if (m.version < MIN_VERSION) {
        rc = SAVE_ERR_TOO_OLD;
        goto done;
    }
    s->read_chunks = m.version < DEFAULTS_VERSION ? SHORT_SCHEMA_CHUNKS : SAVE_SCHEMA_RECORDS;
    if (m.count < s->read_chunks) {
        rc = SAVE_ERR_FORMAT;
        goto done;
    }
    for (uint32_t i = 0; i < SAVE_SCHEMA_RECORDS; i++) {
        const save_schema_record *t = &save_schema[i];
        size_t total = (size_t) t->elem_size * t->elem_count;
        s->chunk[i] = calloc(total ? total : 1, 1);
        if (!s->chunk[i]) {
            rc = SAVE_ERR_FORMAT;
            goto done;
        }
        if (i >= s->read_chunks) {
            continue; /* not in the file: defaults below */
        }
        const uint8_t *rec = data + 12 + 20 * i;
        uint32_t fflag = save_u32(rec), fsize = save_u32(rec + 8), fcount = save_u32(rec + 12);
        /* @bin 0x004D0E10 a converter applies when the file is older than its version threshold */
        if (t->converter_below && m.version < t->converter_below) {
            rc = SAVE_ERR_CONVERTER;
            goto done;
        }
        /* @bin 0x004D0E10 the schema chunk itself (6) and chunks read before it are never migrated */
        if (i <= 6 || (fsize == t->elem_size && fcount == t->elem_count)) {
            /* direct read with the exe's compression flag: a file stored otherwise would be
             * misread by the game, so it is refused here */
            if (fflag != t->compressed || m.size[i] != total) {
                rc = SAVE_ERR_FLAG;
                goto done;
            }
            memcpy(s->chunk[i], m.chunk[i], total);
        } else {
            migrate(s->chunk[i], t->elem_size, t->elem_count, m.chunk[i], fsize, fcount);
        }
    }
    if (s->read_chunks == SHORT_SCHEMA_CHUNKS) {
        /* @bin 0x004D3C40 defaults of chunk 175 (0x006FDCE0, 12 bytes): +0 := 0, byte +4 := 0xFF,
         * +8 := -1. Bytes +5..+7 keep the previous memory content in the game (0 here). Chunks
         * 176-180 are not set either: they keep what the game held before the load. */
        uint8_t *c = s->chunk[SHORT_SCHEMA_CHUNKS];
        memset(c, 0, 4);
        c[4] = 0xFF;
        memset(c + 8, 0xFF, 4);
    }
done:
    map_file_free(&m);
    if (rc != SAVE_OK) {
        save_file_free(s);
    }
    return rc;
}
