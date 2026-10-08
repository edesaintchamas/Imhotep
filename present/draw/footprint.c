/* Imhotep - drawing primitives: isometric footprints (package blit, 7.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A footprint tile is a 58 x 30 diamond of 900 opaque pixels (docs/formats/graphics.md 8.3). The
 * original draws it with unrolled code: one routine for the whole tile and one for each of the
 * halves left when the tile crosses the clip's left or right edge, each with a twin that ANDs the
 * tint [0x0070E120] into the pixels. The tables below list their moves, row by row: (source byte
 * offset, row, destination byte offset from the row's start at (x, y), bytes), transcribed from the
 * instructions word by word (consecutive moves merged). They keep the original's own slips: the half
 * tile at the left edge reads source bytes 972..975 twice in its bottom row 0 (dst 12 and 16), and
 * where a row ends with a word load (mov ax) stored as a dword, the store's upper word is the upper
 * word of the previous load (e.g. its bottom row 10: dst 14 gets source 1730). The
 * vertical clip state picks the halves: 3 (cut at the top) draws only the bottom half, from row 0;
 * 4 (cut at the bottom) only the top half; any other both, the bottom from row 15. */
#include "draw.h"

#include "mem.h"

typedef struct {
    uint16_t src;
    uint8_t row;
    uint8_t dst;
    uint8_t bytes;
} fp_span;

/* @bin 0x00427A20 / 0x00428C00, top half */
static const fp_span tile_top[] = {
    {0, 0, 56, 4}, {4, 1, 52, 12}, {16, 2, 48, 20}, {36, 3, 44, 28}, {64, 4, 40, 36}, {100, 5, 36, 44},
    {144, 6, 32, 52}, {196, 7, 28, 60}, {256, 8, 24, 68}, {324, 9, 20, 76}, {400, 10, 16, 84},
    {484, 11, 12, 92}, {576, 12, 8, 100}, {676, 13, 4, 108}, {784, 14, 0, 116},
};
/* @bin 0x00427A20 / 0x00428C00, bottom half */
static const fp_span tile_bottom[] = {
    {900, 0, 0, 116}, {1016, 1, 4, 108}, {1124, 2, 8, 100}, {1224, 3, 12, 92}, {1316, 4, 16, 84},
    {1400, 5, 20, 76}, {1476, 6, 24, 68}, {1544, 7, 28, 60}, {1604, 8, 32, 52}, {1656, 9, 36, 44},
    {1700, 10, 40, 36}, {1736, 11, 44, 28}, {1764, 12, 48, 20}, {1784, 13, 52, 12}, {1796, 14, 56, 4},
};
/* @bin 0x0042A160 / 0x0042AA00, top half */
static const fp_span tile_left_top[] = {
    {12, 1, 0, 4}, {28, 2, 0, 8}, {52, 3, 0, 12}, {84, 4, 0, 16}, {124, 5, 0, 20}, {172, 6, 0, 24},
    {228, 7, 0, 28}, {292, 8, 0, 32}, {364, 9, 0, 36}, {444, 10, 0, 40}, {532, 11, 0, 44}, {628, 12, 0, 48},
    {732, 13, 0, 52}, {844, 14, 0, 56},
};
/* @bin 0x0042A160 / 0x0042AA00, bottom half */
static const fp_span tile_left_bottom[] = {
    {960, 0, 0, 16}, {972, 0, 16, 4}, {980, 0, 20, 36}, {1072, 1, 0, 52}, {1176, 2, 0, 48}, {1272, 3, 0, 44},
    {1360, 4, 0, 40}, {1440, 5, 0, 36}, {1512, 6, 0, 32}, {1576, 7, 0, 28}, {1632, 8, 0, 24},
    {1680, 9, 0, 20}, {1720, 10, 0, 14}, {1730, 10, 14, 2}, {1752, 11, 0, 12}, {1776, 12, 0, 8},
    {1792, 13, 0, 4},
};
/* @bin 0x0042B440 / 0x0042BCF0, top half */
static const fp_span tile_right_top[] = {
    {4, 1, 52, 4}, {16, 2, 48, 8}, {36, 3, 44, 12}, {64, 4, 40, 16}, {100, 5, 36, 20}, {144, 6, 32, 24},
    {196, 7, 28, 28}, {256, 8, 24, 32}, {324, 9, 20, 36}, {400, 10, 16, 40}, {484, 11, 12, 44},
    {576, 12, 8, 48}, {676, 13, 4, 52}, {784, 14, 0, 56},
};
/* @bin 0x0042B440 / 0x0042BCF0, bottom half */
static const fp_span tile_right_bottom[] = {
    {900, 0, 0, 56}, {1016, 1, 4, 52}, {1124, 2, 8, 48}, {1224, 3, 12, 44}, {1316, 4, 16, 40},
    {1400, 5, 20, 36}, {1476, 6, 24, 32}, {1544, 7, 28, 28}, {1604, 8, 32, 24}, {1656, 9, 36, 20},
    {1700, 10, 40, 16}, {1736, 11, 44, 12}, {1764, 12, 48, 8}, {1784, 13, 52, 4},
};

