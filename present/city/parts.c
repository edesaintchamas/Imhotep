/* Imhotep - the building parts the city view's upper pass 0x00436B60 draws (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The animated parts of buildings whose image animates (0x00438210 .. 0x00438770: a frame of an
 * image group at fixed offsets), the parts placed by the descriptor tables 0x005BE9B8 (0x0041F910,
 * 0x0041FAD0 and the table walks 0x0041FFF0, 0x00420050, 0x004206F0, 0x00420A30, 0x00420D60,
 * 0x00421060, 0x00421920, 0x004E5E90), and the other special parts (0x00401F70, 0x00542E10,
 * 0x004C0160, the 0xB7 tops 0x004EF550). State writes: 0x004206F0 copies a descriptor set into
 * 0x00596150 (flag 0x00704DC0); 0x004EF550 links a 0xB7 building to its neighbour (+0x6C..+0x7C);
 * 0x004C0160 the tint 0x0070E120. */
#include <string.h>

#include "city.h"

#include "frame/view.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define FIGURE_DRAW 0x00438E50u   /* the figure draw (figure, x, y, param) */
#define GOD_INDEX 0x004C53D0u     /* (kind): a god's index, -1 none */
#define PART_DESC 0x005BE9B8u     /* 0xFD descriptor sets of 0x28 bytes */
#define PART_SET_FLAG 0x00704DC0u /* i32 0 / 2: which descriptor set 0x00596150 holds */
#define MONUMENT_MODE 0x005E4EF4u /* i32 (1, 3, 5: the kind of the 0xB7 monument's tables) */

static int32_t entry_dx(game_state *st, int32_t image)
{
    /* @bin 0x0041FA87 the image entry's +0x22 (ids from the count 0x0126517C: entry 0) */
    return S16(st, cv_image_entry(st, image) + 0x22);
}

static int32_t entry_dy(game_state *st, int32_t image)
{
    return S16(st, cv_image_entry(st, image) + 0x24);
}

/* ---- the descriptor parts 0x0041F910 / 0x0041FAD0 ------------------------------------------- */

/* @bin 0x0041F910 / 0x0041FAD0 (thiscall on a part object: +0 set < 0xFD, +4 step, +8 progress,
 * +0x10 i16 (3: none), +0x15 u8 (alternate images)) the part's image and its offset for the
 * direction dir: the set 0x005BE9B8 + 0x28 * set (+0 points, +4 count, +8 groups, +0xC count,
 * +0x10 steps of 0x30 bytes, +0x14 count, +0x20 / +0x24 offset); the step's +0 / +4 points (per
 * direction, 8 bytes) interpolated by progress / +0xC; the image: group (+0 group, +4 first,
 * +8 frames, +0xC frame step) at progress % frames, plus the step's per-direction image +0x10
 * (+0x20 alternate); minus the image's entry offsets +0x22 / +0x24. All 0 when out of range. */
static void part_pos(game_state *st, uint32_t obj, int32_t dir, int32_t *ox, int32_t *oy, int32_t *img)
{
    *ox = 0;
    *oy = 0;
    *img = 0;
    if (S16(st, obj + 0x10) == 3) {
        return;
    }
    int32_t set = S32(st, obj);
    if (set < 0 || set >= 0xFD) {
        return;
    }
    uint32_t d = PART_DESC + 0x28u * (uint32_t) set;
    int32_t step = S32(st, obj + 4);
    if (step < 0 || step >= S32(st, d + 0x14)) {
        return;
    }
    uint32_t s = U32(st, d + 0x10) + 0x30u * (uint32_t) step;
    int32_t extra = (int32_t) (U8(st, obj + 0x15) != 0 ? P32(st, s + 0x20 + 4u * (uint32_t) dir)
                                                       : P32(st, s + 0x10 + 4u * (uint32_t) dir));
    if (extra < 0) {
        return;
    }
    int32_t group = (int32_t) P32(st, s + 8);
    if (group < 0 || group >= S32(st, d + 0xC)) {
        return;
    }
    uint32_t g = U32(st, d + 8) + 0x10u * (uint32_t) group;
    int32_t a = (int32_t) P32(st, s);
    if (a < 0 || a >= S32(st, d + 4)) {
        return;
    }
    int32_t b = (int32_t) P32(st, s + 4);
    if (b < 0 || b >= S32(st, d + 4)) {
        return;
    }
    uint32_t pa = U32(st, d) + 8u * (uint32_t) (dir + a * 4);
    uint32_t pb = U32(st, d) + 8u * (uint32_t) (dir + b * 4);
    int32_t t = S32(st, obj + 8);
    int32_t span = (int32_t) P32(st, s + 0xC);
    if (t < 0 || t > span) {
        return;
    }
    /* @bin 0x0041FA18 idiv: the frame, then the offsets (32-bit products, truncating divisions) */
    int32_t frame = t % (int32_t) P32(st, g + 8);
    *img = (int32_t) ((uint32_t) frame * P32(st, g + 0xC) +
                      (uint32_t) S32(st, GFX + 4u + 8u * P32(st, g)) + P32(st, g + 4) + (uint32_t) extra);
    int32_t ax = (int32_t) P32(st, pa);
    *ox = (int32_t) ((uint32_t) ((int32_t) ((uint32_t) ((int32_t) P32(st, pb) - ax) * (uint32_t) t) / span) +
                     (uint32_t) S32(st, d + 0x20) + (uint32_t) ax);
    int32_t ay = (int32_t) P32(st, pa + 4);
    *oy = (int32_t) ((uint32_t) ((int32_t) ((uint32_t) ((int32_t) P32(st, pb + 4) - ay) * (uint32_t) t) / span) +
                     (uint32_t) S32(st, d + 0x24) + (uint32_t) ay);
    *ox -= entry_dx(st, *img);
    *oy -= entry_dy(st, *img);
}

