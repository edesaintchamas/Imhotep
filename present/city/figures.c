/* Imhotep - the city view's figure draw 0x00438E50 and its helpers (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A figure is drawn from its image (+0x04) and second image (+0x180: a carried load, a cart, ...)
 * at its tile's screen position plus its offset inside the tile: the movement tables 0x005C95B8..
 * for figures between tiles (+0x0C), the standing offsets 0x005C99B8 / 0x005C9A38 by direction, or
 * for figures inside a building's work cycle (+0x120) the building's place (0x004F0D30). */
#include "city.h"

#include "frame/view.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define FIGURE_DIR 0x00E94E00u    /* i32 the drawn figure's direction relative to the view */
#define NO_FIGURES 0x00978D78u    /* i32 1: figures are not drawn */
#define SELECTED_X 0x0070E3E4u    /* i32 the screen position of the figure drawn by number */
#define SELECTED_Y 0x0070E3E0u
#define VIEW_ORIENT 0x00E94E04u   /* i32 the orientation 0..6 (core/command/rotate.c) */
#define CITY_SIZE 0x49D8u

void building_work_offset(game_state *st, int32_t b, int32_t step, int32_t *dx, int32_t *dy)
{
    /* @bin 0x004F08C0 (building, step, &dx, &dy) where a figure stands at step (+1..) of b's work
     * cycle: 6 steps per stage, the stage's height (0x005E68F8 by +0x54 in mode 0x005E4EF4 = 1, else
     * 90 per stage), the place by the part's direction (0x005E6578 in mode 5, else 0x005E4EF8; turned by
     * the view orientation / 2, 8 when +0x57 < +0x58) and its offsets (0x005F57C8.. in mode 3, else
     * 0x005F5618 and 0x005F5BF8 with +0x68) */
    if (step < 1) {
        *dx = 0;
        *dy = 0xF;
        return;
    }
    uint32_t r = BUILDING_ADDR(b);
    int32_t n = step - 1;
    int32_t h = n / 6;
    if (S32(st, 0x005E4EF4u) == 1) {
        h = S32(st, 0x005E68F8u + 4u * (uint32_t) (h + (int32_t) U8(st, r + 0x54) * 6));
    } else {
        h = h * 0x5A;
    }
    *dx = S32(st, 0x0070E45Cu) + 1;
    *dy = -(S32(st, TILE_H) + h);
    int32_t c;
    if (S32(st, 0x005E4EF4u) == 5) {
        c = S32(st, 0x005E6578u + 8u * (uint32_t) (S16(st, r + 0x12) + (int32_t) U8(st, r + 0x54) * 0x24));
    } else {
        c = S32(st, 0x005E4EF8u + 8u * (uint32_t) ((int32_t) U8(st, r + 0x54) * 0x90 + S16(st, r + 0x12)));
    }
    int32_t half = S32(st, ORIENTATION) / 2;
    if (c < 4) {
        c += half;
        if (c >= 4) {
            c -= 4;
        }
    } else if (c < 8) {
        c += half;
        if (c >= 8) {
            c -= 4;
        }
    }
    if (c < 9) {
        if (U8(st, r + 0x57) < U8(st, r + 0x58)) {
            c = 8;
        }
        int32_t k = n % 6 + c * 6;
        if (S32(st, 0x005E4EF4u) == 3) {
            if (S32(st, r + 0x68) != 0) {
                *dx += S32(st, 0x005F57D0u + 16u * (uint32_t) k);
                *dy += S32(st, 0x005F57D4u + 16u * (uint32_t) k);
            } else {
                *dx += S32(st, 0x005F57C8u + 16u * (uint32_t) k);
                *dy += S32(st, 0x005F57CCu + 16u * (uint32_t) k);
            }
            *dx += 5;
            *dy += 8;
        } else {
            *dx += S32(st, 0x005F5618u + 8u * (uint32_t) k);
            *dy += S32(st, 0x005F561Cu + 8u * (uint32_t) k);
            if (S32(st, r + 0x68) != 0) {
                *dx += S32(st, 0x005F5BF8u + 8u * (uint32_t) c);
                *dy += S32(st, 0x005F5BFCu + 8u * (uint32_t) c);
            }
        }
    }
    *dx += 1;
    *dy += 0x13;
}

