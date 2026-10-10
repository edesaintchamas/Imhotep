/* Imhotep - what the gods' actions do to the city (package rest): the callees of god_act
 * 0x004C47A0 (core/city/day2_gods.c) and of 0x0041CC80: monuments' progress, storage yards
 * filled or emptied, buildings destroyed.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/rest_gods.h"

#include "city/monument_rating.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t pyramid_parts(game_state *st, uint32_t r)
{
    /* @bin 0x0056F7EC / 0x0056F7D8 the number of parts by size +0x54 (mission kind 5 or not) */
    uint32_t size = U8(st, r + BLD_MONUMENT_KIND);
    return S32(st, G_PYRAMID_KIND) == 5 ? S32(st, G_PYRAMID_PARTS_KIND5 + 4u * size) : S32(st, G_PYRAMID_PARTS + 4u * size);
}

int32_t pyramid_progress_4f5340(game_state *st)
{
    /* @bin 0x004F5340 with 0x00E38EC3 set, the pyramid (type 0xB7) of the largest size +0x54
     * (first of equals): 2 when its percent (0x004F1310 +0x34) is below 50, 1 below 75, else 0 */
    if (U8(st, G_UE38EC3) == 0) {
        return 0;
    }
    int32_t best = -1, sel = 0;
    for (int32_t b = 1; b <= S32(st, G_BUILDING_HIGHEST); b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) == 1 && S16(st, r + BLD_TYPE) == 0xB7 && (int32_t) U8(st, r + BLD_MONUMENT_KIND) > best) {
            best = U8(st, r + BLD_MONUMENT_KIND);
            sel = b;
        }
    }
    if (best < 0) {
        return 0;
    }
    int32_t s[0x1C];
    pyramid_status(st, sel, s);
    if (s[0xD] < 0x32) {
        return 2;
    }
    if (s[0xD] < 0x4B) {
        return 1;
    }
    return 0;
}

void pyramid_speedup_4f3aa0(game_state *st, int32_t k)
{
    /* @bin 0x004F3AA0 the pyramids' workers (figure types 0x55 from the pyramid on, 0x51, 0x4F,
     * 0x50, 0x60 below its last part) sent back (action 4, 0x004B5420); the smallest of the days
     * left (parts x k, or work left 0x004F1310 capped at parts x k x 2) into 0x0078D7A4, with
     * 0x00EAAF5C set */
    int32_t best = 999999;
    if (S32(st, G_BUILDING_HIGHEST) <= 1) {
        return;
    }
    int32_t b = 1;
    do {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) == 1 && S16(st, r + BLD_TYPE) == 0xB7) {
            int32_t v = 999999;
            for (int32_t f = 1; f < FIGURE_COUNT; f++) {
                uint32_t fr = FIGURE_ADDR(f);
                if (U8(st, fr + FIG_STATE) == 0) {
                    continue;
                }
                uint8_t type = U8(st, fr + FIG_TYPE);
                if (type != 0x55 && type != 0x51 && type != 0x4F && type != 0x50 && type != 0x60) {
                    continue;
                }
                if (type == 0x55 && S16(st, fr + FIG_DEST_BUILDING) < b) {
                    continue;
                }
                if (S16(st, fr + FIG_DEST_BUILDING) < pyramid_parts(st, r) + b) {
                    U8(st, fr + FIG_ACTION) = 4;
                    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, f);
                }
            }
            if (U8(st, r + BLD_U55) > 1) {
                int32_t s[0x1C];
                pyramid_status(st, b, s);
                v = s[0x11] - s[0x10] + 1;
                int32_t cap = (int32_t) ((uint32_t) pyramid_parts(st, r) * (uint32_t) k * 2u);
                if (cap < v) {
                    v = cap;
                }
            } else if ((uint8_t) GAME_CALL(st, FN_PYRAMID_PART_DONE, b) == 0) {
                v = (int32_t) ((uint32_t) pyramid_parts(st, r) * (uint32_t) k);
            }
            if (v < best) {
                best = v;
            }
            b += pyramid_parts(st, r) - 1;
        }
        b++;
    } while (b < S32(st, G_BUILDING_HIGHEST));
    if (best != 999999) {
        U8(st, G_GOD_CURSE_ON) = 1;
        S32(st, G_PYRAMID_CURSE_COUNTDOWN) = best;
    }
}

