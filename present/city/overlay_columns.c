/* Imhotep - the overlay tops' columns (package cityview): 0x00431690 and the per-overlay column
 * functions 0x00435290 .. 0x00436140 called by 0x00439F10, and the column 0x004361A0.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A column is a stack of images of the group 0x00F56754 over a building's corner tile, its height a
 * building value scaled to 0..10 (0x004361A0). The building record is 0x108 bytes at 0x007999D8
 * (+5: in use, +0x10: type). */
#include "city.h"

#include "frame/view.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define COLUMN_GROUP 0x00F56754u

int city_overlay_shows(game_state *st, int32_t b)
{
    /* @bin 0x00431690 overlay 0x33: every building; overlay 8: a building with +0x4A set; else its
     * type (movsx) in the zero-terminated i32 list 0x005D25AC[overlay] */
    int32_t overlay = S32(st, OVERLAY);
    if (overlay == 0x33) {
        return 1;
    }
    if (overlay == 8 && S16(st, BUILDING_ADDR(b) + 0x4A) != 0) {
        return 1;
    }
    uint32_t list = U32(st, 0x005D25ACu + 4u * (uint32_t) overlay);
    if (list == 0) {
        return 0;
    }
    for (int32_t t = S32(st, list); t != 0; t = S32(st, list)) {
        if (t == S16(st, BUILDING_ADDR(b) + 0x10)) {
            return 1;
        }
        list += 4;
    }
    return 0;
}

void city_overlay_column(game_state *st, int32_t level, int32_t x, int32_t y, int32_t mode)
{
    /* @bin 0x004361A0 level capped at 10 (signed); the colour: mode 2 -> +0xC, mode 0 -> +0, else by
     * level (>= 9: +9, >= 7: +6, >= 5: +3); images base + 2 (the foot), base (the head, at the
     * entry's +0x16 above the column) and base + 1 (the body, every 10 pixels) */
    if (level > 10) {
        level = 10;
    }
    int32_t colour = 0;
    if (mode == 2) {
        colour = 0xC;
    } else if (mode != 0) {
        if (level >= 9) {
            colour = 9;
        } else if (level >= 7) {
            colour = 6;
        } else if (level >= 5) {
            colour = 3;
        }
    }
    int32_t base = S32(st, COLUMN_GROUP) + colour;
    int32_t head = S16(st, cv_image_entry(st, base) + 0x16);
    if (level == 0) {
        S32(st, DRAW_IMAGE) = base + 2;
        cv_image_draw(st, base + 2, x + 9, y - 8);
        return;
    }
    if (level == 1) {
        cv_image_draw(st, base + 2, x + 9, y - 8);
        cv_image_draw(st, base, x + 5, y - head + 5);
        return;
    }
    cv_image_draw(st, base + 2, x + 9, y - 8);
    cv_image_draw(st, base, x + 5, y - level * 10 - head + 0xF);
    S32(st, DRAW_IMAGE) = S32(st, COLUMN_GROUP) + colour + 1;
    int32_t n = level - 1;
    if (n > 0) {
        int32_t yy = y - 5;
        do {
            cv_image_draw(st, S32(st, DRAW_IMAGE), x + 0x11, yy);
            yy -= 10;
        } while (--n != 0);
    }
}

static void tile_image(game_state *st)
{
    /* the column functions start with the image drawn := the tile's image */
    S32(st, DRAW_IMAGE) = S32(st, IMAGE_GRID + 4u * (uint32_t) S32(st, VIEW_TILE));
}

static int in_use(game_state *st, int32_t b)
{
    return U8(st, BUILDING_ADDR(b) + 5) != 0;
}