static void corner_tile(game_state *st, int32_t b, int32_t *tx, int32_t *ty)
{
    /* the tile of b whose screen position anchors it: +1 in y (orientation 0), x (4), both (6) */
    int32_t x = S16(st, BUILDING_ADDR(b) + 6);
    int32_t y = S16(st, BUILDING_ADDR(b) + 8);
    int32_t o = S32(st, VIEW_ORIENT);
    if (o == 0) {
        y++;
    } else if (o == 4) {
        x++;
    } else if (o == 6) {
        x++;
        y++;
    }
    *tx = x;
    *ty = y;
}

void figure_work_position(game_state *st, int32_t f, int32_t *x, int32_t *y)
{
    /* @bin 0x004F0D30 (figure, &x, &y) the screen position of a figure walking a building's work
     * cycle: from building +0x4A step +0x4C (or the main part of +0x4E when +0x4A is 0 or +0x4E) to
     * building +0x4E step +0x50, +0x52 of +0x54 of the way */
    uint32_t fr = FIGURE_ADDR(f);
    int32_t from = S16(st, fr + 0x4A);
    int32_t to = S16(st, fr + 0x4E);
    int32_t x1, y1, sx, sy;
    if (from == 0 || from == to) {
        int32_t m = to - S16(st, BUILDING_ADDR(to) + 0x12);
        view_tile_to_screen(st, S16(st, BUILDING_ADDR(m) + 0x26), S16(st, BUILDING_ADDR(m) + 0x28), &x1, &y1);
    } else {
        building_work_offset(st, from, S16(st, fr + 0x4C), &x1, &y1);
        int32_t tx, ty;
        corner_tile(st, from, &tx, &ty);
        view_tile_to_screen(st, tx, ty, &sx, &sy);
        x1 += sx;
        y1 += sy;
    }
    int32_t tx, ty, ax, ay;
    corner_tile(st, to, &tx, &ty);
    building_work_offset(st, to, S16(st, fr + 0x50), &ax, &ay);
    view_tile_to_screen(st, tx, ty, &sx, &sy);
    int32_t x2 = ax + sx;
    int32_t y2 = ay + sy;
    int16_t done = S16(st, fr + 0x52);
    int16_t total = S16(st, fr + 0x54);
    if (done < total) {
        *x = (int32_t) ((uint32_t) (x2 - x1) * (uint32_t) (int32_t) done) / total + x1;
        *y = (int32_t) ((uint32_t) (y2 - y1) * (uint32_t) (int32_t) done) / total + y1;
        return;
    }
    *x = x2;
    *y = y2;
}

