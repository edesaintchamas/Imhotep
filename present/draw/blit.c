/* Imhotep - drawing primitives: the clip and the blitters on the back buffer (package blit, 7.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every blitter reads its parameters from the globals the image draw 0x0053E590 and the clip
 * functions 0x004CDAD0 / 0x004CDB80 set (present/draw/mem.h), and writes the back buffer at
 * [0x00E92E18] + y * [0x00E38D94] + 2 x. The loops follow the instructions: their test-first or
 * do-while forms, the counters they re-read, the run lengths they unroll (a run byte outside 1..16
 * copies 15 pixels in the unrolled RLE blitters) and the rows they skip, so that unusual inputs
 * give the original's pixels too. */
#include "draw.h"

#include "mem.h"

/* ---- clip ---- */

void clip_horizontal(game_state *st)
{
    /* @bin 0x004CDAD0 the image (x [0x00D3B5E0], width i16 [0x00D3B5BE]) against the clip's left
     * [0x00D3B55C] and right [0x00D3B558]: state 1 cut on the left (x moves to the edge, the cut
     * columns in 0x00D3B54C), 2 cut on the right (the visible width in 0x00D3B548), 5 outside or
     * width <= 0; the visible width 0x00D3B544, the source bytes skipped per row 0x00D3B540 and
     * the destination bytes from a row's end to the next 0x00D3B53C (screen width 0x00E38D9C) */
    int16_t w16 = S16(st, IMG_W);
    int32_t w = w16;
    int32_t vis = w;
    int32_t cut = 0;
    U8(st, CLIP_X) = 0;
    S32(st, CLIP_LEFT_COLS) = 0;
    S32(st, CLIP_RIGHT_W) = vis;
    if (w16 <= 0) {
        goto out;
    }
    int32_t x = S32(st, DRAW_X);
    int32_t left = S32(st, CLIP_LEFT);
    if (x < left) {
        if (w + x <= left) {
            goto out;
        }
        cut = left - x;
        U8(st, CLIP_X) = 1;
        S32(st, DRAW_X) = left;
        S32(st, CLIP_LEFT_COLS) = cut;
    } else {
        int32_t right = S32(st, CLIP_RIGHT);
        if (x > right - w) {
            if (x >= right) {
                goto out;
            }
            U8(st, CLIP_X) = 2;
            vis = right - x;
            S32(st, CLIP_RIGHT_W) = vis;
        }
    }
    vis -= cut;
    goto done;
out:
    U8(st, CLIP_X) = 5;
    vis = 0;
done:
    S32(st, VIS_W) = vis;
    S32(st, SRC_SKIP) = (int32_t) ((uint32_t) (w - vis) << 1);
    S32(st, DST_SKIP) = (int32_t) ((uint32_t) (S32(st, SCREEN_W) - vis) << 1);
}

void clip_vertical(game_state *st)
{
    /* @bin 0x004CDB80 the image (y [0x00D3B5DC], height i16 [0x00D3B5BC]) against the clip's top
     * [0x00D3B554] and bottom [0x00D3B550]: state 3 cut at the top (y moves to the edge, the cut
     * rows in 0x00D3B538), 4 cut at the bottom, 5 outside (also when the horizontal state is 5);
     * the rows drawn from the top 0x00D3B534 and the visible height 0x00D3B5D4 */
    int16_t h16 = S16(st, IMG_H);
    int32_t h = h16;
    int32_t cut = 0;
    U8(st, CLIP_Y) = 0;
    S32(st, CLIP_TOP_ROWS) = 0;
    S32(st, CLIP_BOTTOM_H) = h;
    if (h16 <= 0) {
        goto out;
    }
    int32_t y = S32(st, DRAW_Y);
    int32_t top = S32(st, CLIP_TOP);
    if (y < top) {
        if (h + y <= top) {
            goto out;
        }
        cut = top - y;
        S32(st, DRAW_Y) = top;
        int32_t room = S32(st, CLIP_BOTTOM) - top;
        U8(st, CLIP_Y) = 3;
        S32(st, CLIP_TOP_ROWS) = cut;
        if (h > room + cut) {
            h = room + cut;
            S32(st, CLIP_BOTTOM_H) = h;
        }
    } else {
        int32_t bottom = S32(st, CLIP_BOTTOM);
        if (y > bottom - h) {
            if (y >= bottom) {
                goto out;
            }
            U8(st, CLIP_Y) = 4;
            h = bottom - y;
            S32(st, CLIP_BOTTOM_H) = h;
        }
    }
    S32(st, VIS_H) = h - cut;
    goto done;
out:
    U8(st, CLIP_Y) = 5;
    S32(st, VIS_H) = cut;
done:
    if (U8(st, CLIP_X) == 5) {
        U8(st, CLIP_Y) = 5;
    }
}

