/* Imhotep - the advice notices after a construction (package place): the functions 0x004728A0
 * calls with the placed type at 0x00473133 (0x004E2B10 .. 0x004E5070).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each one posts in-game notices (0x004E2970) when the type matches and a city condition holds;
 * none of them writes anything else. The editor (0x00605A3C == 1) skips them all. Many of them say
 * whether a raw material is produced, can be imported (0x00443950) and is imported (trade status
 * CITY+0x2826 + 2 res, 1 or 3). The city fields and counts tested are elements of the per-resource
 * arrays CITY_STORAGE_STOCK, CITY_TRADE_STATUS, G_INDUSTRY_WORKING and G_U78D6DC. */
#include "command/place_notices.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

static void notice(game_state *st, int32_t id)
{
    GAME_CALL(st, FN_NOTICE_ADD_4E2970, id, 0, 0);
}

static int editor(game_state *st)
{
    return S32(st, G_EDITOR) == 1;
}

static int posted(game_state *st)
{
    return S32(st, G_NOTICE_POSTED) != 0; /* @bin a notice was posted by this construction */
}

static int32_t cityf(game_state *st, uint32_t off)
{
    return S32(st, CURRENT_CITY(st) + off);
}

/* trade status CITY+off (i16) is 1 or 3 */
static int traded(game_state *st, uint32_t off)
{
    int16_t s = S16(st, CURRENT_CITY(st) + off);
    return s == 1 || s == 3;
}

static int32_t g443b90(game_state *st, int32_t res)
{
    return GAME_CALL(st, FN_GOODS_CHAIN_B90, res);
}

static int32_t g443950(game_state *st, int32_t res)
{
    return GAME_CALL(st, FN_GOODS_TRADE_CITY_FLAG5, res);
}

static int32_t g4439c0(game_state *st, int32_t res)
{
    return GAME_CALL(st, FN_GOODS_TRADE_CITY, res);
}

/* 0x00443B90(chain) set (chain >= 0): notice n_chain; else 0x00443950(imp) set: notice yes unless
 * traded(off); else notice no */
static void advice(game_state *st, int32_t chain, int32_t n_chain, int32_t imp, uint32_t off, int32_t yes,
                   int32_t no)
{
    if (chain >= 0 && g443b90(st, chain) != 0) {
        notice(st, n_chain);
        return;
    }
    if (g443950(st, imp) != 0) {
        if (!traded(st, off)) {
            notice(st, yes);
        }
        return;
    }
    notice(st, no);
}

/* the raw-material pattern (0x004E33A0 ...): count and city fields all <= 0 (f2 0: none), notice
 * n1, then advice */
static void raw(game_state *st, uint32_t count, uint32_t f1, uint32_t f2, int32_t n1, int32_t chain,
                int32_t n_chain, int32_t imp, uint32_t off, int32_t yes, int32_t no)
{
    if (S32(st, count) > 0 || cityf(st, f1) > 0 || (f2 && cityf(st, f2) > 0)) {
        return;
    }
    notice(st, n1);
    advice(st, chain, n_chain, imp, off, yes, no);
}

/* the two-goods tail (0x004E3E0F ...): r1 available (0x00443900 or 0x004439C0) and none of count2
 * → about r2; else r1 with count2 → about r1; else r2 */
static void two_goods(game_state *st, int32_t r1, uint32_t count2, int32_t r2, int32_t n_no_r2, int32_t n_no_imp_r2,
                      int32_t n_r2, uint32_t off_r2, int only_one, int32_t n_r1, int32_t n_no_r1, uint32_t off_r1)
{
    if ((GAME_CALL(st, FN_PLACE_TRADE_SELLS_443900, r1) != 0 || g4439c0(st, r1) != 0) && S32(st, count2) == 0) {
        if (g4439c0(st, r2) == 0) {
            notice(st, n_no_r2);
            return;
        }
        if (g443950(st, r2) == 0) {
            notice(st, n_no_imp_r2);
            return;
        }
        int16_t s = S16(st, CURRENT_CITY(st) + off_r2);
        if (s == 1 || (!only_one && s == 3)) {
            return;
        }
        notice(st, n_r2);
        return;
    }
    if (g4439c0(st, r1) != 0 && S32(st, count2) != 0) {
        if (g443950(st, r1) != 0) {
            if (!traded(st, off_r1)) {
                notice(st, n_r1);
            }
            return;
        }
        notice(st, n_no_r1);
        return;
    }
    if (g4439c0(st, r2) == 0) {
        return;
    }
    advice(st, -1, 0, r2, off_r2, 0x20, 0x1F);
}