int32_t figure_in_overlay(game_state *st, int32_t f)
{
    /* @bin 0x004387D0 whether the overlay 0x00E94E0C shows figure f (by its type +0x0A, the jump
     * table 0x00438D20; 0x33 all) */
    uint32_t fr = FIGURE_ADDR(f);
    uint8_t t = U8(st, fr + 0x0A);
    uint32_t city = 0x00EA1870u + CITY_SIZE * U32(st, 0x00EAAC28u);
    switch (S32(st, OVERLAY)) {
    case 4:
        return t == 0x1B;
    case 8:
        return t == 0x0A;
    case 9:
        return t == 0x08;
    case 0x0A: {
        if (t == 0x58 || t == 0x59 || t == 0x16 || t == 0x17 || t == 0x18 || t == 0x0B || t == 0x0C ||
            t == 0x0D || t == 0x0E) {
            return 1;
        }
        if (t != 0x1B) {
            return 0;
        }
        /* @bin 0x004388D4 a tax collector of the palace kind of the city's palace (0x004C5120) */
        int32_t b = S16(st, fr + 0x5A);
        if (S16(st, city + 0x4478) == 0) {
            return 0;
        }
        int32_t palace = S32(st, city + 0x472C);
        int32_t level = GAME_CALL(st, 0x004C5120u, palace);
        if (GAME_CALL(st, 0x004C5120u, b) != level) {
            return 0;
        }
        int32_t k = GAME_CALL(st, 0x004C5120u, palace);
        if (k == 1) {
            return (uint8_t) GAME_CALL(st, 0x004C52D0u, S32(st, city + 0x472C), 1) != 0;
        }
        if (k == 3) {
            return (uint8_t) GAME_CALL(st, 0x004C52D0u, S32(st, city + 0x472C), 2) != 0;
        }
        return 0;
    }
    case 0x0B:
        return t == 0x0F || t == 0x10 || t == 0x11 || t == 0x12 || t == 0x69;
    case 0x0C:
        return t == 0x0F;
    case 0x0D:
        return t == 0x10;
    case 0x0E:
        return t == 0x11;
    case 0x0F:
        return t == 0x12;
    case 0x10:
        return t == 0x69;
    case 0x11:
        return t == 0x1E || t == 0x1D;
    case 0x12:
    case 0x14:
        return t == 0x1D;
    case 0x13:
        return t == 0x1E;
    case 0x15:
        return t == 0x1F;
    case 0x16:
        return t == 0x20;
    case 0x17:
    case 0x30:
        return t == 0x21;
    case 0x18:
        return t == 0x22;
    case 0x19:
        return t == 0x07;
    case 0x1A:
        if (t == 0x1A || t == 0x27 || t == 0x42 || t == 0x19) {
            return 1;
        }
        if (t != 4) {
            return 0;
        }
        {
            uint8_t a = U8(st, fr + 0x0B);
            return a == 1 || a == 3 || a == 6 || a == 2 || a == 5 || a == 4 || a == 8 || a == 7;
        }
    case 0x1D:
        return t == 0x05;
    case 0x1E:
        return t == 0x29 || t == 0x40;
    case 0x1F:
        if (t == 4) {
            return U8(st, fr + 0x38) == 8 && U8(st, fr + 0x66) != 0;
        }
        if (t == 5) {
            return (U32(st, BUILDING_ADDR(S16(st, fr + 0x5A)) + 0x100) & 0x1000) != 0;
        }
        return t == 0x6A || t == 0x6D || t == 0x6B;
    case 0x2B:
        return t == 0x20 || t == 0x62;
    case 0x2D:
        return t == 0x57;
    case 0x2E:
        return t == 1 || t == 2;
    case 0x31:
        switch (t) {
        /* @bin 0x00438DEC the byte table of overlay 0x31 */
        case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D:
        case 0x2E: case 0x2F: case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
        case 0x37: case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x4D:
        case 0x4E: case 0x58: case 0x5C: case 0x5D: case 0x63: case 0x64: case 0x65:
            return 1;
        case 0x1B: {
            int16_t bt = S16(st, BUILDING_ADDR(S16(st, fr + 0x5A)) + 0x10);
            if (bt != 0x3F && bt != 0x44) {
                return 0;
            }
            if (S16(st, city + 0x4478) == 0) {
                return 0;
            }
            return (uint8_t) GAME_CALL(st, 0x004C52D0u, S32(st, city + 0x472C), 2) != 0;
        }
        default:
            return 0;
        }
    case 0x32:
        return t == 0x59;
    case 0x33:
        return 1;
    default:
        return 0;
    }
}

static int32_t entry_s16(game_state *st, int32_t image, uint32_t field)
{
    return S16(st, cv_image_entry(st, image) + field);
}