int32_t storage_fill_4c3950(game_state *st)
{
    /* @bin 0x004C3950 the storage building (types 0x48, 0xC1) holding the least (non-zero) of
     * goods 0x12, 0xB, 0xD, 0x10, 0x11, 0x13 (0x0045D510) gets its least-held good topped up to
     * table 0x007900EC (by its +0xC1) through 0x0045B2A0; returns it (0: none) */
    static const int32_t goods[6] = {0x12, 0xB, 0xD, 0x10, 0x11, 0x13};
    int32_t best = 10000, sel = 0;
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, r + BLD_TYPE);
        if (type != 0x48 && type != 0xC1) {
            continue;
        }
        int32_t sum = 0;
        for (int i = 0; i < 6; i++) {
            sum += GAME_CALL(st, FN_BUILDING_PARTS_STOCK, b, goods[i]);
        }
        if (sum != 0 && sum < best) {
            best = sum;
            sel = b;
        }
    }
    if (sel == 0 || best == 0) {
        return sel;
    }
    int32_t least = 10000, good = 0;
    for (int i = 0; i < 6; i++) {
        int32_t v = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, sel, goods[i]);
        if (v != 0 && v < least) {
            good = goods[i];
            least = v;
        }
    }
    if (good != 0) {
        uint32_t idx = (uint32_t) good + 98u * U8(st, BUILDING_ADDR(sel) + BLD_STORAGE_ID);
        int32_t amount = S16(st, G_STORAGE_FILL + 2u * idx) - least;
        if (amount < 0) {
            amount = 0;
        }
        GAME_CALL(st, FN_STORAGE_ADD, S32(st, CURRENT_PLAYER_ADDR), sel, good, amount);
    }
    return sel;
}

static int32_t pick_present(game_state *st, int32_t *types, const int32_t *goods, int n)
{
    /* the god's choices still present in the city (0x00705AF0 by building type, or 0x0078D64C
     * by good); a random one (CRT rand), or 0 */
    int32_t count = 0;
    for (int i = 0; i < n; i++) {
        if (S32(st, G_TYPE_COUNT_TOTAL + 4u * (uint32_t) types[i]) > 0) {
            count++;
        } else if (goods[i] > 0 && S32(st, G_INDUSTRY_WORKING + 4u * (uint32_t) goods[i]) > 0) {
            count++;
        } else {
            types[i] = 0;
        }
    }
    if (count == 0) {
        return 0;
    }
    int32_t r = crt_rand(st) % count;
    for (int i = 0, c = 0; i < n; i++) {
        if (types[i] == 0) {
            continue;
        }
        if (c == r) {
            return types[i];
        }
        c++;
    }
    return 0;
}

int32_t workshops_fill_4c3ac0(game_state *st)
{
    /* @bin 0x004C3AC0 a random present type among 0x4A, 0x6F, 0x71: each such building gets its
     * stock filled to its capacity (300 types 0x20, 0x33; 500 types 0x2F, 0x35; 100000 types
     * 0x54..0x57, 0xB8, 0xBB..0xBD; else 200) in its good (by +0x12) through 0x0045FE20 */
    int32_t types[3] = {0x4A, 0x6F, 0x71};
    static const int32_t goods[3] = {-1, 0x11, 0x13};
    /* @bin 0x004C3E64 the good by +0x12 - 1 */
    static const int32_t good_of[26] = {0x10, -1, 0x18, 0x14, 0xB, 0x19, 0xC, 0xA, 0xE, 0x12, 0x16, 0x17, 9,
                                        0x1C, 0x15, 0x11, 0x1D, 0x1E, -1, 0x20, 2, 8, 0x1F, 0xD, 0x21, 0x22};
    int32_t type = pick_present(st, types, goods, 3);
    if (type == 0 || type == -1) {
        return 0;
    }
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) != 1 || S16(st, r + BLD_TYPE) != type) {
            continue;
        }
        /* @bin 0x004C3D14 the capacity by type */
        int32_t cap;
        switch (type) {
        case 0x20: case 0x33:
            cap = 300;
            break;
        case 0x2F: case 0x35:
            cap = 500;
            break;
        case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD:
            cap = 100000;
            break;
        default:
            cap = 200;
            break;
        }
        int32_t amount = cap - S16(st, r + BLD_STORED);
        if (amount < 0) {
            amount = 0;
        }
        uint32_t g = (uint32_t) (S16(st, r + BLD_SUBTYPE) - 1);
        GAME_CALL(st, FN_BUILDING_ADD_GOOD, b, amount, g <= 0x19 ? good_of[g] : -1);
    }
    return 1;
}

