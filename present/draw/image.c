/* Imhotep - drawing primitives: the image draws (package blit, 7.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The image draw 0x0053E590 and its variants are thiscall on the graphics object 0x00F55BB0
 * (every call site: mov ecx, 0xF55BB0). They read the 64-byte runtime entry of the image
 * (docs/formats/graphics.md 5, 12: +0 data, +4 length, +8 footprint length, +12 slot, +16 mirror
 * offset, +20 / +22 width / height, +50 type, +51 compressed, +52 external, +53 has top, +56 bitmap,
 * +62 mirrored), set the blitters' globals (present/draw/mem.h), clip and pick a blitter. */
#include "draw.h"

#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/loader.h"
#include "state/ported.h"
#include "sim/platform.h"

#define IMAGE_ID_END 0x639C     /* ids from here on are not drawn */

static uint32_t entry_at(game_state *st, uint32_t gfx, int32_t id)
{
    /* the entry of id, or of id 0 when id >= the count (cmp / jl: signed) */
    if (id >= S32(st, gfx + GFX_IMAGE_COUNT)) {
        id = 0;
    }
    return ((uint32_t) id << 6) + gfx + GFX_ENTRIES;
}

/* "data\\" + the bitmap name of the entry of id (slot +12, bitmap +56): the name of an external
 * image's .555 (graphics.md 11: gfx + 0xCC3C slot + 0x2044 + 0x104 bitmap) */
static uint32_t external_name(game_state *st, uint32_t gfx, int32_t id)
{
    uint32_t e = entry_at(st, gfx, id);
    uint32_t slot = U32(st, e + IMGENTRY_SLOT);
    uint32_t bitmap = U8(st, e + 0x38);
    return gfx + slot * GFX_SLOT_SIZE + GFX_SLOTS + 0x104u + bitmap * 0x104u;
}

/* the C string at a state address, up to n - 1 bytes (strcpy / repne scasb: up to the NUL) */
static void cstr_at(game_state *st, uint32_t a, char *out, size_t n)
{
    size_t i = 0;
    for (; i + 1 < n; i++) {
        uint8_t c = mem_rd8(st, a + (uint32_t) i);
        out[i] = (char) c;
        if (c == 0) {
            return;
        }
    }
    out[i] = 0;
}

void path_set_extension(game_state *st, char *path, uint32_t ext)
{
    /* @bin 0x004CE310 the 3 bytes at ext copied to 0x00E38AEC (0x004CBEA0), then 0x004CE370: the
     * characters after the path's first '.' replaced by them (nothing without a '.') */
    for (uint32_t i = 0; i < 3; i++) {
        U8(st, G_FILE_EXT + i) = mem_rd8(st, ext + i);
    }
    path_replace_extension(st, path);
}

void path_replace_extension(game_state *st, char *path)
{
    /* @bin 0x004CE370 after the first '.': the 3 characters at 0x00E38AEC and a NUL */
    for (char *p = path;; p++) {
        char c = *p;
        if (c == '.') {
            p[1] = (char) U8(st, G_FILE_EXT);
            p[2] = (char) U8(st, G_FILE_EXT + 1u);
            p[3] = (char) U8(st, G_FILE_EXT + 2u);
            p[4] = 0;
            return;
        }
        if (c == 0) {
            return;
        }
    }
}

void external_buffer_free(game_state *st, uint32_t slot, int32_t forget)
{
    /* @bin 0x0053F790 the buffer [slot] (0: none): the spare screen [0x00E92E1C] is kept (the slot
     * cleared when forget's low byte is set), any other freed (CRT free 0x00561A2B) and the slot
     * cleared */
    uint32_t p = U32(st, slot);
    if (p == 0) {
        return;
    }
    if (p == U32(st, G_SPARE_SCREEN)) {
        if ((uint8_t) forget != 0) {
            U32(st, slot) = 0;
        }
        return;
    }
    GAME_CALL(st, FN_OPERATOR_DELETE, (int32_t) p);
    U32(st, slot) = 0;
}

