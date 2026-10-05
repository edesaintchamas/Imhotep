/* Imhotep - pyramid parts: building up, redraw, the step pyramid's chapel (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A pyramid (type 0xB7) is a main building and its parts (part number +0x12, kind +0x54 of the
 * main building, count by kind in 0x0056F7D8, or 0x0056F7EC in climate 5). A part's level +0x55,
 * stage +0x56, layer +0x57 (limit +0x58), ramp marks +0x59 / +0x5A and done corners +0x5B drive
 * its image (0x004EDF50, 0x004F4160). 0x004F3850 / 0x004F3690 lay a block, 0x004F31F0 counts a
 * part's work, 0x004EE630 / 0x004EE5D0 start the next level when all parts are done. */
#include "building/pyramid_terr.h"

#include "building/rest_redraw.h"
#include "figure/actions_d.h"
#include "map/map_building.h"
#include "map/terrain_land.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CLIMATE 0x005E4EF4u
#define B(id) BUILDING_ADDR(id)
#define PART(id) S16(st, B(id) + 0x12)
#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define IMG(o) S32(st, 0x00B7482Cu + 4u * (uint32_t) (o))
#define BITS(o) U8(st, 0x00B285CCu + (uint32_t) (o))
#define RND(o) U8(st, 0x00ACF85Cu + (uint32_t) (o))
#define BGRID(o) S16(st, 0x00B4E6FCu + 2u * (uint32_t) (o))

void copy_bytes(game_state *st, uint32_t src, uint32_t dst, int32_t n)
{
    /* @bin 0x0040F860 forward copy, 16 dwords at a time while n >= 64, then bytes */
    if (n > 0 && (!state_ptr(st, src, (uint32_t) n) || !state_ptr(st, dst, (uint32_t) n))) {
        state_unported(st, 0x0040F860u); /* a buffer outside the state (a caller's stack) */
        return;
    }
    while (n >= 0x40) {
        for (uint32_t k = 0; k < 0x40; k += 4) {
            U32(st, dst + k) = U32(st, src + k);
        }
        src += 0x40;
        dst += 0x40;
        n -= 0x40;
    }
    for (; n > 0; n--) {
        U8(st, dst++) = U8(st, src++);
    }
}

int32_t building_keep_removed(game_state *st, int32_t id)
{
    /* @bin 0x00485100 0x00BC95F0 counts the used entries of the 269 (0x00BA746C .. 0x00BA7886);
     * a free entry takes id and a copy of its record (0x0040F860); none free: 0x00BC9694 := 0 */
    if (id <= 0) {
        return 0;
    }
    int32_t used = 0, found = 0;
    S32(st, 0x00BC95F0u) = 0;
    for (uint32_t i = 0; i < 269; i++) {
        int16_t w = S16(st, 0x00BA746Cu + 2u * i);
        used += w != 0;
        found |= w == id;
    }
    S32(st, 0x00BC95F0u) = used;
    if (found) {
        return 0;
    }
    for (uint32_t i = 0; i < 269; i++) {
        if (S16(st, 0x00BA746Cu + 2u * i) == 0) {
            S32(st, 0x00BC95F0u) = used + 1;
            copy_bytes(st, B(id), 0x00BA7888u + 0x108u * i, 0x108);
            S16(st, 0x00BA746Cu + 2u * i) = (int16_t) id;
            return 1;
        }
    }
    S32(st, 0x00BC9694u) = 0;
    return 0;
}

void pyramid_chapel_mark(game_state *st, int32_t id, int32_t remove)
{
    /* @bin 0x004F5E70 (id, remove) the building of type 0xD7 at the main building's tile +
     * offset by kind (0x005700D4, climate 5: 0x005700E8): remove: kept (0x00485100), state 6,
     * 0x00BC971C + 1; else state 2 */
    int32_t main = id - PART(id);
    uint32_t kind = U8(st, B(main) + 0x54);
    int32_t off = S32(st, CLIMATE) == 5 ? S32(st, 0x005700E8u + 4u * kind) : S32(st, 0x005700D4u + 4u * kind);
    int32_t c = BGRID(S32(st, B(main) + 0x0C) + off);
    if (c == 0 || S16(st, B(c) + 0x10) != 0xD7) {
        return;
    }
    if ((int8_t) remove != 0) {
        building_keep_removed(st, c);
        U8(st, B(c)) = 6;
        S32(st, 0x00BC971Cu) += 1;
        return;
    }
    U8(st, B(c)) = 2;
}

