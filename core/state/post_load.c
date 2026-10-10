/* Imhotep - per-building records rebuilt after a load (Pharaoh.exe 0x0046B320, 0x0046F030,
 * 0x0046A320, called by 0x004D5100 after the scenario start).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/post_load.h"

#include <string.h>

#include "city/goods.h"
#include "state/layout.h"

static uint32_t building(int32_t id)
{
    return BUILDING_ADDR(id);
}

/* --- storage records: 0x007900B8 + id * 0xC4, ids 1..199 ----------------------------------------
 * +0x04 i16 building, +0x08 u8 in use, +0x0A + resource u8 accepted, +0x34 and +0x7C i16[36] */
#define STORAGE_SIZE 0xC4u
#define STORAGE_LAST 199     /* loops end at 0x007999E0 */

static int is_storage_type(int16_t t)
{
    return t == 0x47 || t == 0x48 || t == 0x46 || t == 0xBF || t == 0xC1 || t == 0xBE;
}

int32_t storage_create(game_state *st, int32_t building_id)
{
    /* @bin 0x0046B100 */
    goods_available_update(st);
    int32_t id = 1;
    while (U8(st, G_STORAGE + STORAGE_SIZE * (uint32_t) id + STORAGE_IN_USE) != 0) {
        if (++id > STORAGE_LAST) {
            return 0;
        }
    }
    uint32_t r = G_STORAGE + STORAGE_SIZE * (uint32_t) id;
    memset(state_ptr(st, r, STORAGE_SIZE), 0, STORAGE_SIZE);
    U8(st, r + STORAGE_IN_USE) = 1;
    S16(st, r + STORAGE_BUILDING) = (int16_t) building_id;
    if (S32(st, G_STORAGE_HIGHEST) <= id) {
        S32(st, G_STORAGE_HIGHEST) = id;
    }
    int32_t n = S32(st, G_STORAGE_LIST);
    switch (S16(st, building(building_id) + BLD_TYPE)) {
    case 0x46:
    case 0xBE:
        for (int32_t i = 0; i < n; i++) {
            int32_t g = S32(st, G_GOODS_AVAILABLE + 4u * (uint32_t) i);
            if (g >= 1 && g <= 8 ? 1 : (g == 0xD || g == 0xF || g == 0x11 || g == 0x13)) { /* 0x005013A0 */
                U8(st, r + STORAGE_ACCEPTED + (uint32_t) g) = 0;
            }
        }
        break;
    case 0x47:
        for (uint32_t k = 0; k < 0x24; k++) {
            U8(st, r + STORAGE_ACCEPTED + k) = 1;
            S16(st, r + STORAGE_U34 + 2 * k) = 0xC80;
            S16(st, r + STORAGE_U7C + 2 * k) = 0xC80;
        }
        for (int32_t i = 0; i < S32(st, G_GOODS_COUNT); i++) {
            U8(st, r + STORAGE_ACCEPTED + (uint32_t) S32(st, G_GOODS_LIST + 4u * (uint32_t) i)) = 0;
        }
        break;
    case 0x48:
        for (uint32_t k = 0; k < 0x24; k++) {
            U8(st, r + STORAGE_ACCEPTED + k) = 1;
            S16(st, r + STORAGE_U34 + 2 * k) = 0xC80;
            S16(st, r + STORAGE_U7C + 2 * k) = 0xC80;
        }
        for (int32_t i = 0; i < n; i++) {
            int32_t g = S32(st, G_GOODS_AVAILABLE + 4u * (uint32_t) i);
            if (g < 1 || g > 8) {
                U8(st, r + STORAGE_ACCEPTED + (uint32_t) g) = 0;
            }
        }
        break;
    default:
        break;
    }
    return id;
}

void load_rebuild_storage_links(game_state *st)
{
    /* @bin 0x0046B320 */
    for (int32_t id = 1; id <= STORAGE_LAST; id++) {
        S16(st, G_STORAGE + STORAGE_SIZE * (uint32_t) id + STORAGE_BUILDING) = 0;
    }
    int32_t max = S32(st, G_BUILDING_HIGHEST);
    for (int32_t b = 1; b <= max; b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) != 0 && is_storage_type(S16(st, p + BLD_TYPE)) && U8(st, p + BLD_STORAGE_ID) != 0) {
            U8(st, G_STORAGE + STORAGE_SIZE * U8(st, p + BLD_STORAGE_ID) + STORAGE_IN_USE) = 1;
        }
    }
    for (int32_t b = 1; b <= S32(st, G_BUILDING_HIGHEST); b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) == 0 || !is_storage_type(S16(st, p + BLD_TYPE))) {
            continue;
        }
        uint8_t s = U8(st, p + BLD_STORAGE_ID);
        if (s != 0 && S16(st, G_STORAGE + STORAGE_SIZE * s + STORAGE_BUILDING) == 0) {
            S16(st, G_STORAGE + STORAGE_SIZE * s + STORAGE_BUILDING) = (int16_t) b;
        } else {
            U8(st, p + BLD_STORAGE_ID) = (uint8_t) storage_create(st, b);
        }
    }
    S32(st, G_STORAGE_HIGHEST) = 0;
    for (int32_t id = 1; id <= STORAGE_LAST; id++) {
        if (U8(st, G_STORAGE + STORAGE_SIZE * (uint32_t) id + STORAGE_IN_USE) != 0) {
            S32(st, G_STORAGE_HIGHEST) = id;
        }
    }
}