static void destroy_with_routes(game_state *st, int32_t b)
{
    /* the building collapses (0x0046D100, 0x004695D0), the sound 0x11, the routing grids
     * around it filled again (0x00516F90, 0x005173A0) */
    S32(st, G_UE950E0) = 0;
    GAME_CALL(st, FN_BUILDING_COLLAPSE, b, 0);
    GAME_CALL(st, FN_BUILDING_DESTROY_LINKED, b, 1);
    GAME_CALL(st, FN_UI_SOUND, 0x11);
    uint32_t r = BUILDING_ADDR(b);
    int32_t s = U8(st, r + BLD_SIZE), x = S16(st, r + BLD_X), y = S16(st, r + BLD_Y);
    GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x - s, y - s, x + s, y + s, 1);
    x = S16(st, r + BLD_X), y = S16(st, r + BLD_Y), s = U8(st, r + BLD_SIZE);
    GAME_CALL(st, FN_ROUTING_FILL_EC7630, x - s, y - s, x + s, y + s, 1);
}

int32_t buildings_destroy_4c3ed0(game_state *st)
{
    /* @bin 0x004C3ED0 a random present type among 0xA1, 0xA2, 0x6D, 0x4A, 0x6F, 0x71 (or their
     * goods 0x15, 0x12, 0xB, 0x11, 0x13): every building of it collapses; 1 when done */
    int32_t types[6] = {0xA1, 0xA2, 0x6D, 0x4A, 0x6F, 0x71};
    static const int32_t goods[6] = {0x15, 0x12, 0xB, -1, 0x11, 0x13};
    int32_t type = pick_present(st, types, goods, 6);
    if (type == 0 || type == -1) {
        return 0;
    }
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) == 1 && S16(st, r + BLD_TYPE) == type) {
            GAME_CALL(st, FN_BUILDING_COLLAPSE, b, 0);
            GAME_CALL(st, FN_BUILDING_DESTROY_LINKED, b, 1);
        }
    }
    S32(st, G_UE950E0) = 0;
    GAME_CALL(st, FN_UI_SOUND, 0x11);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    return 1;
}

void storage_strike_4c4020(game_state *st, int32_t destroy)
{
    /* @bin 0x004C4020 the storage building (0x48, 0xC1) holding the most of goods 1..0x23 (last
     * of equals): destroyed (destroy == 1), else emptied of good 0x10 (0x0045B700) */
    int32_t best = 0, sel = 0;
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, r + BLD_TYPE);
        if (type != 0x48 && type != 0xC1) {
            continue;
        }
        int32_t sum = 0;
        for (int32_t g = 1; g < 0x24; g++) {
            sum += GAME_CALL(st, FN_BUILDING_PARTS_STOCK, b, g);
        }
        if (sum >= best) {
            best = sum;
            sel = b;
        }
    }
    if (sel == 0) {
        return;
    }
    if (destroy == 1) {
        destroy_with_routes(st, sel);
        return;
    }
    int16_t type = S16(st, BUILDING_ADDR(sel) + BLD_TYPE);
    if (type == 0x48 || type == 0xC1) {
        GAME_CALL(st, FN_STORAGE_TAKE_45B700, sel, 0x10);
    }
}