void pyramid_part_work_done(game_state *st, int32_t part)
{
    /* @bin 0x004F31F0 the main building's work count +0xC8 - 1. A part still rising (+0x55 == 0)
     * counts its stage +0x56 up to a limit (climate 1: 3 when the layer's table byte 0x005E68D8 is
     * set, else 6; climate 5: 1 for the main building, else 6); at the limit the layer +0x57 goes
     * up (past it: stage 1) until +0x58, then the part is done (+0x55 = 1, stage = the limit, at
     * least 1). A done part counts its stage down, clearing the ramp marks +0x59 / +0x5A above it.
     * No work left: 0x004F5E70 (main, 0), and in climate 3 the parts' ramp marks are cleared. */
    uint32_t b = B(part);
    int32_t main = part - PART(part);
    uint32_t mb = B(main);
    uint32_t kind = U8(st, mb + 0x54);
    S16(st, mb + 0xC8) = (int16_t) (S16(st, mb + 0xC8) - 1);
    if (U8(st, b + 0x55) == 0) {
        uint8_t stage = (uint8_t) (U8(st, b + 0x56) + 1);
        int32_t climate = S32(st, CLIMATE), limit;
        U8(st, b + 0x56) = stage;
        if (climate == 1) {
            limit = U8(st, 0x005E68D8u + kind * 6 + U8(st, b + 0x57)) != 0 ? 3 : 6;
        } else if (climate == 5) {
            limit = main == part ? 1 : 6;
        } else {
            limit = 6;
        }
        if (stage >= limit) {
            uint8_t layer = U8(st, b + 0x57);
            if (layer < U8(st, b + 0x58)) {
                if (stage > limit) {
                    U8(st, b + 0x57) = (uint8_t) (layer + 1);
                    U8(st, b + 0x56) = 1;
                }
            } else {
                U8(st, b + 0x55) = 1;
                U8(st, b + 0x56) = (uint8_t) (limit < 1 ? 1 : limit);
            }
        }
    } else {
        uint8_t stage = (uint8_t) (U8(st, b + 0x56) - 1);
        U8(st, b + 0x56) = stage;
        if (U8(st, b + 0x59) > stage) {
            U8(st, b + 0x59) = 0;
        }
        if (U8(st, b + 0x5A) > U8(st, b + 0x56)) {
            U8(st, b + 0x5A) = 0;
        }
    }
    if (S16(st, mb + 0xC8) == 0) {
        pyramid_chapel_mark(st, main, 0);
    }
    if (S32(st, CLIMATE) == 3 && S16(st, mb + 0xC8) == 0) {
        int32_t n = S32(st, 0x0056F7D8u + 4u * kind);
        for (int32_t i = 0; i < n; i++) {
            uint32_t p = B(main + i);
            if (U8(st, p + 0x59) != 0 || U8(st, p + 0x5A) != 0) {
                U8(st, p + 0x59) = 0;
                U8(st, p + 0x5A) = 0;
            }
        }
    }
}

