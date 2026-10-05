/* Imhotep - figure actions, group b: figure type 0x52.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"

#define TERRAIN 0x00AF598Cu /* u32 per tile */

/* @bin 0x0049D13D the tile and its 8 neighbours all have terrain bit 0x04 (water) */
static int in_open_water(game_state *st, int32_t off)
{
    static const int32_t around[9] = {0, -228, 1, 228, -1, -227, -229, 229, 227};
    for (int i = 0; i < 9; i++) {
        if ((U8(st, TERRAIN + 4u * (uint32_t) (off + around[i])) & 4) == 0) {
            return 0;
        }
    }
    return 1;
}

/* @bin 0x0049D444 a random tile around a point of the herd record (0x004EAB10, offsets at
 * 0x005CD978); returns 0 when none was found */
static int wander_to(game_state *st, uint32_t rec)
{
    random_next_state(st);
    uint32_t f = FIGB_F(st);
    int32_t r = GAME_CALL(st, 0x004EAB10u, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, 0x00D3B370u) & 0xF),
                          S16(st, f + 0x14), S16(st, f + 0x16), 0);
    if (r < 0 || r >= 0x10) {
        return 0;
    }
    GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
    f = FIGB_F(st);
    S16(st, f + 0x24) = (int16_t) (S16(st, 0x005CD978u + 8u * (uint32_t) r) + S16(st, rec));
    S16(st, f + 0x26) = (int16_t) (S16(st, 0x005CD97Cu + 8u * (uint32_t) r) + S16(st, rec + 2));
    U8(st, f + 0x38) = 9;
    return 1;
}

/* @bin 0x0049D9E7 arrived or lost: a pause of +0x62 * 5 ticks (action 0x17) */
static void to_pause(game_state *st, uint32_t f)
{
    U8(st, f + 0x11) = U8(st, f + 0x12);
    U8(st, f + 0x38) = 0x17;
    S16(st, f + 0x36) = (int16_t) (uint16_t) (U8(st, f + 0x62) * 5);
}

/* @bin 0x0049D895 follow the target figure (+0x5E) when it has moved */
static void retarget(game_state *st, int32_t target)
{
    uint32_t t = FIGURE_ADDR(target);
    uint32_t f = FIGB_F(st);
    if (S16(st, t + 0x14) != S16(st, f + 0x24) || S16(st, t + 0x16) != S16(st, f + 0x26)) {
        GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        f = FIGB_F(st);
        U16(st, f + 0x24) = U16(st, t + 0x14);
        U16(st, f + 0x26) = U16(st, t + 0x16);
    }
}

void figb_action_52(game_state *st)
{
    /* @bin 0x0049D0F0 a water animal of a herd (+0x60): slower in open water, which also changes
     * its images; hunts a figure (+0x5E) in action 0x15 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    S32(st, f + 0x148) += 1;
    int water = in_open_water(st, S32(st, f + 0x20));
    if (water) {
        S32(st, f + 0x144) = 0x0B;
    }
    if (S32(st, f + 0x148) >= S32(st, f + 0x144)) {
        S32(st, f + 0x148) = 0;
        U8(st, f + 0x01) += 1;
        stepped = 1;
    }
    U8(st, f + 0x72) = 6;
    S16(st, CURRENT_CITY(st) + 0x447A) += 1; /* 0x00EA5CEA */
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 0;
    int32_t herd = S16(st, f + 0x60);
    figb_anim_advance(st, f, 12);
    if (water) {
        speed >>= 1;
    }
    uint32_t rec = 0x00BCB9FEu + 0x90u * (uint32_t) herd;
    int8_t dir;
    switch (U8(st, f + 0x38)) { /* @bin 0x0049DC04 */
    case 4:
        S32(st, f + 0x144) = 8;
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x0053B4A0u, id, 7, 2);
        S32(st, FIGB_F(st) + 0x144) = 7;
        GAME_CALL(st, 0x004B17D0u, FIGB_CUR(st));
        break;
    case 8:
        GAME_CALL(st, 0x004ECD40u, id);
        break;
    case 9:
    case 0x0A:
    case 0x13:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            GAME_CALL(st, 0x004ECD40u, FIGB_CUR(st));
        }
        break;
    case 0x0C:
        if (S16(st, f + 0x36) == 0) {
            GAME_CALL(st, 0x004B5050u, id);
        }
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 500) {
            U8(st, f + 0x38) = 4;
        }
        break;
    case 0x0D:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        GAME_CALL(st, 0x004EAEC0u, FIGB_CUR(st));
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            to_pause(st, f);
        }
        break;
    case 0x12:
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) <= 0) {
            if (!wander_to(st, rec + 4)) {
                S16(st, FIGB_F(st) + 0x36) = 100;
            }
        }
        if (stepped) {
            GAME_CALL(st, 0x004EAEC0u, FIGB_CUR(st));
        }
        break;
    case 0x14:
        GAME_CALL(st, 0x004EAEC0u, id);
        break;
    case 0x15: {
        int32_t target = S16(st, f + 0x5E);
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) <= 0) {
            U8(st, f + 0x6B) = 0;
            U8(st, f + 0x38) = 0x17;
            break;
        }
        if (target != 0) {
            retarget(st, target);
        }
        GAME_CALL(st, 0x004A5DA0u, FIGB_CUR(st), speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10) {
            to_pause(st, f);
        } else if (dir == 8) {
            uint32_t t = FIGURE_ADDR(target);
            if (target != 0 && (S16(st, t + 0x14) != S16(st, f + 0x24) || S16(st, t + 0x16) != S16(st, f + 0x26))) {
                retarget(st, target);
            } else {
                to_pause(st, f);
            }
        }
        break;
    }
    case 0x17:
        if (!wander_to(st, rec + 4) && !wander_to(st, rec)) {
            U8(st, FIGB_F(st) + 0x0E) = 2;
            return;
        }
        break;
    case 0x18:
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) <= 0) {
            S32(st, f + 0x148) = 0;
            S32(st, f + 0x144) = 8;
            U8(st, f + 0x38) = 8;
        }
        U32(st, f + 0x04) = 0;
        return;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    uint32_t fr = 8u * U32(st, f + 0x148) + (uint32_t) d;
    switch (U8(st, f + 0x38)) { /* @bin 0x0049DC34 */
    case 4: {
        int16_t t = S16(st, f + 0x36);
        if (water) {
            U32(st, f + 0x04) = t >= 0x17 ? U32(st, 0x00F56E14u) + 0x16u : (uint32_t) t + U32(st, 0x00F56E14u);
        } else {
            U32(st, f + 0x04) = t >= 8 ? U32(st, 0x00F55DACu) + 7u : (uint32_t) t + U32(st, 0x00F55DACu);
        }
        break;
    }
    case 5:
        U32(st, f + 0x04) = U32(st, 0x00F55DB4u) + fr;
        break;
    case 8:
        U32(st, f + 0x04) = U32(st, 0x00F55DA4u) + (uint32_t) d;
        break;
    case 9:
    case 0x0A:
    case 0x13:
    case 0x15:
        U32(st, f + 0x04) = U32(st, water ? 0x00F55DBCu : 0x00F55DA4u) + fr;
        break;
    case 0x17:
        U32(st, f + 0x04) = U32(st, 0x00F55DA4u) + fr;
        break;
    default:
        U32(st, f + 0x04) = water ? U32(st, 0x00F55DC4u) + fr : U32(st, 0x00F55DA4u) + (uint32_t) d;
        break;
    }
}