int32_t external_image_read(game_state *st, uint32_t gfx, uint32_t name, uint32_t buf, int32_t row_bytes,
                            int32_t rows, int32_t offset)
{
    /* @bin 0x0053F7D0 thiscall: "data\\" name with the extension "555" (0x005E06E0, 0x004CE310), its
     * bytes from `offset` read row by row into buf: row_bytes per row, then a seek of 2 i16
     * [0x00D3B5BE] - row_bytes (the rest of the image's row). The original opens the file with the
     * CRT (_open 0x00568EAB, _lseek, _read, _close) and retries with the CD path (0x005E05AC,
     * 0x004CE9E0) and the other directories (0x004CE100, 0x004CE2D0): the platform serves the game
     * files wherever they are (platform_file_read). The bytes read are converted to 565 when the
     * display is 565 (0x004CDF20 on [gfx + 0x30F5C0]); returns their count, 0 when none. */
    char path[0x110] = "data\\";
    cstr_at(st, name, path + 5, sizeof(path) - 5);
    path_set_extension(st, path, G_STR_555);
    int32_t gap = (int32_t) S16(st, G_UD3B5BE) * 2 - row_bytes;
    int32_t total = 0;
    int32_t size = platform_file_read(st, path, 0, NULL, 0);
    if (size >= 0 && rows > 0) {
        int64_t pos = offset;
        for (int32_t r = 0; r < rows; r++) {
            int32_t got = 0;
            if (pos >= 0 && pos < size && row_bytes > 0) {
                int64_t left = size - pos;
                got = (int32_t) (left < row_bytes ? left : row_bytes);
                uint8_t *tmp = malloc((size_t) got);
                if (tmp) {
                    platform_file_read(st, path, (uint32_t) pos, tmp, (uint32_t) got);
                    for (int32_t i = 0; i < got; i++) {
                        mem_wr8(st, buf + (uint32_t) i, tmp[i]);
                    }
                    free(tmp);
                }
            }
            total += got;
            pos += got + gap;
            buf += (uint32_t) row_bytes;
        }
    }
    if (total < 1) {
        return 0;
    }
    GAME_CALL(st, FN_PIXELS_555_TO_565, (int32_t) U32(st, gfx + GFX_EXTERNAL_BUF), total);
    return total;
}

/* the external image of id (entry e) read into the spare screen and converted: 1, or 0 when the
 * file could not be read (the read 0x004CE480 of the original's 0x0053E590) */
static int external_small_read(game_state *st, uint32_t gfx, int32_t id, uint32_t e)
{
    char path[0x108] = "data\\";
    cstr_at(st, external_name(st, gfx, id), path + 5, sizeof(path) - 5);
    path_set_extension(st, path, G_STR_555);
    external_buffer_free(st, gfx + GFX_EXTERNAL_BUF, 0);
    uint32_t buf = U32(st, G_SPARE_SCREEN);
    U32(st, gfx + GFX_EXTERNAL_BUF) = buf;
    uint32_t len = U32(st, e + 4);
    uint8_t *tmp = malloc(len ? len : 1);
    if (!tmp) {
        return 0;
    }
    int32_t got = loader_file_read_at(st, path, tmp, len, U32(st, e) - 1u);
    for (int32_t i = 0; i < got; i++) {
        mem_wr8(st, buf + (uint32_t) i, tmp[i]); /* byte by byte: the buffer may be mapped in parts */
    }
    free(tmp);
    if (got == 0) {
        return 0;
    }
    if (U8(st, e + 0x33) != 0) {
        GAME_CALL(st, FN_RLE_555_TO_565, (int32_t) U32(st, gfx + GFX_EXTERNAL_BUF), (int32_t) U32(st, e + 4));
    } else {
        GAME_CALL(st, FN_PIXELS_555_TO_565, (int32_t) U32(st, gfx + GFX_EXTERNAL_BUF), (int32_t) U32(st, e + 4));
    }
    return 1;
}