void pyramid_part_redraw_stage(game_state *st, int32_t part, int32_t half, int32_t kind)
{
    /* @bin 0x004F4160 (part, orientation / 2 (unused), kind) a part past level 1, by level:
     * 3: its tile of the kind's list (0x005F6CD0, climate 5: 0x005F6D58) gets image
     * [0x00F5736C] (bit-field 0x20 on odd random); 4: the done corners lose terrain 0x91;
     * 5-10: each corner tile's image by 0x004F3F70 (level, or level - 1 when not done);
     * 11: the part's square by 0x0047BF50 */
    (void) half;
    uint32_t table = S32(st, CLIMATE) == 5 ? 0x005F6D58u : 0x005F6CD0u;
    uint32_t b = B(part);
    switch (U8(st, b + 0x55)) {
    case 3: {
        int32_t n = S32(st, table + 8u * (uint32_t) kind + 4);
        if (n <= 0) {
            return;
        }
        uint32_t list = U32(st, table + 8u * (uint32_t) kind);
        int32_t p = PART(part);
        for (int32_t i = 0; i < n; i++) {
            if (p != S32(st, list + 8u * (uint32_t) i)) {
                continue;
            }
            int32_t o = S32(st, list + 8u * (uint32_t) i + 4) + S32(st, b + 0x0C);
            IMG(o) = S32(st, 0x00F5736Cu);
            if (RND(o) & 1) {
                BITS(o) |= 0x20;
            }
            return;
        }
        return;
    }
    case 4: {
        int32_t c = S32(st, b + 0x0C);
        uint32_t done = U8(st, b + 0x5B);
        for (uint32_t i = 0; i < 4; i++) {
            if (done & (1u << i)) {
                TER(c + S32(st, 0x0056F880u + 4u * i)) &= 0xFFFFFF6Eu;
            }
        }
        return;
    }
    case 5: case 6: case 7: case 8: case 9: case 10:
        for (uint32_t i = 0; i < 4; i++) {
            int32_t o = S32(st, 0x0056F880u + 4u * i) + S32(st, b + 0x0C);
            int32_t level = U8(st, b + 0x5B) & (1u << i) ? U8(st, b + 0x55) : U8(st, b + 0x55) - 1;
            land_monument_tile_image(st, part, o, level, 1);
        }
        return;
    case 11: {
        int32_t h = S32(st, 0x00BCA2E0u) / 2;
        int32_t image = S32(st, 0x0056F840u + 4u * (uint32_t) (PART(part) + h * 4)) + S32(st, 0x00F57364u);
        map_building_tiles(st, part, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), U8(st, b + 3), image, 8);
        return;
    }
    default:
        return;
    }
}

/* the ramp mark offsets (+0x5C / +0x5E from +0x59, +0x60 / +0x62 from +0x5A) */
static void ramp_offsets(game_state *st, uint32_t b, uint32_t mark, uint32_t dst, int32_t w, int32_t mode)
{
    int32_t v = U8(st, b + mark);
    if (v == 0) {
        return;
    }
    if (mode == 1) { /* @bin 0x004EE205 climate 1, layer flagged: (2v - 2) % 6, y halved */
        uint32_t e = 8u * (uint32_t) ((v * 2 - 2) % 6 + w * 6);
        S16(st, b + dst) = (int16_t) (S16(st, 0x005F5B28u + 8u * (uint32_t) w) + S16(st, 0x005F5618u + e));
        S16(st, b + dst + 2) = (int16_t) (S32(st, 0x005F561Cu + e) / 2 + S16(st, 0x005F5B2Cu + 8u * (uint32_t) w));
    } else if (mode == 3) { /* @bin 0x004EE2BF climate 3: two positions by (v - 1) % 6 >= 3 */
        uint32_t e = 16u * (uint32_t) (((v - 1) % 6 >= 3 ? 3 : 0) + w * 6);
        S16(st, b + dst) = S16(st, 0x005F57C8u + e);
        S16(st, b + dst + 2) = S16(st, 0x005F57CCu + e);
    } else { /* @bin 0x004EE35E */
        uint32_t e = 8u * (uint32_t) ((v - 1) % 6 + w * 6);
        S16(st, b + dst) = (int16_t) (S16(st, 0x005F5B28u + 8u * (uint32_t) w) + S16(st, 0x005F5618u + e));
        S16(st, b + dst + 2) = (int16_t) (S16(st, 0x005F5B2Cu + 8u * (uint32_t) w) + S16(st, 0x005F561Cu + e));
    }
}

