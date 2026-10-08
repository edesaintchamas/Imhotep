/* Imhotep - drawing primitives: the tiled panels of the windows (package blit).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The windows' backgrounds and frames are grids of 16 x 16 images: corner, edge and middle pieces
 * of an image group, the middle pieces cycling through the group's variants so that the texture
 * does not repeat. */
#include "draw.h"

#include "mem.h"

typedef struct {
    uint32_t base;            /* i32: the group's first image */
    int32_t top_right;
    int32_t mid_left, mid, mid_right, mid_step;
    int32_t bottom_left, bottom, bottom_right;
    int32_t col_wrap, row_wrap;
} panel_kind;

/* the loops of 0x00536820 (ported by package sidebar: panel_draw, present/sidebar) and 0x00536A30: rows of w pieces, h rows, at (x + 16 col, y + 16 row); the
 * top and bottom rows' middle pieces step through the variants (col_wrap of them), the middle rows'
 * pieces also by a row offset that grows by mid_step per row (wrapping at row_wrap); 0x00D3B5C0
 * holds each piece's id, 0x00D3B5E0 / 0x00D3B5DC its place */
static void panel_tiles(game_state *st, const panel_kind *k, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t step = 0;     /* ebx: the last piece's row step */
    int32_t offset = 0;   /* ebp */
    for (int32_t r = 0; r < h; r++) {
        int32_t variant = 0;
        for (int32_t c = 0; c < w; c++) {
            int32_t px = c * 16 + x;
            int32_t py = r * 16 + y;
            S32(st, DRAW_X) = px;
            S32(st, DRAW_Y) = py;
            int32_t base = S32(st, k->base);
            int32_t id;
            if (r == 0) {
                if (c == 0) {
                    id = base;
                } else if (c < w - 1) {
                    id = base + variant + 1;
                    variant++;
                } else {
                    id = base + k->top_right;
                }
                step = 0;
            } else if (r < h - 1) {
                if (c == 0) {
                    id = base + offset + k->mid_left;
                } else if (c < w - 1) {
                    id = base + variant + offset + k->mid;
                    variant++;
                } else {
                    id = base + offset + k->mid_right;
                }
                step = k->mid_step;
            } else {
                if (c == 0) {
                    id = base + k->bottom_left;
                } else if (c < w - 1) {
                    id = base + variant + k->bottom;
                    variant++;
                } else {
                    id = base + k->bottom_right;
                }
                step = 0;
            }
            S32(st, 0x00D3B5C0u) = id;
            image_draw(st, id, px, py);
            if (variant >= k->col_wrap) {
                variant = 0;
            }
        }
        offset += step;
        if (offset >= k->row_wrap) {
            offset = 0;
        }
    }
}

void panel_draw_inner(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x00536A30 the inner panel (group [0x00F56844]) */
    static const panel_kind k = {0x00F56844u, 6, 7, 8, 0xD, 7, 0x2A, 0x2B, 0x30, 5, 0x23};
    panel_tiles(st, &k, x, y, w, h);
}

void panel_draw_frame(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit)
{
    /* @bin 0x00536C30 a frame of 16 x 16 pieces (group [0x00F5698C], + 8 when lit) around w x h
     * pixels: ceil(w / 16) x ceil(h / 16) places (division towards 0, then + 1 for a remainder), the
     * right column moved left and the bottom row up by the excess; the middle is not drawn. `kind`
     * picks nothing (both of its branches are the same). */
    (void) kind;
    int32_t cols = w / 16 + (w % 16 != 0);
    int32_t rows = h / 16 + (h % 16 != 0);
    int32_t dx = cols * 16 - w;
    int32_t dy = rows * 16 - h;
    int32_t base = S32(st, 0x00F5698Cu);
    if (lit != 0) {
        base += 8;
    }
    for (int32_t r = 0; r < rows; r++) {
        for (int32_t c = 0; c < cols; c++) {
            int32_t px = c * 16 + x;
            int32_t py = r * 16 + y;
            S32(st, DRAW_X) = px;
            S32(st, DRAW_Y) = py;
            if (r == 0) {
                if (c == 0) {
                    image_draw(st, base, px, py);
                } else if (c < cols - 1) {
                    image_draw(st, base + 1, px, py);
                } else {
                    image_draw(st, base + 2, px - dx, py);
                }
            } else if (r >= rows - 1) {
                if (c == 0) {
                    image_draw(st, base + 6, px, py - dy);
                } else if (c < cols - 1) {
                    image_draw(st, base + 5, px, py - dy);
                } else {
                    image_draw(st, base + 4, px - dx, py - dy);
                }
            } else if (c == 0) {
                image_draw(st, base + 7, px, py);
            } else if (c >= cols - 1) {
                image_draw(st, base + 3, px - dx, py);
            }
        }
    }
}