static int32_t view_dir(game_state *st)
{
    /* @bin 0x0041F956 the orientation / 2 (cdq; sub; sar) */
    return S32(st, ORIENTATION) / 2;
}

static void parts_walk(game_state *st, uint32_t start, uint32_t end, int32_t x, int32_t y)
{
    /* @bin 0x0041FFF0 (and 0x00420050, 0x004206F0, 0x00420A30, 0x00420D60, 0x00421060) each entry
     * (16 bytes, +0: the part object) of [start, end): its image at (x, y) + its offset */
    for (uint32_t e = start; e < end; e += 0x10) {
        int32_t ox, oy, img;
        part_pos(st, U32(st, e), view_dir(st), &ox, &oy, &img);
        if (img != 0) {
            cv_image_draw(st, img, ox + x, oy + y);
        }
    }
}

void cv_parts_41fff0(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0041FFF0 the parts of type 0xD2 (table 0x005C2750 .. 0x005C28B0) */
    parts_walk(st, 0x005C2750u, 0x005C28B0u, x, y);
}

void cv_parts_420050(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00420050 the parts of type 0xD5 (table 0x00596088 .. 0x00596158) */
    parts_walk(st, 0x00596088u, 0x00596158u, x, y);
}

int32_t cv_part_set_odd(game_state *st)
{
    /* @bin 0x0041F8B0 (0x00704DC0 / 2) & 1 */
    return (S32(st, PART_SET_FLAG) / 2) & 1;
}

void cv_part_set_select(game_state *st, int32_t odd)
{
    /* @bin 0x0041F8C0 0 or 4: the set 0x00596F30, flag 0; else the set 0x00597D10, flag 2; copied
     * (0x378 dwords) into 0x00596150 */
    uint32_t src;
    if (odd != 0 && odd != 4) {
        S32(st, PART_SET_FLAG) = 2;
        src = 0x00597D10u;
    } else {
        S32(st, PART_SET_FLAG) = 0;
        src = 0x00596F30u;
    }
    memmove(state_ptr(st, 0x00596150u, 0x378u * 4), state_ptr(st, src, 0x378u * 4), 0x378u * 4);
}

void cv_parts_4206f0(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004206F0 the parts of type 0xE1: the descriptor set of the orientation's parity first
     * (0x0041F8C0), then the table 0x0059EDF0 .. 0x0059EFA0 */
    int32_t odd = (S32(st, ORIENTATION) / 2) & 1;
    if (odd != cv_part_set_odd(st)) {
        cv_part_set_select(st, odd);
    }
    parts_walk(st, 0x0059EDF0u, 0x0059EFA0u, x, y);
}

void cv_parts_420a30(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00420A30 the parts of type 0xE6 (table 0x005A8148 .. 0x005A84C8) */
    parts_walk(st, 0x005A8148u, 0x005A84C8u, x, y);
}

void cv_parts_420d60(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00420D60 the parts of type 0xE3 (table 0x005AD218 .. 0x005AD3C8) */
    parts_walk(st, 0x005AD218u, 0x005AD3C8u, x, y);
}

void cv_parts_421060(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00421060 the parts of type 0xE4 (table 0x005B0060 .. 0x005B0150) */
    parts_walk(st, 0x005B0060u, 0x005B0150u, x, y);
}

void cv_parts_421920(game_state *st, int32_t x, int32_t y, int32_t b)
{
    /* @bin 0x00421920 the parts of a building of type 0xE5, 0xEA..0xEC by its +0x68 (jump table
     * 0x00422370: 0 .. 0x1D, three entries of 16 bytes each); an entry is drawn when its +0
     * (at the table's -8) is the building's +0x60; the part's direction (0x0041FAD0) is the
     * building's +0x90 less the orientation / 2 */
    static const uint32_t tables[0x1E] = {
        0x005BE600u, 0x005BE510u, 0x005BE540u, 0x005BE570u, 0x005BE750u, 0x005BE780u, 0x005BE630u, 0x005BE7B0u,
        0x005BE7E0u, 0x005BE660u, 0x005BE690u, 0x005BE8A0u, 0x005BE810u, 0x005BE8D0u, 0x005BE900u, 0x005BE5A0u,
        0x005BE5D0u, 0, 0, 0x005BE6C0u, 0x005BE6F0u, 0x005BE720u, 0x005BE840u, 0x005BE870u,
        0x005BE930u, 0x005BE960u, 0x005BE990u, 0, 0, 0x005BE510u,
    };
    uint32_t r = BUILDING_ADDR(b);
    uint32_t k = U32(st, r + 0x68);
    if (k > 0x1D || tables[k] == 0) {
        return;
    }
    for (uint32_t e = tables[k]; e < tables[k] + 0x30; e += 0x10) {
        if (U32(st, r + 0x60) != U32(st, e - 8)) {
            continue;
        }
        /* @bin 0x0041FB41 the direction: +0x90 - orientation / 2, + 4 when negative */
        int32_t dir = S32(st, r + 0x90) - view_dir(st);
        if (dir < 0) {
            dir += 4;
        }
        int32_t ox, oy, img;
        part_pos(st, U32(st, e), dir, &ox, &oy, &img);
        if (img != 0) {
            cv_image_draw(st, img, ox + x, oy + y);
        }
    }
}