void pyramid_part_redraw(game_state *st, int32_t part, int32_t half, int32_t kind)
{
    /* @bin 0x004EDF50 (part, orientation / 2, kind) +0x6C := 0; past level 1: 0x004F4160. Else
     * the part's shape u (table 0x005E4EF8 by kind and part, climate 5: 0x005E6578) turned by the
     * view gives the image: rising (level 0, or climate 5): stage 0 the base of 0x0056F840 (parts
     * 0-3; others, and climate 5: no image, the 4 corner tiles drawn by 0x004F3F70), else by
     * stage and layer (group [0x00F57334], climate 1 flagged layers [0x00F5738C]); done: counting
     * down. The ramp's image +0x68 (groups 0x6D.. of [0x00F57334]) and its offsets come with the
     * ramp marks +0x59 / +0x5A. Drawn by 0x0047BF50 (flags 8). */
    uint32_t b = B(part);
    U32(st, b + 0x6C) = 0;
    if (U8(st, b + 0x55) > 1) {
        pyramid_part_redraw_stage(st, part, half, kind);
        return;
    }
    int32_t climate = S32(st, CLIMATE);
    int16_t p = PART(part);
    int32_t u = climate == 5 ? S32(st, 0x005E6578u + 8u * (uint32_t) (p + kind * 0x24))
                             : S32(st, 0x005E4EF8u + 8u * (uint32_t) (kind * 0x90 + p));
    if (u < 4) {
        u += half;
        if (u >= 4) {
            u -= 4;
        }
    } else if (u < 8) {
        u += half;
        if (u >= 8) {
            u -= 4;
        }
    } else if (u == 9) {
        u = half + 9;
    }
    int32_t base = S32(st, 0x00F57334u);
    int32_t ramp = 0, w = 0;
    if (climate != 5) {
        w = u;
        if (u == 5) {
            ramp = base + 0x6D;
        } else if (u == 6) {
            ramp = climate == 3 ? base + 0x70 : base + 0x6E;
        } else if (climate == 3) {
            ramp = u == 1 ? base + 0x73 : u == 2 ? base + 0x77 : u == 3 ? base + 0x7B : 0;
        }
    }
    int corners = 0;
    int32_t image;
    uint8_t level = U8(st, b + 0x55);
    if (level != 0 && S32(st, CLIMATE) != 5) {
        if (S32(st, CLIMATE) == 1 && U8(st, 0x005E68D8u + (uint32_t) kind * 6 + U8(st, b + 0x57)) != 0) {
            image = S32(st, 0x00F5738Cu) + (5 - U8(st, b + 0x56)) * 8 + u;
        } else {
            image = base + (0xB - U8(st, b + 0x56)) * 8 + u;
        }
    } else {
        int32_t stage = U8(st, b + 0x56);
        if (stage == 0) {
            if (p < 4 && S32(st, CLIMATE) != 5) {
                image = S32(st, 0x0056F840u + 4u * (uint32_t) (p + half * 4)) + S32(st, 0x00F57364u);
            } else {
                image = 0;
                corners = 1;
            }
        } else {
            uint8_t layer = U8(st, b + 0x57);
            if (layer < U8(st, b + 0x58) || u == 8) {
                image = base + stage + (RND(S32(st, b + 0x0C)) & 1) * 6 + 0x5F;
            } else if (S32(st, CLIMATE) == 1) {
                image = (U8(st, 0x005E68D8u + (uint32_t) kind * 6 + layer) != 0 ? S32(st, 0x00F5738Cu) : base) +
                        stage * 8 + u - 8;
            } else if (u < 8) {
                image = base + stage * 8 + u - 8;
            } else {
                image = (u & 1) + base + 0x6D;
            }
        }
    }
    if ((U8(st, b + 0x59) == 0 && U8(st, b + 0x5A) == 0) || ramp == 0) {
        S32(st, b + 0x68) = 0;
    } else {
        int32_t mode = 0;
        if (S32(st, CLIMATE) == 1) {
            mode = U8(st, 0x005E68D8u + (uint32_t) kind * 6 + U8(st, b + 0x57)) != 0 ? 1 : 0;
        } else if (S32(st, CLIMATE) == 3) {
            mode = 3;
        }
        ramp_offsets(st, b, 0x59, 0x5C, w, mode);
        ramp_offsets(st, b, 0x5A, 0x60, w, mode);
        S32(st, b + 0x68) = ramp;
    }
    if (!corners) {
        map_building_tiles(st, part, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), U8(st, b + 3), image, 8);
        return;
    }
    for (uint32_t i = 0; i < 4; i++) {
        int32_t o = S32(st, b + 0x0C) + S32(st, 0x0056F880u + 4u * i);
        BGRID(o) = (int16_t) part;
        TER(o) = (TER(o) & 0xEFFFFFFFu) | 8;
        land_monument_tile_image(st, part, o, S32(st, CLIMATE) == 5 ? 5 : 10, 1);
    }
}