static void plain_draw(game_state *st, uint32_t src, int mirrored)
{
    /* a plain image by the horizontal clip state */
    uint8_t cx = U8(st, G_CLIP_X);
    if (mirrored) {
        if (cx == 1 || cx == 2) {
            blit_plain_mirrored_clipped(st, src);
        } else {
            blit_plain_mirrored(st, src);
        }
    } else if (cx == 1 || cx == 2) {
        blit_plain_clipped(st, src);
    } else {
        blit_plain(st, src);
    }
}

static void rle_draw(game_state *st, uint32_t src, int mirrored)
{
    uint8_t cx = U8(st, G_CLIP_X);
    if (mirrored) {
        if (cx == 1) {
            blit_rle_mirrored_left(st, src);
        } else if (cx == 2) {
            blit_rle_mirrored_right(st, src);
        } else {
            blit_rle_mirrored(st, src);
        }
    } else if (cx == 1) {
        blit_rle_left(st, src);
    } else if (cx == 2) {
        blit_rle_right(st, src);
    } else {
        blit_rle(st, src);
    }
}

static void rle_tint(game_state *st, uint32_t src, int32_t mask)
{
    /* the second pass of a tinted RLE image: never mirrored (0x004109D0, 0x00410A70, 0x00410700) */
    uint8_t cx = U8(st, G_CLIP_X);
    if (cx == 1) {
        blit_rle_mask_left(st, src, mask);
    } else if (cx == 2) {
        blit_rle_mask_right(st, src, mask);
    } else {
        blit_rle_mask(st, src, mask);
    }
}

static void plain_tint(game_state *st, uint32_t src, int32_t mask)
{
    uint8_t cx = U8(st, G_CLIP_X);
    if (cx == 1 || cx == 2) {
        blit_plain_mask_clipped(st, src, mask);
    } else {
        blit_plain_mask(st, src, mask);
    }
}

/* the offset of the first visible pixel of a plain image: 2 (width * rows cut + columns cut) */
static uint32_t plain_offset(game_state *st)
{
    return 2u * ((uint32_t) ((int32_t) S16(st, G_UD3B5BE) * S32(st, G_CLIP_TOP_ROWS)) + U32(st, G_CLIP_LEFT_COLS));
}

/* 0x0053E590 (flipped 0) and its copy 0x0053FA40 (flipped 1: every image drawn right to left, the
 * mirror entries not followed, no tint pass after a plain type-0 image) */