/* ---- the animated parts (upper pass 0x0043773E) ---------------------------------------------- */

static void frame_at(game_state *st, uint32_t group_ptr, int32_t frame, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
    /* a frame of the image group whose first image is at group_ptr (0x00F55BB4 + 8 * group), at
     * (x + dx - entry +0x22, y + dy - entry +0x24) */
    cv_image_draw_offset(st, S32(st, group_ptr) - 1 + frame, x, y, dx, dy);
}

int cv_anim_4a(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y, int32_t v)
{
    /* @bin 0x00438210 type 0x4A by its +0xB4 v (0: nothing): 0x1000 | 0x19: group 0x277, 0x1000 |
     * 0x4D: 0x275; 0x2000 | figure: nothing unless its +0x0E = 1 and +0x38 != 4, 0x275 for a
     * figure of type 0x4D; else 0x26E. The offsets by the image's direction (image - group 0x20B,
     * the stack table 0x00438252); 0 when the direction is outside it (the original reads other
     * stack words there) */
    static const int32_t offsets[4][2] = {{0x6E, 0x14}, {0x55, 0x14}, {0x55, 0x14}, {100, 10}};
    if (v == 0) {
        return 1;
    }
    int32_t lo = v & 0xFFF;
    int32_t group = 0x26E;
    if ((v & 0xF000) == 0x1000) {
        if (lo == 0x19) {
            group = 0x277;
        } else if (lo == 0x4D) {
            group = 0x275;
        }
    } else if ((v & 0xF000) == 0x2000) {
        uint32_t f = FIGURE_ADDR(lo);
        if (U8(st, f + 0x0E) != 1 || U8(st, f + 0x38) == 4) {
            return 1;
        }
        if (U8(st, f + 0x0A) == 0x4D) {
            group = 0x275;
        }
    }
    int32_t dir = img - S32(st, 0x00F56C0Cu);
    if (dir < 0 || dir > 3) {
        return 0;
    }
    frame_at(st, GFX + 4u + 8u * (uint32_t) group, frame, x, y, offsets[dir][0], offsets[dir][1]);
    return 1;
}

int cv_anim_4c(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y, int32_t f)
{
    /* @bin 0x00438350 type 0x4C: the direction image - group 0x203; group 0x278, or 0x26F and the
     * second half of the offsets when the figure +0xB4 f is in action 0xC (+0x38); the image
     * group + 4 * frame + (direction - 1) & 3 - 4 (the stack table 0x00438399) */
    static const int32_t offsets[8][2] = {{0x4B, 0xA}, {0x4E, 7}, {0x3E, 0x12}, {0x30, 8},
                                          {0x4B, 0xA}, {0x4E, 7}, {0x3E, 0x12}, {0x30, 8}};
    int32_t dir = img - S32(st, 0x00F56BCCu);
    int32_t prev = (dir - 1) & 3;
    int32_t group = 0x278;
    if (f != 0 && U8(st, FIGURE_ADDR(f) + 0x38) == 0xC) {
        group = 0x26F;
        dir += 4;
    }
    if (dir < 0 || dir > 7) {
        return 0;
    }
    int32_t image = (int32_t) ((uint32_t) S32(st, GFX + 4u + 8u * (uint32_t) group) + (uint32_t) frame * 4 +
                               (uint32_t) prev - 4);
    cv_image_draw_offset(st, image, x, y, offsets[dir][0], offsets[dir][1]);
    return 1;
}

void cv_anim_a1(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x00438470 types 0xA1, 0xA2, 0xD9: group 0x271 at (0x36, 0xF) */
    frame_at(st, 0x00F56F3Cu, frame, x, y, 0x36, 0xF);
}

void cv_anim_6a(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x004384D0 types 0x6A, 0x6B, 0xD8, 0xDD: group 0x272 at (0x5A, 0x11), group 0x273 at
     * (0x28, 0x14) */
    frame_at(st, 0x00F56F44u, frame, x, y, 0x5A, 0x11);
    frame_at(st, 0x00F56F4Cu, frame, x, y, 0x28, 0x14);
}

void cv_anim_1e(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x00438590 type 0x1E: the image of group 0x16A (0x00F566FC): group 0x24A at (0x34, -3);
     * else group 0x24B at (0x14, 0) */
    if (img == S32(st, 0x00F566FCu)) {
        frame_at(st, 0x00F56E04u, frame, x, y, 0x34, -3);
    } else {
        frame_at(st, 0x00F56E0Cu, frame, x, y, 0x14, 0);
    }
}