/* the simple column functions: a building field scaled to the level, a mode */
enum { F_S32_100, F_S8_10, F_U8_10, F_S8_X17 };
static const struct {
    uint32_t addr;
    uint16_t field;
    uint8_t kind;
    uint8_t mode;
} simple_columns[] = {
    {0x004354B0u, 0xFC, F_S32_100, 1}, /* overlay 0x2B (via 0x00436180) */
    {0x00435510u, 0xE8, F_S32_100, 1}, /* 0x30 (via 0x00436180) */
    {0x00435570u, 0xE4, F_S32_100, 1}, /* 0xA */
    {0x004355D0u, 0x80, F_S8_10, 0},   /* 0xB */
    {0x004356A0u, 0x6C, F_U8_10, 0},   /* 0xC */
    {0x00435700u, 0x6D, F_U8_10, 0},   /* 0xD */
    {0x00435760u, 0x6F, F_U8_10, 0},   /* 0xE */
    {0x004357C0u, 0x71, F_U8_10, 0},   /* 0xF */
    {0x00435820u, 0x86, F_U8_10, 0},   /* 0x10 */
    {0x00435A10u, 0x77, F_U8_10, 2},   /* 0x2D */
    {0x00435A70u, 0x83, F_S8_X17, 0},  /* 4 */
    {0x00435AC0u, 0x72, F_U8_10, 0},   /* 0x12 */
    {0x00435B20u, 0x73, F_U8_10, 0},   /* 0x13 */
    {0x00435B80u, 0x75, F_U8_10, 0},   /* 0x15 */
    {0x00435BE0u, 0x6E, F_U8_10, 0},   /* 0x16 */
    {0x00435C40u, 0x76, F_U8_10, 0},   /* 0x17 */
    {0x00435CA0u, 0x78, F_U8_10, 0},   /* 0x18 */
    {0x00435D00u, 0x70, F_U8_10, 0},   /* 0x32 */
};

static void simple_column(game_state *st, int k, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x004354B0 .. 0x00435D00 (each: the field, its width and divisor in the disassembly) */
    tile_image(st);
    if (!in_use(st, b)) {
        return;
    }
    uint32_t r = BUILDING_ADDR(b) + simple_columns[k].field;
    int32_t level;
    switch (simple_columns[k].kind) {
    case F_S32_100:
        level = S32(st, r) / 100;
        break;
    case F_S8_10:
        level = S8(st, r) / 10;
        break;
    case F_U8_10:
        level = U8(st, r) / 10;
        break;
    default:
        level = S8(st, r) * 0x11;
        break;
    }
    city_overlay_column(st, level, x, y, simple_columns[k].mode);
}

static void water_column(game_state *st, int32_t b, int32_t x, int32_t y, uint32_t field)
{
    /* @bin 0x00435290 / 0x004353A0 (overlays 8, 9): not with +0x38 set; a 0x00488030 building only on
     * its tile of the orientation (draw flags 0x50, 0x40, 0x42, 0x52 for 0, 2, 4, 6); types
     * 0x41 .. 0x45 at the screen position (0x004F0BA0) of +6 / +8 moved by the table 0x005D1FF8
     * [+0xAA >> 1] */
    uint32_t r = BUILDING_ADDR(b);
    if (S16(st, r + 0x38) != 0) {
        return;
    }
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, b) != 0) {
        uint32_t draw = U8(st, TILE_DRAW + (uint32_t) S32(st, VIEW_TILE));
        switch (S32(st, ORIENTATION)) {
        case 0:
            if (draw != 0x50) {
                return;
            }
            break;
        case 2:
            if (draw != 0x40) {
                return;
            }
            break;
        case 4:
            if (draw != 0x42) {
                return;
            }
            break;
        case 6:
            if (draw != 0x52) {
                return;
            }
            break;
        default:
            break;
        }
    }
    int32_t level = S16(st, r + field) / 100;
    int32_t type = S16(st, r + 0x10);
    if (type >= 0x41 && type <= 0x45) {
        uint32_t k = (uint32_t) (U8(st, r + 0xAA) >> 1) * 8u;
        view_tile_to_screen(st, S16(st, r + 6) + S32(st, 0x005D1FF8u + k), S16(st, r + 8) + S32(st, 0x005D1FFCu + k),
                            &x, &y);
    }
    city_overlay_column(st, level, x, y, 1);
}

static void entertainment_column(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00435630 (overlay 0x11) +0x81: 1, 2, 3 -> level 2, 5, 8 */
    tile_image(st);
    if (!in_use(st, b)) {
        return;
    }
    int32_t level;
    switch (U8(st, BUILDING_ADDR(b) + 0x81)) {
    case 1:
        level = 2;
        break;
    case 2:
        level = 5;
        break;
    case 3:
        level = 8;
        break;
    default:
        return;
    }
    city_overlay_column(st, level, x, y, 0);
}

