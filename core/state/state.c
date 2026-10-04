/* Imhotep - the game state (mirror of Pharaoh.exe's data segments).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int state_create(game_state *st)
{
    st->mem = calloc(STATE_SIZE, 1);
    st->heap = NULL;
    st->heap_used = st->heap_cap = 0;
    st->blocks = NULL;
    st->nblocks = st->blocks_cap = 0;
    st->unported = 0;
    return st->mem ? 0 : -1;
}

void state_free(game_state *st)
{
    free(st->mem);
    free(st->heap);
    free(st->blocks);
    st->mem = NULL;
    st->heap = NULL;
    st->blocks = NULL;
    st->heap_used = st->heap_cap = 0;
    st->nblocks = st->blocks_cap = 0;
}

int state_heap_resize(game_state *st, uint32_t used)
{
    if (used > STATE_HEAP_LIMIT) {
        return -1;
    }
    if (used > st->heap_cap) {
        uint32_t cap = st->heap_cap ? st->heap_cap : 0x100000u;
        while (cap < used) {
            cap *= 2;
        }
        uint8_t *h = realloc(st->heap, cap);
        if (!h) {
            return -1;
        }
        memset(h + st->heap_cap, 0, cap - st->heap_cap);
        st->heap = h;
        st->heap_cap = cap;
    }
    st->heap_used = used;
    return 0;
}

static int add_block(game_state *st, uint32_t offset, uint32_t size)
{
    if (st->nblocks == st->blocks_cap) {
        uint32_t cap = st->blocks_cap ? 2 * st->blocks_cap : 1024;
        uint32_t *b = realloc(st->blocks, 2 * sizeof(uint32_t) * cap);
        if (!b) {
            return -1;
        }
        st->blocks = b;
        st->blocks_cap = cap;
    }
    st->blocks[2 * st->nblocks] = offset;
    st->blocks[2 * st->nblocks + 1] = size;
    st->nblocks++;
    return 0;
}

/* @bin 0x00561A1D operator new, 0x00561B68 malloc, 0x00561BDC free: where the game allocates;
 * the addresses follow the oracle's convention (docs/formats/state.md, Heap) */
int state_heap_add_block(game_state *st, uint32_t offset, uint32_t size)
{
    return add_block(st, offset, size);
}

uint32_t state_alloc(game_state *st, uint32_t n)
{
    /* the oracle's allocator (tools/oracle/emu.py Pharaoh.alloc): 16-byte steps, at least 16 */
    uint32_t at = st->heap_used;
    uint32_t step = ((n ? n : 1) + 15u) & ~15u;
    if (at + (uint64_t) step > STATE_HEAP_LIMIT || state_heap_resize(st, at + step) != 0 || add_block(st, at, n) != 0) {
        return 0;
    }
    return STATE_HEAP_BASE + at;
}

void state_dealloc(game_state *st, uint32_t address)
{
    if (address < STATE_HEAP_BASE || address - STATE_HEAP_BASE >= st->heap_used) {
        return;
    }
    uint32_t off = address - STATE_HEAP_BASE, lo = 0, hi = st->nblocks;
    while (lo < hi) { /* blocks are in address order */
        uint32_t mid = (lo + hi) / 2;
        if (st->blocks[2 * mid] < off) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < st->nblocks && st->blocks[2 * lo] == off) {
        memset(st->heap + off, 0, st->blocks[2 * lo + 1]);
    }
}

int state_write_file(const game_state *st, void *file)
{
    FILE *f = file;
    uint8_t n[4] = {(uint8_t) st->heap_used, (uint8_t) (st->heap_used >> 8), (uint8_t) (st->heap_used >> 16),
                    (uint8_t) (st->heap_used >> 24)};
    if (fwrite(st->mem, 1, STATE_SIZE, f) != STATE_SIZE || fwrite(n, 1, 4, f) != 4 ||
        fwrite(st->heap, 1, st->heap_used, f) != st->heap_used) {
        return -1;
    }
    uint8_t b[8];
    for (int k = 0; k < 4; k++) {
        b[k] = (uint8_t) (st->nblocks >> (8 * k));
    }
    if (fwrite(b, 1, 4, f) != 4) {
        return -1;
    }
    for (uint32_t i = 0; i < st->nblocks; i++) {
        for (int k = 0; k < 4; k++) {
            b[k] = (uint8_t) (st->blocks[2 * i] >> (8 * k));
            b[4 + k] = (uint8_t) (st->blocks[2 * i + 1] >> (8 * k));
        }
        if (fwrite(b, 1, 8, f) != 8) {
            return -1;
        }
    }
    return 0;
}

int state_read_file(game_state *st, void *file)
{
    FILE *f = file;
    uint8_t n[4];
    if (fread(st->mem, 1, STATE_SIZE, f) != STATE_SIZE || fread(n, 1, 4, f) != 4) {
        return -1;
    }
    uint32_t used = n[0] | n[1] << 8 | n[2] << 16 | (uint32_t) n[3] << 24;
    if (state_heap_resize(st, used) != 0 || fread(st->heap, 1, used, f) != used || fread(n, 1, 4, f) != 4) {
        return -1;
    }
    uint32_t count = n[0] | n[1] << 8 | n[2] << 16 | (uint32_t) n[3] << 24;
    st->nblocks = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t b[8];
        if (fread(b, 1, 8, f) != 8 ||
            add_block(st, b[0] | b[1] << 8 | b[2] << 16 | (uint32_t) b[3] << 24,
                      b[4] | b[5] << 8 | b[6] << 16 | (uint32_t) b[7] << 24) != 0) {
            return -1;
        }
    }
    return 0;
}

/* @bin PE section table: .rdata 0x0056F000, .data 0x00576000 (virtual size 0xCF1AA8) */
void state_load_section(game_state *st, uint32_t address, const uint8_t *data, size_t len)
{
    uint64_t lo = address, hi = (uint64_t) address + len;
    if (lo < STATE_BASE) {
        data += STATE_BASE - lo;
        lo = STATE_BASE;
    }
    if (hi > STATE_END) {
        hi = STATE_END;
    }
    if (lo < hi) {
        memcpy(st->mem + (lo - STATE_BASE), data, (size_t) (hi - lo));
    }
}

uint8_t *state_ptr(const game_state *st, uint32_t address, uint32_t len)
{
    if (address >= STATE_BASE && (uint64_t) address + len <= STATE_END) {
        return st->mem + (address - STATE_BASE);
    }
    if (address >= STATE_HEAP_BASE && (uint64_t) address + len <= (uint64_t) STATE_HEAP_BASE + st->heap_used) {
        return st->heap + (address - STATE_HEAP_BASE);
    }
    return NULL;
}

void state_unported(game_state *st, uint32_t address)
{
    if (st->unported == 0) {
        st->unported = address;
    }
}

int state_copy(game_state *dst, const game_state *src)
{
    memcpy(dst->mem, src->mem, STATE_SIZE);
    if (state_heap_resize(dst, src->heap_used) != 0) {
        return -1;
    }
    memcpy(dst->heap, src->heap, src->heap_used);
    dst->nblocks = 0;
    for (uint32_t i = 0; i < src->nblocks; i++) {
        if (add_block(dst, src->blocks[2 * i], src->blocks[2 * i + 1]) != 0) {
            return -1;
        }
    }
    dst->unported = src->unported;
    return 0;
}