int32_t fort_destroy_4c41b0(game_state *st)
{
    /* @bin 0x004C41B0 the formation (1..49 of 0x00BCB9C8) in use with +0x42 set and the largest
     * +0x7A (last of equals): the building +6 collapses; the figures +8 of formation (that id)
     * get state 2 */
    int32_t best = 0, b = 0;
    for (int32_t f = 1; f < 50; f++) {
        uint32_t r = G_FORMATION_BASE + 0x90u * (uint32_t) f;
        if (U8(st, r) == 1 && U8(st, r + 0x42) != 0 && S16(st, r + 0x7A) >= best) {
            b = S16(st, r + 6);
            best = S16(st, r + 0x7A);
        }
    }
    if (b == 0) {
        return 0;
    }
    S32(st, G_UE950E0) = 0;
    uint32_t list = G_FORMATION_BASE + 8u + 0x90u * (uint32_t) b;
    for (int i = 0; i < 0x10; i++) {
        int16_t f = S16(st, list + 2u * (uint32_t) i);
        if (f == 0) {
            break;
        }
        U8(st, FIGURE_ADDR(f) + FIG_STATE) = 2;
    }
    GAME_CALL(st, FN_BUILDING_COLLAPSE, b, 0);
    GAME_CALL(st, FN_BUILDING_DESTROY_LINKED, b, 1);
    GAME_CALL(st, FN_UI_SOUND, 0x11);
    uint32_t r = BUILDING_ADDR(b);
    int32_t s = U8(st, r + 3), x = S16(st, r + 6), y = S16(st, r + 8);
    GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x - s, y - s, x + s, y + s, 1);
    x = S16(st, r + 6), y = S16(st, r + 8), s = U8(st, r + 3);
    GAME_CALL(st, FN_ROUTING_FILL_EC7630, x - s, y - s, x + s, y + s, 1);
    return 1;
}

void houses_stock_4c42b0(game_state *st)
{
    /* @bin 0x004C42B0 houses (types 0xA..0x1D): their stocks +0x54.. (12 words) set to their
     * needs (food: population +0x1C x 6; goods 0xF, 0xD, 0x11, 0x13 from table 0x0070CDF8 by
     * level +0x12, x 2, x 8 above size 1); storage yards (0x46): the city's goods (list 0x00F53E70,
     * 0x005013A0) in stock raised to at least 800 (good 1), 600 (2..8) or 400, at most 20000 */
    int32_t cnt = S32(st, G_STORAGE_LIST);
    for (int32_t b = 0; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, r + BLD_TYPE);
        if (type >= 0xA && type <= 0x1D) {
            for (uint32_t k = 0; k < 12; k++) {
                uint32_t w = r + BLD_HOUSE_INVENTORY + 2u * k;
                if (S16(st, w) == 0) {
                    continue;
                }
                if (k <= 7) {
                    S16(st, w) = (int16_t) (S16(st, r + BLD_POPULATION) * 6);
                    continue;
                }
                /* @bin 0x004C4530 by good (k 8..11: 0xF, 0xD, 0x11, 0x13) */
                uint32_t table = k == 8 ? G_HOUSE_GOODS_D : k == 9 ? G_HOUSE_LEVEL_GOODS : k == 10 ? G_HOUSE_GOODS_B : G_HOUSE_GOODS_C;
                int32_t v = S32(st, table + 0x58u * (uint32_t) (int32_t) S16(st, r + BLD_SUBTYPE));
                if (U8(st, r + BLD_SIZE) > 1) {
                    v <<= 2;
                }
                S16(st, w) = (int16_t) (v + v);
            }
        } else if (type == 0x46) {
            if (cnt <= 0) {
                continue;
            }
            int32_t j = 0;
            do {
                int32_t g = S32(st, G_GOODS_AVAILABLE + 4u * (uint32_t) j);
                /* @bin 0x004C454C the stock slot by good */
                int32_t idx;
                switch (g) {
                case 1: idx = 0; break;
                case 2: idx = 1; break;
                case 3: idx = 2; break;
                case 4: idx = 3; break;
                case 5: idx = 4; break;
                case 6: idx = 5; break;
                case 7: idx = 6; break;
                case 8: idx = 7; break;
                case 0xD: idx = 9; break;
                case 0xF: idx = 8; break;
                case 0x11: idx = 10; break;
                case 0x13: idx = 11; break;
                default: idx = -1; break;
                }
                /* @bin 0x005013A0 (storage_kind, ported inline in goods.c) goods 1..8, 0xD, 0xF, 0x11, 0x13 */
                if ((g >= 1 && g <= 8) || g == 0xD || g == 0xF || g == 0x11 || g == 0x13) {
                    uint32_t w = r + BLD_INVENTORY + 2u * (uint32_t) idx;
                    int16_t have = S16(st, w);
                    if (have != 0) {
                        int32_t lim = g >= 1 && g <= 8 ? (g != 1 ? 600 : 800) : 400;
                        int32_t v = have * 2;
                        if (v > lim) {
                            lim = v;
                        }
                        if (lim >= 20000) {
                            lim = 20000;
                        }
                        S16(st, w) = (int16_t) lim;
                    }
                }
                j++;
                cnt = S32(st, G_STORAGE_LIST);
            } while (j < cnt);
        }
    }
}

