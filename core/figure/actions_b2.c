/* Imhotep - figure actions, group b: figure types 0x15-0x18.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21); city data: CURRENT_CITY. */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"

#define DIFFICULTY 0x00E39080u /* 0..4 */

int32_t figb_leader_of(game_state *st, int32_t id)
{
    /* @bin 0x00492360 the figure at +0x68 if it has type 0x13; if it has type 0x15, its own +0x68 */
    int32_t l = S16(st, FIGURE_ADDR(id) + 0x68);
    uint8_t t = U8(st, FIGURE_ADDR(l) + 0x0A);
    if (t == 0x13) {
        return l;
    }
    if (t == 0x15) {
        return S16(st, FIGURE_ADDR(l) + 0x68);
    }
    return 0;
}

void figb_action_15(game_state *st)
{
    /* @bin 0x004923A0 a follower of a type 0x13 (or 0x15) figure */
    int32_t id = FIGB_CUR(st);
    int32_t leader = S16(st, FIGURE_ADDR(id) + 0x68);
    int32_t head = figb_leader_of(st, id);
    id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 3;
    figb_anim_advance(st, f, 12);
    U32(st, f + 0x180) = 0;
    uint32_t lf = FIGURE_ADDR(leader);
    uint32_t hf = FIGURE_ADDR(head);
    if (leader <= 0) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, lf + 0x38) == 4) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, lf + 0x0E) != 1) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, lf + 0x0A) != 0x13 && U8(st, lf + 0x0A) != 0x15) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, hf + 0x38) != 6 && U8(st, hf + 0x38) != 7) {
        GAME_CALL(st, 0x004B22E0u, id, S16(st, f + 0x68), 1); /* follow the leader */
        id = FIGB_CUR(st);
    }
    if (U8(st, lf + 0x65) != 0) {
        U8(st, FIGURE_ADDR(id) + 0x65) = 1;
    }
    f = FIGURE_ADDR(id);
    int32_t d = S8(st, f + 0x11);
    if ((int8_t) d >= 8) {
        d = S8(st, f + 0x12);
    }
    d -= S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
    }
    uint8_t ha = U8(st, hf + 0x38);
    if (ha == 6) {
        U32(st, f + 0x04) = 0;
        return;
    }
    if (ha == 7) {
        U8(st, f + 0x01) = 0;
    }
    if (U8(st, f + 0x0E) == 2) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F55E94u);
    } else {
        U32(st, f + 0x04) = U32(st, 0x00F55E8Cu) + 8u * U8(st, f + 0x01) + (uint32_t) d;
    }
    U8(st, f + 0x65) = 0;
}

void figb_action_16(game_state *st)
{
    /* @bin 0x00492580 64 frames, removed after 200 ticks */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    figb_anim_advance(st, f, 0x40);
    U32(st, f + 0x180) = 0;
    if (U8(st, f + 0x38) == 4) {
        GAME_CALL(st, 0x004B1D50u, id);
        f = FIGB_F(st);
    }
    S16(st, f + 0x36) += 1;
    if (S16(st, f + 0x36) > 200) {
        U8(st, f + 0x0E) = 2;
        U8(st, f + 0x01) = 0;
    }
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F55DF4u);
        return;
    }
    U32(st, f + 0x04) = U32(st, 0x00F55DECu) + 8u * U8(st, f + 0x01);
}

/* @bin 0x00492B2B (0x00493498 in 0x004930B0) the game RNG stepped inline, then a wait of r % 10 */
static void wait_random(game_state *st, uint32_t f)
{
    U8(st, f + 0x38) = 8;
    random_next_state(st);
    S16(st, f + 0x36) = (int16_t) (S32(st, 0x00D3B370u) % 10);
}

/* the walker image without storing the relative direction (0x00E94E00) */
static void image_local(game_state *st, uint32_t walk, uint32_t die)
{
    uint32_t f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11) - S32(st, FIGB_VIEW_DIR);
    if (d < 0) {
        d += 8;
    }
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, die);
        return;
    }
    U32(st, f + 0x04) = U32(st, walk) + 8u * U8(st, f + 0x01) + (uint32_t) d;
}