int32_t clip_contains(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004CDC30 (x, y) inside the clip rectangle, edges included */
    return x >= S32(st, CLIP_LEFT) && x <= S32(st, CLIP_RIGHT) && y >= S32(st, CLIP_TOP) && y <= S32(st, CLIP_BOTTOM);
}

void clip_reset(game_state *st)
{
    /* @bin 0x005370D0 the clip rectangle back to the whole screen (0x00E38DB0 x 0x00E38DAC) */
    S32(st, CLIP_RIGHT) = S32(st, SCREEN_W2);
    S32(st, CLIP_LEFT) = 0;
    S32(st, CLIP_TOP) = 0;
    S32(st, CLIP_BOTTOM) = S32(st, SCREEN_H2);
}

/* ---- pixels ---- */

void pixel_put(game_state *st, int32_t x, int32_t y, int32_t colour)
{
    /* @bin 0x0040FA40 */
    mem_wr16(st, screen_at(st, x, y), (uint16_t) colour);
}

void pixel_and(game_state *st, int32_t x, int32_t y, int32_t mask)
{
    /* @bin 0x0040FA70 */
    uint32_t a = screen_at(st, x, y);
    mem_wr16(st, a, (uint16_t) (mem_rd16(st, a) & (uint16_t) mask));
}

int32_t pixel_get_masked(game_state *st, int32_t x, int32_t y, int32_t mask)
{
    /* @bin 0x0040FAA0 the pixel AND mask, kept in 0x00605B70 too; the upper half of the value is the
     * upper half of y * pitch (mov ax, [edi] after mul) */
    uint32_t row = (uint32_t) y * U32(st, PITCH);
    uint32_t a = U32(st, BACK_BUFFER) + 2u * (uint32_t) x + row;
    uint32_t v = ((row & 0xFFFF0000u) | mem_rd16(st, a)) & (uint32_t) mask;
    U32(st, 0x00605B70u) = v;
    return (int32_t) v;
}

void pixel_or(game_state *st, int32_t x, int32_t y, int32_t bits)
{
    /* @bin 0x0040FAE0 */
    uint32_t a = screen_at(st, x, y);
    mem_wr16(st, a, (uint16_t) (mem_rd16(st, a) | (uint16_t) bits));
}

void pixel_put_checked(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0040FB10 the colour [0x00D3B4F0] at (x, y) when 0 <= x < width 0x00E38D9C and
     * 0 <= y < height 0x00E38D98 - 32 */
    if (x < 0 || x >= S32(st, SCREEN_W) || y < 0 || y >= S32(st, SCREEN_H) - 0x20) {
        return;
    }
    mem_wr16(st, screen_at(st, x, y), U16(st, 0x00D3B4F0u));
}

void pixels_put2(game_state *st, uint32_t src)
{
    /* @bin 0x0040FBE0 two pixels (a dword) from src at (0x00D3B5E0, 0x00D3B5DC) */
    mem_wr32(st, screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y)), mem_rd32(st, src));
}

void pixels_put2_value(game_state *st, uint32_t value)
{
    /* @bin 0x0040FC10 two pixels (a dword) at (0x00D3B5E0, 0x00D3B5DC) */
    mem_wr32(st, screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y)), value);
}

/* ---- block copies ---- */