void figure_draw(game_state *st, int32_t f, int32_t x, int32_t y, int32_t param)
{
    /* @bin 0x00438E50 (figure, tile x, tile y, param: 0, 9999 = overlay figures only, else only that
     * figure, its position kept in 0x0070E3E4 / 0x0070E3E0) */
    uint32_t fr = FIGURE_ADDR(f);
    int32_t img2 = S32(st, fr + 0x180);
    int32_t dy;
    switch (U8(st, fr + 0x0A)) {
    /* @bin 0x004396BC the figure types drawn higher (+5 more with +0x41) */
    case 0x01: case 0x0B: case 0x0C: case 0x0D: case 0x2B: case 0x2C: case 0x2D: case 0x36: case 0x37:
    case 0x38: case 0x52: case 0x53: case 0x54: case 0x63: case 0x66: case 0x67: case 0x68: case 0x6A:
    case 0x6D:
        dy = (U8(st, fr + 0x41) != 0 ? 5 : 0) + 0xC;
        break;
    default:
        dy = 10;
        break;
    }
    int32_t dir = S8(st, fr + 0x11);
    if (dir >= 8) {
        dir = S8(st, fr + 0x12);
        if (dir >= 8) {
            dir = 0;
        }
    }
    dir -= S32(st, ORIENTATION);
    S32(st, FIGURE_DIR) = dir;
    if (dir < 0) {
        dir += 8;
        S32(st, FIGURE_DIR) = dir;
    }
    int32_t img = S32(st, fr + 4);
    if (S32(st, NO_FIGURES) == 1) {
        return;
    }
    int32_t dx = 2;
    if (U8(st, fr + 0x0C) != 0) {
        /* @bin 0x00438F04 between tiles: the offsets 0x005C95B8 / 0x005C96B8 (0x005C97B8 / 0x005C98B8)
         * of the progress (+0x4A % 15, +0x4C % 15), mirrored by the orientation */
        int32_t a = S16(st, fr + 0x4A) % 0xF;
        int32_t b = S16(st, fr + 0x4C) % 0xF;
        uint32_t k = (uint32_t) (a + b * 16);
        switch (S32(st, ORIENTATION)) {
        case 0:
            dx = S8(st, 0x005C95B8u + k) + 2;
            dy += S8(st, 0x005C96B8u + k);
            break;
        case 2:
            dx = S8(st, 0x005C97B8u + k) + 2;
            dy -= S8(st, 0x005C98B8u + k);
            break;
        case 4:
            dx = 2 - S8(st, 0x005C95B8u + k);
            dy -= S8(st, 0x005C96B8u + k);
            break;
        case 6:
            dx = 2 - S8(st, 0x005C97B8u + k);
            dy += S8(st, 0x005C98B8u + k);
            break;
        default:
            break;
        }
        dy -= U8(st, fr + 0x1C);
        if (U8(st, fr + 0x0A) == 0x3C) {
            uint32_t g = FIGURE_ADDR(S16(st, fr + 0x5A));
            if (U8(st, g + 0x0A) == 0x3E) {
                dy += -0x12 - (int32_t) U8(st, g + 0x78);
            }
        }
    } else if (S16(st, fr + 0x120) > 0) {
        figure_work_position(st, f, &x, &y);
    } else {
        /* @bin 0x004390CF standing: 0x005C99B8 / 0x005C9A38 by direction * 16 + +0x39 */
        uint32_t k = (uint32_t) (dir * 16 + S8(st, fr + 0x39));
        dx = S8(st, 0x005C99B8u + k) + 2;
        dy += S8(st, 0x005C9A38u + k) - (int32_t) U8(st, fr + 0x78);
    }
    dy += S32(st, TILE_H);
    dx += S32(st, 0x0070E45Cu);
    int32_t y2 = dy - 10;
    int32_t x2 = dx - 2;
    dx -= entry_s16(st, img, 0x22);
    dy -= entry_s16(st, img, 0x24);
    x2 -= entry_s16(st, img2, 0x22);
    y2 -= entry_s16(st, img2, 0x24);
    if (param != 0) {
        if (param == 9999) {
            if (figure_in_overlay(st, f) == 0) {
                return;
            }
        } else {
            if (f != param) {
                return;
            }
            S32(st, SELECTED_X) = dx + x;
            S32(st, SELECTED_Y) = dy + y;
        }
    }
    if (img2 == 0) {
        cv_image_draw(st, img, dx + x, dy + y);
        return;
    }
    uint8_t t = U8(st, fr + 0x0A);
    int32_t ox = S8(st, fr + 0x6C);
    int32_t oy = S8(st, fr + 0x6D);
    if (t == 4 || t == 0x26 || t == 9) {
        /* @bin 0x004395E3 the load behind (direction 3 after turning) or in front */
        int32_t d = S8(st, fr + 0x11) + S32(st, ORIENTATION);
        if (d >= 8) {
            d -= 8;
        }
        if ((uint32_t) d > 7 || d == 3) {
            cv_image_draw(st, img, dx + x, dy + y);
            cv_image_draw(st, img2, S8(st, fr + 0x6C) + x2 + x, S8(st, fr + 0x6D) + y2 + y);
        } else {
            cv_image_draw(st, img2, S8(st, fr + 0x6C) + x2 + x, S8(st, fr + 0x6D) + y2 + y);
            cv_image_draw(st, img, dx + x, dy + y);
        }
        return;
    }
    if (t == 0x3A || t == 0x0D || t == 0x38 || t == 0x2D || t == 1 || t == 2 || t == 0x6A) {
        /* @bin 0x0043951D the second image before (+0x65 < 0) or after the figure */
        if (oy < 0) {
            cv_image_draw(st, img2, ox + dx + x, oy + dy + y);
            cv_image_draw(st, img, dx + x, dy + y);
        } else {
            cv_image_draw(st, img, dx + x, dy + y);
            cv_image_draw(st, img2, S8(st, fr + 0x6C) + dx + x, S8(st, fr + 0x6D) + dy + y);
        }
        return;
    }
    if (t == 0x0E) {
        /* @bin 0x004391B8 with its record of 0x90 bytes at 0x00BCB9CA + 0x90 * +0x60 */
        uint32_t g = 0x90u * (uint32_t) (int32_t) S16(st, fr + 0x60);
        if (S16(st, 0x00BCBA4Au + g) >= 0) {
            return;
        }
        if (S32(st, 0x00BCBA4Cu + g) == 0) {
            cv_image_draw(st, img, dx + x, dy + y);
            cv_image_draw(st, S32(st, 0x00F56B1Cu) - (S16(st, 0x00BCBA42u + g) + 3) / 5 + 0x14, dx + x, dy + y);
            int32_t h = entry_s16(st, img2, 0x16);
            cv_image_draw(st, img2, dx + x, dy - h + y);
            int32_t k = (int32_t) U8(st, 0x00BCB9CAu + g) + S32(st, 0x00F56814u);
            cv_image_draw(st, k, dx + x, dy - entry_s16(st, k, 0x16) - h + y);
            return;
        }
        int32_t h = entry_s16(st, img2, 0x16);
        cv_image_draw(st, (int32_t) U8(st, 0x00BCB9CAu + g) + S32(st, 0x00F56814u), dx + x, dy - h + y);
        return;
    }
    if (t == 0x24) {
        /* @bin 0x004392FC with a number (+0x0B) */
        if (S16(st, fr + 0x14) < 0 || S16(st, fr + 0x16) < 0) {
            return;
        }
        if (U8(st, fr + 1) & 1) {
            dy--;
        }
        cv_image_draw(st, img, dx + x, dy + y);
        cv_image_draw(st, img2, dx + x, dy - entry_s16(st, img2, 0x16) + y);
        uint8_t a = U8(st, fr + 0x0B);
        int32_t n;
        if (a > 3 && a < 0xC) {
            n = a - 3;
        } else if (a >= 0xE && a <= 0x15) {
            n = a - 0xD;
        } else if (a >= 0x16 && a <= 0x19) {
            n = a - 0x15;
        } else if (a >= 0x1A && a <= 0x1D) {
            n = a - 0x19;
        } else if (a >= 0x1E && a <= 0x20) {
            n = a - 0x1D;
        } else if (a >= 0x32 && a <= 0x41) {
            n = a - 0x31;
        } else {
            return;
        }
        if (n == 0) {
            return;
        }
        GAME_CALL(st, 0x004CCE00u, n, 0x20, 0x0057C048, dx + x + 9, dy + y + 0xE, 0x6B, 0);
        return;
    }
    if (t == 0x4A || t == 0x3C || t == 0x47) {
        cv_image_draw(st, img2, dx + x, dy + y + 0xA);
        cv_image_draw(st, img, dx + x, dy + y);
        return;
    }
    if (t == 0x4D || t == 0x4E) {
        cv_image_draw(st, img, dx + x, dy + y);
        cv_image_draw(st, img2, S8(st, fr + 0x6C) + x2 + x, S8(st, fr + 0x6D) + y2 + y - 0x18);
        return;
    }
    cv_image_draw(st, img, dx + x, dy + y);
}

static int32_t run_draw(game_state *st, const int32_t *a)
{
    figure_draw(st, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_in_overlay(game_state *st, const int32_t *a) { return figure_in_overlay(st, a[0]); }

const ported_function cityview_figure_ports[] = {
    {0x00438E50u, "figure_draw", 4, 0, 0, run_draw, 0},
    {0x004387D0u, "figure_in_overlay", 1, 32, 0, run_in_overlay, 0},
    /* their outputs go through pointers: called directly */
    {0x004F0D30u, "figure_work_position", 3, 0, 0, NULL, 0},
    {0x004F08C0u, "building_work_offset", 4, 0, 0, NULL, 0},
};
const int cityview_figure_ports_count = sizeof(cityview_figure_ports) / sizeof(cityview_figure_ports[0]);