void cv_anim_22(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x00438650 type 0x22: group 0x24B at (0x52, 0xE) */
    frame_at(st, 0x00F56E0Cu, frame, x, y, 0x52, 0xE);
}

void cv_anim_1f(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x004386B0 type 0x1F: group 0x248 at (0x1D, 0xE) */
    frame_at(st, 0x00F56DF4u, frame, x, y, 0x1D, 0xE);
}

void cv_anim_21(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x00438710 type 0x21: group 0x247 at (0x40, 0) */
    frame_at(st, 0x00F56DECu, frame, x, y, 0x40, 0);
}

void cv_anim_23(game_state *st, int32_t frame, int32_t x, int32_t y)
{
    /* @bin 0x00438770 type 0x23: group 0x247 at (0x6C, 1) */
    frame_at(st, 0x00F56DECu, frame, x, y, 0x6C, 1);
}

/* ---- the other special parts ------------------------------------------------------------------ */

void cv_part_e6(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00401F70 (thiscall on 0x006DE650, unused) type 0xE6: the 12 flags +0x61.. of the
     * building's main part (b - +0x12), in rows of 3: images 0x00F57584 (rows 0, 1) or 0x00F5758C,
     * + (orientation / 2) & 1, at the stack table's offsets (0x00401FF8): orientation 0 at
     * (+0xF3, -0x63), orientation 6 also the row's and the column's shifts (-0x19B, -0x127); other
     * orientations: nothing */
    static const int32_t t[31] = {0x30, 0xCB, 0x153, 0, 0, 0x1E, 0x46, 0, 0, 0x9E, -0x4F, 0x129, -0x91, 0,
                                  -0x3C, 0x9E, -0x8B, 0x129, -0xCD, -0xF, -0xAA, 0x8F, -0xF9, 0x11A, -0x13B,
                                  -0x23, -0x11D, 0x7B, -0x16C, 0x106, -0x1AE};
    int32_t dir = S32(st, ORIENTATION) / 2;
    uint32_t flags = BUILDING_ADDR(b - S16(st, BUILDING_ADDR(b) + 0x12)) + 0x61;
    for (int32_t row = 0; row < 0xC; row += 3) {
        for (int32_t col = 0; col < 3; col++) {
            if (U8(st, flags + (uint32_t) row + (uint32_t) col) == 0) {
                continue;
            }
            int32_t img = (row <= 3 ? S32(st, 0x00F57584u) : S32(st, 0x00F5758Cu)) + (dir & 1);
            int32_t k = (row + col) * 2;
            if (dir == 0) {
                cv_image_draw(st, img, t[k + 7] + x + 0xF3, t[k + 8] + y - 0x63);
            } else if (dir == 3) {
                cv_image_draw(st, img, t[k + 7] + t[3 + row / 3] + x - 0x19B, t[k + 8] + t[col] + y - 0x127);
            }
        }
    }
}

void cv_part_d5(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00542E10 type 0xD5: by the building's +0x58 (jump table 0x00542EA0) 1..4 images
     * 0x00F56D74 + 0xD / + 0xE alternately at the offsets 0x00603238 (8 bytes each) */
    int32_t n;
    switch (U32(st, BUILDING_ADDR(b) + 0x58)) {
    case 1: case 0xA: case 0xB:
        n = 1;
        break;
    case 2: case 8: case 9:
        n = 2;
        break;
    case 3: case 6: case 7:
        n = 3;
        break;
    case 4: case 5:
        n = 4;
        break;
    default:
        n = 0;
        break;
    }
    for (int32_t i = 0; i < n; i++) {
        int32_t img = (i & 1) ? S32(st, 0x00F56D74u) + 0xE : S32(st, 0x00F56D74u) + 0xD;
        uint32_t o = 0x00603238u + 8u * (uint32_t) i;
        cv_image_draw(st, img, x + S32(st, o), S32(st, o + 4) + y);
    }
}

void cv_part_cf(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x004E5E90 types 0xCF / 0xD0 (nothing when +0x55 >= 100). The level p from +0x55 v:
     * 0 below 0x21, 3v - 0x63 below 0x42, else 3 (100 - v); at most 100. Images 0x00F56D74 while
     * the float sum of steps 100 / n * 0.5 (n = 2 for 0xCF, else 4: 25.0 or 12.5, exact in
     * float, as are the sums and p) stays below p: counted here in halves (step 50 or 25 against
     * 2p). Then the parts of the tables 0x00586B08 (0xCF, 10 entries) / 0x0058FE48 (18) */
    uint32_t r = BUILDING_ADDR(b);
    int32_t v = U8(st, r + 0x55);
    if (v >= 100) {
        return;
    }
    int32_t p = 0;
    if (v >= 0x42) {
        p = (100 - v) * 3;
    } else if (v >= 0x21) {
        p = v * 3 - 0x63;
    }
    if (p >= 100) {
        p = 100;
    }
    int cf = S16(st, r + 0x10) == 0xCF;
    int32_t dx = cf ? 0x5A : 0x98;
    int32_t dy = cf ? 0x12 : 0xE;
    int32_t w = cf ? 1 : 6;
    /* @bin 0x004E5F3C fild n; fdivr 100.0 (0x0056F6DC); fmul 0.5 (0x0056F6D8); the loop
     * 0x004E5F8C: draw, sum += step (fstp to float), while sum < p (fcomp); first test 0.0 < p */
    int32_t half_step = cf ? 50 : 25;
    int32_t img = S32(st, 0x00F56D74u);
    x += dx;
    y += dy;
    if (0 < p) {
        int32_t sum = 0;
        int32_t i = 0;
        do {
            int32_t ox = (i & 1) ? w : -0x3C - w;
            cv_image_draw(st, img, ox + x, -(w / 2) - (i / 2 + 1) * 0x3C + y);
            sum += half_step;
            i++;
        } while (sum < 2 * p);
    }
    for (int32_t i = 0; i < (S16(st, r + 0x10) != 0xCF ? 0x12 : 0xA); i++) {
        uint32_t table = S16(st, r + 0x10) == 0xCF ? 0x00586B08u : 0x0058FE48u;
        int32_t ox, oy, part;
        part_pos(st, U32(st, table + 0x10u * (uint32_t) i + 8), view_dir(st), &ox, &oy, &part);
        if (part != 0) {
            cv_image_draw(st, part, ox + x, oy + y);
        }
    }
}