/* --- records of type-0x88 buildings: 0x00EAAF68 + id * 0xE0, ids 1..49 --------------------------
 * +0x00 u8 in use, +0xDC i32 building; the building's +0xCA holds the id */
#define REC88_SIZE 0xE0u
#define REC88_LAST 49

int32_t rec88_create(game_state *st, int32_t building_id)
{
    /* @bin 0x0046EFD0 the first record whose dword +0 is 0 */
    for (int32_t id = 1; id <= REC88_LAST; id++) {
        uint32_t r = G_FERRY_QUEUES + REC88_SIZE * (uint32_t) id;
        if (S32(st, r) == 0) {
            memset(state_ptr(st, r, REC88_SIZE), 0, REC88_SIZE);
            U8(st, r) = 1;
            S32(st, r + REC88_BUILDING) = building_id;
            return id;
        }
    }
    return 0;
}

void load_rebuild_0x88_links(game_state *st)
{
    /* @bin 0x0046F030 */
    for (int32_t id = 1; id <= REC88_LAST; id++) {
        S32(st, G_FERRY_QUEUES + REC88_SIZE * (uint32_t) id + REC88_BUILDING) = 0;
    }
    int32_t max = S32(st, G_BUILDING_HIGHEST);
    for (int32_t b = 1; b <= max; b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) != 0 && S16(st, p + BLD_TYPE) == 0x88 && U8(st, p + BLD_UCA) != 0) {
            U8(st, G_FERRY_QUEUES + REC88_SIZE * U8(st, p + BLD_UCA)) = 1;
        }
    }
    for (int32_t b = 1; max > 0 && b <= S32(st, G_BUILDING_HIGHEST); b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) == 0 || S16(st, p + BLD_TYPE) != 0x88) {
            continue;
        }
        uint8_t s = U8(st, p + BLD_UCA);
        if (s != 0 && S32(st, G_FERRY_QUEUES + REC88_SIZE * s + REC88_BUILDING) == 0) {
            S32(st, G_FERRY_QUEUES + REC88_SIZE * s + REC88_BUILDING) = b;
        } else {
            U8(st, p + BLD_UCA) = (uint8_t) rec88_create(st, b);
        }
    }
}

/* --- records of type-0x4A buildings: 0x009783A4 + id * 0x1C, ids 1..10 --------------------------
 * +0x00 u8 in use, +0x18 i32 building; the building's +0xCB holds the id */
#define REC4A_SIZE 0x1Cu
#define REC4A_LAST 10

void rec4a_create(game_state *st, int32_t building_id)
{
    /* @bin 0x0046A280 the first free record; none free: a debug message only */
    for (int32_t id = 1; id <= REC4A_LAST; id++) {
        uint32_t r = G_REC4A + REC4A_SIZE * (uint32_t) id;
        if (U8(st, r) == 0) {
            U8(st, building(building_id) + BLD_UCB) = (uint8_t) id;
            U8(st, r) = 1;
            S32(st, r + REC4A_BUILDING) = building_id;
            return;
        }
    }
}

void load_rebuild_0x4a_links(game_state *st)
{
    /* @bin 0x0046A320 */
    for (int32_t id = 1; id <= REC4A_LAST; id++) {
        S32(st, G_REC4A + REC4A_SIZE * (uint32_t) id + REC4A_BUILDING) = 0;
    }
    int32_t max = S32(st, G_BUILDING_HIGHEST);
    for (int32_t b = 1; b <= max; b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) != 0 && S16(st, p + BLD_TYPE) == 0x4A && U8(st, p + BLD_UCB) != 0) {
            U8(st, G_REC4A + REC4A_SIZE * U8(st, p + BLD_UCB)) = 1;
        }
    }
    for (int32_t b = 1; b <= S32(st, G_BUILDING_HIGHEST); b++) {
        uint32_t p = building(b);
        if (U8(st, p + BLD_STATE) == 0 || S16(st, p + BLD_TYPE) != 0x4A) {
            continue;
        }
        uint8_t s = U8(st, p + BLD_UCB);
        if (s != 0 && S32(st, G_REC4A + REC4A_SIZE * s + REC4A_BUILDING) == 0) {
            S32(st, G_REC4A + REC4A_SIZE * s + REC4A_BUILDING) = b;
        } else {
            rec4a_create(st, b);
        }
    }
}
