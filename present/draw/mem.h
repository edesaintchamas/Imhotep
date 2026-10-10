/* Imhotep - drawing primitives: memory access and the blitters' globals (package blit, internal).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The blitters read pixel data and write the back buffer through addresses the state holds (the
 * back buffer [0x00E92E18], the image entries' data pointers +0), which may lie outside the data
 * segments (state_map_external). Every access goes through state_ptr; an address that is not
 * mapped would fault in the original: here the read gives 0 and the write is dropped. */
#ifndef IMHOTEP_PRESENT_DRAW_MEM_H
#define IMHOTEP_PRESENT_DRAW_MEM_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* @bin globals of the blitters (docs/presentation.md 2.3, docs/formats/graphics.md 12): named in
 * present_layout.h */
#define KEY 0xF81Fu                /* the transparent colour of plain images */

static inline uint8_t mem_rd8(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 1);
    return p ? *p : 0;
}

static inline uint16_t mem_rd16(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 2);
    return p ? (uint16_t) (p[0] | p[1] << 8) : 0;
}

static inline uint32_t mem_rd32(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 4);
    return p ? (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24 : 0;
}

static inline void mem_wr8(game_state *st, uint32_t a, uint8_t v)
{
    uint8_t *p = state_ptr(st, a, 1);
    if (p) {
        *p = v;
    }
}

static inline void mem_wr16(game_state *st, uint32_t a, uint16_t v)
{
    uint8_t *p = state_ptr(st, a, 2);
    if (p) {
        p[0] = (uint8_t) v;
        p[1] = (uint8_t) (v >> 8);
    }
}

static inline void mem_wr32(game_state *st, uint32_t a, uint32_t v)
{
    /* as two words: a dword at the end of a mapped buffer is written in part, as the original's
     * write lands in whatever memory follows (test tools map the back buffer over another copy) */
    mem_wr16(st, a, (uint16_t) v);
    mem_wr16(st, a + 2, (uint16_t) (v >> 16));
}

/* rep movsb / movsd: n bytes forward (the original's order when the ranges overlap), written a word
 * at a time (see mem_wr32) */
static inline void mem_copy(game_state *st, uint32_t dst, uint32_t src, uint32_t n)
{
    const uint8_t *s = state_ptr(st, src, n);
    uint32_t i = 0;
    for (; i + 1 < n; i += 2) {
        uint16_t v = s ? (uint16_t) (s[i] | s[i + 1] << 8) : mem_rd16(st, src + i);
        mem_wr16(st, dst + i, v);
    }
    if (i < n) {
        mem_wr8(st, dst + i, s ? s[i] : mem_rd8(st, src + i));
    }
}

/* the back-buffer address of (x, y): [0x00E92E18] + 2 x + y * pitch (mul / imul: the low 32 bits) */
static inline uint32_t screen_at(game_state *st, int32_t x, int32_t y)
{
    return U32(st, G_BACK_BUFFER) + 2u * (uint32_t) x + (uint32_t) y * U32(st, G_SCREEN_PITCH);
}

#endif