void cv_part_ca(game_state *st, int32_t b)
{
    /* @bin 0x004C0160 type 0xCA: two images at the tiles of the building's (+6, +8) shifted by the
     * table 0x005DF78C (by +0xAA + orientation, less 8 from 8 up: entries 2 and 3, each an index
     * into 0x005DF814 (16 bytes: +0 a shift, +4 / +8 x / y by +0xAA)), placed by 0x004F0BA0;
     * entry 2: image 0x00F56AFC + the variant, at (x + dx, y + dy) by the direction's jump table
     * 0x004C03A8; entry 3: the god group 0x005DF778[god 2, else god 1, else 0] + the variant.
     * Tinted 0xF863 when the building's +0 is 6 */
    uint32_t r = BUILDING_ADDR(b);
    int32_t dx = 0x40;
    int32_t dyv = -0x5A;
    int32_t tx = S16(st, r + 6);
    int32_t o = (int32_t) U8(st, r + 0xAA) + S32(st, ORIENTATION);
    int32_t ty = S16(st, r + 8);
    int32_t god = GAME_CALL(st, GOD_INDEX, 2);
    if (god == -1) {
        god = GAME_CALL(st, GOD_INDEX, 1);
        if (god == -1) {
            god = 0;
        }
    }
    int32_t group = S32(st, 0x005DF778u + 4u * (uint32_t) god);
    if (o >= 8) {
        o -= 8;
    }
    uint32_t row = 0x005DF78Cu + 16u * (uint32_t) o;
    for (int32_t k = 2; k < 4; k++) {
        int32_t e = S32(st, row + 4u * (uint32_t) k);
        int32_t variant = 0;
        switch ((uint32_t) o) {
        case 0: case 3: case 4: case 7:
            dx = -1;
            dyv = -0x41;
            variant = 1;
            break;
        case 1: case 2: case 5: case 6:
            dx = 0xC;
            dyv = -0x40;
            break;
        default:
            break;
        }
        uint32_t t = 0x005DF814u + 16u * (uint32_t) e;
        uint8_t side = U8(st, r + 0xAA);
        int32_t ax = tx, ay = ty;
        if (side == 0) {
            ax = S32(st, t + 8) + tx;
            ay = S32(st, t + 4) + ty;
        } else if (side == 1) {
            ax = S32(st, t + 4) + tx;
            ay = S32(st, t + 8) + ty;
        }
        int32_t img, sx = 0, sy = b; /* the original's outputs: a stack word and b's slot */
        if (k == 2) {
            img = S32(st, 0x00F56AFCu) + variant;
            view_tile_to_screen(st, ax, ay, &sx, &sy);
            if (U8(st, r) == 6) {
                U16(st, TINT) = 0xF863;
            }
            cv_image_draw(st, img, sx + dx, sy + dyv);
            sx += dx;
            sy += dyv;
        } else {
            int32_t orient = S32(st, ORIENTATION);
            if (orient != 0) {
                int32_t shift = S32(st, t);
                if (orient == 2) {
                    ax = ax + shift - 1;
                } else if (orient == 4) {
                    ax = ax + shift - 1;
                    ay = ay + shift - 1;
                } else {
                    ay = ay + shift - 1;
                }
            }
            img = S32(st, GFX + 4u + 8u * (uint32_t) group) + variant;
            view_tile_to_screen(st, ax, ay, &sx, &sy);
            if (U8(st, r) == 6) {
                U16(st, TINT) = 0xF863;
            }
            cv_image_draw(st, img, sx, sy);
        }
        cv_image_draw_top(st, img, sx, sy);
        U16(st, TINT) = 0;
    }
}

/* ---- the 0xB7 tops 0x004EF550 ----------------------------------------------------------------- */

