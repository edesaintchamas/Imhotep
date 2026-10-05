/* Imhotep - figure actions, group b: figure types 0x43, 0x44.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"

void figb_action_43(game_state *st)
{
    /* @bin 0x0049B610 a 36-frame animation placed once (+0x36 < 1000) on a tile from 0x00480E90 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x77) = 0;
    U8(st, f + 0x76) = 1;
    uint8_t a = (uint8_t) (U8(st, f + 0x01) + 1);
    U8(st, f + 0x01) = a;
    if (a > 0x23) {
        U8(st, f + 0x0E) = 2;
    }
    if (S16(st, f + 0x36) < 1000) {
        GAME_CALL(st, 0x004B5420u, id);
        int32_t r = GAME_CALL(st, 0x00480E90u, FIGB_CUR(st));
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (r != 0) {
            U16(st, f + 0x14) = U16(st, 0x00D3B4E4u);
            int16_t y = S16(st, 0x00D3B4E0u);
            S16(st, f + 0x16) = y;
            U32(st, f + 0x20) = U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14) + 228u * (uint32_t) y;
            S16(st, f + 0x4A) = (int16_t) (S16(st, f + 0x14) * 15);
            S16(st, f + 0x4C) = (int16_t) (S16(st, f + 0x16) * 15);
            S16(st, f + 0x4A) += 7;
            S16(st, f + 0x4C) += 7;
        }
        GAME_CALL(st, 0x004B52F0u, id);
        f = FIGB_F(st);
        S16(st, f + 0x36) = 1000;
    }
    U32(st, f + 0x04) = U8(st, f + 0x01) + U32(st, 0x00F56DBCu);
}

void figb_action_44(game_state *st)
{
    /* @bin 0x0049B750 an animal of a herd (+0x60, records of 0x90 bytes at 0x00BCB9C8): waits,
     * walks to a random tile around the herd point (0x004EAB10), flees, is hunted */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + 0x148) += 1;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    int32_t hb = S32(st, f + 0x138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + 0x10) != 0x73 || U8(st, hba + 0xA6) == 0 || U8(st, hba) != 1 || S16(st, hba + 0x40) == 0) {
        S32(st, f + 0x138) = 0;
    }
    if (S32(st, f + 0x148) >= S32(st, f + 0x144)) {
        S32(st, f + 0x148) = 0;
        U8(st, f + 0x01) += 1;
    }
    U8(st, f + 0x72) = 5;
    S16(st, CURRENT_CITY(st) + 0x447A) += 1; /* 0x00EA5CEA: animals counted */
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 0;
    int32_t herd = S16(st, f + 0x60);
    figb_anim_advance(st, f, 0x32);
    int8_t dir;
    switch (U8(st, f + 0x38)) { /* @bin 0x0049BD0C */
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 8:
    case 0x12: {
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) > 0) {
            break;
        }
        random_next_state(st);
        uint32_t rec = 0x00BCB9FEu + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, 0x004EAB10u, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, 0x00D3B370u) & 0xF),
                              S16(st, f + 0x14), S16(st, f + 0x16), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            /* @bin 0x005CDB78 offsets (x, y) by direction, 8 bytes each */
            S16(st, f + 0x24) = (int16_t) (S16(st, 0x005CDB78u + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + 0x26) = (int16_t) (S16(st, 0x005CDB7Cu + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + 0x38) = 0x0A;
        } else {
            S16(st, f + 0x36) = 100;
        }
        break;
    }
    case 9:
    case 0x0A:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + 0x0E) = 2;
        } else if (dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x12;
            random_next_state(st);
            S16(st, f + 0x36) = (int16_t) (S32(st, 0x00D3B370u) % 0x96 + 0x96);
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
    case 0x0E:
    case 0x0F:
        GAME_CALL(st, 0x004EB000u, id);
        break;
    case 0x10:
    case 0x11:
        GAME_CALL(st, 0x004A5DA0u, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0E;
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
    figb_dir_rel(st);
    f = FIGB_F(st);
    uint8_t act = U8(st, f + 0x38);
    if (act == 4 || act == 0x0C) { /* @bin 0x0049BD30 */
        int16_t t = S16(st, f + 0x36);
        U32(st, f + 0x04) = t >= 0x19 ? 0u : (uint32_t) (t + (int32_t) U32(st, 0x00F56084u));
        return;
    }
    U32(st, f + 0x04) = (uint32_t) (U8(st, f + 0x01) >> 1) + U32(st, 0x00F5607Cu);
}

/* @bin 0x0049BF3A the walk after a pause: 0x12 when bit 0 of the random value is set, else 0x13 */
static uint8_t pause_kind(game_state *st)
{
    return (U32(st, 0x00D3B370u) & 1) ? 0x12 : 0x13;
}

/* @bin 0x0049C3E7 a pause of 150..299 ticks */
static void pause_random(game_state *st, uint32_t f)
{
    S16(st, f + 0x36) = (int16_t) (S32(st, 0x00D3B370u) % 0x96 + 0x96);
}

void figb_action_45(game_state *st)
{
    /* @bin 0x0049BD40 an animal of a herd (+0x60, point at +4/+6 of the 0x90-byte records at
     * 0x00BCB9FE): grazes, wanders, flees, is hunted. +0x148 counts ticks per frame (+0x144). */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    U8(st, f + 0x72) = 5;
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 0;
    S16(st, CURRENT_CITY(st) + 0x447A) += 1; /* 0x00EA5CEA */
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    int32_t herd = S16(st, f + 0x60);
    S32(st, f + 0x148) += 1;
    int32_t hb = S32(st, f + 0x138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + 0x10) != 0x73 || U8(st, hba + 0xA6) == 0 || U8(st, hba) != 1 || S16(st, hba + 0x40) == 0) {
        S32(st, f + 0x138) = 0;
    }
    if (S32(st, f + 0x148) >= S32(st, f + 0x144)) {
        S32(st, f + 0x148) = 0;
        U8(st, f + 0x01) += 1;
        stepped = 1;
    }
    int8_t dir;
    int to_eb000 = 0; /* @bin 0x0049C407 */
    switch (U8(st, f + 0x38)) { /* @bin 0x0049C704 */
    case 4:
        S32(st, f + 0x144) = 8;
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        S32(st, f + 0x144) = 7;
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 8:
        random_next_state(st);
        U8(st, f + 0x38) = pause_kind(st);
        S32(st, f + 0x148) = 0;
        S32(st, f + 0x144) = 8;
        random_next_state(st);
        pause_random(st, f);
        return;
    case 9:
    case 0x0A:
        S32(st, f + 0x144) = 0x0C;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
            f = FIGB_F(st);
            U8(st, f + 0x11) = U8(st, f + 0x12);
        } else if (dir == 10) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = (int16_t) (U8(st, f + 0x62) * 5);
            S32(st, f + 0x148) = 0;
            return;
        } else if (dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            random_next_state(st);
            U8(st, f + 0x38) = pause_kind(st);
            random_next_state(st);
            pause_random(st, f);
        }
        to_eb000 = 1;
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
    case 0x0E:
    case 0x0F:
        S32(st, f + 0x144) = 1;
        GAME_CALL(st, 0x004EB000u, id);
        break;
    case 0x10:
    case 0x11:
        S32(st, f + 0x144) = 0x0C;
        S32(st, f + 0x148) += 1;
        if (S32(st, f + 0x148) >= 0x0C) {
            S32(st, f + 0x148) = 0;
        }
        GAME_CALL(st, 0x004A5DA0u, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
            f = FIGB_F(st);
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0E;
        } else if (dir == 10) {
            U8(st, f + 0x0E) = 2;
            return;
        } else if (dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0E;
            S32(st, f + 0x148) = 0;
            return;
        }
        break;
    case 0x12: {
        S32(st, f + 0x144) = 8;
        int16_t t = (int16_t) (S16(st, f + 0x36) - 1);
        S16(st, f + 0x36) = t;
        if (t < 0) {
            S16(st, f + 0x36) = 0;
        }
        if (t > 0) {
            to_eb000 = 1;
            break;
        }
        if (!stepped) {
            break;
        }
        random_next_state(st);
        uint32_t rec = 0x00BCBA02u + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, 0x004EAB10u, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, 0x00D3B370u) & 0xF),
                              S16(st, f + 0x14), S16(st, f + 0x16), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            S16(st, f + 0x24) = (int16_t) (S16(st, 0x005CDB78u + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + 0x26) = (int16_t) (S16(st, 0x005CDB7Cu + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + 0x38) = 0x0A;
            return;
        }
        S16(st, f + 0x36) = 100;
        to_eb000 = 1;
        break;
    }
    case 0x13:
        S32(st, f + 0x144) = 8;
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) > 0) {
            to_eb000 = 1;
            break;
        }
        if (stepped) {
            U8(st, f + 0x38) = 0x12;
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
    if (to_eb000 && stepped) {
        GAME_CALL(st, 0x004EB000u, FIGB_CUR(st));
    }
    /* @bin 0x0049C547 direction kept in 0..7 */
    f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11) - S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
        if (d < 0) {
            d += (int32_t) (((uint32_t) (7 - d) >> 3) * 8u);
            S32(st, FIGB_DIR_REL) = d;
        }
    }
    if (d >= 8) {
        d -= (int32_t) (((uint32_t) d >> 3) * 8u);
        S32(st, FIGB_DIR_REL) = d;
    }
    if (S32(st, f + 0x148) > S32(st, f + 0x144)) {
        S32(st, f + 0x148) -= S32(st, f + 0x144);
    }
    if (S32(st, f + 0x148) < 0) {
        S32(st, f + 0x148) = 0;
    }
    uint32_t fr = 8u * U32(st, f + 0x148) + (uint32_t) d;
    switch (U8(st, f + 0x38)) { /* @bin 0x0049C738 */
    case 4:
        U32(st, f + 0x04) = U32(st, 0x00F561D4u) + 7u;
        break;
    case 5:
        U32(st, f + 0x04) = U32(st, 0x00F561DCu) + fr;
        break;
    case 8:
    case 0x12:
        U32(st, f + 0x04) = U32(st, 0x00F561E4u) + fr;
        break;
    case 0x0C: {
        int16_t t = S16(st, f + 0x36);
        U32(st, f + 0x04) = t >= 8 ? U32(st, 0x00F561D4u) + 7u : (uint32_t) t + U32(st, 0x00F561D4u);
        break;
    }
    case 0x0E:
    case 0x0F:
        U32(st, f + 0x04) = U32(st, 0x00F561ECu) + (uint32_t) d;
        break;
    case 0x13:
        U32(st, f + 0x04) = U32(st, 0x00F561ECu) + fr;
        break;
    default:
        U32(st, f + 0x04) = U32(st, 0x00F561CCu) + fr;
        break;
    }
}