void block_to_screen(game_state *st, uint32_t src, int32_t dst_step, int32_t src_step)
{
    /* @bin 0x0040F9A0 rows of i16 [0x00D3B5BE] * 2 dwords from src to (0x00D3B5E0, 0x00D3B5DC),
     * i16 [0x00D3B5BC] rows (at least one), each row then advanced by the steps */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S16(st, IMG_H);
    if (S16(st, IMG_W) < 0) {
        return; /* rep movsd of 2^32 - 2 w dwords: the original faults */
    }
    do {
        uint32_t n = (uint32_t) (int32_t) S16(st, IMG_W) * 2u;
        mem_copy(st, d, src, n * 4u);
        d += n * 4u + (uint32_t) dst_step;
        src += n * 4u + (uint32_t) src_step;
        rows--;
    } while (rows > 0);
}

void block_from_screen(game_state *st, uint32_t dst, int32_t screen_step, int32_t dst_step)
{
    /* @bin 0x0040F9F0 the reverse of 0x0040F9A0: the screen block into dst */
    uint32_t s = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S16(st, IMG_H);
    if (S16(st, IMG_W) < 0) {
        return; /* the original faults */
    }
    do {
        uint32_t n = (uint32_t) (int32_t) S16(st, IMG_W) * 2u;
        mem_copy(st, dst, s, n * 4u);
        s += n * 4u + (uint32_t) screen_step;
        dst += n * 4u + (uint32_t) dst_step;
        rows--;
    } while (rows > 0);
}

void blit_opaque(game_state *st, uint32_t src)
{
    /* @bin 0x0040FB60 opaque copy of [0x00D3B5D4] rows (at least one) of [0x00D3B544] pixels; source
     * and destination rows advanced by 0x00D3B540 / 0x00D3B53C (bytes when the width's low byte is
     * not a multiple of 4, else dwords: the same bytes) */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    uint32_t n;
    if ((S8(st, VIS_W) & 3) != 0) {
        do {
            n = U32(st, VIS_W) * 2u;
            mem_copy(st, d, src, n);
            src += n + U32(st, SRC_SKIP);
            d += n + U32(st, DST_SKIP);
        } while (--rows > 0);
    } else {
        do {
            n = (U32(st, VIS_W) >> 1) * 4u;
            mem_copy(st, d, src, n);
            src += n + U32(st, SRC_SKIP);
            d += n + U32(st, DST_SKIP);
        } while (--rows > 0);
    }
}

/* ---- plain images (key 0xF81F transparent) ---- */

void blit_plain(game_state *st, uint32_t src)
{
    /* @bin 0x0040FFD0 [0x00D3B5D4] rows (at least one) of the full width i16 [0x00D3B5BE], source
     * rows contiguous (no horizontal clip), destination rows advanced by 0x00D3B53C; a width that is
     * a multiple of 4 is copied 4 pixels at a time (4 even for width 0) */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S16(st, IMG_W);
        int step = (n & 3) != 0 ? 1 : 4;
        do {
            for (int k = 0; k < step; k++) {
                uint16_t v = mem_rd16(st, src);
                if (v != KEY) {
                    mem_wr16(st, d, v);
                }
                src += 2;
                d += 2;
            }
            n -= step;
        } while (n > 0);
        d += U32(st, DST_SKIP);
    } while (--rows > 0);
}

void blit_plain_clipped(game_state *st, uint32_t src)
{
    /* @bin 0x00410090 [0x00D3B5D4] rows of [0x00D3B544] pixels (both at least one), source rows
     * advanced by 0x00D3B540 */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    uint32_t skip = U32(st, SRC_SKIP);
    do {
        int32_t n = S32(st, VIS_W);
        do {
            uint16_t v = mem_rd16(st, src);
            if (v != KEY) {
                mem_wr16(st, d, v);
            }
            src += 2;
            d += 2;
        } while (--n > 0);
        src += skip;
        d += U32(st, DST_SKIP);
    } while (--rows > 0);
}

void blit_plain_mirrored(game_state *st, uint32_t src)
{
    /* @bin 0x0040FED0 right to left from x + width (one pixel right of the image's last column),
     * [0x00D3B5D4] rows of the full width */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t w = S16(st, IMG_W);
    uint32_t w2 = 2u * (uint32_t) w;
    if (S32(st, VIS_H) <= 0) {
        return;
    }
    int32_t row = 0;
    do {
        d += w2;
        if (w > 0) {
            int32_t i = 0;
            do {
                uint16_t v = mem_rd16(st, src);
                if (v != KEY) {
                    mem_wr16(st, d, v);
                }
                d -= 2;
                src += 2;
                i++;
            } while (i < S16(st, IMG_W));
        }
        d += U32(st, DST_SKIP) + w2;
        row++;
    } while (row < S32(st, VIS_H));
}