static void goods_column(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00435880 (overlay 0x1A) the goods 1..8 that 0x00443B90 or 0x004439C0 accept: the sum of
     * the building's i16 +0x54[good - 1], as a percentage (0x004CDE60) of +0x1C when the type's
     * 0x0070CDF4 entry (0x58 bytes) is set */
    tile_image(st);
    if (!in_use(st, b)) {
        return;
    }
    uint32_t r = BUILDING_ADDR(b);
    int32_t of = S32(st, 0x0070CDF4u + 0x58u * (uint32_t) S16(st, r + 0x12));
    if (of != 0) {
        of = S16(st, r + 0x1C);
    }
    int32_t sum = 0;
    for (int32_t good = 1; good <= 8; good++) {
        if (GAME_CALL(st, 0x00443B90u, good) != 0 || GAME_CALL(st, 0x004439C0u, good) != 0) {
            sum += S16(st, r + 0x54 + 2u * (uint32_t) (good - 1));
        }
    }
    int32_t v = of != 0 ? GAME_CALL(st, 0x004CDE60u, sum, of) : 0;
    city_overlay_column(st, v / 100, x, y, 0);
}

int32_t city_building_0x46f1c0(game_state *st, int32_t b)
{
    /* @bin 0x0046F1C0 (+0x8C + 5000) / 10000 for a building in use, else 0 */
    if (!in_use(st, b)) {
        return 0;
    }
    return (int32_t) ((uint32_t) S32(st, BUILDING_ADDR(b) + 0x8C) + 0x1388u) / 10000;
}

static void coins(game_state *st, int32_t v, int32_t x, int32_t y)
{
    /* @bin 0x00435DE0 v capped at 200, at least 3 when > 0; v / 3 images of 0x00F56A34 (+ 3 per full
     * four, the rest - 1), 12 pixels apart from (x + 0xF, y + 0xA) up */
    if (v >= 0xC8) {
        v = 0xC8;
    }
    int32_t least = v <= 0 ? 0 : 3;
    if (v <= least) {
        v = least;
    }
    int32_t n = v / 3;
    int32_t yy = y + 0xA;
    while (n > 0) {
        int32_t k = n >= 4 ? 3 : n % 4 - 1;
        cv_image_draw(st, S32(st, 0x00F56A34u) + k, x + 0xF, yy);
        n -= 4;
        yy -= 0xC;
    }
}

static void tax_column(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00435D60 (overlay 0x19) the column of +0x4E * 20 / 100, then the coins of 0x0046F1C0 */
    tile_image(st);
    if (!in_use(st, b)) {
        return;
    }
    city_overlay_column(st, (int32_t) U8(st, BUILDING_ADDR(b) + 0x4E) * 20 / 100, x, y, 0);
    coins(st, city_building_0x46f1c0(st, b), x, y);
}

static void column_2c(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x004360A0 (overlay 0x2C) +0x87 1 / 2: the image 0x00F56724 / 0x00F572E4 at (+3 - 1) tile
     * widths + 0x12 */
    tile_image(st);
    if (!in_use(st, b)) {
        return;
    }
    uint32_t r = BUILDING_ADDR(b);
    uint8_t k = U8(st, r + 0x87);
    int32_t dx = ((int32_t) U8(st, r + 3) - 1) * S32(st, 0x0070E45Cu) + x + 0x12;
    if (k == 1) {
        cv_image_draw(st, S32(st, 0x00F56724u), dx, y);
    } else if (k == 2) {
        cv_image_draw(st, S32(st, 0x00F572E4u), dx, y);
    }
}

static void column_1d(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00436140 (overlay 0x1D) a building NOT in use: +0x18 as a percentage of 300 */
    if (in_use(st, b)) {
        return;
    }
    int32_t v = GAME_CALL(st, 0x004CDE60u, (int32_t) S16(st, BUILDING_ADDR(b) + 0x18), 0x12C);
    city_overlay_column(st, v, x, y, 0);
}

static int32_t run_shows(game_state *st, const int32_t *a) { return city_overlay_shows(st, a[0]); }
static int32_t run_column(game_state *st, const int32_t *a)
{
    city_overlay_column(st, a[0], a[1], a[2], a[3]);
    return 0;
}
#define SIMPLE(k) \
    static int32_t run_simple##k(game_state *st, const int32_t *a) \
    { \
        simple_column(st, k, a[0], a[1], a[2]); \
        return 0; \
    }