void pyramid_reset_redraw(game_state *st, int32_t id)
{
    /* @bin 0x004EE5D0 every part's level +0x55 := 0, then 0x004EE4A0 (id) */
    int32_t climate = S32(st, CLIMATE);
    uint32_t kind = U8(st, B(id) + 0x54);
    for (int32_t i = 0;; i++) {
        int32_t n = climate == 5 ? S32(st, 0x0056F7ECu + 4u * kind) : S32(st, 0x0056F7D8u + 4u * kind);
        if (i >= n) {
            break;
        }
        U8(st, B(id + i) + 0x55) = 0;
    }
    mastaba_redraw_4ee4a0(st, id);
}

void pyramid_check_level_done(game_state *st, int32_t id)
{
    /* @bin 0x004EE630 every part done (climate 5: level >= 5 and the 4 corners; else the 4 first
     * parts at level >= 11, the others at level >= 10 with the 4 corners): 0x004EE5D0 */
    int32_t climate = S32(st, CLIMATE);
    uint32_t kind = U8(st, B(id) + 0x54);
    for (int32_t i = 0;; i++) {
        int32_t n = climate == 5 ? S32(st, 0x0056F7ECu + 4u * kind) : S32(st, 0x0056F7D8u + 4u * kind);
        if (i >= n) {
            break;
        }
        uint32_t p = B(id + i);
        if (climate == 5) {
            if (U8(st, p + 0x55) < 5 || U8(st, p + 0x5B) < 0xF) {
                return;
            }
        } else if (i < 4) {
            if (U8(st, p + 0x55) < 0xB) {
                return;
            }
        } else if (U8(st, p + 0x55) < 0xA || U8(st, p + 0x5B) < 0xF) {
            return;
        }
    }
    pyramid_reset_redraw(st, id);
}

int32_t pyramid_block_laid(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004F3690 (x, y) a block laid on the part at the tile: its corner (offsets
     * 0x005F6BDC) is marked done in +0x5B; a part with all 4 corners done goes up a level
     * (0x004F3360; level 5: all corners at once); at level >= 10 the main building raises its 4
     * first parts by one (up to 11) and is redrawn. Levels 3 and below draw the tile (and at 3 the
     * corners) as construction ground. Returns the part's id (0: a part, not the main building,
     * at level >= 10). */
    int32_t o = S32(st, 0x00BCA2D8u) + y * 228 + x;
    int32_t id = BGRID(o);
    uint32_t b = B(id);
    uint8_t corner = 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (S32(st, b + 0x0C) + S32(st, 0x005F6BDCu + 4u * i) == o) {
            corner = (uint8_t) (1u << i);
            break;
        }
    }
    uint8_t done = U8(st, b + 0x5B);
    if (done == 0xF) {
        uint8_t level = U8(st, b + 0x55);
        if (level >= 0xA) {
            if (PART(id) != 0) {
                return 0;
            }
            int32_t next = level + 1;
            if (next <= 0xB) {
                for (int32_t i = 0; i < 4; i++) {
                    U8(st, B(id + i) + 0x55) = (uint8_t) next;
                    U8(st, B(id + i) + 0x5B) = 0xF;
                }
                mastaba_redraw_4ee4a0(st, id);
                pyramid_check_level_done(st, id);
                return id;
            }
        } else {
            uint8_t v = (uint8_t) pyramid_level_next(level);
            U8(st, b + 0x55) = v;
            if (v == 5) {
                corner = 0xF;
            }
            U8(st, b + 0x5B) = corner;
        }
    } else {
        if (U8(st, b + 0x55) == 5) {
            corner = 0xF;
        }
        U8(st, b + 0x5B) = done | corner;
        if (U8(st, b + 0x55) == 3) {
            int32_t ground = S32(st, 0x00F56B54u);
            U8(st, b + 0x55) = 4;
            int32_t c = S32(st, b + 0x0C);
            for (uint32_t i = 0; i < 4; i++) {
                int32_t t = c + S32(st, 0x005F6BDCu + 4u * i);
                TER(t) = 0x10000008u;
                IMG(t) = (RND(t) & 7) + ground;
            }
        }
    }
    if (U8(st, b + 0x55) <= 4) {
        TER(o) = 0x10000008u;
        IMG(o) = (RND(o) & 7) + S32(st, 0x00F56B54u);
    }
    pyramid_check_level_done(st, id - PART(id));
    mastaba_redraw_4ee4a0(st, id - PART(id));
    return id;
}