static int32_t image_draw_any(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y, int flipped)
{
    S32(st, G_LAST_X) = x;
    S32(st, G_LAST_IMAGE) = id;
    S32(st, G_LAST_Y) = y;
    int mirrored = flipped;
    uint32_t e = entry_at(st, gfx, id);
    if (id <= 0 || id >= IMAGE_ID_END || e == 0) {
        return 0;
    }
    if (!flipped && U8(st, e + 0x3E) != 0) {
        /* @bin 0x0053E625 a mirrored entry: the image at id + its offset +16, drawn right to left */
        int32_t t = S32(st, e + 0x10) + id;
        if (t >= S32(st, gfx + GFX_IMAGE_COUNT)) {
            t = 0;
        }
        mirrored = 1;
        e = ((uint32_t) t << 6) + gfx + GFX_ENTRIES;
        if (e == 0) {
            return 0;
        }
    }
    U32(st, G_MEM_SRC) = U32(st, e);
    if (S32(st, e) <= 0) {
        return 0;
    }
    int16_t w16 = S16(st, e + 0x14);
    S16(st, G_UD3B5BE) = w16;
    S16(st, G_UD3B5BC) = S16(st, e + 0x16);
    S32(st, G_BLIT_X) = x;
    U8(st, G_IMG_TYPE) = U8(st, e + 0x32);
    S32(st, G_BLIT_Y) = y;
    int32_t w = w16;
    int32_t screen_w = S32(st, G_DRAW_W);
    S32(st, G_VIS_W) = w;
    if (U8(st, e + 0x34) != 0) {
        if (w <= screen_w) {
            /* @bin 0x0053E6BD an external image as wide as the screen at most: read whole into the
             * spare screen, then drawn as the internal ones */
            if (!external_small_read(st, gfx, id, e)) {
                return 0;
            }
            uint32_t buf = U32(st, gfx + GFX_EXTERNAL_BUF);
            clip_horizontal(st);
            clip_vertical(st);
            int32_t width = S16(st, G_UD3B5BE);
            if (U8(st, G_CLIP_Y) == 5) {
                return 0;
            }
            if (U8(st, G_IMG_TYPE) == 0 && U8(st, e + 0x33) != 0) {
                rle_draw(st, buf, mirrored);
                return width;
            }
            uint32_t off = 2u * ((uint32_t) (width * S32(st, G_CLIP_TOP_ROWS)) + U32(st, G_CLIP_LEFT_COLS));
            U32(st, G_MEM_SRC) = off;
            plain_draw(st, buf + off, mirrored);
            return width;
        }
        /* @bin 0x0053EA86 wider than the screen (the 1024 x 768 pictures): the centred screen-sized
         * part ((1024 - W) / 2, (768 - H) / 2; cdq / sar: signed halves) read into the spare screen
         * from the start of the file and copied opaque at (0, 0) */
        int32_t sw = S32(st, G_SCREEN_W);
        int32_t sh = S32(st, G_SCREEN_H);
        S32(st, G_PIC_W) = sw;
        int32_t cx = (0x400 - sw) / 2;
        int32_t cy = (0x300 - sh) / 2;
        S32(st, G_VIS_W) = sw;
        int32_t offset = (int32_t) ((uint32_t) (w * cy + cx) << 1);
        S32(st, G_PIC_X) = cx;
        S32(st, G_PIC_Y) = cy;
        S32(st, G_PIC_H) = sh;
        S32(st, G_VIS_H) = sh;
        uint32_t name = external_name(st, gfx, id);
        for (uint32_t i = 0;; i++) {
            uint8_t c = mem_rd8(st, name + i);
            U8(st, G_EXTERNAL_NAME + i) = c;
            if (c == 0) {
                break;
            }
        }
        external_buffer_free(st, gfx + GFX_EXTERNAL_BUF, 0);
        U32(st, gfx + GFX_EXTERNAL_BUF) = U32(st, G_SPARE_SCREEN);
        S32(st, G_BLIT_Y) = 0;
        S32(st, G_BLIT_X) = 0;
        external_image_read(st, gfx, G_EXTERNAL_NAME, U32(st, gfx + GFX_EXTERNAL_BUF), 2 * S32(st, G_VIS_W), S32(st, G_VIS_H),
                            offset);
        S32(st, G_SRC_SKIP) = 0;
        S32(st, G_DST_SKIP) = 0;
        blit_opaque(st, U32(st, gfx + GFX_EXTERNAL_BUF));
        return S32(st, G_VIS_W);
    }
    uint8_t window = U8(st, G_WINDOW);
    if ((window == 0x15 || window == 0x14) && id == S32(st, G_IMAGE_GROUP(576))) {
        /* @bin 0x0053EBD8 in windows 0x14 / 0x15, image [0x00F56DB4]: the part of it the last wide
         * external image showed (0x00D3B568 .. 0x00D3B574), opaque */
        int32_t vw = S32(st, G_PIC_W);
        S32(st, G_VIS_H) = S32(st, G_PIC_H);
        S32(st, G_DST_SKIP) = (int32_t) ((uint32_t) (screen_w - vw) << 1);
        S32(st, G_VIS_W) = vw;
        S32(st, G_SRC_SKIP) = (int32_t) ((uint32_t) (w - vw) << 1);
        uint32_t src = U32(st, G_MEM_SRC) + 2u * ((uint32_t) (w * S32(st, G_PIC_Y)) + U32(st, G_PIC_X));
        blit_opaque(st, src);
        return S32(st, G_VIS_W);
    }
    if (U8(st, G_IMG_TYPE) == 0x1E) {
        /* @bin 0x0053EC37 isometric: by width, a tile (58) or its 26 / 10 wide variants, else the
         * footprints 2 x 2 .. 6 x 6 */
        switch (w16) {
        case 0x3A:
            S16(st, G_UD3B5BC) = 0x1E;
            break;
        case 0x1A:
            S16(st, G_UD3B5BC) = 0xE;
            break;
        case 0xA:
            S16(st, G_UD3B5BC) = 6;
            break;
        case 0x76:
            return footprint_draw(st, 2);
        case 0xB2:
            return footprint_draw(st, 3);
        case 0xEE:
            return footprint_draw(st, 4);
        case 0x12A:
            return footprint_draw(st, 5);
        case 0x166:
            return footprint_draw(st, 6);
        default:
            break;
        }
    }
    clip_horizontal(st);
    clip_vertical(st);
    uint8_t clip_y = U8(st, G_CLIP_Y);
    if (clip_y == 5) {
        return 0;
    }
    uint8_t type = U8(st, G_IMG_TYPE);
    uint8_t cx = U8(st, G_CLIP_X);
    if (type == 0) {
        if (U8(st, e + 0x33) != 0) {
            rle_draw(st, U32(st, G_MEM_SRC), mirrored);
            uint16_t mask = U16(st, G_TINT);
            if (mask != 0) {
                rle_tint(st, U32(st, G_MEM_SRC), mask);
            }
            return S32(st, G_VIS_W);
        }
        U32(st, G_MEM_SRC) = U32(st, G_MEM_SRC) + plain_offset(st);
        plain_draw(st, U32(st, G_MEM_SRC), mirrored);
        uint16_t mask = U16(st, G_TINT);
        if (mask != 0 && !flipped) {
            plain_tint(st, U32(st, G_MEM_SRC), mask);
        }
        return S32(st, G_VIS_W);
    }
    if (type == 0x1E) {
        /* @bin 0x0053EF14 */
        int16_t width = S16(st, G_UD3B5BE);
        uint32_t src = U32(st, G_MEM_SRC);
        if (width == 0x3A) {
            if (cx == 1) {
                footprint_tile_left(st, src, clip_y);
            } else if (cx == 2) {
                footprint_tile_right(st, src, clip_y);
            } else {
                footprint_tile(st, src, clip_y);
            }
        } else if (width == 0x1A || width == 0xA) {
            GAME_CALL(st, FN_NOTHING, (int32_t) src, clip_y); /* an empty function */
        } else if (cx == 1) {
            blit_plain_clipped(st, src);
        } else if (cx == 2) {
            blit_plain_mirrored_clipped(st, src);
        } else {
            blit_plain_mirrored(st, src);
        }
        return S32(st, G_VIS_W);
    }
    /* @bin 0x0053F0FC types 13, 12, 10, 2 cut horizontally: the fixed-size copies (no clip) */
    if (cx != 0) {
        if (type == 0xD) {
            blit_32x32(st, U32(st, G_MEM_SRC));
            return S32(st, G_VIS_W);
        }
        if (type == 0xC) {
            blit_24x24(st, U32(st, G_MEM_SRC));
            return S32(st, G_VIS_W);
        }
        if (type == 0xA) {
            blit_16x16(st, U32(st, G_MEM_SRC));
            return S32(st, G_VIS_W);
        }
        if (type == 2) {
            blit_type2(st, U32(st, G_MEM_SRC));
            return S32(st, G_VIS_W);
        }
    }
    if (type == 0x14) {
        if (cx == 1) {
            blit_glyph_left(st, U32(st, G_MEM_SRC));
        } else if (cx == 2) {
            blit_glyph_right(st, U32(st, G_MEM_SRC));
        } else {
            blit_glyph(st, U32(st, G_MEM_SRC));
        }
        return S32(st, G_VIS_W);
    }
    /* @bin 0x0053F1AF the other types: plain, opaque when not clipped at all */
    U32(st, G_MEM_SRC) = U32(st, G_MEM_SRC) + plain_offset(st);
    uint32_t src = U32(st, G_MEM_SRC);
    if (mirrored) {
        if (cx == 1 || cx == 2) {
            blit_plain_mirrored_clipped(st, src);
        } else {
            blit_plain_mirrored(st, src);
        }
    } else if (cx == 1 || cx == 2) {
        blit_plain_clipped(st, src);
    } else if (clip_y == 0) {
        blit_opaque(st, src);
    } else {
        blit_plain(st, src);
    }
    return S32(st, G_VIS_W);
}