void blit_plain_mirrored_clipped(game_state *st, uint32_t src)
{
    /* @bin 0x0040FF50 right to left, [0x00D3B5D4] rows of [0x00D3B544] pixels (counted down to 0) */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t w2 = 2u * U32(st, VIS_W);
    uint32_t rows = U32(st, VIS_H);
    if (rows == 0) {
        return;
    }
    do {
        uint32_t n = U32(st, VIS_W);
        d += w2;
        for (; n != 0; n--) {
            uint16_t v = mem_rd16(st, src);
            if (v != KEY) {
                mem_wr16(st, d, v);
            }
            d -= 2;
            src += 2;
        }
        d += U32(st, DST_SKIP) + w2;
        src += U32(st, SRC_SKIP);
    } while (--rows != 0);
}

void blit_plain_mask(game_state *st, uint32_t src, int32_t mask)
{
    /* @bin 0x004100F0 as 0x0040FFD0, ANDing mask into the destination where the source is not the key */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint16_t m = (uint16_t) mask;
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S16(st, IMG_W);
        int step = (n & 3) != 0 ? 1 : 4;
        do {
            for (int k = 0; k < step; k++) {
                if (mem_rd16(st, src) != KEY) {
                    mem_wr16(st, d, (uint16_t) (mem_rd16(st, d) & m));
                }
                src += 2;
                d += 2;
            }
            n -= step;
        } while (n > 0);
        d += U32(st, DST_SKIP);
    } while (--rows > 0);
}

void blit_plain_mask_clipped(game_state *st, uint32_t src, int32_t mask)
{
    /* @bin 0x004101C0 as 0x00410090, ANDing mask */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint16_t m = (uint16_t) mask;
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S32(st, VIS_W);
        do {
            if (mem_rd16(st, src) != KEY) {
                mem_wr16(st, d, (uint16_t) (mem_rd16(st, d) & m));
            }
            src += 2;
            d += 2;
        } while (--n > 0);
        src += U32(st, SRC_SKIP);
        d += U32(st, DST_SKIP);
    } while (--rows > 0);
}

/* ---- fixed-size opaque copies (types 2, 10, 12, 13: no clip) ---- */

void blit_type2(game_state *st, uint32_t src)
{
    /* @bin 0x00410F70 a dword from src at (x, y), a word from src + 2 at (x, y + 1) */
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    mem_wr32(st, d, mem_rd32(st, src));
    mem_wr16(st, d + U32(st, PITCH), mem_rd16(st, src + 2));
}

static void blit_square(game_state *st, uint32_t src, uint32_t n)
{
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t pitch = U32(st, PITCH);
    for (uint32_t r = 0; r < n; r++) {
        mem_copy(st, d + r * pitch, src + r * 2u * n, 2u * n);
    }
}

void blit_16x16(game_state *st, uint32_t src)
{
    /* @bin 0x00410FB0 16 rows of 16 pixels, unrolled */
    blit_square(st, src, 16);
}

void blit_24x24(game_state *st, uint32_t src)
{
    /* @bin 0x004114A0 24 rows of 24 pixels, unrolled */
    blit_square(st, src, 24);
}

void blit_32x32(game_state *st, uint32_t src)
{
    /* @bin 0x00411FD0 32 rows of 32 pixels, unrolled */
    blit_square(st, src, 32);
}

/* ---- RLE images (0xFF n: skip n; c: c pixels follow) ---- */

uint32_t rle_skip_rows(game_state *st, uint32_t src)
{
    /* @bin 0x00410F20 the stream after the [0x00D3B538] rows cut at the top (rows of i16 [0x00D3B5BE]) */
    int32_t rows = S32(st, CLIP_TOP_ROWS);
    while (rows > 0) {
        rows--;
        int32_t n = S16(st, IMG_W);
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                n -= mem_rd8(st, src + 1);
                src += 2;
            } else {
                n -= c;
                src += 2u * c + 1u;
            }
        }
    }
    return src;
}