static int difficulty_ok(game_state *st, uint32_t addr)
{
    uint32_t d = U32(st, DIFFICULTY);
    if (d > 4) {
        state_unported(st, addr); /* the original reads its stack out of the table */
        return 0;
    }
    return 1;
}

void figb_action_17(game_state *st)
{
    /* @bin 0x00492770 waits, walks to a building (0x00492650) and takes a share of money */
    static const int32_t pct[5] = {3, 6, 9, 12, 15}; /* @bin 0x0049278C by difficulty */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    U32(st, f + 0x180) = 0;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    figb_anim_advance(st, f, 12);
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 8: {
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) <= 0x28) {
            break;
        }
        int32_t b = GAME_CALL(st, 0x00492650u, id);
        f = FIGB_F(st);
        if (b == 0) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        U8(st, f + 0x38) = 9;
        U16(st, f + 0x24) = U16(st, BUILDING_ADDR(b) + 0x26);
        U16(st, f + 0x26) = U16(st, BUILDING_ADDR(b) + 0x28);
        S16(st, f + 0x5E) = (int16_t) b;
        S16(st, f + 0x36) = 0;
        break;
    }
    case 9: {
        GAME_CALL(st, 0x004A5DA0u, id, speed * 2);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir != 8) {
            if (dir == 9 || dir == 10) {
                U8(st, f + 0x0E) = 2;
            }
            break;
        }
        int32_t b = S16(st, f + 0x5E);
        uint32_t ba = BUILDING_ADDR(b);
        if (b != 0 && U8(st, ba) == 1) {
            if (GAME_CALL(st, 0x00459EC0u, b) != 0) {
                if (!difficulty_ok(st, 0x00492770u)) {
                    return;
                }
                uint32_t c = CURRENT_CITY(st);
                int32_t amount = calc_percentage(S32(st, c + 0x4450), pct[U32(st, DIFFICULTY)]);
                if (amount != 0) {
                    S32(st, c + 0x4760) += 1;
                    S32(st, c + 0x4450) -= amount;
                    S32(st, c + 0x4784) += amount;
                    GAME_CALL(st, 0x004E2970u, 0x67, 0, 0);
                    S32(st, CURRENT_CITY(st) + 0x4418) += 1;
                }
            } else if (GAME_CALL(st, 0x00459EF0u, b) != 0) {
                if (!difficulty_ok(st, 0x00492770u)) {
                    return;
                }
                int32_t amount = calc_percentage(S32(st, ba + 0xB8), pct[U32(st, DIFFICULTY)]);
                if (amount != 0) {
                    GAME_CALL0(st, 0x004684E0u);
                    S32(st, 0x00E950DCu) = 1;
                    uint32_t c = CURRENT_CITY(st);
                    S16(st, c + 0x448C) += 1;
                    S32(st, c + 0x4760) += 1;
                    S32(st, c + 0x2C84) += amount;
                    S32(st, c + 0x4764) += amount;
                    S32(st, c + 0x0C) -= amount;
                    int16_t bt = S16(st, ba + 0x10);
                    int32_t msg = 0; /* @bin 0x00492CB0 by building type */
                    if (bt >= 0x54 && bt <= 0x57) {
                        msg = 0xC5;
                    } else if (bt == 0xB8) {
                        msg = 0xC6;
                    } else if (bt >= 0xBB && bt <= 0xBD) {
                        msg = 0x66;
                    }
                    if (msg) {
                        GAME_CALL(st, 0x004E2970u, msg, 0, 0);
                    }
                    S32(st, CURRENT_CITY(st) + 0x4418) += 1;
                }
            }
        }
        U8(st, FIGB_F(st) + 0x0E) = 2;
        break;
    }
    case 10: {
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) <= 10) {
            break;
        }
        uint32_t c = CURRENT_CITY(st);
        S16(st, c + 0x448C) += 1;
        S32(st, c + 0x4760) += 1;
        S32(st, 0x00E950DCu) = 1;
        S32(st, c + 0x2C84) += 100;
        S32(st, c + 0x4764) += 100;
        GAME_CALL(st, 0x004E2970u, 0xCB, 0, 0);
        U8(st, FIGB_F(st) + 0x0E) = 2;
        break;
    }
    default:
        wait_random(st, f);
        break;
    }
    image_local(st, 0x00F55DECu, 0x00F55DF4u);
}