#define N(a) ((int) (sizeof(a) / sizeof((a)[0])))

static void tile_spans(game_state *st, uint32_t src, int32_t clip_y, const fp_span *top, int ntop,
                       const fp_span *bottom, int nbottom, int tinted)
{
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t pitch = U32(st, PITCH);
    uint16_t mask = U16(st, TINT);
    uint32_t row0 = 0;
    if (clip_y != 3) {
        for (int i = 0; i < ntop; i++) {
            const fp_span *s = &top[i];
            uint32_t a = d + s->row * pitch + s->dst;
            for (uint32_t k = 0; k < s->bytes; k += 2) {
                uint16_t v = mem_rd16(st, src + s->src + k);
                mem_wr16(st, a + k, tinted ? (uint16_t) (v & mask) : v);
            }
        }
        if (clip_y == 4) {
            return;
        }
        row0 = 15; /* the top half's 15 row advances */
    }
    for (int i = 0; i < nbottom; i++) {
        const fp_span *s = &bottom[i];
        uint32_t a = d + (row0 + s->row) * pitch + s->dst;
        for (uint32_t k = 0; k < s->bytes; k += 2) {
            uint16_t v = mem_rd16(st, src + s->src + k);
            mem_wr16(st, a + k, tinted ? (uint16_t) (v & mask) : v);
        }
    }
}

void footprint_tile_tinted(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x00428C00 the tile with each pixel ANDed with [0x0070E120] */
    tile_spans(st, src, clip_y, tile_top, N(tile_top), tile_bottom, N(tile_bottom), 1);
}

void footprint_tile(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x00427A20 one 58 x 30 tile at (x, y); the tinted twin 0x00428C00 when [0x0070E120] != 0 */
    if (U16(st, TINT) != 0) {
        footprint_tile_tinted(st, src, clip_y);
        return;
    }
    tile_spans(st, src, clip_y, tile_top, N(tile_top), tile_bottom, N(tile_bottom), 0);
}

void footprint_tile_left_tinted(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x0042AA00 */
    tile_spans(st, src, clip_y, tile_left_top, N(tile_left_top), tile_left_bottom, N(tile_left_bottom), 1);
}

void footprint_tile_left(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x0042A160 the tile cut by the clip's left edge: its columns 30..57 at x (the edge) */
    if (U16(st, TINT) != 0) {
        footprint_tile_left_tinted(st, src, clip_y);
        return;
    }
    tile_spans(st, src, clip_y, tile_left_top, N(tile_left_top), tile_left_bottom, N(tile_left_bottom), 0);
}

void footprint_tile_right_tinted(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x0042BCF0 */
    tile_spans(st, src, clip_y, tile_right_top, N(tile_right_top), tile_right_bottom, N(tile_right_bottom), 1);
}

void footprint_tile_right(game_state *st, uint32_t src, int32_t clip_y)
{
    /* @bin 0x0042B440 the tile cut by the clip's right edge: its columns 0..27 */
    if (U16(st, TINT) != 0) {
        footprint_tile_right_tinted(st, src, clip_y);
        return;
    }
    tile_spans(st, src, clip_y, tile_right_top, N(tile_right_top), tile_right_bottom, N(tile_right_bottom), 0);
}

int32_t footprint_draw(game_state *st, int32_t n)
{
    /* @bin 0x0042C740 (2 x 2), 0x0042C9A0 (3 x 3), 0x0042CEE0 (4 x 4), 0x0042D810 (5 x 5),
     * 0x0042E680 (6 x 6): the image's n x n tiles (1800 bytes each from [0x00D3B5B8] +
     * [0x00E36C80]), rows of 1, 2, .., n, .., 1 tiles from the top, 60 px apart, each row 30 px
     * further left and 15 px lower than the one above (positions from (x, y) as given, before any
     * clip); each tile is clipped as a 58 x 30 image (0x004CDAD0, 0x004CDB80) and drawn whole,
     * as its left or right half, or not (clip state 5). Returns 58. */
    int32_t x0 = S32(st, DRAW_X);
    int32_t y0 = S32(st, DRAW_Y);
    S16(st, IMG_W) = 0x3A;
    S16(st, IMG_H) = 0x1E;
    uint32_t tile = 0;
    for (int32_t r = 0; r < 2 * n - 1; r++) {
        int32_t count = r < n ? r + 1 : 2 * n - 1 - r;
        for (int32_t k = 0; k < count; k++, tile++) {
            if (tile != 0) {
                S32(st, DRAW_X) = x0 - 30 * (count - 1) + 60 * k;
                S32(st, DRAW_Y) = y0 + 15 * r;
            }
            clip_horizontal(st);
            clip_vertical(st);
            uint8_t cy = U8(st, CLIP_Y);
            if (cy == 5) {
                continue;
            }
            uint32_t src = U32(st, SRC) + U32(st, 0x00E36C80u) + 0x708u * tile;
            uint8_t cx = U8(st, CLIP_X);
            if (cx == 1) {
                footprint_tile_left(st, src, cy);
            } else if (cx == 2) {
                footprint_tile_right(st, src, cy);
            } else {
                footprint_tile(st, src, cy);
            }
        }
    }
    return 0x3A;
}
