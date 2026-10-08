/* Imhotep - the platform's memory at game addresses (see memory.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/memory.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef struct {
    uint32_t offset, size; /* size 0: the entry is unused */
    int used;
} block;

static uint8_t *mem;
static uint32_t top;      /* the bump pointer (offset) */
static uint32_t preload;  /* bytes of the preloaded image */
static block *blocks;     /* sorted by offset */
static uint32_t nblocks, cap;
static uint32_t in_use;

int pmem_init(void)
{
    if (mem) {
        return 0;
    }
    void *p = mmap(NULL, PMEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        return -1;
    }
    mem = p;
    top = 0x10; /* address 0 of the range is never handed out */
    return 0;
}

int pmem_preload(const uint8_t *data, uint32_t size)
{
    if (!mem || size > PMEM_SIZE / 2) {
        return -1;
    }
    memcpy(mem, data, size);
    preload = size;
    top = (size + 0xFFFu) & ~0xFFFu;
    in_use = size;
    return 0;
}

int pmem_map(game_state *st)
{
    return state_map_external(st, PMEM_BASE, mem, PMEM_SIZE);
}

static int add_block(uint32_t offset, uint32_t size, int used)
{
    if (nblocks == cap) {
        uint32_t c = cap ? cap * 2 : 256;
        block *b = realloc(blocks, c * sizeof(block));
        if (!b) {
            return -1;
        }
        blocks = b;
        cap = c;
    }
    blocks[nblocks++] = (block){offset, size, used};
    return 0;
}

uint32_t pmem_alloc(uint32_t n)
{
    if (!mem) {
        return 0;
    }
    uint32_t size = (n + 15u) & ~15u;
    if (size == 0) {
        size = 16;
    }
    /* first fit in a freed block (split when it is larger) */
    for (uint32_t i = 0; i < nblocks; i++) {
        block *b = &blocks[i];
        if (!b->used && b->size >= size) {
            if (b->size - size >= 4096 && add_block(b->offset + size, b->size - size, 0) == 0) {
                b = &blocks[i];
                b->size = size;
                /* keep the table sorted: move the new free block after i */
                block nb = blocks[nblocks - 1];
                memmove(&blocks[i + 2], &blocks[i + 1], (nblocks - 2 - i) * sizeof(block));
                blocks[i + 1] = nb;
            }
            b->used = 1;
            memset(mem + b->offset, 0, b->size);
            in_use += b->size;
            return PMEM_BASE + b->offset;
        }
    }
    if ((uint64_t) top + size > PMEM_SIZE) {
        return 0;
    }
    uint32_t off = top;
    if (add_block(off, size, 1) != 0) {
        return 0;
    }
    top += size;
    in_use += size;
    return PMEM_BASE + off; /* fresh pages are zero */
}

void pmem_free(uint32_t address)
{
    if (!mem || address < PMEM_BASE) {
        return;
    }
    uint32_t off = address - PMEM_BASE;
    uint32_t lo = 0, hi = nblocks;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        if (blocks[mid].offset < off) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo >= nblocks || blocks[lo].offset != off || !blocks[lo].used) {
        return;
    }
    blocks[lo].used = 0;
    in_use -= blocks[lo].size;
    /* large freed blocks give their pages back to the system */
    if (blocks[lo].size >= (1u << 20)) {
        madvise(mem + off, blocks[lo].size, MADV_FREE);
    }
    /* merge with free neighbours */
    if (lo + 1 < nblocks && !blocks[lo + 1].used) {
        blocks[lo].size += blocks[lo + 1].size;
        memmove(&blocks[lo + 1], &blocks[lo + 2], (nblocks - lo - 2) * sizeof(block));
        nblocks--;
    }
    if (lo > 0 && !blocks[lo - 1].used) {
        blocks[lo - 1].size += blocks[lo].size;
        memmove(&blocks[lo], &blocks[lo + 1], (nblocks - lo - 1) * sizeof(block));
        nblocks--;
    }
}

uint8_t *pmem_ptr(uint32_t address, uint32_t len)
{
    if (!mem || address < PMEM_BASE || (uint64_t) address - PMEM_BASE + len > PMEM_SIZE) {
        return NULL;
    }
    return mem + (address - PMEM_BASE);
}

uint32_t pmem_used(void)
{
    return in_use;
}