int32_t image_draw_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x0053E590 thiscall (id, x, y): returns the drawn width (the visible width [0x00D3B544]
     * in most paths, 0 when nothing is drawn) */
    return image_draw_any(st, gfx, id, x, y, 0);
}

int32_t image_draw_flipped_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x0053FA40 thiscall (id, x, y): the image flipped horizontally (the formation icons of
     * 0x005101B0 by orientation) */
    return image_draw_any(st, gfx, id, x, y, 1);
}

int32_t image_draw(game_state *st, int32_t id, int32_t x, int32_t y)
{
    return image_draw_on(st, G_GRAPHICS_GFX, id, x, y);
}

int32_t image_draw_top_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x0053F250 thiscall (id, x, y): the RLE top of an isometric image (has_top +53), which
     * starts after its footprint (+0 data + +8 footprint length), placed by the image's width
     * (n x n footprint: x - 30 (n - 1), y - height + 30 n; its height minus 15 n + 1); tinted by
     * [0x0070E120] in a second pass. Returns 1, 0 when the image has no top. An entry address of 0
     * (the original then reads address 0) is not followed. */
    uint32_t e = entry_at(st, gfx, id);
    if (id <= 0 || id >= IMAGE_ID_END) {
        return 0;
    }
    if (U8(st, e + 0x35) == 0) {
        return 0;
    }
    U32(st, G_MEM_SRC) = U32(st, e);
    if (S32(st, e) <= 0) {
        return 0;
    }
    U32(st, G_MEM_SRC) = U32(st, e) + U32(st, e + 8);
    int16_t w = S16(st, e + 0x14);
    S16(st, G_UD3B5BE) = w;
    int16_t h = S16(st, e + 0x16);
    static const struct {
        int16_t width;
        int16_t dx, dy, dh;
    } place[] = {
        /* @bin 0x0053F2B1 .. 0x0053F3E5 */
        {0x166, -0x96, 0xB4, 0x5B}, {0x12A, -0x78, 0x96, 0x4C}, {0xEE, -0x5A, 0x78, 0x3D},
        {0xB2, -0x3C, 0x5A, 0x2E},  {0x76, -0x1E, 0x3C, 0x1F},  {0x3A, 0, 0x1E, 0x10},
    };
    for (unsigned i = 0; i < sizeof(place) / sizeof(place[0]); i++) {
        if (w == place[i].width) {
            S32(st, G_BLIT_X) = x + place[i].dx;
            S32(st, G_BLIT_Y) = y - h + place[i].dy;
            S16(st, G_UD3B5BC) = (int16_t) (h - place[i].dh);
            break;
        }
    }
    clip_horizontal(st);
    clip_vertical(st);
    if (U8(st, G_CLIP_Y) == 5) {
        return 1;
    }
    uint8_t cx = U8(st, G_CLIP_X);
    if (cx == 1) {
        blit_rle_left(st, U32(st, G_MEM_SRC));
    } else if (cx == 2) {
        blit_rle_right(st, U32(st, G_MEM_SRC));
    } else {
        blit_rle(st, U32(st, G_MEM_SRC));
    }
    uint16_t mask = U16(st, G_TINT);
    if (mask != 0) {
        rle_tint(st, U32(st, G_MEM_SRC), mask);
    }
    return 1;
}

