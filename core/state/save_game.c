/* Imhotep - write a saved game from the game state (Pharaoh.exe 0x004D4E20).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/save_game.h"

#include <stdlib.h>
#include <string.h>

#include "io/pkware.h"

typedef struct {
    uint8_t *data;
    size_t len, cap;
    int failed;
} out_buf;

static void put(out_buf *o, const void *p, size_t n)
{
    if (o->failed) {
        return;
    }
    if (o->len + n > o->cap) {
        size_t cap = o->cap ? o->cap : 1 << 20;
        while (cap < o->len + n) {
            cap *= 2;
        }
        uint8_t *d = realloc(o->data, cap);
        if (!d) {
            o->failed = 1;
            return;
        }
        o->data = d;
        o->cap = cap;
    }
    memcpy(o->data + o->len, p, n);
    o->len += n;
}

static void put_u32(out_buf *o, uint32_t v)
{
    uint8_t b[4] = {(uint8_t) v, (uint8_t) (v >> 8), (uint8_t) (v >> 16), (uint8_t) (v >> 24)};
    put(o, b, 4);
}

/* @bin 0x0055BC50 a compressed chunk: nothing above 1,200,000 bytes; else the compressor
 * (0x0055BF10, dictionary 0x00603650, output room = input size): u32 size then the data, or when
 * it does not fit, u32 0x80000000 then the raw bytes */
static void put_compressed(game_state *st, out_buf *o, const uint8_t *data, uint32_t n)
{
    if (n > 1200000) {
        return;
    }
    uint8_t *tmp = malloc(n ? n : 1);
    size_t clen;
    if (tmp && pkware_implode(data, n, tmp, n, &clen, S32(st, 0x00603650u)) == 0) {
        put_u32(o, (uint32_t) clen);
        put(o, tmp, clen);
    } else {
        put_u32(o, 0x80000000u);
        put(o, data, n);
    }
    free(tmp);
}

void terrain_flag10_to_1(game_state *st)
{
    /* @bin 0x004771C0 */
    for (int32_t r = 0; r < S32(st, 0x00BCA2D4u); r++) {
        int32_t lo = S32(st, 0x00BC9F24u + 4u * (uint32_t) r), hi = S32(st, 0x00BC9B94u + 4u * (uint32_t) r);
        int32_t o = S32(st, 0x00BC9804u + 4u * (uint32_t) r);
        for (int32_t k = lo; k < hi; k++, o++) {
            uint32_t *t = (uint32_t *) state_ptr(st, 0x00AF598Cu + 4u * (uint32_t) o, 4);
            if (*t & 0x10) {
                *t = (*t & ~0x10u) | 1;
            }
        }
    }
}

/* @bin 0x004F6A10 (this = array) bring an index into 0 .. capacity - 1 (when the capacity is 0:
 * unchanged) */
static void wrap_index(game_state *st, uint32_t vec, uint32_t field)
{
    int32_t cap = S32(st, vec + 0x10);
    if (cap == 0) {
        return;
    }
    while (S32(st, field) < 0) {
        S32(st, field) += S32(st, vec + 0x10);
    }
    while (S32(st, vec + 0x10) <= S32(st, field)) {
        S32(st, field) -= S32(st, vec + 0x10);
    }
}

/* @bin 0x004F6810 (this = queue, record) pack a queue into its 0x334-byte saved record: capacity
 * at most 200, indexes wrapped, header, then the elements (or 200 zeros without a block) */
static void queue_pack(game_state *st, uint32_t q, uint32_t rec)
{
    if (S32(st, q + 0x10) > 200) {
        S32(st, q + 0x10) = 200;
    }
    wrap_index(st, q, q + 4);
    wrap_index(st, q, q + 8);
    wrap_index(st, q, q + 0x0C);
    S32(st, rec) = S32(st, q + 0x0C);
    S32(st, rec + 4) = S32(st, q + 0x10);
    S32(st, rec + 8) = S32(st, q + 4);
    S32(st, rec + 0x0C) = S32(st, q + 8);
    U8(st, rec + 0x10) = U8(st, q);
    uint32_t block = U32(st, q + 0x14);
    if (block != 0) {
        uint32_t n = (uint32_t) S32(st, q + 0x10) & 0x3FFFFFFFu;
        for (uint32_t i = 0; i < n; i++) {
            S32(st, rec + 0x14 + 4 * i) = (int32_t) P32(st, block + 4 * i);
        }
    } else {
        memset(state_ptr(st, rec + 0x14, 800), 0, 800);
    }
}

int game_save(game_state *st, uint8_t **out, size_t *out_len)
{
    /* @bin 0x004D0A40(0) the 10 shipwright queues into chunk 140's records */
    for (uint32_t j = 0; j < 10; j++) {
        queue_pack(st, 0x009783C0u + 0x1Cu * j, 0x009763B8u + 0x334u * j);
    }
    /* @bin 0x004D4E20 version 0xA0; "MAPS" copied over 0x005E17C4; player name into chunk 30 */
    S32(st, 0x00E945C0u) = 0xA0;
    memcpy(state_ptr(st, 0x005E17C4u, 5), state_ptr(st, 0x005E17CCu, 5), 5);
    char *name = (char *) state_ptr(st, 0x00EA184Cu, 32);
    const char *src = (const char *) state_ptr(st, 0x00E38E74u, 32);
    size_t nl = strnlen(src, 31);
    memmove(name, src, nl);
    name[nl] = 0;
    terrain_flag10_to_1(st); /* @bin 0x004771C0 */
    /* @bin 0x004D4E20 routes: +0x141 / +0x134 of record 0 := 0, every point buffer released */
    U8(st, 0x007770F1u) = 0;
    S32(st, 0x007770E4u) = 0;
    for (uint32_t k = 0; k < 50; k++) {
        uint32_t r = 0x00776FB0u + 0x144u * k;
        if (U32(st, r + 4) != 0) {
            state_dealloc(st, U32(st, r + 4));
            U32(st, r + 4) = 0;
            S32(st, r + 0x138) = 0;
        }
    }
    /* @bin 0x004D4E20 the schema (chunk 6, 0x00E92E50) := the 181 records of 0x005E09A0 */
    memset(state_ptr(st, 0x00E92E50u, 0x5DC * 4), 0, 0x5DC * 4);
    memcpy(state_ptr(st, 0x00E92E50u, 0x389 * 4), state_ptr(st, 0x005E09A0u, 0x389 * 4), 0x389 * 4);
    S32(st, 0x00E945C4u) = 0xB5;
    out_buf o = {NULL, 0, 0, 0};
    for (uint32_t i = 0; i < 0xB5; i++) {
        uint32_t rec = 0x005E09A0u + 20u * i;
        uint32_t n = U32(st, rec + 8) * U32(st, rec + 12);
        const uint8_t *data = state_ptr(st, U32(st, rec + 4), n);
        if (!data) {
            free(o.data);
            return -1;
        }
        if (U32(st, rec) == 0) {
            put(&o, data, n);
        } else {
            put_compressed(st, &o, data, n);
        }
    }
    if (o.failed) {
        free(o.data);
        return -1;
    }
    *out = o.data;
    *out_len = o.len;
    return 0;
}