/* @bin 0x004937A0 set dying (action 4), the wait counter to -1 unless already dying */
static void set_dying(game_state *st, uint32_t f)
{
    if (U8(st, f + 0x38) != 4) {
        S16(st, f + 0x36) = -1;
    }
    U8(st, f + 0x38) = 4;
}

/* @bin 0x004938B3 head for the building's road tile (0x004806C0), keep it as the return point */
static void go_to_building(game_state *st, int32_t b)
{
    uint32_t ba = BUILDING_ADDR(b);
    GAME_CALL(st, 0x004806C0u, S16(st, ba + 0x06), S16(st, ba + 0x08), U8(st, ba + 0x03));
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    uint16_t x = U16(st, 0x00D3B4E4u), y = U16(st, 0x00D3B4E0u);
    U16(st, f + 0x24) = x;
    U16(st, f + 0x26) = y;
    U16(st, f + 0x2C) = x;
    U16(st, f + 0x2E) = y;
    GAME_CALL(st, 0x004B3250u, id);
}

/* @bin 0x00493C00 after a move: 9 (reroute) drops the path, 10 (lost) dies */
static void moved_other(game_state *st, uint32_t f, int8_t dir)
{
    if (dir == 9) {
        GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
    } else if (dir == 10) {
        set_dying(st, f);
    }
}