static void figures_on(game_state *st, uint32_t r, int32_t x, int32_t y, int which, int32_t *last_tile)
{
    /* @bin 0x004EF782 the figures on the building's 4 tiles (+0x0C + the offsets 0x00570084: 0, 1,
     * 0xE4, 0xE5), in list order (+8 next), drawn at (x, y) by their +0x120: which 0: = 3; 1: != 0;
     * 2: > 0 and != 3 */
    for (uint32_t o = 0x00570084u; o < 0x00570094u; o += 4) {
        int32_t tile = S32(st, r + 0x0C) + S32(st, o);
        if (last_tile) {
            *last_tile = tile;
        }
        for (int32_t f = S16(st, FIGURE_GRID + 2u * (uint32_t) tile); f > 0; f = S16(st, FIGURE_ADDR(f) + 8)) {
            int16_t v = S16(st, FIGURE_ADDR(f) + 0x120);
            if ((which == 0 && v == 3) || (which == 1 && v != 0) || (which == 2 && v > 0 && v != 3)) {
                GAME_CALL(st, FIGURE_DRAW, f, x, y, 0);
            }
        }
    }
}

static int32_t mod6_index(uint8_t v, int five_keeps, int32_t keep)
{
    /* @bin 0x004EF90E the jump tables 0x004EFD3C .. 0x004EFD78 on v % 6 (1..5): 2 -> 1, 4 -> 2,
     * 5 -> 3 (0x004EFD50: 5 keeps the previous value, 3), else 0 */
    switch (v % 6) {
    case 2:
        return 1;
    case 4:
        return 2;
    case 5:
        return five_keeps ? keep : 3;
    default:
        return 0;
    }
}

static void b7_variants(game_state *st, uint32_t r, int32_t kind, int32_t *v1, int32_t *v2, int second)
{
    /* @bin 0x004EF8C1 / 0x004EFADE the variants of the two layers (+0x59, +0x5A): in monument mode
     * 3, by the part's kind 0x005E4EF8[8 * (+0x12 + 0x90 * kind)]: below 4, the mod-6 tables; else
     * (v - 1) % 3 (signed); other modes 0 */
    if (S32(st, MONUMENT_MODE) != 3) {
        *v1 = 0;
        *v2 = 0;
        return;
    }
    if (S32(st, 0x005E4EF8u + 8u * (uint32_t) (S16(st, r + 0x12) + kind * 0x90)) < 4) {
        *v1 = mod6_index(U8(st, r + 0x59), 0, 0);
        /* the partner's +0x5A table 0x004EFD50 keeps 3 for 5; the building's own 0x004EFD78 sets 3 */
        *v2 = mod6_index(U8(st, r + 0x5A), second == 0, 3);
    } else {
        *v1 = ((int32_t) U8(st, r + 0x59) - 1) % 3;
        *v2 = ((int32_t) U8(st, r + 0x5A) - 1) % 3;
    }
}

static void b7_layers(game_state *st, uint32_t r, int32_t v1, int32_t v2, int32_t x, int32_t y)
{
    /* @bin 0x004EF993 the two layer images +0x68 + variant (when +0x59 / +0x5A are set) at
     * +0x5C/+0x5E and +0x60/+0x62, shifted in mode 3 by 0x005F5B68[image - 0x00F57334 - 0x6D] */
    int32_t base = S32(st, r + 0x68);
    if (U8(st, r + 0x59) != 0) {
        int32_t img = base + v1;
        int32_t sx = 0, sy = 0;
        if (S32(st, MONUMENT_MODE) == 3) {
            int32_t k = img - S32(st, 0x00F57334u) - 0x6D;
            sx = S32(st, 0x005F5B68u + 8u * (uint32_t) k);
            sy = S32(st, 0x005F5B6Cu + 8u * (uint32_t) k);
        }
        cv_image_draw(st, img, S16(st, r + 0x5C) + sx + x, S16(st, r + 0x5E) + sy + y);
    }
    if (U8(st, r + 0x5A) != 0) {
        int32_t img = v2 + S32(st, r + 0x68);
        int32_t sx = 0, sy = 0;
        if (S32(st, MONUMENT_MODE) == 3) {
            int32_t k = img - S32(st, 0x00F57334u) - 0x6D;
            sx = S32(st, 0x005F5B68u + 8u * (uint32_t) k);
            sy = S32(st, 0x005F5B6Cu + 8u * (uint32_t) k);
        }
        cv_image_draw(st, img, S16(st, r + 0x60) + sx + x, S16(st, r + 0x62) + sy + y);
    }
}

