/* Imhotep - the gods' blessings and wrath (package day2): 0x004C47A0, called once a month by the
 * gods' pass 0x004E9890 (day_city.c) with param -1, and by the cheat keys 0x0041CA80.. with a
 * forced outcome (1 blessing, 2 wrath, 3 small blessing... see below).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_gods.h"

#include "city/month_message.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define POPUP 0x00E950DCu
#define GOD_MODE(g) (0x007848E2u + 2u * (uint32_t) (g)) /* i16 */
#define GOD_MOOD 0x439Au      /* i8 per god */
#define GOD_WRATH 0x43A4u     /* u8 per god */
#define GOD_BLESSING 0x4816u  /* u8 per god */
#define GOD_MONTHS 0x43E0u    /* i32 per god */
#define FLOOD 0x00D38360u     /* `this` of 0x004BD5E0 */

static void post(game_state *st, int32_t id)
{
    message_post(st, id, 0, 0, NULL, 0);
}

/* @bin 0x004C4842 rand() % 2 == 0 (signed remainder: and 0x80000001 with the sign fix-up) */
static int even(game_state *st)
{
    return crt_rand(st) % 2 == 0;
}

/* @bin 0x004C485C the message 0x83 of a blessing or wrath of the pyramid gods: block +0 `first`,
 * +2 0x251, +0x10 by god. The original's block is its own stack frame (0x20 bytes at esp+0x10)
 * and it writes only these three words: +0x04..+0x0F and +0x12..+0x1D are whatever the stack
 * held, and message_post copies them into the message record (+0x14..+0x1F, +0x22..+0x2D).
 * They are 0 here (see the package report). */
static void post_pyramid(game_state *st, int32_t god, int32_t first)
{
    uint8_t blk[32] = {0};
    blk[0] = (uint8_t) first;
    blk[1] = (uint8_t) (first >> 8);
    blk[2] = 0x51;
    blk[3] = 0x02;
    if ((uint32_t) god <= 4) {
        int32_t v = 0x254 + god; /* @bin 0x004C50A8 / 0x004C50D0 */
        blk[0x10] = (uint8_t) v;
        blk[0x11] = (uint8_t) (v >> 8);
    }
    S32(st, POPUP) = 1;
    message_post(st, 0x83, 0, 0, blk, 0);
}