static int in(int32_t v, int32_t lo, int32_t hi)
{
    return v >= lo && v <= hi;
}

void notice_access_4e2b10(game_state *st, int32_t type, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x004E2B10 types without road advice skipped; no road access (0x00480180) → notice 0xB
     * (0xCF for 0x8C..0x95), while 0x00E950D4 < 3 */
    static const int32_t skip[] = {0x29, 0x2A, 0x2B, 0x5C, 0x39, 0x3A, 0xAA, 0xAB, 0xC8, 0xC9, 0xCA,
                                   0x38, 0x0A, 0x28, 0x2C, 0x2D, 0xD3, 0xD4, 0xB7, 0xD2, 0xE3, 0xE1,
                                   0xE6, 0xE4, 0xDE, 0xCF, 0xD0, 0xD5, 0xE5, 0xEA, 0xEC, 0xEB};
    if (editor(st)) {
        return;
    }
    for (unsigned i = 0; i < sizeof(skip) / sizeof(skip[0]); i++) {
        if (type == skip[i]) {
            return;
        }
    }
    if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, x, y, size) != 0) {
        return;
    }
    if ((type == 0x48 || type == 0xC1) && GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, x, y, 3) != 0) {
        return;
    }
    if (in(type, 0x41, 0x45)) {
        return;
    }
    if (in(type, 0x8C, 0x95) && GAME_CALL(st, FN_ROAD_NEXT_TO_4802C0, x, y) != 0) {
        return;
    }
    if (S32(st, G_UE950D4) >= 3) {
        return;
    }
    notice(st, in(type, 0x8C, 0x95) ? 0xCF : 0xB);
}

void notice_workers_4e2d30(game_state *st, int32_t type)
{
    /* @bin 0x004E2D30 a type with workers (model +0x14 > 0) while CITY+0x2C04 >= 10 → 0x11 */
    if (editor(st) || type == 0x5C || posted(st)) {
        return;
    }
    if (S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) type) <= 0) {
        return;
    }
    if (cityf(st, CITY_LABOUR_SHORTAGE) < 10) {
        return;
    }
    notice(st, 0x11);
}

void notice_4e2e20(game_state *st, int32_t type)
{
    /* @bin 0x004E2E20 types 0x3C..0x45, 0x8C..0x95 and the jump-table list (0x004E2ED4 / 0x004E2EDC):
     * the first event (0x0044BB90 on 0x00785D70) pending (0x0044D700) with no fort (0x00BCB9B4) and
     * CITY+0x4668 clear → 0x68 */
    if (editor(st)) {
        return;
    }
    int match = in(type, 0x41, 0x45) || in(type, 0x3C, 0x40) || in(type, 0x8C, 0x95);
    switch (type) {
    case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x37: case 0x4D:
    case 0x4E: case 0x4F: case 0x51: case 0x56: case 0x8A: case 0xA7: case 0xB8: case 0xBB: case 0xBC:
    case 0xBD: case 0xE2:
        match = 1;
        break;
    default:
        break;
    }
    if (!match) {
        return;
    }
    int32_t e = GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, 0, 0);
    while (e != 0) {
        if ((uint8_t) GAME_CALLT0(st, (uint32_t) e, FN_EVENT_PENDING) != 0) {
            if (S32(st, G_FORMATION_COUNT) != 0 || U8(st, CURRENT_CITY(st) + CITY_WARSHIPS) != 0) {
                return;
            }
            notice(st, 0x68);
            return;
        }
        e = GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, e, 0);
    }
}

/* @bin 0x004E3100 .. 0x004E31C0: one type, a count <= 0 → a notice */
static void first_of(game_state *st, int32_t type, int32_t want, uint32_t count, int32_t id)
{
    if (editor(st) || posted(st) || type != want) {
        return;
    }
    if (S32(st, count) > 0) {
        return;
    }
    notice(st, id);
}

void notice_4e3100(game_state *st, int32_t type)
{
    first_of(st, type, 0x47, G_TYPE_COUNT + 4u * 0x46u, 0x13); /* @bin 0x004E3100 */
}

void notice_4e3140(game_state *st, int32_t type)
{
    first_of(st, type, 0x1F, G_TYPE_COUNT + 4u * 0x24u, 0x2C); /* @bin 0x004E3140 */
}