/* @bin 0x00562069 the CRT qsort (MSVC 6: insertion-free short sort up to 8 elements, else
 * partition around the middle element), on 4-byte elements with a comparator */
static void qsort_swap(int32_t *a, int32_t *b)
{
    int32_t t = *a;
    *a = *b;
    *b = t;
}

static void qsort_short(int32_t *lo, int32_t *hi, int (*cmp)(game_state *, int32_t, int32_t), game_state *st)
{
    while (hi > lo) {
        int32_t *max = lo;
        for (int32_t *p = lo + 1; p <= hi; p++) {
            if (cmp(st, *p, *max) > 0) {
                max = p;
            }
        }
        qsort_swap(max, hi);
        hi--;
    }
}

void crt_qsort4(game_state *st, int32_t *base, uint32_t num, int (*cmp)(game_state *, int32_t, int32_t))
{
    int32_t *lostk[30], *histk[30];
    int stkptr = 0;
    if (num < 2) {
        return;
    }
    int32_t *lo = base, *hi = base + (num - 1);
    for (;;) {
        uint32_t size = (uint32_t) (hi - lo) + 1;
        if (size <= 8) {
            qsort_short(lo, hi, cmp, st);
        } else {
            int32_t *mid = lo + size / 2;
            qsort_swap(mid, lo);
            int32_t *loguy = lo, *higuy = hi + 1;
            for (;;) {
                do {
                    loguy++;
                } while (loguy <= hi && cmp(st, *loguy, *lo) <= 0);
                do {
                    higuy--;
                } while (higuy > lo && cmp(st, *higuy, *lo) >= 0);
                if (higuy < loguy) {
                    break;
                }
                qsort_swap(loguy, higuy);
            }
            qsort_swap(lo, higuy);
            if ((higuy - lo) * 4 - 1 >= (hi - loguy) * 4) {
                if (lo + 1 < higuy) {
                    lostk[stkptr] = lo;
                    histk[stkptr] = higuy - 1;
                    stkptr++;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[stkptr] = loguy;
                    histk[stkptr] = hi;
                    stkptr++;
                }
                if (lo + 1 < higuy) {
                    hi = higuy - 1;
                    continue;
                }
            }
        }
        stkptr--;
        if (stkptr < 0) {
            return;
        }
        lo = lostk[stkptr];
        hi = histk[stkptr];
    }
}

static void state_swap(game_state *st, uint32_t a, uint32_t b, uint32_t width)
{
    /* @bin 0x0056220B */
    if (a == b) {
        return;
    }
    for (uint32_t i = 0; i < width; i++) {
        uint8_t t = P8(st, a + i); /* arrays in the segments or the simulation heap */
        P8(st, a + i) = P8(st, b + i);
        P8(st, b + i) = t;
    }
}

static int32_t state_cmp(game_state *st, uint32_t cmp, uint32_t a, uint32_t b)
{
    return GAME_CALL(st, cmp, (int32_t) a, (int32_t) b);
}