int32_t pyramid_lay_next_block(game_state *st, int32_t id)
{
    /* @bin 0x004F3850 the free block slot of pyramid id (0x004F3370, no busy check) gets its
     * block (0x004F3690); returns 1 then */
    int32_t px = 0, py = id;
    if (!pyramid_block_slot(st, id, &px, &py, 0)) {
        return 0;
    }
    pyramid_block_laid(st, px, py);
    return 1;
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_copy(game_state *st, const int32_t *a)
{
    copy_bytes(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
    return 0;
}

static int32_t run_keep(game_state *st, const int32_t *a) { return building_keep_removed(st, a[0]); }
static int32_t run_chapel(game_state *st, const int32_t *a) { pyramid_chapel_mark(st, a[0], a[1]); return 0; }
static int32_t run_work(game_state *st, const int32_t *a) { pyramid_part_work_done(st, a[0]); return 0; }
static int32_t run_redraw(game_state *st, const int32_t *a) { pyramid_part_redraw(st, a[0], a[1], a[2]); return 0; }
static int32_t run_redraw_stage(game_state *st, const int32_t *a) { pyramid_part_redraw_stage(st, a[0], a[1], a[2]); return 0; }
static int32_t run_reset(game_state *st, const int32_t *a) { pyramid_reset_redraw(st, a[0]); return 0; }
static int32_t run_check(game_state *st, const int32_t *a) { pyramid_check_level_done(st, a[0]); return 0; }
static int32_t run_laid(game_state *st, const int32_t *a) { return pyramid_block_laid(st, a[0], a[1]); }
static int32_t run_lay(game_state *st, const int32_t *a) { return pyramid_lay_next_block(st, a[0]); }

const ported_function terr_pyramid_ports[] = {
    {0x0040F860u, "copy_bytes", 3, 0, 0, run_copy, 0},
    {0x00485100u, "building_keep_removed", 1, 32, 0, run_keep, 0},
    {0x004F5E70u, "pyramid_chapel_mark", 2, 0, 0, run_chapel, 0},
    {0x004F31F0u, "pyramid_part_work_done", 1, 0, 0, run_work, 0},
    {0x004EDF50u, "pyramid_part_redraw", 3, 0, 0, run_redraw, 0},
    {0x004F4160u, "pyramid_part_redraw_stage", 3, 0, 0, run_redraw_stage, 0},
    {0x004EE5D0u, "pyramid_reset_redraw", 1, 0, 0, run_reset, 0},
    {0x004EE630u, "pyramid_check_level_done", 1, 0, 0, run_check, 0},
    {0x004F3690u, "pyramid_block_laid", 2, 32, 0, run_laid, 0},
    {0x004F3850u, "pyramid_lay_next_block", 1, 8, 0, run_lay, 0},
};
const int terr_pyramid_ports_count = sizeof(terr_pyramid_ports) / sizeof(terr_pyramid_ports[0]);