enum { RLE_COPY, RLE_AND, RLE_FILL };

static inline void rle_op(game_state *st, int op, uint32_t d, uint32_t s, uint16_t v)
{
    if (op == RLE_COPY) {
        mem_wr16(st, d, mem_rd16(st, s));
    } else if (op == RLE_AND) {
        mem_wr16(st, d, (uint16_t) (mem_rd16(st, d) & v));
    } else {
        mem_wr16(st, d, v);
    }
}

static void rle_full(game_state *st, uint32_t src, int op, uint16_t v)
{
    /* the unrolled blitters: a run byte 1..16 is that many pixels, any other (0, 17..254) is 15 */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S16(st, IMG_W);
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src + 1);
                d += 2u * k;
                src += 2;
                n -= k;
                continue;
            }
            int32_t k = (c >= 1 && c <= 16) ? c : 15;
            for (int32_t i = 0; i < k; i++) {
                rle_op(st, op, d + 2u * (uint32_t) i, src + 1u + 2u * (uint32_t) i, v);
            }
            src += 1u + 2u * (uint32_t) k;
            d += 2u * (uint32_t) k;
            n -= k;
        }
        d += U32(st, DST_SKIP);
    } while (--rows > 0);
}

static void rle_left(game_state *st, uint32_t src, int op, uint16_t v)
{
    /* cut on the left: the drawing starts 0x00D3B540 / 2 pixels left of x, pixels are written once
     * that count is used up */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S16(st, IMG_W);
        int32_t cut = (int32_t) (U32(st, SRC_SKIP) >> 1);
        uint32_t row = d;
        d -= 2u * (uint32_t) cut;
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src + 1);
                d += 2u * k;
                src += 2;
                cut -= k;
                n -= k;
                continue;
            }
            src++;
            int32_t k = c;
            do {
                if (cut <= 0) {
                    rle_op(st, op, d, src, v);
                }
                src += 2;
                d += 2;
                n--;
                cut--;
            } while (--k > 0);
        }
        d = row + U32(st, PITCH);
    } while (--rows > 0);
}

static void rle_right(game_state *st, uint32_t src, int op, uint16_t v)
{
    /* cut on the right: pixels are written while the visible width [0x00D3B544] lasts */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    int32_t rows = S32(st, VIS_H);
    do {
        int32_t n = S16(st, IMG_W);
        int32_t left = S32(st, VIS_W);
        uint32_t row = d;
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src + 1);
                d += 2u * k;
                src += 2;
                left -= k;
                n -= k;
                continue;
            }
            src++;
            int32_t k = c;
            do {
                if (left > 0) {
                    rle_op(st, op, d, src, v);
                }
                src += 2;
                d += 2;
                left--;
                n--;
            } while (--k > 0);
        }
        d = row + U32(st, PITCH);
    } while (--rows > 0);
}

void blit_rle(game_state *st, uint32_t src)
{
    /* @bin 0x00410230 */
    rle_full(st, src, RLE_COPY, 0);
}

void blit_rle_left(game_state *st, uint32_t src)
{
    /* @bin 0x004105C0 */
    rle_left(st, src, RLE_COPY, 0);
}

void blit_rle_right(game_state *st, uint32_t src)
{
    /* @bin 0x00410660 */
    rle_right(st, src, RLE_COPY, 0);
}

void blit_rle_mask(game_state *st, uint32_t src, int32_t mask)
{
    /* @bin 0x00410700 the RLE image's pixels ANDed with mask (the source pixels are not read) */
    rle_full(st, src, RLE_AND, (uint16_t) mask);
}

void blit_rle_mask_left(game_state *st, uint32_t src, int32_t mask)
{
    /* @bin 0x004109D0 */
    rle_left(st, src, RLE_AND, (uint16_t) mask);
}

void blit_rle_mask_right(game_state *st, uint32_t src, int32_t mask)
{
    /* @bin 0x00410A70 */
    rle_right(st, src, RLE_AND, (uint16_t) mask);
}