void figb_action_46(game_state *st)
{
    /* @bin 0x0049C750 another herd animal, as 0x0049BD40 with its own moves and images */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    U8(st, f + 0x72) = 5;
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 0;
    S16(st, CURRENT_CITY(st) + 0x447A) += 1; /* 0x00EA5CEA */
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    int32_t herd = S16(st, f + 0x60);
    S32(st, f + 0x148) += 1;
    int32_t hb = S32(st, f + 0x138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + 0x10) != 0x73 || U8(st, hba + 0xA6) == 0 || U8(st, hba) != 1 || S16(st, hba + 0x40) == 0) {
        S32(st, f + 0x138) = 0;
    }
    if (S32(st, f + 0x148) >= S32(st, f + 0x144)) {
        S32(st, f + 0x148) = 0;
        U8(st, f + 0x01) += 1;
        stepped = 1;
    }
    int8_t dir;
    int to_eb000 = 0; /* @bin 0x0049CBD2 */
    switch (U8(st, f + 0x38)) { /* @bin 0x0049D0A8 */
    case 4:
        S32(st, f + 0x144) = 8;
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        S32(st, f + 0x144) = 7;
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 8:
        random_next_state(st);
        U8(st, f + 0x38) = pause_kind(st);
        S32(st, f + 0x148) = 0;
        S32(st, f + 0x144) = 8;
        random_next_state(st);
        pause_random(st, f);
        return;
    case 9:
    case 0x0A:
        S32(st, f + 0x144) = 0x0C;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
            to_eb000 = 1;
        } else if (dir == 10) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = (int16_t) (U8(st, f + 0x62) * 5);
            S32(st, f + 0x148) = 0;
            return;
        } else if (dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            random_next_state(st);
            U8(st, f + 0x38) = pause_kind(st);
            random_next_state(st);
            pause_random(st, f);
        } else {
            to_eb000 = 1;
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
    case 0x0E:
    case 0x0F:
        S32(st, f + 0x144) = 1;
        GAME_CALL(st, 0x004EB000u, id);
        break;
    case 0x10:
    case 0x11:
        S32(st, f + 0x144) = 8;
        S32(st, f + 0x148) += 1;
        if (S32(st, f + 0x148) >= 0x0C) {
            S32(st, f + 0x148) = 0;
        }
        GAME_CALL(st, 0x004A5DA0u, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0E;
            S32(st, f + 0x148) = 0;
            return;
        }
        break;
    case 0x12: {
        S32(st, f + 0x144) = 8;
        int16_t t = (int16_t) (S16(st, f + 0x36) - 1);
        S16(st, f + 0x36) = t;
        if (t < 0) {
            S16(st, f + 0x36) = 0;
        }
        if (t > 0) {
            to_eb000 = 1;
            break;
        }
        if (!stepped) {
            break;
        }
        random_next_state(st);
        uint32_t rec = 0x00BCBA02u + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, 0x004EAB10u, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, 0x00D3B370u) & 0xF),
                              S16(st, f + 0x14), S16(st, f + 0x16), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            S16(st, f + 0x24) = (int16_t) (S16(st, 0x005CDB78u + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + 0x26) = (int16_t) (S16(st, 0x005CDB7Cu + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + 0x38) = 0x0A;
            return;
        }
        S16(st, f + 0x36) = 100;
        to_eb000 = 1;
        break;
    }
    case 0x13:
        S32(st, f + 0x144) = 8;
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) > 0) {
            to_eb000 = 1;
            break;
        }
        if (stepped) {
            U8(st, f + 0x38) = 0x12;
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
        U8(st, f + 0x65) = 1;
        return;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    if (to_eb000 && stepped) {
        GAME_CALL(st, 0x004EB000u, FIGB_CUR(st));
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    uint32_t fr = 8u * U32(st, f + 0x148) + (uint32_t) d;
    switch (U8(st, f + 0x38)) { /* @bin 0x0049D0E0 */
    case 4:
        U32(st, f + 0x04) = U32(st, 0x00F56EB4u) + 7u;
        break;
    case 5:
        U32(st, f + 0x04) = U32(st, 0x00F56EBCu) + fr;
        break;
    case 8:
    case 0x0E:
    case 0x0F:
        U32(st, f + 0x04) = U32(st, 0x00F56EACu) + (uint32_t) d;
        break;
    case 0x0C: {
        int16_t t = S16(st, f + 0x36);
        U32(st, f + 0x04) = t >= 8 ? U32(st, 0x00F56EB4u) + 7u : (uint32_t) t + U32(st, 0x00F56EB4u);
        break;
    }
    case 0x10:
    case 0x11:
        U32(st, f + 0x04) = U32(st, 0x00F56ED4u) + fr;
        break;
    case 0x12:
        U32(st, f + 0x04) = U32(st, 0x00F56EC4u) + fr;
        break;
    case 0x13:
        U32(st, f + 0x04) = U32(st, 0x00F56ECCu) + fr;
        break;
    default:
        U32(st, f + 0x04) = U32(st, 0x00F56EACu) + fr;
        break;
    }
}