int32_t image_draw_top(game_state *st, int32_t id, int32_t x, int32_t y)
{
    return image_draw_top_on(st, G_GRAPHICS_GFX, id, x, y);
}

int32_t image_draw_pixel_pair_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x0053F4A0 thiscall (id, x, y): the first two pixels of the image's data at (x, y)
     * (the minimap's dots, 0x0043E810, 0x00439980); returns eax as left: the pixels, or the id /
     * data value that stopped it */
    if (id <= 0 || id >= IMAGE_ID_END) {
        return id;
    }
    int32_t i = id >= S32(st, gfx + GFX_IMAGE_COUNT) ? 0 : id;
    uint32_t data = U32(st, ((uint32_t) i << 6) + gfx + GFX_ENTRIES);
    U32(st, G_MEM_SRC) = data;
    if ((int32_t) data <= 0) {
        return (int32_t) data;
    }
    S32(st, G_BLIT_X) = x;
    S32(st, G_BLIT_Y) = y;
    uint32_t v = mem_rd32(st, data);
    pixels_put2(st, data);
    return (int32_t) v;
}

int32_t image_draw_tinted_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y, int32_t mask)
{
    /* @bin 0x0053F4F0 thiscall (id, x, y, mask): only the image's mask pass (0x004100F0 /
     * 0x004101C0 plain, from its first pixel even when clipped; 0x00410700 / 0x004109D0 /
     * 0x00410A70 RLE): the screen ANDed with mask under the image's opaque pixels. Returns the
     * visible width, 0 when nothing is drawn. */
    if (id <= 0 || id >= IMAGE_ID_END) {
        return 0;
    }
    uint32_t e = entry_at(st, gfx, id);
    U32(st, G_MEM_SRC) = U32(st, e);
    if (S32(st, e) <= 0) {
        return 0;
    }
    int16_t w = S16(st, e + 0x14);
    S16(st, G_UD3B5BE) = w;
    S16(st, G_UD3B5BC) = S16(st, e + 0x16);
    U8(st, G_IMG_TYPE) = U8(st, e + 0x32);
    S32(st, G_BLIT_X) = x;
    S32(st, G_BLIT_Y) = y;
    S32(st, G_VIS_W) = w;
    clip_horizontal(st);
    clip_vertical(st);
    if (U8(st, G_CLIP_Y) == 5) {
        return 0;
    }
    uint8_t cx = U8(st, G_CLIP_X);
    if (U8(st, e + 0x33) == 0) {
        if (cx == 1 || cx == 2) {
            blit_plain_mask_clipped(st, U32(st, G_MEM_SRC), mask);
        } else {
            blit_plain_mask(st, U32(st, G_MEM_SRC), mask);
        }
    } else {
        rle_tint(st, U32(st, G_MEM_SRC), mask);
    }
    return S32(st, G_VIS_W);
}

int32_t image_draw_tinted(game_state *st, int32_t id, int32_t x, int32_t y, int32_t mask)
{
    return image_draw_tinted_on(st, G_GRAPHICS_GFX, id, x, y, mask);
}

int32_t image_size_on(game_state *st, uint32_t gfx, int32_t id, int32_t *width, int32_t *height)
{
    /* @bin 0x0053E530 thiscall (id, &width, &height): both 0, then the entry's (movsx) when
     * 0 < id < 0x639C; returns 1 then, else 0 (al) */
    *height = 0;
    *width = 0;
    uint32_t e = entry_at(st, gfx, id);
    if (id <= 0 || id >= IMAGE_ID_END || e == 0) {
        return 0;
    }
    *width = S16(st, e + 0x14);
    *height = S16(st, e + 0x16);
    return 1;
}