SIMPLE(0) SIMPLE(1) SIMPLE(2) SIMPLE(3) SIMPLE(4) SIMPLE(5) SIMPLE(6) SIMPLE(7) SIMPLE(8)
SIMPLE(9) SIMPLE(10) SIMPLE(11) SIMPLE(12) SIMPLE(13) SIMPLE(14) SIMPLE(15) SIMPLE(16) SIMPLE(17)
static int32_t run_water8(game_state *st, const int32_t *a)
{
    water_column(st, a[0], a[1], a[2], 0x48);
    return 0;
}
static int32_t run_water9(game_state *st, const int32_t *a)
{
    water_column(st, a[0], a[1], a[2], 0x46);
    return 0;
}
static int32_t run_ent(game_state *st, const int32_t *a)
{
    entertainment_column(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_goods(game_state *st, const int32_t *a)
{
    goods_column(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_tax(game_state *st, const int32_t *a)
{
    tax_column(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_coins(game_state *st, const int32_t *a)
{
    coins(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_46f1c0(game_state *st, const int32_t *a) { return city_building_0x46f1c0(st, a[0]); }
static int32_t run_2c(game_state *st, const int32_t *a)
{
    column_2c(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_1d(game_state *st, const int32_t *a)
{
    column_1d(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function cityview_overlay_column_ports[] = {
    {0x00431690u, "city_overlay_shows", 1, 8, 0, run_shows, 0},
    {0x004361A0u, "city_overlay_column", 4, 0, 0, run_column, 0},
    {0x00436180u, "city_overlay_column_436180", 4, 0, 0, run_column, 0},
    {0x004354B0u, "overlay_column_2b", 3, 0, 0, run_simple0, 0},
    {0x00435510u, "overlay_column_30", 3, 0, 0, run_simple1, 0},
    {0x00435570u, "overlay_column_0a", 3, 0, 0, run_simple2, 0},
    {0x004355D0u, "overlay_column_0b", 3, 0, 0, run_simple3, 0},
    {0x004356A0u, "overlay_column_0c", 3, 0, 0, run_simple4, 0},
    {0x00435700u, "overlay_column_0d", 3, 0, 0, run_simple5, 0},
    {0x00435760u, "overlay_column_0e", 3, 0, 0, run_simple6, 0},
    {0x004357C0u, "overlay_column_0f", 3, 0, 0, run_simple7, 0},
    {0x00435820u, "overlay_column_10", 3, 0, 0, run_simple8, 0},
    {0x00435A10u, "overlay_column_2d", 3, 0, 0, run_simple9, 0},
    {0x00435A70u, "overlay_column_04", 3, 0, 0, run_simple10, 0},
    {0x00435AC0u, "overlay_column_12", 3, 0, 0, run_simple11, 0},
    {0x00435B20u, "overlay_column_13", 3, 0, 0, run_simple12, 0},
    {0x00435B80u, "overlay_column_15", 3, 0, 0, run_simple13, 0},
    {0x00435BE0u, "overlay_column_16", 3, 0, 0, run_simple14, 0},
    {0x00435C40u, "overlay_column_17", 3, 0, 0, run_simple15, 0},
    {0x00435CA0u, "overlay_column_18", 3, 0, 0, run_simple16, 0},
    {0x00435D00u, "overlay_column_32", 3, 0, 0, run_simple17, 0},
    {0x00435290u, "overlay_column_08", 3, 0, 0, run_water8, 0},
    {0x004353A0u, "overlay_column_09", 3, 0, 0, run_water9, 0},
    {0x00435630u, "overlay_column_11", 3, 0, 0, run_ent, 0},
    {0x00435880u, "overlay_column_1a", 3, 0, 0, run_goods, 0},
    {0x00435D60u, "overlay_column_19", 3, 0, 0, run_tax, 0},
    {0x00435DE0u, "overlay_coins", 3, 0, 0, run_coins, 0},
    {0x0046F1C0u, "building_0x46f1c0", 1, 32, 0, run_46f1c0, 0},
    {0x004360A0u, "overlay_column_2c", 3, 0, 0, run_2c, 0},
    {0x00436140u, "overlay_column_1d", 3, 0, 0, run_1d, 0},
};
const int cityview_overlay_column_ports_count =
    sizeof(cityview_overlay_column_ports) / sizeof(cityview_overlay_column_ports[0]);