void notice_4e3180(game_state *st, int32_t type)
{
    first_of(st, type, 0x1E, G_TYPE_COUNT + 4u * 0x22u, 0x2D); /* @bin 0x004E3180 */
}

void notice_4e31c0(game_state *st, int32_t type)
{
    first_of(st, type, 0x21, G_TYPE_COUNT + 4u * 0x23u, 0x2E); /* @bin 0x004E31C0 */
}

void notice_4e3200(game_state *st, int32_t type)
{
    /* @bin 0x004E3200 forts, 0x007058B8 <= 0 → 0x2A */
    if (editor(st) || posted(st) || !(type == 0x28 || type == 0x2C || type == 0x2D)) {
        return;
    }
    if (S32(st, G_TYPE_COUNT + 4u * 0x5Fu) > 0) {
        return;
    }
    notice(st, 0x2A);
}

void notice_4e3240(game_state *st, int32_t type)
{
    /* @bin 0x004E3240 type 0x5F with CITY+0x4918 <= 0 → 0x2B */
    if (editor(st) || posted(st) || type != 0x5F) {
        return;
    }
    if (cityf(st, CITY_STORAGE_STOCK + 4u * 0xAu) > 0) {
        return;
    }
    notice(st, 0x2B);
}

void notice_4e3280(game_state *st, int32_t type, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x004E3280 towers 0x3B, 0xAC, 0xAD: 0x00480770(x, y, size) == 0 → 0x29 */
    if (editor(st) || posted(st) || !(type == 0x3B || type == 0xAC || type == 0xAD)) {
        return;
    }
    if (GAME_CALL(st, FN_PLACE_TOWER_WALL_NEAR, x, y, size) == 0) {
        notice(st, 0x29);
    }
}

void notice_4e32e0(game_state *st, int32_t type, int32_t x, int32_t y)
{
    /* @bin 0x004E32E0 type 0x5C on a tile without terrain bit 0x80 → 0x26 */
    if (editor(st) || posted(st) || type != 0x5C) {
        return;
    }
    uint32_t o = (uint32_t) (S32(st, G_MAP_START) + y * GRID_SIZE + x);
    if (U8(st, G_GRID_TERRAIN + 4 * o) & 0x80) {
        return;
    }
    notice(st, 0x26);
}

void notice_4e3330(game_state *st, int32_t type)
{
    /* @bin 0x004E3330 type 10 with CITY+0x24 >= 300, 0x00784BBC clear and the ratio
     * 0x004CDE60(0x00447530(CITY+0x45DC), CITY+0x45DC) <= 0x5F → 0x5E */
    if (editor(st) || posted(st) || type != 0xA) {
        return;
    }
    if (cityf(st, CITY_POPULATION) < 300 || S32(st, G_U784BBC) != 0) {
        return;
    }
    int32_t v = cityf(st, CITY_FOOD_EATEN);
    int32_t r = GAME_CALL(st, FN_PLACE_FOOD_RATIO_447530, v);
    if (GAME_CALL(st, FN_CALC_RATIO_PERCENT, r, v) > 0x5F) {
        return;
    }
    notice(st, 0x5E);
}