void figb_action_19(game_state *st)
{
    /* @bin 0x004937A0 a worker attached to a building (+0xB4), with a count at +0x128 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    int32_t b = S16(st, f + 0x5A);
    if (U8(st, BUILDING_ADDR(b)) != 1) {
        set_dying(st, f);
    }
    if (S32(st, f + 0x128) <= 0) {
        set_dying(st, f);
    }
    uint8_t a = U8(st, f + 0x38);
    if (a != 8 && a != 4 && S16(st, BUILDING_ADDR(b) + 0xB4) != id) {
        b = GAME_CALL(st, 0x00480B80u, id);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (b != 0) {
            S16(st, BUILDING_ADDR(b) + 0xB4) = (int16_t) id;
            S16(st, f + 0x5A) = (int16_t) b;
            U8(st, f + 0x38) = 0x0B;
            go_to_building(st, b);
            id = FIGB_CUR(st);
        } else {
            set_dying(st, f);
        }
    }
    f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x76) = 1;
    figb_anim_advance(st, f, 12);
    U32(st, f + 0x180) = 0;
    switch (U8(st, f + 0x38)) {
    case 8:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) >= 0x32) {
            S16(st, f + 0x36) = 0;
            int16_t h = S16(st, f + 0x5A);
            if (h != 0) {
                U8(st, f + 0x38) = 0x0B;
                S16(st, BUILDING_ADDR(h) + 0xB4) = (int16_t) id;
                go_to_building(st, b);
            }
        }
        break;
    case 9: {
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        U8(st, f + 0x77) = 0;
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            if (GAME_CALL(st, 0x00480D60u, id) == 0) {
                GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
                f = FIGB_F(st);
                U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
                U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
                U8(st, f + 0x11) = U8(st, f + 0x12);
            } else {
                f = FIGB_F(st);
                U8(st, f + 0x38) = 0x0A;
                S16(st, f + 0x36) = 0;
            }
        } else if (dir == 9 || dir == 10) {
            if (S32(st, f + 0x128) > 0) {
                S32(st, f + 0x128) -= 1;
            }
            U8(st, f + 0x38) = 0x0C;
            U16(st, f + 0x24) = U16(st, f + 0x2C);
            U16(st, f + 0x26) = U16(st, f + 0x2E);
        }
        break;
    }
    case 0x0A:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) >= 200) {
            S16(st, f + 0x36) = 0;
            U8(st, f + 0x38) = 0x0D;
            U16(st, f + 0x24) = U16(st, f + 0x2C);
            U16(st, f + 0x26) = U16(st, f + 0x2E);
            GAME_CALL(st, 0x004B3250u, id);
        }
        break;
    case 0x0B: {
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        U8(st, f + 0x77) = 0;
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            U8(st, f + 0x38) = 0x0C;
            S16(st, f + 0x36) = 0;
        } else {
            moved_other(st, f, dir);
        }
        break;
    }
    case 0x0C: {
        uint32_t ba = BUILDING_ADDR(b);
        int32_t v = GAME_CALL(st, 0x004CDE60u, S16(st, ba + 0x40), U32(st, 0x00705EB8u + 0x28u * (uint32_t) S16(st, ba + 0x10)));
        int32_t limit = (0x66 - v) * 5;
        if (S16(st, ba + 0x64) > 0) {
            v = 0;
        }
        if (U32(st, 0x0078D7B0u) != 0 || v <= 0) {
            break;
        }
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) < limit) {
            break;
        }
        S16(st, f + 0x36) = 0;
        if (GAME_CALL(st, 0x00480C60u, FIGB_CUR(st)) == 0) {
            break;
        }
        f = FIGB_F(st);
        U8(st, f + 0x38) = 9;
        U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
        U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
        GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        break;
    }
    case 0x0D: {
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        U8(st, f + 0x77) = 0;
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            U8(st, f + 0x38) = 0x0C;
            S16(st, f + 0x36) = 0;
            U8(st, BUILDING_ADDR(b) + 0x32) = 1;
            S16(st, BUILDING_ADDR(b) + 0x64) += 1;
        } else {
            moved_other(st, f, dir);
        }
        break;
    }
    case 4:
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    id = FIGB_CUR(st);
    if (U8(st, FIGURE_ADDR(id) + 0x38) == 4) {
        GAME_CALL(st, 0x004B1D50u, id);
    }
    f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11);
    if ((int8_t) d >= 8) {
        d = S8(st, f + 0x12);
    }
    d -= S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    uint8_t act = U8(st, f + 0x38);
    if (act == 0x0A) {
        d -= 2;
        S32(st, FIGB_DIR_REL) = d;
    }
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
    }
    switch (act) { /* @bin 0x00493E90 */
    case 4: {
        int16_t t = S16(st, f + 0x36);
        if (t < 0x21) {
            U32(st, f + 0x04) = (uint32_t) (d / 8 + (int32_t) U32(st, 0x00F5612Cu) + t / 3);
        } else {
            int32_t k = t - 0x21;
            if (k >= 0x17) {
                k = 0x16;
            }
            U32(st, f + 0x04) = (uint32_t) k + U32(st, 0x00F56E14u);
        }
        break;
    }
    case 9:
    case 0x0B:
    case 0x0D:
        U32(st, f + 0x04) = U32(st, 0x00F5611Cu) + 8u * (uint32_t) (U8(st, f + 0x01) / 3) + (uint32_t) d;
        break;
    case 0x0A:
        U32(st, f + 0x04) = U32(st, 0x00F56124u) + 8u * (uint32_t) (S16(st, f + 0x36) % 9) + (uint32_t) d;
        break;
    default:
        U32(st, f + 0x04) = U32(st, 0x00F56134u) + (uint32_t) d;
        break;
    }
}

/* Tiles next to monuments, written to *x, *y (the original's output pointers); the thiscall ones
 * ignore their object. Offsets by the building's part index (+0x12), measured from the originals
 * (scratchpad tool, every index), checked against their jump tables. */
typedef struct {
    int8_t dx, dy, set;
} tile_offset;

