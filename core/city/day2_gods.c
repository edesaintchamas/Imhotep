/* Imhotep - the gods' blessings and wrath (package day2): 0x004C47A0, called once a month by the
 * gods' pass 0x004E9890 (day_city.c) with param -1, and by the cheat keys 0x0041CA80.. with a
 * forced outcome (1 blessing, 2 wrath, 3 small blessing... see below).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_gods.h"

#include "city/month_message.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define GOD_MODE(g) (G_GOD_MODE + 2u * (uint32_t) (g)) /* i16 */

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
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x83, 0, 0, blk, 0);
}

void god_act(game_state *st, int32_t god, int32_t kind, int32_t force)
{
    (void) kind; /* @bin 0x004C47A0 the second argument is not read */
    if (god > 4 || S16(st, GOD_MODE(god)) == 0) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    uint32_t months = c + CITY_GOD_MONTHS + 4u * (uint32_t) god;
    int8_t blessing = S8(st, c + CITY_GOD_BLESSING + (uint32_t) god);
    if ((blessing == 0x32 && S32(st, months) < 0xF && force == -1) || force == 1) {
        /* @bin 0x004C4819 a blessing */
        U8(st, c + CITY_GOD_BLESSING + (uint32_t) god) = 0;
        S32(st, G_MESSAGE_POPUP) = 1;
        U8(st, c + CITY_GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + CITY_GOD_MOOD + (uint32_t) god) + 0xE2);
        if (GAME_CALL0(st, FN_PYRAMID_PROGRESS_4F5340) != 0 && even(st)) {
            post_pyramid(st, god, 0x252);
            if (GAME_CALL0(st, FN_PYRAMID_PROGRESS_4F5340) == 2) {
                GAME_CALL(st, FN_PYRAMID_SPEEDUP_4F3AA0, 2);
            } else {
                GAME_CALL(st, FN_PYRAMID_SPEEDUP_4F3AA0, 1);
            }
            return;
        }
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x127);
                S32(st, CURRENT_CITY(st) + CITY_U4994) = 1; /* 0x00EA6204 */
            } else {
                post(st, 0x154);
                GAME_CALLT(st, G_FLOODPLAIN, FN_FLOOD_QUALITY_ADD, ((crt_rand(st) % 3) * 5 + 10) * 2);
            }
            return;
        case 1:
            if (even(st)) {
                post(st, 0x128);
                S32(st, CURRENT_CITY(st) + CITY_U45CC) = 0xC; /* 0x00EA5E3C */
            } else {
                post(st, 0x149);
                GAME_CALL(st, FN_CITY_ADD_0X42D8, 0xF);
            }
            return;
        case 2:
            post(st, GAME_CALL0(st, FN_STORAGE_FILL_4C3950) != 0 ? 0x129 : 0x91);
            return;
        case 3:
            post(st, 0x12A);
            S32(st, CURRENT_CITY(st) + CITY_U45D0) = 0xA; /* 0x00EA5E40 */
            return;
        case 4:
            post(st, 0x12B);
            GAME_CALL0(st, FN_HOUSES_STOCK_4C42B0);
            return;
        default:
            return;
        }
    }
    if ((blessing >= 0x14 && S32(st, months) < 0xF && force == -1) || force == 2) {
        /* @bin 0x004C4A83 a small blessing */
        U8(st, c + CITY_GOD_BLESSING + (uint32_t) god) = 0;
        S32(st, G_MESSAGE_POPUP) = 1;
        U8(st, c + CITY_GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + CITY_GOD_MOOD + (uint32_t) god) + 0xF4);
        if (GAME_CALL0(st, FN_PYRAMID_PROGRESS_4F5340) == 2 && even(st)) {
            post_pyramid(st, god, 0x253);
            GAME_CALL(st, FN_PYRAMID_SPEEDUP_4F3AA0, 1);
            return;
        }
        switch (god) {
        case 0: {
            post(st, 0x141);
            int32_t r = crt_rand(st) % 4;
            GAME_CALLT(st, G_FLOODPLAIN, FN_FLOOD_QUALITY_ADD, r * 5 + 5);
            return;
        }
        case 1:
            if (even(st)) {
                post(st, 0x14B);
                S16(st, CURRENT_CITY(st) + CITY_U4984) = 0xC; /* 0x00EA61F4 */
            } else {
                post(st, 0x142);
                GAME_CALL(st, FN_CITY_ADD_0X42D8, 5);
            }
            return;
        case 2:
            post(st, 0x143);
            GAME_CALL0(st, FN_WORKSHOPS_FILL_4C3AC0);
            return;
        case 3:
            post(st, 0x144);
            S32(st, CURRENT_CITY(st) + CITY_U498C) = 0xA; /* 0x00EA61FC */
            return;
        case 4: {
            post(st, 0x145);
            uint32_t k = CURRENT_CITY(st);
            S32(st, k + CITY_FESTIVAL_SIZE) = 4; /* 0x00EA5D68 */
            S32(st, k + CITY_FESTIVAL_MONTHS) = 1;
            S32(st, k + CITY_U4528) = 1;
            S32(st, k + CITY_FESTIVAL_GOD) = 0;
            GAME_CALL0(st, FN_MONTH_0X004EA000);
            k = CURRENT_CITY(st);
            S32(st, k + CITY_GOD_MONTHS + 4 * 1) = 0; /* 0x00EA5C54.. */
            S32(st, k + CITY_GOD_MONTHS + 4 * 2) = 0;
            S32(st, k + CITY_GOD_MONTHS + 4 * 3) = 0;
            return;
        }
        default:
            return;
        }
    }
    int8_t wrath = S8(st, c + CITY_GOD_WRATH + (uint32_t) god);
    if ((wrath == 0x32 && S32(st, months) > 3 && force == -1) || force == 3) {
        /* @bin 0x004C4CD3 wrath */
        U8(st, c + CITY_GOD_WRATH + (uint32_t) god) = 0;
        U8(st, c + CITY_GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + CITY_GOD_MOOD + (uint32_t) god) + 0x1E);
        S32(st, G_MESSAGE_POPUP) = 1;
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x8C);
                GAME_CALLT(st, G_FLOODPLAIN, FN_FLOOD_QUALITY_ADD, (-2 - crt_rand(st) % 3) * 10);
            } else {
                post(st, 0x93);
                GAME_CALL0(st, FN_PLAGUE_FROGS_END);
            }
            return;
        case 1:
            if (crt_rand(st) % 3 == 0) {
                post(st, 0x14C);
                S16(st, CURRENT_CITY(st) + CITY_U4986) = 0xC; /* 0x00EA61F6 */
            } else if (crt_rand(st) % 3 == 1) {
                post(st, 0x118);
                GAME_CALL(st, FN_CITY_ADD_0X42D8, -0xF);
            } else {
                post(st, 0x14D);
                S32(st, CURRENT_CITY(st) + CITY_U47B4) = 0xC; /* 0x00EA6024 */
            }
            return;
        case 2:
            if (even(st)) {
                post(st, GAME_CALL0(st, FN_BUILDINGS_DESTROY_4C3ED0) != 0 ? 0x8E : 0x8D);
            } else {
                post(st, 0x94);
                GAME_CALL(st, FN_PLAGUE_FROGS, 6);
            }
            return;
        case 3:
            if (even(st)) {
                post(st, 0x119);
                GAME_CALL0(st, FN_WORKERS_RECALLED_4B4710);
            } else {
                post(st, 0x95);
                GAME_CALL0(st, FN_PLAGUE_HAIL);
            }
            return;
        case 4:
            post(st, 0x90);
            GAME_CALL0(st, FN_HOUSES_DESTROY_4C45E0);
            return;
        default:
            return;
        }
    }
    if ((wrath >= 0x14 && S32(st, months) > 3 && force == -1) || force == 4) {
        /* @bin 0x004C4F05 small wrath */
        U8(st, c + CITY_GOD_WRATH + (uint32_t) god) = 0;
        S32(st, G_MESSAGE_POPUP) = 1;
        U8(st, c + CITY_GOD_MOOD + (uint32_t) god) = (uint8_t) (U8(st, c + CITY_GOD_MOOD + (uint32_t) god) + 0xC);
        switch (god) {
        case 0:
            if (even(st)) {
                post(st, 0x122);
                S32(st, CURRENT_CITY(st) + CITY_U4998) = 1; /* 0x00EA6208 */
            } else {
                post(st, 0x155);
                int32_t r = crt_rand(st) % 4;
                GAME_CALLT(st, G_FLOODPLAIN, FN_FLOOD_QUALITY_ADD, (-1 - r) * 5);
            }
            return;
        case 1:
            if (even(st)) {
                post(st, 0x14E);
                S16(st, CURRENT_CITY(st) + CITY_U4988) = 0xC; /* 0x00EA61F8 */
            } else {
                post(st, 0x123);
                GAME_CALL(st, FN_CITY_ADD_0X42D8, -5);
            }
            return;
        case 2:
            if (S32(st, G_U705DF4) + S32(st, G_U705C10) != 0) {
                post(st, 0x124);
                GAME_CALL(st, FN_STORAGE_STRIKE_4C4020, 1);
            } else {
                post(st, 0x8D);
            }
            return;
        case 3:
            post(st, GAME_CALL0(st, FN_FORT_DESTROY_4C41B0) != 0 ? 0x125 : 0x8F);
            return;
        case 4:
            post(st, 0x126);
            GAME_CALL(st, FN_CITY_RIOT, 1);
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