void notice_4e33a0(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0x6E) { /* @bin 0x004E33A0 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0xEu, CITY_STORAGE_STOCK + 4u * 0xFu, CITY_STORAGE_STOCK + 4u * 0xEu, 0x1B, 0xE, 0x22, 0xE, CITY_TRADE_STATUS + 2u * 0xEu, 0x20, 0x1F);
    }
}

void notice_4e3460(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0x6F) { /* @bin 0x004E3460 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x10u, CITY_STORAGE_STOCK + 4u * 0x11u, CITY_STORAGE_STOCK + 4u * 0x10u, 0x1C, 0x10, 0x23, 0x10, CITY_TRADE_STATUS + 2u * 0x10u, 0x20, 0x1F);
    }
}

void notice_4e3520(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0x71) { /* @bin 0x004E3520 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x12u, CITY_STORAGE_STOCK + 4u * 0x13u, CITY_STORAGE_STOCK + 4u * 0x12u, 0x1E, 0x12, 0x25, 0x12, CITY_TRADE_STATUS + 2u * 0x12u, 0x20, 0x1F);
    }
}

void notice_4e35e0(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0x72) { /* @bin 0x004E35E0 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0xBu, CITY_STORAGE_STOCK + 4u * 0xDu, CITY_STORAGE_STOCK + 4u * 0xBu, 0x1D, 0xB, 0x24, 0xB, CITY_TRADE_STATUS + 2u * 0xBu, 0x20, 0x1F);
    }
}

void notice_4e36a0(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0xCB) { /* @bin 0x004E36A0 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x16u, CITY_STORAGE_STOCK + 4u * 0x17u, CITY_STORAGE_STOCK + 4u * 0x16u, 0x50, 0x16, 0x51, 0x16, CITY_TRADE_STATUS + 2u * 0x16u, 0x20, 0x1F);
    }
}

void notice_4e39d0(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0xE9) { /* @bin 0x004E39D0 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x20u, CITY_STORAGE_STOCK + 4u * 0x21u, CITY_STORAGE_STOCK + 4u * 0x20u, 0x105, 0x20, 0x106, 0x20, CITY_TRADE_STATUS + 2u * 0x20u, 0x107, 0x108);
    }
}

void notice_4e3cf0(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0xC2) { /* @bin 0x004E3CF0 */
        raw(st, G_INDUSTRY_WORKING + 4u * 1u, CITY_STORAGE_STOCK + 4u * 9u, 0, 0x52, 1, 0x53, 9, CITY_TRADE_STATUS + 2u * 9u, 0x20, 0x1F);
    }
}

void notice_4e4490(game_state *st, int32_t type)
{
    if (!editor(st) && type == 0xB1) { /* @bin 0x004E4490 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x14u, CITY_STORAGE_STOCK + 4u * 0x14u, 0, 0x8F, 0x14, 0x90, 0x14, CITY_TRADE_STATUS + 2u * 0x14u, 0x20, 0x1F);
    }
}

void notice_4e4e10(game_state *st, int32_t type)
{
    if (!editor(st) && (type == 0xD5 || type == 0xDE)) { /* @bin 0x004E4E10 */
        raw(st, G_INDUSTRY_WORKING + 4u * 0x1Eu, CITY_STORAGE_STOCK + 4u * 0x1Eu, 0, 0xA4, 0x1E, 0xA5, 0x1E, CITY_TRADE_STATUS + 2u * 0x1Eu, 0x20, 0x1F);
    }
}

/* @bin 0x004E3760 / 0x004E3AA0 / 0x004E47D0 cases 0 and 1: two inputs (counts c1, c2; city fields
 * f1, g1 for the first, f2 for the second, g2 the blocking field); none of the first → n_both and
 * the advice of both (first_res -1: no chain step); none of the second only → n_second; none of the
 * first only → n_first */
typedef struct {
    uint32_t c1, c2, f_block, f1, f2;
    int32_t n_both, chain1, n_chain1, imp1, yes1, no1; /* first part of n_both */
    uint32_t off1;
    int32_t chain2, n_chain2, imp2, yes2, no2; /* second part of n_both */
    uint32_t off2;
    int32_t n_second, n_first;
    int32_t chain_s, n_chain_s, imp_s; /* advice after n_second */
    uint32_t off_s;
    int32_t chain_f, n_chain_f, imp_f; /* advice after n_first */
    uint32_t off_f;
} two_inputs;

static void inputs(game_state *st, const two_inputs *t)
{
    int32_t a = S32(st, t->c1), b = S32(st, t->c2);
    if (a > 0 && b > 0) {
        return;
    }
    if (t->f_block && cityf(st, t->f_block) > 0) {
        return;
    }
    int32_t f1 = cityf(st, t->f1);
    if (f1 > 0 && cityf(st, t->f2) > 0) {
        return;
    }
    if (a > 0 || f1 > 0) {
        /* the first input exists */
        if (b > 0 || cityf(st, t->f2) > 0) {
            return;
        }
        notice(st, t->n_first);
        advice(st, t->chain_f, t->n_chain_f, t->imp_f, t->off_f, 0x20, 0x1F);
        return;
    }
    if (b > 0 || cityf(st, t->f2) > 0) {
        notice(st, t->n_second);
        advice(st, t->chain_s, t->n_chain_s, t->imp_s, t->off_s, 0x20, 0x1F);
        return;
    }
    notice(st, t->n_both);
    advice(st, t->chain1, t->n_chain1, t->imp1, t->off1, t->yes1, t->no1);
    advice(st, t->chain2, t->n_chain2, t->imp2, t->off2, t->yes2, t->no2);
}