/* @bin 0x004E0CC0 (building +0x68 + 0x16, +0x6C + 4) */
void figb_tile_4e0cc0(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    uint32_t ba = BUILDING_ADDR(b);
    *x = S32(st, ba + 0x68) + 0x16;
    *y = S32(st, ba + 0x6C) + 4;
}

/* @bin 0x004F6420 the road tile of the part given by tables 0x005F5498 / 0x005F3878 */
void figb_tile_4f6420(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    int32_t main = b - S16(st, BUILDING_ADDR(b) + 0x12);
    uint32_t k = U8(st, BUILDING_ADDR(main) + 0x54);
    int32_t part;
    if (S32(st, 0x005E4EF4u) == 5) {
        part = main + S32(st, 0x005F5498u + 0x80u * k);
    } else {
        part = main + S32(st, 0x005F3878u + 0x3C0u * k);
    }
    *x = S16(st, BUILDING_ADDR(part) + 0x26);
    *y = S16(st, BUILDING_ADDR(part) + 0x28);
}

static void tile_by_part(game_state *st, int32_t b, int32_t *x, int32_t *y, const tile_offset *t, int n,
                         int out_of_range_sets)
{
    uint32_t ba = BUILDING_ADDR(b);
    int32_t k = S16(st, ba + 0x12);
    int32_t dx = 0, dy = 0;
    if ((uint32_t) k < (uint32_t) n) {
        dx = t[k].dx;
        dy = t[k].dy;
    } else if (!out_of_range_sets) {
        return;
    }
    *x = S16(st, ba + 0x06) + dx;
    *y = S16(st, ba + 0x08) + dy;
}

void figb_tile_545a30(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00545A30 jump table 0x00545B94 */
    static const tile_offset t[9] = {{2, 1, 1}, {3, 2, 1}, {3, 1, 1}, {2, -1, 1}, {2, -1, 1},
                                     {1, -1, 1}, {1, -1, 1}, {-1, 0, 1}, {-1, 0, 1}};
    tile_by_part(st, b, x, y, t, 9, 1);
}

void figb_tile_558f00(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00558F00 jump table 0x0055906C */
    static const tile_offset t[10] = {{2, 1, 1}, {3, 2, 1}, {3, 0, 1}, {1, 0, 1}, {3, 2, 1},
                                      {1, 3, 1}, {0, 1, 1}, {3, 5, 1}, {-1, 0, 1}, {-1, 1, 1}};
    tile_by_part(st, b, x, y, t, 10, 0);
}

void figb_tile_556230(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00556230 jump table 0x005563B0 */
    static const tile_offset t[21] = {{1, -1, 1}, {2, -1, 1}, {1, -1, 1}, {0, -1, 1}, {1, -1, 1}, {1, 0, 1},
                                      {3, 1, 1},  {2, -1, 1}, {1, -1, 1}, {2, -1, 1}, {3, -1, 1}, {1, 0, 1},
                                      {2, 0, 1},  {-1, 0, 1}, {-1, 0, 1}, {1, 0, 1},  {2, 1, 1},  {-1, 0, 1},
                                      {-1, 1, 1}, {0, -1, 1}, {2, -1, 1}};
    tile_by_part(st, b, x, y, t, 21, 0);
}

void figb_tile_54f8a0(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x0054F8A0 jump table 0x0054FA98 */
    static const tile_offset t[31] = {{2, 1, 1}, {3, 2, 1}, {2, 1, 1},  {1, 0, 1},  {2, 1, 1},  {1, 0, 1},  {3, 1, 1},
                                      {0, 1, 1}, {1, 3, 1}, {0, 2, 1},  {3, 2, 1},  {3, 2, 1},  {5, 4, 1},  {0, 1, 1},
                                      {3, 6, 1}, {1, 0, 1}, {2, 1, 1},  {-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}, {-1, 1, 1},
                                      {1, 0, 1}, {2, 0, 1}, {0, 1, 1},  {1, 3, 1},  {0, 1, 1},  {1, 3, 1},  {-1, 0, 1},
                                      {-1, 1, 1}, {0, 1, 1}, {1, 3, 1}};
    tile_by_part(st, b, x, y, t, 31, 0);
}