void crt_qsort_state(game_state *st, uint32_t base, uint32_t num, uint32_t width, uint32_t cmp)
{
    /* @bin 0x00562069 the same algorithm on the state's bytes, the comparator called with the
     * elements' addresses */
    uint32_t lostk[30], histk[30];
    int stkptr = 0;
    if (num < 2 || width == 0) {
        return;
    }
    uint32_t lo = base, hi = base + width * (num - 1);
    for (;;) {
        uint32_t size = (hi - lo) / width + 1;
        if (size <= 8) {
            /* @bin 0x005621BD */
            while (hi > lo) {
                uint32_t max = lo;
                for (uint32_t p = lo + width; p <= hi; p += width) {
                    if (state_cmp(st, cmp, p, max) > 0) {
                        max = p;
                    }
                }
                state_swap(st, max, hi, width);
                hi -= width;
            }
        } else {
            uint32_t mid = lo + (size / 2) * width;
            state_swap(st, mid, lo, width);
            uint32_t loguy = lo, higuy = hi + width;
            for (;;) {
                do {
                    loguy += width;
                } while (loguy <= hi && state_cmp(st, cmp, loguy, lo) <= 0);
                do {
                    higuy -= width;
                } while (higuy > lo && state_cmp(st, cmp, higuy, lo) >= 0);
                if (higuy < loguy) {
                    break;
                }
                state_swap(st, loguy, higuy, width);
            }
            state_swap(st, lo, higuy, width);
            if ((int32_t) (higuy - lo - 1) >= (int32_t) (hi - loguy)) {
                if (lo + width < higuy) {
                    lostk[stkptr] = lo;
                    histk[stkptr] = higuy - width;
                    stkptr++;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[stkptr] = loguy;
                    histk[stkptr] = hi;
                    stkptr++;
                }
                if (lo + width < higuy) {
                    hi = higuy - width;
                    continue;
                }
            }
        }
        stkptr--;
        if (stkptr < 0) {
            return;
        }
        lo = lostk[stkptr];
        hi = histk[stkptr];
    }
}

static int by_part(game_state *st, int32_t a, int32_t b)
{
    /* @bin 0x004C45A0 by +0x12 */
    int32_t x = S16(st, BUILDING_ADDR(a) + BLD_SUBTYPE), y = S16(st, BUILDING_ADDR(b) + BLD_SUBTYPE);
    return x < y ? -1 : x > y;
}

void houses_destroy_4c45e0(game_state *st)
{
    /* @bin 0x004C45E0 up to 20 houses (types 0xA..0x1D) collapse: the first 20, sorted by level
     * +0x12 (0x00562069 with 0x004C45A0), then those from building 21 on above the last one's
     * level inserted before the first lower one */
    int32_t list[20];
    int32_t n = 0;
    int full = 0;
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t r = BUILDING_ADDR(b);
        int16_t type = S16(st, r + BLD_TYPE);
        if (type >= 0xA && type <= 0x1D && U8(st, r + BLD_STATE) == 1) {
            list[n++] = b;
            if (n >= 20) {
                full = 1;
                break;
            }
        }
    }
    if (full) {
        int32_t pos = 0x13;
        crt_qsort4(st, list, 20, by_part);
        for (int32_t b = 21; b < BUILDING_COUNT; b++) {
            uint32_t r = BUILDING_ADDR(b);
            int16_t type = S16(st, r + BLD_TYPE);
            if (type < 0xA || type > 0x1D || U8(st, r + BLD_STATE) != 1) {
                continue;
            }
            int16_t lv = S16(st, r + BLD_SUBTYPE);
            if (lv <= S16(st, BUILDING_ADDR(list[19]) + BLD_SUBTYPE)) {
                continue;
            }
            for (int32_t i = 0; i < 20; i++) {
                if (S16(st, BUILDING_ADDR(list[i]) + BLD_SUBTYPE) < lv) {
                    pos = i;
                    break;
                }
            }
            for (int32_t i = 19; i > pos; i--) {
                list[i] = list[i - 1];
            }
            list[pos] = b;
        }
    }
    for (int32_t i = 0; i < n; i++) {
        destroy_with_routes(st, list[i]);
    }
}

void workers_recalled_4b4710(game_state *st)
{
    /* @bin 0x004B4710 figures in use (+0xE == 1) of types 0x19, 0x4D, 0x4E: type 0x43, their
     * building +0x5A and +0x36 cleared, the building's +0xB4 cleared */
    for (int32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t fr = FIGURE_ADDR(f);
        if (U8(st, fr + FIG_STATE) != 1) {
            continue;
        }
        uint8_t type = U8(st, fr + FIG_TYPE);
        if (type != 0x19 && type != 0x4D && type != 0x4E) {
            continue;
        }
        int16_t b = S16(st, fr + FIG_HOME);
        S16(st, fr + FIG_HOME) = 0;
        U8(st, fr + FIG_TYPE) = 0x43;
        S16(st, fr + FIG_WAIT_TICKS) = 0;
        S16(st, BUILDING_ADDR(b) + BLD_SECOND_WALKER) = 0;
    }
}