void blit_glyph(game_state *st, uint32_t src)
{
    /* @bin 0x00410B10 type 20: the RLE image's pixels in the colour [0x00D3B51C] */
    rle_full(st, src, RLE_FILL, U16(st, GLYPH_COLOUR));
}

void blit_glyph_left(game_state *st, uint32_t src)
{
    /* @bin 0x00410DE0 */
    rle_left(st, src, RLE_FILL, U16(st, GLYPH_COLOUR));
}

void blit_glyph_right(game_state *st, uint32_t src)
{
    /* @bin 0x00410E80 */
    rle_right(st, src, RLE_FILL, U16(st, GLYPH_COLOUR));
}

/* ---- mirrored RLE: right to left from x + width ---- */

void blit_rle_mirrored(game_state *st, uint32_t src)
{
    /* @bin 0x0040FC40 a run that reaches the row's end mid-way leaves its other pixels unread */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t w2 = 2u * (uint32_t) (int32_t) S16(st, IMG_W);
    int32_t rows = S32(st, VIS_H);
    if (rows <= 0) {
        return;
    }
    do {
        int32_t n = S16(st, IMG_W);
        d += w2;
        while (n > 0) {
            uint8_t c = mem_rd8(st, src++);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src++);
                d -= 2u * k;
                n -= k;
                continue;
            }
            int stop = 0;
            for (uint8_t k = c; k != 0; k--) {
                if (n == 0) {
                    stop = 1;
                    break;
                }
                mem_wr16(st, d, mem_rd16(st, src));
                src += 2;
                d -= 2;
                n--;
            }
            if (stop) {
                break;
            }
        }
        d += U32(st, DST_SKIP) + w2;
    } while (--rows != 0);
}

void blit_rle_mirrored_right(game_state *st, uint32_t src)
{
    /* @bin 0x0040FCF0 cut on the right (of the screen): the first width - [0x00D3B544] pixels read
     * are not written */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t w2 = 2u * (uint32_t) (int32_t) S16(st, IMG_W);
    int32_t rows = S32(st, VIS_H);
    if (rows <= 0) {
        return;
    }
    do {
        int32_t n = S16(st, IMG_W);
        int32_t hidden = n - S32(st, VIS_W);
        d += w2;
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src + 1);
                src += 2;
                n -= k;
                hidden -= k;
                d -= 2u * k;
                continue;
            }
            src++;
            uint32_t k = c;
            if (k == 0) {
                continue;
            }
            n -= (int32_t) k;
            do {
                if (hidden <= 0) {
                    mem_wr16(st, d, mem_rd16(st, src));
                }
                d -= 2;
                src += 2;
                hidden--;
            } while (--k != 0);
        }
        d += U32(st, PITCH);
    } while (--rows != 0);
}

void blit_rle_mirrored_left(game_state *st, uint32_t src)
{
    /* @bin 0x0040FDD0 cut on the left (of the screen): pixels are written while width - [0x00D3B540] / 2
     * (sar) lasts */
    src = rle_skip_rows(st, src);
    uint32_t d = screen_at(st, S32(st, DRAW_X), S32(st, DRAW_Y));
    uint32_t w2 = 2u * (uint32_t) (int32_t) S16(st, IMG_W);
    int32_t rows = S32(st, VIS_H);
    if (rows <= 0) {
        return;
    }
    do {
        int32_t n = S16(st, IMG_W);
        int32_t skip = S32(st, SRC_SKIP);
        int32_t shown = n - (skip >> 1);
        uint32_t row = d;
        d += w2 - (uint32_t) skip;
        while (n > 0) {
            uint8_t c = mem_rd8(st, src);
            if (c == 0xFF) {
                uint8_t k = mem_rd8(st, src + 1);
                src += 2;
                shown -= k;
                n -= k;
                d -= 2u * k;
                continue;
            }
            src++;
            uint32_t k = c;
            if (k == 0) {
                continue;
            }
            n -= (int32_t) k;
            do {
                if (shown > 0) {
                    mem_wr16(st, d, mem_rd16(st, src));
                }
                d -= 2;
                src += 2;
                shown--;
            } while (--k != 0);
        }
        d = row + U32(st, PITCH);
    } while (--rows != 0);
}