void notice_4e3760(game_state *st, int32_t type)
{
    /* @bin 0x004E3760 type 0xCC: inputs counted by 0x0078D678 / 0x0078D650, CITY+0x491C / +0x4914,
     * blocked by CITY+0x4920 */
    static const two_inputs t = {G_INDUSTRY_WORKING + 4u * 0xBu, G_INDUSTRY_WORKING + 4u * 1u, CITY_STORAGE_STOCK + 4u * 0xCu, CITY_STORAGE_STOCK + 4u * 0xBu, CITY_STORAGE_STOCK + 4u * 9u,
                                 0x91, 0xB, 0x24, 0xB, 0x93, 0x92, CITY_TRADE_STATUS + 2u * 0xBu,
                                 1, 0x53, 9, 0x95, 0x94, CITY_TRADE_STATUS + 2u * 9u,
                                 0x1D, 0x52,
                                 0xB, 0x24, 0xB, CITY_TRADE_STATUS + 2u * 0xBu,
                                 1, 0x53, 9, CITY_TRADE_STATUS + 2u * 9u};
    if (!editor(st) && type == 0xCC) {
        inputs(st, &t);
    }
}

void notice_4e3aa0(game_state *st, int32_t type)
{
    /* @bin 0x004E3AA0 type 0xE8: 0x0078D6C8 / 0x0078D680, CITY+0x496C / +0x4924, blocked by +0x4978 */
    static const two_inputs t = {G_INDUSTRY_WORKING + 4u * 0x1Fu, G_INDUSTRY_WORKING + 4u * 0xDu, CITY_STORAGE_STOCK + 4u * 0x22u, CITY_STORAGE_STOCK + 4u * 0x1Fu, CITY_STORAGE_STOCK + 4u * 0xDu,
                                 0xFF, -1, 0, 0x1F, 0x103, 0x104, CITY_TRADE_STATUS + 2u * 0x1Fu,
                                 0xD, 0x100, 0xD, 0x101, 0x102, CITY_TRADE_STATUS + 2u * 0xDu,
                                 0x109, 0x10A,
                                 -1, 0, 0x1F, CITY_TRADE_STATUS + 2u * 0x1Fu,
                                 0xD, 0x100, 0xD, CITY_TRADE_STATUS + 2u * 0xDu};
    if (!editor(st) && type == 0xE8) {
        inputs(st, &t);
    }
}

void notice_4e47d0(game_state *st, int32_t type)
{
    /* @bin 0x004E47D0 the pyramid (0xB7) by its kind 0x004F44E0 (jump table 0x004E4DF0) */
    static const two_inputs a = {G_INDUSTRY_WORKING + 4u * 0x18u, G_INDUSTRY_WORKING + 4u * 0x19u, 0, CITY_STORAGE_STOCK + 4u * 0x18u, CITY_STORAGE_STOCK + 4u * 0x19u,
                                 0x96, 0x18, 0x97, 0x18, 0x99, 0x98, CITY_TRADE_STATUS + 2u * 0x18u,
                                 0x19, 0x9A, 0x19, 0x9C, 0x9B, CITY_TRADE_STATUS + 2u * 0x19u,
                                 0x9D, 0x9E,
                                 0x18, 0x97, 0x18, CITY_TRADE_STATUS + 2u * 0x18u,
                                 0x19, 0x9A, 0x19, CITY_TRADE_STATUS + 2u * 0x19u};
    static const two_inputs b = {G_INDUSTRY_WORKING + 4u * 0xCu, G_INDUSTRY_WORKING + 4u * 0x19u, 0, CITY_STORAGE_STOCK + 4u * 0xCu, CITY_STORAGE_STOCK + 4u * 0x19u,
                                 0x9F, 0xC, 0xA0, 0xC, 0xA2, 0xA1, CITY_TRADE_STATUS + 2u * 0xCu,
                                 0x19, 0x9A, 0x19, 0x9C, 0x9B, CITY_TRADE_STATUS + 2u * 0x19u,
                                 0xA3, 0x9E,
                                 0xC, 0xA0, 0xC, CITY_TRADE_STATUS + 2u * 0xCu,
                                 0x19, 0x9A, 0x19, CITY_TRADE_STATUS + 2u * 0x19u};
    if (editor(st) || type != 0xB7) {
        return;
    }
    uint32_t k = (uint32_t) GAME_CALL0(st, FN_PYRAMID_KIND_4F44E0) - 1u;
    if (k > 4) {
        return;
    }
    switch (k) {
    case 0:
    case 3:
        inputs(st, &a);
        break;
    case 1:
        inputs(st, &b);
        break;
    case 2: /* @bin 0x004E4CAD */
        if (S32(st, G_INDUSTRY_WORKING + 4u * 0x18u) > 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0x18u) > 0) {
            return;
        }
        notice(st, 0x9D);
        advice(st, 0x18, 0x97, 0x18, CITY_TRADE_STATUS + 2u * 0x18u, 0x20, 0x1F);
        break;
    default: /* @bin 0x004E4D4D */
        if (S32(st, G_INDUSTRY_WORKING + 4u * 0xCu) > 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0xCu) > 0) {
            return;
        }
        notice(st, 0xA3);
        advice(st, 0xC, 0xA0, 0xC, CITY_TRADE_STATUS + 2u * 0xCu, 0x20, 0x1F);
        break;
    }
}