static int32_t run_qsort(game_state *st, const int32_t *a)
{
    uint32_t n = (uint32_t) a[1], w = (uint32_t) a[2];
    if (n >= 2 && w != 0 && state_ptr(st, (uint32_t) a[0], n * w) == NULL) {
        state_unported(st, FN_CRT_QSORT); /* an array outside the state: its caller's stack */
        return 0;
    }
    crt_qsort_state(st, (uint32_t) a[0], n, w, (uint32_t) a[3]);
    return 0;
}

static int32_t run_swap(game_state *st, const int32_t *a)
{
    uint32_t w = (uint32_t) a[2];
    if (w != 0 && (state_ptr(st, (uint32_t) a[0], w) == NULL || state_ptr(st, (uint32_t) a[1], w) == NULL)) {
        state_unported(st, FN_CRT_QSORT_SWAP); /* outside the state: its caller's stack */
        return 0;
    }
    state_swap(st, (uint32_t) a[0], (uint32_t) a[1], w);
    return 0;
}

static int32_t run_4c45a0(game_state *st, const int32_t *a)
{
    return by_part(st, (int32_t) U32(st, (uint32_t) a[0]), (int32_t) U32(st, (uint32_t) a[1]));
}

static int32_t run_4f5340(game_state *st, const int32_t *a)
{
    (void) a;
    return pyramid_progress_4f5340(st);
}

static int32_t run_4f3aa0(game_state *st, const int32_t *a)
{
    pyramid_speedup_4f3aa0(st, a[0]);
    return 0;
}

static int32_t run_4c3950(game_state *st, const int32_t *a)
{
    (void) a;
    return storage_fill_4c3950(st);
}

static int32_t run_4c3ac0(game_state *st, const int32_t *a)
{
    (void) a;
    return workshops_fill_4c3ac0(st);
}

static int32_t run_4c3ed0(game_state *st, const int32_t *a)
{
    (void) a;
    return buildings_destroy_4c3ed0(st);
}

static int32_t run_4c4020(game_state *st, const int32_t *a)
{
    storage_strike_4c4020(st, a[0]);
    return 0;
}

static int32_t run_4c41b0(game_state *st, const int32_t *a)
{
    (void) a;
    return fort_destroy_4c41b0(st);
}

static int32_t run_4c42b0(game_state *st, const int32_t *a)
{
    (void) a;
    houses_stock_4c42b0(st);
    return 0;
}

static int32_t run_4c45e0(game_state *st, const int32_t *a)
{
    (void) a;
    houses_destroy_4c45e0(st);
    return 0;
}

static int32_t run_4b4710(game_state *st, const int32_t *a)
{
    (void) a;
    workers_recalled_4b4710(st);
    return 0;
}

const ported_function rest_gods_ports[] = {
    {0x004F5340u, "pyramid_progress_4f5340", 0, 32, 0, run_4f5340, 0},
    {0x004F3AA0u, "pyramid_speedup_4f3aa0", 1, 0, 0, run_4f3aa0, 0},
    {0x004C3950u, "storage_fill_4c3950", 0, 32, 0, run_4c3950, 0},
    {0x004C3AC0u, "workshops_fill_4c3ac0", 0, 32, 0, run_4c3ac0, 0},
    {0x004C3ED0u, "buildings_destroy_4c3ed0", 0, 32, 0, run_4c3ed0, 0},
    {0x004C4020u, "storage_strike_4c4020", 1, 0, 0, run_4c4020, 0},
    {0x004C41B0u, "fort_destroy_4c41b0", 0, 32, 0, run_4c41b0, 0},
    {0x004C42B0u, "houses_stock_4c42b0", 0, 0, 0, run_4c42b0, 0},
    {0x004C45E0u, "houses_destroy_4c45e0", 0, 0, 0, run_4c45e0, 0},
    {0x004C45A0u, "by_part", 2, 32, 0, run_4c45a0, 0},
    {0x00562069u, "crt_qsort", 4, 0, 0, run_qsort, 0},
    {0x0056220Bu, "crt_qsort_swap", 3, 0, 0, run_swap, 0},
    {0x005621BDu, "crt_qsort_short (in rest_gods.c)", 4, 0, 0, NULL, 0},
    {0x004B4710u, "workers_recalled_4b4710", 0, 0, 0, run_4b4710, 0},
};
const int rest_gods_ports_count = sizeof(rest_gods_ports) / sizeof(rest_gods_ports[0]);