void cv_part_b7(game_state *st, int32_t image, int32_t x, int32_t y, int32_t b)
{
    /* @bin 0x004EF550 a tile of a 0xB7 building (a monument part): its top raised by the height
     * (+0x57: 0x005E68F8[+0x57 + 6 * +0x54] in mode 1, else 90 * +0x57), a part whose layer the
     * neighbour draws (it links itself there: +0x6C = b, +0x70.. its positions), the figures on its
     * tiles around the layers */
    uint32_t r = BUILDING_ADDR(b);
    int32_t kind = U8(st, r + 0x54);
    uint8_t level = U8(st, r + 0x57);
    int32_t height;
    if (S32(st, MONUMENT_MODE) == 1) {
        height = S32(st, 0x005E68F8u + 4u * (uint32_t) (level + kind * 6));
    } else {
        height = level * 0x5A;
    }
    int32_t dx = x, dy = y;
    uint8_t stage = U8(st, r + 0x55);
    /* @bin 0x004EF5C2 */
    if (stage <= 1 && (stage != 0 || level != 0 || U8(st, r + 0x56) != 0 || S32(st, MONUMENT_MODE) == 5 ||
                       S16(st, r + 0x12) < 4)) {
        dx = S32(st, 0x0070E45Cu) + x + 1;
        dy = y - S32(st, TILE_H) - height;
    }
    int linked = 0;
    int32_t v = U8(st, r + 0x59);
    int test = 0;
    if (v >= 7) {
        /* @bin 0x004EF631 mode 3: (v - 1) % 6 < 3; else v % 6 = 1 */
        if (S32(st, MONUMENT_MODE) == 3) {
            test = (v - 1) % 6 < 3;
        } else {
            test = v % 6 == 1;
        }
    }
    if (test) {
        int32_t side = S32(st, 0x005E4EF8u + 8u * (uint32_t) (S16(st, r + 0x12) + kind * 0x90));
        int32_t dir = S32(st, ORIENTATION) / 2;
        /* @bin 0x004EF690 the side turned with the view (0..3, 4..7) */
        if (side < 4) {
            side += dir;
            if (side >= 4) {
                side -= 4;
            }
        } else if (side < 8) {
            side += dir;
            if (side >= 8) {
                side -= 4;
            }
        }
        int near;
        if (S32(st, MONUMENT_MODE) == 3) {
            near = !(side == 0 || side == 4 || side == 7);
        } else {
            near = side == 5 || side == 6;
        }
        if (near) {
            /* @bin 0x004EF6DF the neighbour's tile by 0x005700F4 / 0x005700F8 (12 bytes per dir) */
            int32_t off = side == 2 ? S32(st, 0x005700F4u + 12u * (uint32_t) dir) : S32(st, 0x005700F8u + 12u * (uint32_t) dir);
            int32_t n = S16(st, BUILDING_GRID + 2u * (uint32_t) (off + S32(st, r + 0x0C)));
            if (n != 0) {
                uint32_t nr = BUILDING_ADDR(n);
                if (S16(st, nr + 0x10) == 0xB7) {
                    S32(st, nr + 0x6C) = b;
                    S32(st, nr + 0x70) = dx;
                    S32(st, nr + 0x74) = dy;
                    S32(st, nr + 0x78) = x;
                    S32(st, nr + 0x7C) = y;
                    linked = 1;
                }
            }
        }
    }
    int32_t partner = S32(st, r + 0x6C);
    int figures_done = 0;
    int32_t tile = 0;
    if (!linked && S32(st, r + 0x68) == 0 && partner == 0) {
        figures_on(st, r, x, y, 0, &tile);
        figures_done = 1;
    }
    if (height != 0) {
        /* @bin 0x004EF7F6 mode 5, +0x54 >= 2, 0x005F5D30[+0x12] and +0x57: the image 0x00F57334 +
         * 0x65 (+ 6 by the tile's bit 0 of 0x00ACF85C) under the raised top */
        if (S32(st, MONUMENT_MODE) == 5 && U8(st, r + 0x54) >= 2 && U8(st, 0x005F5D30u + (uint32_t) S16(st, r + 0x12)) != 0 &&
            U8(st, r + 0x57) != 0) {
            int32_t img = S32(st, 0x00F57334u) + (U8(st, 0x00ACF85Cu + (uint32_t) tile) & 1) * 6 + 0x65;
            cv_image_draw(st, img, dx, dy + height);
            cv_image_draw_top(st, img, dx, dy + height);
        }
        cv_image_draw(st, image, dx, dy);
    }
    cv_image_draw_top(st, image, dx, dy);
    if (partner != 0) {
        /* @bin 0x004EF89C the linked part's layers at the positions it left here */
        int32_t px = S32(st, r + 0x70);
        int32_t py = S32(st, r + 0x74);
        int32_t fx = S32(st, r + 0x78);
        int32_t fy = S32(st, r + 0x7C);
        uint32_t pr = BUILDING_ADDR(partner);
        int32_t v1, v2;
        /* the partner's part, with this building's kind (+0x54) */
        if (S32(st, MONUMENT_MODE) != 3) {
            v1 = 0;
            v2 = 0;
        } else if (S32(st, 0x005E4EF8u + 8u * (uint32_t) (S16(st, pr + 0x12) + kind * 0x90)) < 4) {
            v1 = mod6_index(U8(st, pr + 0x59), 0, 0);
            v2 = mod6_index(U8(st, pr + 0x5A), 1, 3);
        } else {
            v1 = ((int32_t) U8(st, pr + 0x59) - 1) % 3;
            v2 = ((int32_t) U8(st, pr + 0x5A) - 1) % 3;
        }
        if (S32(st, pr + 0x68) != 0) {
            b7_layers(st, pr, v1, v2, px, py);
        }
        figures_on(st, pr, fx, fy, 1, NULL);
    }
    if (linked) {
        return;
    }
    if (S32(st, r + 0x68) != 0) {
        int32_t v1, v2;
        b7_variants(st, r, kind, &v1, &v2, 1);
        b7_layers(st, r, v1, v2, dx, dy);
    }
    if (!figures_done) {
        figures_on(st, r, x, y, 0, NULL);
    }
    figures_on(st, r, x, y, 2, NULL);
}