void notice_4e3da0(game_state *st, int32_t type)
{
    /* @bin 0x004E3DA0 type 0x20 */
    if (editor(st) || type != 0x20 || cityf(st, CITY_STORAGE_STOCK + 4u * 0xFu) > 0) {
        return;
    }
    notice(st, 0x89);
    if (S32(st, G_U78D6DC + 4u * 0xEu) > 0) {
        return;
    }
    two_goods(st, 0xE, G_U78D6DC + 4u * 0xFu, 0xF, 0x8A, 0xC0, 0xBF, CITY_TRADE_STATUS + 2u * 0xFu, 0, 0xA8, 0xA9, CITY_TRADE_STATUS + 2u * 0xEu);
}

void notice_4e4120(game_state *st, int32_t type)
{
    /* @bin 0x004E4120 types 0x33, 0x35 (the second status test is "== 1" only, 0x004E41D3) */
    if (editor(st) || !(type == 0x33 || type == 0x35) || cityf(st, CITY_STORAGE_STOCK + 4u * 0x17u) > 0) {
        return;
    }
    notice(st, 0x8B);
    if (S32(st, G_U78D6DC + 4u * 0x16u) > 0) {
        return;
    }
    two_goods(st, 0x16, G_U78D6DC + 4u * 0x17u, 0x17, 0x8C, 0xC2, 0xC1, CITY_TRADE_STATUS + 2u * 0x17u, 1, 0xAA, 0xAB, CITY_TRADE_STATUS + 2u * 0x16u);
}

void notice_4e42d0(game_state *st, int32_t type)
{
    /* @bin 0x004E42D0 type 0x2F */
    if (editor(st) || type != 0x2F || cityf(st, CITY_STORAGE_STOCK + 4u * 0x11u) > 0) {
        return;
    }
    notice(st, 0x8D);
    if (S32(st, G_U78D6DC + 4u * 0x10u) > 0) {
        return;
    }
    two_goods(st, 0x10, G_U78D6DC + 4u * 0x11u, 0x11, 0x8E, 0xC4, 0xC3, CITY_TRADE_STATUS + 2u * 0x11u, 0, 0xAE, 0xAF, CITY_TRADE_STATUS + 2u * 0x10u);
}

void notice_4e3f60(game_state *st, int32_t type)
{
    /* @bin 0x004E3F60 type 0xE2: notice 0xED, its goods, then the 0x52 advice */
    if (editor(st) || type != 0xE2 || cityf(st, CITY_STORAGE_STOCK + 4u * 8u) > 0) {
        return;
    }
    notice(st, 0xED);
    if (S32(st, G_U78D6DC + 4u * 8u) > 0) {
        return;
    }
    if ((GAME_CALL(st, FN_PLACE_TRADE_SELLS_443900, 8) != 0 || g4439c0(st, 8) != 0) && S32(st, G_U78D6DC + 4u * 8u) == 0) {
        if (g4439c0(st, 8) == 0) {
            notice(st, 0xEE);
        } else if (g443950(st, 8) == 0) {
            notice(st, 0xF0);
        } else if (!traded(st, CITY_TRADE_STATUS + 2u * 2u)) { /* @bin 0x004E4008 the status of resource 2 */
            notice(st, 0xEF);
        }
    } else if (g4439c0(st, 8) != 0) {
        advice(st, -1, 0, 8, CITY_TRADE_STATUS + 2u * 8u, 0x20, 0x1F);
    }
    /* @bin 0x004E4082 */
    if (S32(st, G_INDUSTRY_WORKING + 4u * 1u) > 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 9u) > 0) {
        return;
    }
    notice(st, 0x52);
    advice(st, 1, 0x53, 9, CITY_TRADE_STATUS + 2u * 9u, 0x95, 0x94);
}