void figb_action_18(game_state *st)
{
    /* @bin 0x004930B0 waits, walks to a monument (0x00492D20) and takes a share of money */
    static const int32_t loss[5] = {-2, -4, -6, -8, -10}; /* @bin 0x004930CA by difficulty */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t amount = 0;
    U8(st, f + 0x72) = 3;
    U32(st, f + 0x180) = 0;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    figb_anim_advance(st, f, 12);
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        f = FIGB_F(st);
        if (U8(st, f + 0x0E) == 2 && S16(st, f + 0x90) > 0) {
            GAME_CALL(st, 0x004E96C0u, 1);
            GAME_CALL(st, 0x004E2970u, 0xFE, 0, 0);
        }
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 8: {
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) <= 0x28) {
            break;
        }
        int32_t b = GAME_CALL(st, 0x00492D20u, id);
        f = FIGB_F(st);
        if (b == 0) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        int32_t x = 0, y = 0;
        U8(st, f + 0x38) = 9;
        S16(st, f + 0x5E) = (int16_t) b;
        int16_t bt = S16(st, BUILDING_ADDR(b) + 0x10);
        if (bt == 0xDE) {
            figb_tile_4e0cc0(st, b, &x, &y);
        } else if (bt == 0xB7) {
            figb_tile_4f6420(st, b, &x, &y);
        } else if (bt == 0xE5) {
            figb_tile_545a30(st, b, &x, &y);
        } else if (bt == 0xEA) {
            figb_tile_558f00(st, b, &x, &y);
        } else if (bt == 0xEB) {
            figb_tile_556230(st, b, &x, &y);
        } else if (bt == 0xEC) {
            figb_tile_54f8a0(st, b, &x, &y);
        }
        f = FIGB_F(st);
        U16(st, f + 0x24) = (uint16_t) x;
        U16(st, f + 0x26) = (uint16_t) y;
        S16(st, f + 0x36) = 0;
        break;
    }
    case 9: {
        GAME_CALL(st, 0x004A5DA0u, id, speed * 2);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir != 8) {
            if (dir == 9 || dir == 10) {
                U8(st, f + 0x0E) = 2;
            }
            break;
        }
        int32_t b = S16(st, f + 0x5E);
        uint32_t ba = BUILDING_ADDR(b);
        if (b != 0 && U8(st, ba) == 1) {
            if (U32(st, 0x00E39080u) > 4) {
                state_unported(st, 0x004930B0u); /* the original reads its stack out of the table */
                return;
            }
            int16_t bt = S16(st, ba + 0x10);
            int monument = bt == 0xE5 || bt == 0xEB || bt == 0xEC || bt == 0xEA;
            if (bt == 0xDE && S16(st, ba + 0x16) == -1) {
                GAME_CALL(st, 0x004E2970u, 0xFC, 0, 0);
                amount = loss[U32(st, 0x00E39080u)];
            } else if ((bt == 0xDE || monument) && S16(st, ba + 0x16) == -1) {
                GAME_CALL(st, 0x004E2970u, 0xFB, 0, 0);
                amount = loss[U32(st, 0x00E39080u)];
            } else if (bt == 0xB7 || bt == 0xDE || monument) {
                GAME_CALL(st, 0x004E2970u, 0xFD, 0, 0);
                GAME_CALL0(st, 0x00492FC0u);
                amount = loss[U32(st, 0x00E39080u)] / 2;
            }
            uint32_t c = CURRENT_CITY(st);
            S16(st, c + 0x448C) += 1;
            S32(st, c + 0x4760) += 1;
            S32(st, c + 0x4418) += 1;
            GAME_CALL(st, 0x004E96C0u, amount);
        }
        U8(st, FIGB_F(st) + 0x0E) = 2;
        break;
    }
    default:
        wait_random(st, f);
        break;
    }
    image_local(st, 0x00F5712Cu, 0x00F57134u);
}