/* ---- registration ------------------------------------------------------------------------------ */

static int32_t run_b7(game_state *st, const int32_t *a)
{
    cv_part_b7(st, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_e6(game_state *st, const int32_t *a)
{
    cv_part_e6(st, a[0], a[1], a[2]);
    return 0;
}
#define RUN_XY(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0], a[1]); \
        return 0; \
    }
RUN_XY(run_41fff0, cv_parts_41fff0)
RUN_XY(run_420050, cv_parts_420050)
RUN_XY(run_4206f0, cv_parts_4206f0)
RUN_XY(run_420a30, cv_parts_420a30)
RUN_XY(run_420d60, cv_parts_420d60)
RUN_XY(run_421060, cv_parts_421060)
static int32_t run_421920(game_state *st, const int32_t *a)
{
    cv_parts_421920(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_set_odd(game_state *st, const int32_t *a)
{
    (void) a;
    return cv_part_set_odd(st);
}
static int32_t run_set_select(game_state *st, const int32_t *a)
{
    cv_part_set_select(st, a[0]);
    return 0;
}
static int32_t run_cf(game_state *st, const int32_t *a)
{
    cv_part_cf(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_d5(game_state *st, const int32_t *a)
{
    cv_part_d5(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_ca(game_state *st, const int32_t *a)
{
    cv_part_ca(st, a[0]);
    return 0;
}
static int32_t run_4a(game_state *st, const int32_t *a)
{
    if (!cv_anim_4a(st, a[0], a[1], a[2], a[3], a[4])) {
        state_unported(st, 0x00438210u);
    }
    return 0;
}
static int32_t run_4c(game_state *st, const int32_t *a)
{
    if (!cv_anim_4c(st, a[0], a[1], a[2], a[3], a[4])) {
        state_unported(st, 0x00438350u);
    }
    return 0;
}
#define RUN_FXY(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[1], a[2], a[3]); \
        return 0; \
    }
RUN_FXY(run_a1, cv_anim_a1)
RUN_FXY(run_6a, cv_anim_6a)
RUN_FXY(run_22, cv_anim_22)
RUN_FXY(run_1f, cv_anim_1f)
RUN_FXY(run_21, cv_anim_21)
RUN_FXY(run_23, cv_anim_23)
static int32_t run_1e(game_state *st, const int32_t *a)
{
    cv_anim_1e(st, a[0], a[1], a[2], a[3]);
    return 0;
}

const ported_function cityview_parts_ports[] = {
    {0x004EF550u, "cv_part_b7", 4, 0, 0, run_b7, 0},
    {0x00401F70u, "cv_part_e6", 3, 0, 0x006DE650u, run_e6, 0},
    {0x0041FFF0u, "cv_parts_41fff0", 2, 0, 0, run_41fff0, 0},
    {0x00420050u, "cv_parts_420050", 2, 0, 0, run_420050, 0},
    {0x004206F0u, "cv_parts_4206f0", 2, 0, 0, run_4206f0, 0},
    {0x00420A30u, "cv_parts_420a30", 2, 0, 0, run_420a30, 0},
    {0x00420D60u, "cv_parts_420d60", 2, 0, 0, run_420d60, 0},
    {0x00421060u, "cv_parts_421060", 2, 0, 0, run_421060, 0},
    {0x00421920u, "cv_parts_421920", 3, 0, 0, run_421920, 0},
    {0x0041F8B0u, "cv_part_set_odd", 0, 32, 0, run_set_odd, 0},
    {0x0041F8C0u, "cv_part_set_select", 1, 0, 0, run_set_select, 0},
    {0x0041F910u, "cv_part_pos", 3, 0, 0, NULL, 0},     /* output pointers: called by the ports above */
    {0x0041FAD0u, "cv_part_pos_dir", 4, 0, 0, NULL, 0}, /* output pointers: called by 0x00421920 */
    {0x004E5E90u, "cv_part_cf", 3, 0, 0, run_cf, 0},
    {0x00542E10u, "cv_part_d5", 3, 0, 0, run_d5, 0},
    {0x004C0160u, "cv_part_ca", 1, 0, 0, run_ca, 0},
    {0x00438210u, "cv_anim_4a", 5, 0, 0, run_4a, 0},
    {0x00438350u, "cv_anim_4c", 5, 0, 0, run_4c, 0},
    {0x00438470u, "cv_anim_a1", 4, 0, 0, run_a1, 0},
    {0x004384D0u, "cv_anim_6a", 4, 0, 0, run_6a, 0},
    {0x00438590u, "cv_anim_1e", 4, 0, 0, run_1e, 0},
    {0x00438650u, "cv_anim_22", 4, 0, 0, run_22, 0},
    {0x004386B0u, "cv_anim_1f", 4, 0, 0, run_1f, 0},
    {0x00438710u, "cv_anim_21", 4, 0, 0, run_21, 0},
    {0x00438770u, "cv_anim_23", 4, 0, 0, run_23, 0},
};
const int cityview_parts_ports_count = sizeof(cityview_parts_ports) / sizeof(cityview_parts_ports[0]);