void notice_4e4550(game_state *st, int32_t type)
{
    /* @bin 0x004E4550 type 0xE7 */
    if (editor(st) || type != 0xE7) {
        return;
    }
    if (S32(st, G_INDUSTRY_WORKING + 4u * 0xBu) > 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0xBu) > 0) {
        if (S32(st, G_INDUSTRY_WORKING + 4u * 0x21u) > 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0x21u) > 0) {
            return;
        }
    }
    notice(st, 0x10E);
    if (S32(st, G_INDUSTRY_WORKING + 4u * 0xBu) == 0 && cityf(st, CITY_STORAGE_STOCK + 4u * 0xBu) == 0) {
        advice(st, 0xB, 0x24, 0xB, CITY_TRADE_STATUS + 2u * 0xBu, 0x93, 0x92);
    }
    /* @bin 0x004E463B */
    if (S32(st, G_INDUSTRY_WORKING + 4u * 0x21u) != 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0x21u) != 0) {
        return;
    }
    two_goods(st, 0x20, G_U78D6DC + 4u * 0x21u, 0x21, 0x10D, 0x10B, 0x10C, CITY_TRADE_STATUS + 2u * 0x21u, 0, 0x107, 0x108, CITY_TRADE_STATUS + 2u * 0x20u);
}

void notice_4e4ed0(game_state *st, int32_t type)
{
    /* @bin 0x004E4ED0 types 0xE4, 0xE3, 0xE1 */
    if (editor(st) || !(type == 0xE4 || type == 0xE3 || type == 0xE1)) {
        return;
    }
    if (cityf(st, CITY_STORAGE_STOCK + 4u * 0x23u) == 0) {
        notice(st, 0xF2);
        advice(st, -1, 0, 0x23, CITY_TRADE_STATUS + 2u * 0x23u, 0x20, 0x1F);
    }
    if (type == 0xE1) {
        if (S32(st, G_INDUSTRY_WORKING + 4u * 0x1Du) != 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0x1Du) != 0) {
            return;
        }
        notice(st, 0xF1);
        advice(st, -1, 0, 0x1D, CITY_TRADE_STATUS + 2u * 0x1Du, 0x20, 0x1F);
    } else if (type == 0xE3) {
        if (S32(st, G_INDUSTRY_WORKING + 4u * 0x1Au) != 0 || cityf(st, CITY_STORAGE_STOCK + 4u * 0x1Au) != 0) {
            return;
        }
        notice(st, 0xF8);
        advice(st, -1, 0, 0x1A, CITY_TRADE_STATUS + 2u * 0x1Au, 0x20, 0x1F);
    }
}

void notice_4e5070(game_state *st, int32_t type)
{
    /* @bin 0x004E5070 types 0x1F, 0x21, 0x1E */
    if (editor(st) || !(type == 0x1F || type == 0x21 || type == 0x1E)) {
        return;
    }
    if (S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * 0x24u) <= 0) {
        notice(st, 0xD0);
    }
    if (S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * 0x22u) <= 0 && type != 0x1F) {
        notice(st, 0xD1);
    }
    if (S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * 0x23u) <= 0 && type == 0x21) {
        notice(st, 0xD2);
    }
}

#define RUN1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0]); \
        return 0; \
    }
RUN1(run_4e2d30, notice_workers_4e2d30)
RUN1(run_4e2e20, notice_4e2e20)
RUN1(run_4e3100, notice_4e3100)
RUN1(run_4e3140, notice_4e3140)
RUN1(run_4e3180, notice_4e3180)
RUN1(run_4e31c0, notice_4e31c0)
RUN1(run_4e3200, notice_4e3200)
RUN1(run_4e3240, notice_4e3240)
RUN1(run_4e3330, notice_4e3330)
RUN1(run_4e33a0, notice_4e33a0)
RUN1(run_4e3460, notice_4e3460)
RUN1(run_4e3520, notice_4e3520)
RUN1(run_4e35e0, notice_4e35e0)
RUN1(run_4e36a0, notice_4e36a0)
RUN1(run_4e3760, notice_4e3760)
RUN1(run_4e39d0, notice_4e39d0)
RUN1(run_4e3aa0, notice_4e3aa0)
RUN1(run_4e3cf0, notice_4e3cf0)
RUN1(run_4e3da0, notice_4e3da0)
RUN1(run_4e3f60, notice_4e3f60)
RUN1(run_4e4120, notice_4e4120)
RUN1(run_4e42d0, notice_4e42d0)
RUN1(run_4e4490, notice_4e4490)
RUN1(run_4e4550, notice_4e4550)
RUN1(run_4e47d0, notice_4e47d0)
RUN1(run_4e4e10, notice_4e4e10)
RUN1(run_4e4ed0, notice_4e4ed0)
RUN1(run_4e5070, notice_4e5070)