void god_act(game_state *st, int32_t god, int32_t kind, int32_t force)
{
    (void) kind; /* @bin 0x004C47A0 the second argument is not read */
    if (god > 4 || S16(st, GOD_MODE(god)) == 0) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    uint32_t months = c + GOD_MONTHS + 4u * (uint32_t) god;
    int8_t blessing = S8(st, c + GOD_BLESSING + (uint32_t) god);
    if ((blessing == 0x32 && S32(st, months) < 0xF && force == -1) || force == 1) {
        /* @bin 0x004C4819 a blessing */
        U8(st, c + GOD_BLESSING + (uint32_t) god) = 0;
        S32(st, POPUP) = 1;
        U8(st, c + GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + GOD_MOOD + (uint32_t) god) + 0xE2);
        if (GAME_CALL0(st, 0x004F5340u) != 0 && even(st)) {
            post_pyramid(st, god, 0x252);
            if (GAME_CALL0(st, 0x004F5340u) == 2) {
                GAME_CALL(st, 0x004F3AA0u, 2);
            } else {
                GAME_CALL(st, 0x004F3AA0u, 1);
            }
            return;
        }
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x127);
                S32(st, CURRENT_CITY(st) + 0x4994) = 1; /* 0x00EA6204 */
            } else {
                post(st, 0x154);
                GAME_CALLT(st, FLOOD, 0x004BD5E0u, ((crt_rand(st) % 3) * 5 + 10) * 2);
            }
            return;
        case 1:
            if (even(st)) {
                post(st, 0x128);
                S32(st, CURRENT_CITY(st) + 0x45CC) = 0xC; /* 0x00EA5E3C */
            } else {
                post(st, 0x149);
                GAME_CALL(st, 0x004E96C0u, 0xF);
            }
            return;
        case 2:
            post(st, GAME_CALL0(st, 0x004C3950u) != 0 ? 0x129 : 0x91);
            return;
        case 3:
            post(st, 0x12A);
            S32(st, CURRENT_CITY(st) + 0x45D0) = 0xA; /* 0x00EA5E40 */
            return;
        case 4:
            post(st, 0x12B);
            GAME_CALL0(st, 0x004C42B0u);
            return;
        default:
            return;
        }
    }
    if ((blessing >= 0x14 && S32(st, months) < 0xF && force == -1) || force == 2) {
        /* @bin 0x004C4A83 a small blessing */
        U8(st, c + GOD_BLESSING + (uint32_t) god) = 0;
        S32(st, POPUP) = 1;
        U8(st, c + GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + GOD_MOOD + (uint32_t) god) + 0xF4);
        if (GAME_CALL0(st, 0x004F5340u) == 2 && even(st)) {
            post_pyramid(st, god, 0x253);
            GAME_CALL(st, 0x004F3AA0u, 1);
            return;
        }
        switch (god) {
        case 0: {
            post(st, 0x141);
            int32_t r = crt_rand(st) % 4;
            GAME_CALLT(st, FLOOD, 0x004BD5E0u, r * 5 + 5);
            return;
        }
        case 1:
            if (even(st)) {
                post(st, 0x14B);
                S16(st, CURRENT_CITY(st) + 0x4984) = 0xC; /* 0x00EA61F4 */
            } else {
                post(st, 0x142);
                GAME_CALL(st, 0x004E96C0u, 5);
            }
            return;
        case 2:
            post(st, 0x143);
            GAME_CALL0(st, 0x004C3AC0u);
            return;
        case 3:
            post(st, 0x144);
            S32(st, CURRENT_CITY(st) + 0x498C) = 0xA; /* 0x00EA61FC */
            return;
        case 4: {
            post(st, 0x145);
            uint32_t k = CURRENT_CITY(st);
            S32(st, k + 0x44F8) = 4; /* 0x00EA5D68 */
            S32(st, k + 0x44FC) = 1;
            S32(st, k + 0x4528) = 1;
            S32(st, k + 0x4500) = 0;
            GAME_CALL0(st, 0x004EA000u);
            k = CURRENT_CITY(st);
            S32(st, k + 0x43E4) = 0; /* 0x00EA5C54.. */
            S32(st, k + 0x43E8) = 0;
            S32(st, k + 0x43EC) = 0;
            return;
        }
        default:
            return;
        }
    }
    int8_t wrath = S8(st, c + GOD_WRATH + (uint32_t) god);
    if ((wrath == 0x32 && S32(st, months) > 3 && force == -1) || force == 3) {
        /* @bin 0x004C4CD3 wrath */
        U8(st, c + GOD_WRATH + (uint32_t) god) = 0;
        U8(st, c + GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + GOD_MOOD + (uint32_t) god) + 0x1E);
        S32(st, POPUP) = 1;
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x8C);
                GAME_CALLT(st, FLOOD, 0x004BD5E0u, (-2 - crt_rand(st) % 3) * 10);
            } else {
                post(st, 0x93);
                GAME_CALL0(st, 0x004E6EB0u);
            }
            return;
        case 1:
            if (crt_rand(st) % 3 == 0) {
                post(st, 0x14C);
                S16(st, CURRENT_CITY(st) + 0x4986) = 0xC; /* 0x00EA61F6 */
            } else if (crt_rand(st) % 3 == 1) {
                post(st, 0x118);
                GAME_CALL(st, 0x004E96C0u, -0xF);
            } else {
                post(st, 0x14D);
                S32(st, CURRENT_CITY(st) + 0x47B4) = 0xC; /* 0x00EA6024 */
            }
            return;
        case 2:
            if (even(st)) {
                post(st, GAME_CALL0(st, 0x004C3ED0u) != 0 ? 0x8E : 0x8D);
            } else {
                post(st, 0x94);
                GAME_CALL(st, 0x004E70B0u, 6);
            }
            return;
        case 3:
            if (even(st)) {
                post(st, 0x119);
                GAME_CALL0(st, 0x004B4710u);
            } else {
                post(st, 0x95);
                GAME_CALL0(st, 0x004E6ED0u);
            }
            return;
        case 4:
            post(st, 0x90);
            GAME_CALL0(st, 0x004C45E0u);
            return;
        default:
            return;
        }
    }
    if ((wrath >= 0x14 && S32(st, months) > 3 && force == -1) || force == 4) {
        /* @bin 0x004C4F05 small wrath */
        U8(st, c + GOD_WRATH + (uint32_t) god) = 0;
        S32(st, POPUP) = 1;
        U8(st, c + GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + GOD_MOOD + (uint32_t) god) + 0xC);
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x122);
                S32(st, CURRENT_CITY(st) + 0x4998) = 1; /* 0x00EA6208 */
            } else {
                post(st, 0x155);
                int32_t r = crt_rand(st) % 4;
                GAME_CALLT(st, FLOOD, 0x004BD5E0u, (-1 - r) * 5);
            }
            return;
        case 1:
            if (even(st)) {
                post(st, 0x14E);
                S16(st, CURRENT_CITY(st) + 0x4988) = 0xC; /* 0x00EA61F8 */
            } else {
                post(st, 0x123);
                GAME_CALL(st, 0x004E96C0u, -5);
            }
            return;
        case 2:
            if (S32(st, 0x00705DF4u) + S32(st, 0x00705C10u) != 0) {
                post(st, 0x124);
                GAME_CALL(st, 0x004C4020u, 1);
            } else {
                post(st, 0x8D);
            }
            return;
        case 3:
            post(st, GAME_CALL0(st, 0x004C41B0u) != 0 ? 0x125 : 0x8F);
            return;
        case 4:
            post(st, 0x126);
            GAME_CALL(st, 0x0042FCF0u, 1);
            return;
        default:
            return;
        }
    }
}

static int32_t run_god_act(game_state *st, const int32_t *a)
{
    god_act(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function day2_gods_ports[] = {
    {0x004C47A0u, "god_act", 3, 0, 0, run_god_act, 0},
};
const int day2_gods_ports_count = sizeof(day2_gods_ports) / sizeof(day2_gods_ports[0]);
