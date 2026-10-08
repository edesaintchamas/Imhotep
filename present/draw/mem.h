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

#include "state/state.h"

/* @bin globals of the blitters (docs/presentation.md 2.3, docs/formats/graphics.md 12) */
#define BACK_BUFFER 0x00E92E18u    /* u32 address of the back buffer (16-bit pixels) */
#define SPARE_SCREEN 0x00E92E1Cu   /* u32 address of the spare screen (external images are read there) */
#define PITCH 0x00E38D94u          /* i32 bytes per back-buffer row */
#define SCREEN_W 0x00E38D9Cu       /* i32 screen width (copies at 0x00E38DB0, 0x00D3B558 at set-up) */
#define SCREEN_H 0x00E38D98u       /* i32 screen height (copy 0x00E38DAC) */
#define SCREEN_W2 0x00E38DB0u
#define SCREEN_H2 0x00E38DACu
#define PIXEL_FORMAT 0x00E92DFCu   /* i32 2: 565, else 555 */
#define SRC 0x00D3B5B8u            /* u32 the image's pixel data (later the clipped start) */
#define DRAW_X 0x00D3B5E0u         /* i32 x on the screen (moved to the clip edge when clipped) */
#define DRAW_Y 0x00D3B5DCu         /* i32 y */
#define IMG_W 0x00D3B5BEu          /* i16 the image's width */
#define IMG_H 0x00D3B5BCu          /* i16 the image's height */
#define IMG_TYPE 0x00D3B5B4u       /* u8 the image's type (+50) */
#define VIS_W 0x00D3B544u          /* i32 visible width after the horizontal clip */
#define VIS_H 0x00D3B5D4u          /* i32 visible height after the vertical clip */
#define SRC_SKIP 0x00D3B540u       /* i32 source bytes to skip per row (clipped columns) */
#define DST_SKIP 0x00D3B53Cu       /* i32 destination bytes from a row's end to the next row's start */
#define CLIP_X 0x00D3B531u         /* u8 horizontal clip: 0 none, 1 left, 2 right, 5 out */
#define CLIP_Y 0x00D3B530u         /* u8 vertical clip: 0 none, 3 top, 4 bottom, 5 out */
#define CLIP_LEFT_COLS 0x00D3B54Cu /* i32 columns cut on the left */
#define CLIP_RIGHT_W 0x00D3B548u   /* i32 the width (the visible width when cut on the right) */
#define CLIP_TOP_ROWS 0x00D3B538u  /* i32 rows cut at the top */
#define CLIP_BOTTOM_H 0x00D3B534u  /* i32 the height (the visible rows from the top when cut at the bottom) */
#define CLIP_BOTTOM 0x00D3B550u    /* i32 the clip rectangle: bottom, top, right, left */
#define CLIP_TOP 0x00D3B554u
#define CLIP_RIGHT 0x00D3B558u
#define CLIP_LEFT 0x00D3B55Cu
#define TINT 0x0070E120u           /* u16 colour mask ANDed into the drawn pixels (0: none) */
#define GLYPH_COLOUR 0x00D3B51Cu   /* u16 the colour of type-20 images (font glyphs) */
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
    return U32(st, BACK_BUFFER) + 2u * (uint32_t) x + (uint32_t) y * U32(st, PITCH);
}

#endif