static int32_t run_4e2b10(game_state *st, const int32_t *a)
{
    notice_access_4e2b10(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_4e3280(game_state *st, const int32_t *a)
{
    notice_4e3280(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_4e32e0(game_state *st, const int32_t *a)
{
    notice_4e32e0(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function place_notice_ports[] = {
    {0x004E2B10u, "notice_access_4e2b10", 4, 0, 0, run_4e2b10, 0},
    {0x004E2D30u, "notice_workers_4e2d30", 1, 0, 0, run_4e2d30, 0},
    {0x004E2E20u, "notice_4e2e20", 1, 0, 0, run_4e2e20, 0},
    {0x004E3100u, "notice_4e3100", 1, 0, 0, run_4e3100, 0},
    {0x004E3140u, "notice_4e3140", 1, 0, 0, run_4e3140, 0},
    {0x004E3180u, "notice_4e3180", 1, 0, 0, run_4e3180, 0},
    {0x004E31C0u, "notice_4e31c0", 1, 0, 0, run_4e31c0, 0},
    {0x004E3200u, "notice_4e3200", 1, 0, 0, run_4e3200, 0},
    {0x004E3240u, "notice_4e3240", 1, 0, 0, run_4e3240, 0},
    {0x004E3280u, "notice_4e3280", 4, 0, 0, run_4e3280, 0},
    {0x004E32E0u, "notice_4e32e0", 3, 0, 0, run_4e32e0, 0},
    {0x004E3330u, "notice_4e3330", 1, 0, 0, run_4e3330, 0},
    {0x004E33A0u, "notice_4e33a0", 1, 0, 0, run_4e33a0, 0},
    {0x004E3460u, "notice_4e3460", 1, 0, 0, run_4e3460, 0},
    {0x004E3520u, "notice_4e3520", 1, 0, 0, run_4e3520, 0},
    {0x004E35E0u, "notice_4e35e0", 1, 0, 0, run_4e35e0, 0},
    {0x004E36A0u, "notice_4e36a0", 1, 0, 0, run_4e36a0, 0},
    {0x004E3760u, "notice_4e3760", 1, 0, 0, run_4e3760, 0},
    {0x004E39D0u, "notice_4e39d0", 1, 0, 0, run_4e39d0, 0},
    {0x004E3AA0u, "notice_4e3aa0", 1, 0, 0, run_4e3aa0, 0},
    {0x004E3CF0u, "notice_4e3cf0", 1, 0, 0, run_4e3cf0, 0},
    {0x004E3DA0u, "notice_4e3da0", 1, 0, 0, run_4e3da0, 0},
    {0x004E3F60u, "notice_4e3f60", 1, 0, 0, run_4e3f60, 0},
    {0x004E4120u, "notice_4e4120", 1, 0, 0, run_4e4120, 0},
    {0x004E42D0u, "notice_4e42d0", 1, 0, 0, run_4e42d0, 0},
    {0x004E4490u, "notice_4e4490", 1, 0, 0, run_4e4490, 0},
    {0x004E4550u, "notice_4e4550", 1, 0, 0, run_4e4550, 0},
    {0x004E47D0u, "notice_4e47d0", 1, 0, 0, run_4e47d0, 0},
    {0x004E4E10u, "notice_4e4e10", 1, 0, 0, run_4e4e10, 0},
    {0x004E4ED0u, "notice_4e4ed0", 1, 0, 0, run_4e4ed0, 0},
    {0x004E5070u, "notice_4e5070", 1, 0, 0, run_4e5070, 0},
};
const int place_notice_ports_count = sizeof(place_notice_ports) / sizeof(place_notice_ports[0]);
