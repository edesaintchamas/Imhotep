/* Imhotep - helpers of the figure actions of group A: where cart pushers take or fetch goods.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). These functions are reached only by direct
 * calls (absent from the Ghidra export, so the oracle cannot record them): they are ported with
 * their callers. */
#include "figure/actions_a.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/records.h"


static int32_t workshop_good(int16_t v);

/* @bin 0x007900C2 the storage settings: 0xC4 bytes per storage (building +0xC1), one per good */
static uint8_t storage_setting(game_state *st, uint32_t storage, uint32_t good)
{
    return U8(st, G_STORAGE_ORDERS + 0xC4u * storage + good);
}

/* @bin 0x00EA42F8 per player, per good (0x1276 dwords per player): non-zero, the good is held */
static int32_t good_held(game_state *st, uint32_t good)
{
    return S32(st, CITY_BASE + CITY_U2A88 + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + good));
}

/* append building `id` at grid offset `offset` to the candidate lists (count `n` before) */
static void add_candidate(game_state *st, int32_t n, int32_t id, uint32_t offset)
{
    U32(st, G_CANDIDATE_IDS + 4u + 4u * (uint32_t) n) = (uint32_t) id;
    U32(st, G_CANDIDATES + 4u + 4u * (uint32_t) n) = offset;
}

/* 0x004B28F0 chooses among the candidates the one figure `fid` reaches; returns its index (1 ..) */
static int32_t choose_candidate(game_state *st, int32_t fid, int32_t n)
{
    return figure_route_to_any(st, fid, G_CANDIDATES + 4u, n);
}

int32_t cart_storage_for_good(game_state *st, int32_t fid, int32_t good, int32_t terrain)
{
    /* @bin 0x0045DCC0 the storage yards (types 0x47, 0xBF) of terrain `terrain` that accept `good`
     * (1 .. 8) and still have room; the figure picks the one it reaches */
    if (S32(st, G_U784BBC) != 0) {
        return 0;
    }
    if (good <= 0 || good > 8) {
        return 0;
    }
    if (good_held(st, (uint32_t) good) != 0) {
        return 0;
    }
    int32_t n = 0;
    S32(st, G_CANDIDATES) = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        int16_t t = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || (t != 0x47 && t != 0xBF) || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0 ||
            U8(st, b + BLD_ROAD_NETWORK) != terrain) {
            continue;
        }
        /* @bin 0x0045DD7A the type's table 0x00705EB8 (40 bytes per building type) */
        int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 40u * (uint32_t) t));
        if (r < 100) {
            continue;
        }
        uint32_t k = U8(st, b + BLD_STORAGE_ID);
        if (storage_setting(st, k, (uint32_t) good) != 2) {
            continue;
        }
        /* @bin 0x0045DDB0 the settings' limits (0x00790134, 0x62 words per storage) */
        if (S16(st, G_STORAGE_GET + 2u * (k * 0x62u + (uint32_t) good)) <= S16(st, b + BLD_INVENTORY + 2u * (uint32_t) good) ||
            S16(st, b + BLD_INVENTORY) <= 100) {
            continue;
        }
        U32(st, G_CANDIDATE_IDS + 4u + 4u * (uint32_t) n) = (uint32_t) i;
        n++;
        S32(st, G_CANDIDATES) = n;
        U32(st, G_CANDIDATES + 4u * (uint32_t) n) =
            U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_X) + 228u * (uint32_t) S16(st, b + BLD_Y) + 0xE5u;
    }
    if (n == 0) {
        return 0;
    }
    int32_t r = choose_candidate(st, fid, n);
    if (r == 0) {
        return 0;
    }
    int32_t id = (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r);
    uint32_t b = BUILDING_ADDR(id);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, b + BLD_Y) + 1;
    S32(st, G_ROAD_ACCESS_X) = S16(st, b + BLD_X) + 1;
    return id;
}

int32_t cart_storage_to_empty(game_state *st, int32_t bid)
{
    /* @bin 0x0045DE80 among the storage list 0x0078D8D4 (0x0078D9A0 entries) of building `bid`'s
     * terrain, the nearest one (0x0045DC50, doubled up to 400 units) that wants goods `bid` is set
     * to empty (setting 2); 0x00D3B4E0/E4 its entry point (building 0's when none) */
    uint32_t b = BUILDING_ADDR(bid);
    uint32_t k = U8(st, b + BLD_STORAGE_ID);
    int32_t emptying = 0;
    for (uint32_t g = 1; g <= 8; g++) {
        if (storage_setting(st, k, g) == 2) {
            emptying++;
        }
    }
    if (S32(st, G_U784BBC) != 0 || emptying <= 0) {
        return 0;
    }
    int32_t best = 10000, found = 0;
    for (int32_t i = 0; i < S32(st, G_STORAGE_LIST_COUNT); i++) {
        int16_t s = S16(st, G_STORAGE_BUILDINGS + 2u * (uint32_t) i);
        uint32_t c = BUILDING_ADDR(s);
        if (U8(st, c + BLD_ROAD_NETWORK) != U8(st, b + BLD_ROAD_NETWORK)) {
            continue;
        }
        uint32_t kc = U8(st, c + BLD_STORAGE_ID);
        int32_t room = 0;
        for (uint32_t g = 1; g <= 8; g++) {
            if (storage_setting(st, k, g) == 2 && storage_setting(st, kc, g) != 2) {
                room += S16(st, c + BLD_INVENTORY + 2u * g);
            }
        }
        if (room <= 0) {
            continue;
        }
        /* @bin 0x0045DFA0 the original passes `bid`'s +0x26 twice (not +0x28) */
        int32_t d = GAME_CALL(st, FN_BUILDING_DISTANCE, S16(st, c + BLD_ROAD_ACCESS_X), S16(st, c + BLD_ROAD_ACCESS_Y), S16(st, b + BLD_ROAD_ACCESS_X),
                              S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_DISTANCE), S16(st, c + BLD_DISTANCE));
        if (room <= 400) {
            d += d;
        }
        if (d < best) {
            best = d;
            found = s;
        }
    }
    uint32_t f = BUILDING_ADDR(found);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, f + BLD_ROAD_ACCESS_Y);
    S32(st, G_ROAD_ACCESS_X) = S16(st, f + BLD_ROAD_ACCESS_X);
    return found;
}

/* @bin 0x0045F47A (jump table 0x0045F654) the good -> the workshop kind (building +0x12) that uses
 * it; 0: none */
static int32_t workshop_for_good(int32_t good)
{
    switch (good) {
    case 0x02: return 0x15;
    case 0x08: return 0x16;
    case 0x09: return 0x0D;
    case 0x0A: return 0x08;
    case 0x0B: return 0x05;
    case 0x0C: return 0x07;
    case 0x0D: return 0x18;
    case 0x0E: return 0x09;
    case 0x0F: return 0x13;
    case 0x10: return 0x01;
    case 0x11: return 0x10;
    case 0x12: return 0x0A;
    case 0x14: return 0x04;
    case 0x15: return 0x0F;
    case 0x16: return 0x0B;
    case 0x17: return 0x0C;
    case 0x18: return 0x03;
    case 0x19: return 0x06;
    case 0x1C: return 0x0E;
    case 0x1D: return 0x11;
    case 0x1E: return 0x12;
    case 0x1F: return 0x17;
    case 0x20: return 0x14;
    case 0x21: return 0x19;
    case 0x22: return 0x1A;
    default: return 0;
    }
}

/* @bin 0x0045F583 (jump table 0x0045F6D8), 0x0048CAD3 (0x0048CCEC): building types 0x6E .. 0x72 and
 * these are workshops (the same list as figure_delete's) */
static int is_workshop(int16_t t)
{
    if (t >= 0x6E && t <= 0x72) {
        return 1;
    }
    switch (t) {
    case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
    case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
    case 0xE7: case 0xE8: case 0xE9:
        return 1;
    default:
        return 0;
    }
}

int32_t cart_workshop_for_good(game_state *st, int32_t fid, int32_t good, int32_t terrain)
{
    /* @bin 0x0045F450 the workshops of terrain `terrain` working `good`'s kind, in use and staffed
     * (+0x44, +0x20); the figure picks the one it reaches */
    if (good_held(st, (uint32_t) good) != 0) {
        return 0;
    }
    int32_t kind = workshop_for_good(good);
    if (kind == 0) {
        return 0;
    }
    int32_t n = 0, found = 0;
    S32(st, G_CANDIDATES) = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) != 1 || !is_workshop(S16(st, b + BLD_TYPE)) || U8(st, b + BLD_U44) == 0 ||
            S16(st, b + BLD_DISTANCE) <= 0 || S16(st, b + BLD_SUBTYPE) != kind || U8(st, b + BLD_ROAD_NETWORK) != terrain) {
            continue;
        }
        add_candidate(st, n, i,
                      U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_X) + 228u * (uint32_t) S16(st, b + BLD_Y));
        n++;
    }
    S32(st, G_CANDIDATES) = n;
    if (n == 0) {
        return 0;
    }
    int32_t r = choose_candidate(st, fid, n);
    if (r <= 0) {
        return 0;
    }
    found = (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r);
    uint32_t b = BUILDING_ADDR(found);
    S32(st, G_ROAD_ACCESS_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    return found;
}

int32_t cart_take_from_storage(game_state *st, int32_t from, int32_t to)
{
    /* @bin 0x0045E150 the good storage `to` gets most of that `from` is set to empty and `to` is
     * not (at most 800, and what `from` holds at +0x56); 0x0045E360 takes it out of `to`.
     * 0x0078D99C: the good chosen */
    uint32_t a = BUILDING_ADDR(from);
    uint32_t ka = U8(st, a + BLD_STORAGE_ID);
    uint32_t kb = U8(st, BUILDING_ADDR(to) + BLD_STORAGE_ID);
    int32_t best = 0, good = 0;
    for (uint32_t g = 1; g <= 8; g++) {
        if (storage_setting(st, ka, g) != 2 || storage_setting(st, kb, g) == 2) {
            continue;
        }
        int32_t v = S16(st, BUILDING_ADDR(to) + BLD_INVENTORY + 2u * g);
        if (v > best) {
            best = v;
            good = (int32_t) g;
        }
    }
    if (best > 0x320) {
        best = 0x320;
    }
    if (best > S16(st, a + BLD_INVENTORY)) {
        best = S16(st, a + BLD_INVENTORY);
    }
    GAME_CALL(st, FN_STORAGE_TAKE_STOCK, (int32_t) U32(st, CURRENT_PLAYER_ADDR), to, good, best);
    S32(st, G_GOOD_CHOSEN) = good;
    return best;
}

/* @bin 0x0048CAD3 .. 0x0048CBCE (jump table 0x0048CDC0) building +0xCE -> the good its cart
 * pushers carry; -1: none (as in figure_delete) */
static int32_t workshop_good(int16_t v)
{
    switch (v) {
    case 1: return 0x10;
    case 3: return 0x18;
    case 4: return 0x14;
    case 5: return 0x0B;
    case 6: return 0x19;
    case 7: return 0x0C;
    case 8: return 0x0A;
    case 9: return 0x0E;
    case 10: return 0x12;
    case 0x0B: return 0x16;
    case 0x0C: return 0x17;
    case 0x0D: return 9;
    case 0x0E: return 0x1C;
    case 0x0F: return 0x15;
    case 0x10: return 0x11;
    case 0x11: return 0x1D;
    case 0x12: return 0x1E;
    case 0x14: return 0x20;
    case 0x15: return 2;
    case 0x16: return 8;
    case 0x17: return 0x1F;
    case 0x18: return 0x0D;
    case 0x19: return 0x21;
    case 0x1A: return 0x22;
    default: return -1;
    }
}

void cart_delivered(game_state *st, int32_t taken, int32_t fid, int32_t terrain, int32_t last)
{
    /* @bin 0x0048CAA0 a cart pusher left `taken` of its load (+0x74) at its destination (+0x5E):
     * the destination's goods in transit (+0x104 / +0x106) go down; all of it: it goes home (or
     * dies, `last`), else 0x0048AE50 looks for where to take the rest */
    uint32_t f = FIGURE_ADDR(fid);
    uint32_t d = BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING));
    int16_t t = S16(st, d + BLD_TYPE);
    if (is_workshop(t)) {
        int32_t good = workshop_good(S16(st, d + BLD_INPUT2));
        if (t == 0xCC && U8(st, f + FIG_RESOURCE) == good) {
            S16(st, d + BLD_INCOMING_AMOUNT) -= 1;
        } else {
            S16(st, d + BLD_INCOMING_CARTS) -= 1;
        }
    } else if (t == 0x47) {
        S16(st, d + BLD_INCOMING_CARTS) -= 1;
        S16(st, d + BLD_INCOMING_AMOUNT) -= S16(st, f + FIG_AMOUNT);
    }
    int16_t load = S16(st, f + FIG_AMOUNT);
    if (taken == load) {
        if ((uint8_t) last != 0) {
            U8(st, f + FIG_STATE) = 2;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            return;
        }
        U8(st, f + FIG_ACTION) = 0x0B;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
        S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
        S16(st, f + FIG_AMOUNT) = 0;
        return;
    }
    S16(st, f + FIG_AMOUNT) = (int16_t) (load - taken);
    if (cart_find_destination(st, fid, terrain) != 0) {
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        return;
    }
    U8(st, f + FIG_ACTION) = 8;
    U8(st, f + FIG_SERVICE_MAX) = 2;
    S16(st, f + FIG_WAIT_TICKS) = 0;
}

int32_t cart_find_destination(game_state *st, int32_t fid, int32_t terrain)
{
    /* @bin 0x0048AE50 where a cart pusher takes its good (+0x0B): a workshop, a storage yard (0x0045EAB0),
     * then 0x0045ACF0, 0x0045D9E0 (0, then 1); +0x5E the building found (0: none) */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t r = GAME_CALL(st, FN_CART_CITY_BUILDING, S16(st, f + FIG_X), S16(st, f + FIG_Y), U8(st, f + FIG_RESOURCE), terrain);
    if (r == 0) {
        r = cart_workshop_with_room(st, fid, U8(st, f + FIG_RESOURCE), terrain);
    }
    if (r == 0) {
        r = cart_dock_for_good(st, 0, fid, U8(st, f + FIG_RESOURCE), terrain);
    }
    if (r == 0) {
        r = cart_storage_with_room(st, 0, fid, U8(st, f + FIG_RESOURCE), terrain);
    }
    if (r == 0) {
        r = cart_storage_with_room(st, 1, fid, U8(st, f + FIG_RESOURCE), terrain);
    }
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) r;
    return r;
}

int32_t cart_good_sprites(int32_t good)
{
    /* @bin 0x0048B320 (jump table 0x0048B408) good 1 .. 0x23 -> the sprite group of the loaded cart */
    static const uint16_t groups[0x23] = {
        0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
        0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x74, 0x90, 0x92, 0x93, 0x2A2, 0x2A3, 0x2A4,
        0x2A5, 0x2A6,
    };
    uint32_t i = (uint32_t) good - 1u;
    return i <= 0x22u ? groups[i] : 0x74;
}

/* @bin 0x0048B4EC (jump tables 0x0048B5D8, 0x0048B5EC, 0x0048B600) goods drawn like 0x0C */
static int cart_goods_turned(uint8_t g)
{
    return g == 0x18 || g == 0x19 || g == 0x1A || g == 0x1E || g == 0x23 || g == 0x0C;
}

void cart_sprite_load(game_state *st, int32_t fid)
{
    /* @bin 0x0048B4A0 +0x180: the cart's sprite by the good and the load (+0x74: none, up to 200,
     * up to 400, more) */
    uint32_t f = FIGURE_ADDR(fid);
    int16_t load = S16(st, f + FIG_AMOUNT);
    int32_t e = load == 0 ? -1 : load <= 0xC8 ? 0 : load > 0x190 ? 0x10 : 8;
    uint8_t g = U8(st, f + FIG_RESOURCE);
    if (cart_goods_turned(g) && e != -1) {
        e = 0;
    }
    int empty;
    if (U8(st, f + FIG_TYPE) == 0x26) {
        uint8_t a = U8(st, f + FIG_ACTION);
        empty = a == 0x10 || a == 0x0C || (a == 7 && (U8(st, f + FIG_ACTION_BEFORE_ATTACK) == 0x10 || U8(st, f + FIG_ACTION_BEFORE_ATTACK) == 0x0C));
        if (!empty) {
            e = 0;
        }
    } else {
        empty = e < 0;
    }
    if (!empty) {
        S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) cart_good_sprites(g)) + e;
        return;
    }
    int turned = cart_goods_turned(g) || (g == 0 && cart_goods_turned(U8(st, f + FIG_COLLECTING)));
    S32(st, f + FIG_U180) = S32(st, turned ? G_IMAGE_GROUP_145 : G_IMAGE_GROUP_116);
}

/* @bin 0x0045ECF2 (jump tables 0x0045F120, 0x0045F1D0, 0x0045F280, 0x0045F330) a workshop's stock
 * capacity by building type */
static int32_t workshop_capacity(int16_t t)
{
    switch (t) {
    case 0x20: case 0x33: return 300;
    case 0x2F: case 0x35: return 500;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD: return 100000;
    default: return 200;
    }
}

int32_t cart_workshop_with_room(game_state *st, int32_t fid, int32_t good, int32_t terrain)
{
    /* @bin 0x0045EAB0 the workshops of terrain `terrain` using `good` (in either of their two good
     * slots: kind +0x12 / +0xCE, stock +0x3C / +0xCC, carts on the way +0x104 / +0x106) with room
     * for 100 more, not stocked (0x00459240); the least awaited, then the least stocked; the figure
     * picks the one it reaches (0x004B28F0), whose carts on the way go up */
    if (good_held(st, (uint32_t) good) != 0) {
        return 0;
    }
    int32_t kind = workshop_for_good(good);
    if (kind == 0) {
        return 0;
    }
    int32_t best_wait = 10000, best_stock = 10000;
    S32(st, G_CANDIDATES) = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        if (GAME_CALL(st, FN_BUILDING_RESOURCE_IN_STOCK, i) != 0) {
            continue;
        }
        int16_t t = S16(st, b + BLD_TYPE);
        if (!is_workshop(t) || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0) {
            continue;
        }
        if (S16(st, b + BLD_SUBTYPE) != kind && S16(st, b + BLD_INPUT2) != kind) {
            continue;
        }
        if (U8(st, b + BLD_ROAD_NETWORK) != terrain) {
            continue;
        }
        if ((t == 0xBB || t == 0xBC || t == 0xBD) && S16(st, b + BLD_WORKERS) == 0) {
            continue;
        }
        int second = S16(st, b + BLD_INPUT2) != 0 && S16(st, b + BLD_INPUT2) == kind;
        int32_t cap = workshop_capacity(t);
        int32_t stock = S16(st, b + (second ? 0xCC : 0x3C));
        if (stock + 100 > cap) {
            continue;
        }
        int16_t wait = S16(st, b + (second ? 0x106 : 0x104));
        if (wait > 0 && cap - wait * 100 - stock <= 0) {
            continue;
        }
        if (wait > best_wait || stock > best_stock) {
            continue;
        }
        if (stock < best_stock) {
            S32(st, G_CANDIDATES) = 0;
            best_wait = wait;
            best_stock = stock;
        }
        int32_t n = S32(st, G_CANDIDATES);
        U32(st, G_CANDIDATE_IDS + 4u + 4u * (uint32_t) n) = (uint32_t) i;
        n++;
        S32(st, G_CANDIDATES) = n;
        U32(st, G_CANDIDATES + 4u * (uint32_t) n) =
            U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, b + BLD_ROAD_ACCESS_Y);
    }
    int32_t r = figure_route_to_any(st, fid, G_CANDIDATES + 4u, S32(st, G_CANDIDATES));
    int32_t id = r != 0 ? (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r) : 0;
    uint32_t b = BUILDING_ADDR(id);
    S32(st, G_ROAD_ACCESS_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    if (id == 0) {
        return 0;
    }
    if (S16(st, b + BLD_TYPE) == 0xCC && good == workshop_good(S16(st, b + BLD_INPUT2))) {
        S16(st, b + BLD_INCOMING_AMOUNT) += 1;
        return id;
    }
    S16(st, b + BLD_INCOMING_CARTS) += 1;
    return id;
}

int32_t walker_find_workplace(game_state *st, int32_t fid)
{
    /* @bin 0x00467450 (jump table 0x0046774C) for figure types 0x0F .. 0x12, 0x69: the buildings of
     * the types they serve (0x0F: 0x1F, 0x1E, 0x21; 0x10: 0x1E, 0x21; 0x11: 0x21; 0x12: 0x20; 0x69:
     * 0xE2), in use, staffed (+0x40), main parts (0x00475B50) and free (+0xA7 / +0xA8 / +0xA9 by
     * figure type): the one the figure reaches; none free: the nearest by the flood 0x00519A30 plus
     * that busy count (+0x5E and the target set) */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t k1 = 0, k2 = 0, k3 = 0;
    S32(st, G_WORK_COUNT) = 0;
    switch (U8(st, f + FIG_TYPE)) {
    case 0x0F: k1 = 0x1F; /* fall through */
    case 0x10: k2 = 0x1E; /* fall through */
    case 0x11: k3 = 0x21; break;
    case 0x12: k1 = 0x20; break;
    case 0x69: k1 = 0xE2; break;
    default: return 0;
    }
    S32(st, G_CANDIDATES) = 0;
    int32_t n = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        int16_t t = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WORKERS) <= 0 || t == 0 || (t != k1 && t != k2 && t != k3)) {
            continue;
        }
        if (GAME_CALL(st, FN_BUILDING_MAIN_PART, i) != i) {
            continue;
        }
        uint8_t ft = U8(st, f + FIG_TYPE);
        if (((ft == 0x0F || ft == 0x12 || ft == 0x69) && U8(st, b + BLD_UA7) != 0) ||
            (ft == 0x10 && U8(st, b + BLD_UA8) != 0) || (ft == 0x11 && U8(st, b + BLD_UA9) != 0)) {
            continue;
        }
        n = S32(st, G_CANDIDATES);
        U32(st, G_CANDIDATE_IDS + 4u + 4u * (uint32_t) n) = (uint32_t) i;
        n++;
        U32(st, G_CANDIDATES + 4u * (uint32_t) n) =
            U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, b + BLD_ROAD_ACCESS_Y);
        S32(st, G_CANDIDATES) = n;
    }
    n = S32(st, G_CANDIDATES);
    if (n != 0) {
        if (n <= 0) {
            return 0;
        }
        int32_t r = figure_route_to_any(st, fid, G_CANDIDATES + 4u, n);
        return r > 0 ? (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r) : 0;
    }
    int32_t best = 1000, found = 0;
    GAME_CALL(st, FN_ROUTE_FLOOD_EE0C50, S16(st, f + FIG_X), S16(st, f + FIG_Y));
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        int16_t t = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WORKERS) <= 0 || t == 0 || (t != k1 && t != k2 && t != k3)) {
            continue;
        }
        if (GAME_CALL(st, FN_BUILDING_MAIN_PART, i) != i) {
            continue;
        }
        uint32_t tile = U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, b + BLD_ROAD_ACCESS_Y);
        int32_t d = S32(st, G_GRID_ROAD_DISTANCE + 4u * tile);
        uint8_t ft = U8(st, f + FIG_TYPE);
        if (ft == 0x0F && U8(st, b + BLD_UA7) != 0) {
            d += U8(st, b + BLD_UA7);
        }
        if (ft == 0x10 && U8(st, b + BLD_UA8) != 0) {
            d += U8(st, b + BLD_UA8);
        }
        if (ft == 0x11 && U8(st, b + BLD_UA9) != 0) {
            d += U8(st, b + BLD_UA9);
        }
        if (d < best) {
            best = d;
            found = i;
        }
    }
    if (best >= 1000) {
        return 0;
    }
    uint32_t b = BUILDING_ADDR(found);
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) found;
    S16(st, f + FIG_DEST_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    return found;
}

int32_t cart_storage_with_room(game_state *st, int32_t force, int32_t fid, int32_t good, int32_t terrain)
{
    /* @bin 0x0045D9E0 the working storage yards (types 0x47, 0xBF; 0x004CDE60 ratio 100 or more,
     * else 0x00D05628 counts them) of terrain `terrain` accepting `good` (1 .. 8; setting 0 or 2
     * with its limit, table 0x007900EC / 0x00790134) with room for the carts on the way (+0x104,
     * +0x106) and at least 100 free (+0x56); not for a held good unless `force`. The figure picks
     * the one it reaches; it gets a cart more on the way (+0x104) carrying +0x74 (+0x106) */
    if (S32(st, G_U784BBC) != 0) {
        return 0;
    }
    if (good <= 0 || good > 8) {
        return 0;
    }
    if (good_held(st, (uint32_t) good) != 0 && force == 0) {
        return 0;
    }
    S32(st, G_CANDIDATES) = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        int16_t t = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || (t != 0x47 && t != 0xBF) || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0 ||
            U8(st, b + BLD_ROAD_NETWORK) != terrain) {
            continue;
        }
        if (GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 40u * (uint32_t) t)) < 100) {
            S32(st, G_YARDS_LOW) += 1;
            continue;
        }
        uint32_t k = U8(st, b + BLD_STORAGE_ID);
        uint8_t s = storage_setting(st, k, (uint32_t) good);
        if (s != 0 && s != 2) {
            continue;
        }
        uint32_t lim = (s == 0 ? G_STORAGE_FILL : G_STORAGE_GET) + 2u * (k * 0x62u + (uint32_t) good);
        int16_t stock = S16(st, b + BLD_INVENTORY + 2u * (uint32_t) good);
        if (S16(st, lim) <= stock) {
            continue;
        }
        if (S16(st, b + BLD_INCOMING_CARTS) > 0 && S16(st, lim) - S16(st, b + BLD_INCOMING_AMOUNT) - stock <= 0) {
            continue;
        }
        if (S16(st, b + BLD_INVENTORY) < 100) {
            continue;
        }
        int32_t n = S32(st, G_CANDIDATES);
        U32(st, G_CANDIDATE_IDS + 4u + 4u * (uint32_t) n) = (uint32_t) i;
        GAME_CALL(st, FN_BUILDING_ROAD_ACCESS_4, S16(st, b + BLD_X), S16(st, b + BLD_Y));
        U32(st, G_CANDIDATES + 4u + 4u * (uint32_t) S32(st, G_CANDIDATES)) =
            (uint32_t) S32(st, G_ROAD_ACCESS_X) + 228u * (uint32_t) S32(st, G_ROAD_ACCESS_Y) + U32(st, G_MAP_START);
        S32(st, G_CANDIDATES) += 1;
    }
    int32_t n = S32(st, G_CANDIDATES);
    if (n == 0) {
        return 0;
    }
    int32_t r = figure_route_to_any(st, fid, G_CANDIDATES + 4u, n);
    if (r == 0) {
        return 0;
    }
    int32_t id = (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r);
    uint32_t b = BUILDING_ADDR(id);
    S32(st, G_ROAD_ACCESS_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    if (id != 0) {
        S16(st, b + BLD_INCOMING_CARTS) += 1;
        S16(st, b + BLD_INCOMING_AMOUNT) += S16(st, FIGURE_ADDR(fid) + FIG_AMOUNT);
    }
    return id;
}

int32_t cart_dock_for_good(game_state *st, int32_t skip, int32_t fid, int32_t good, int32_t terrain)
{
    /* @bin 0x0045ACF0 (not for good 0x15) the working buildings of type 0x49 of terrain `terrain`
     * whose main part (0x00475B50; not `skip` unless set to 2 for the good) accepts `good` under its
     * limit (0x0045D510 the stock; settings 0 / 2, tables 0x007900EC / 0x00790134), with a ratio
     * (0x004CDE60) of 100 or more (else 0x00D05628 counts it), and free for the good (+0x12 none,
     * or the good under 400 at +0x3C); reached at the main part's entry (+0x44 1) or the road next
     * to it (0x00480180). The figure picks the one it reaches */
    S32(st, G_CANDIDATES) = 0;
    if (good == 0x15) {
        return 0;
    }
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_TYPE) != 0x49 || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0 ||
            U8(st, b + BLD_ROAD_NETWORK) != terrain) {
            continue;
        }
        int32_t m = GAME_CALL(st, FN_BUILDING_MAIN_PART, i);
        uint32_t mb = BUILDING_ADDR(m);
        uint32_t k = U8(st, mb + BLD_STORAGE_ID);
        if (skip == m && storage_setting(st, k, (uint32_t) good) != 2) {
            continue;
        }
        uint8_t s = storage_setting(st, k, (uint32_t) good);
        if (s != 0 && s != 2) {
            continue;
        }
        int32_t stock = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, m, good);
        if (S16(st, (s == 0 ? G_STORAGE_FILL : G_STORAGE_GET) + 2u * (k * 0x62u + (uint32_t) good)) <= stock) {
            continue;
        }
        if (GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, mb + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 40u * (uint32_t) S16(st, mb + BLD_TYPE))) <
            100) {
            S32(st, G_YARDS_LOW) += 1;
            continue;
        }
        int16_t k2 = S16(st, b + BLD_SUBTYPE);
        if (k2 != 0 && (k2 != good || S16(st, b + BLD_STORED) >= 400)) {
            continue;
        }
        int32_t m2 = GAME_CALL(st, FN_BUILDING_MAIN_PART, i);
        uint32_t m2b = BUILDING_ADDR(m2);
        uint32_t tile;
        if (U8(st, m2b + BLD_U44) == 1) {
            tile = U32(st, G_MAP_START) + (uint32_t) S16(st, m2b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, m2b + BLD_ROAD_ACCESS_Y);
        } else {
            if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, m2b + BLD_X), S16(st, m2b + BLD_Y), 3) == 0) {
                continue;
            }
            tile = (uint32_t) S32(st, G_ROAD_ACCESS_X) + 228u * (uint32_t) S32(st, G_ROAD_ACCESS_Y) + U32(st, G_MAP_START);
        }
        int32_t n = S32(st, G_CANDIDATES);
        add_candidate(st, n, i, tile);
        S32(st, G_CANDIDATES) = n + 1;
    }
    int32_t n = S32(st, G_CANDIDATES);
    if (n == 0) {
        return 0;
    }
    int32_t r = figure_route_to_any(st, fid, G_CANDIDATES + 4u, n);
    if (r == 0) {
        return 0;
    }
    int32_t id = (int32_t) U32(st, G_CANDIDATE_IDS + 4u * (uint32_t) r);
    uint32_t mb = BUILDING_ADDR(GAME_CALL(st, FN_BUILDING_MAIN_PART, id));
    S32(st, G_ROAD_ACCESS_X) = S16(st, mb + BLD_ROAD_ACCESS_X);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, mb + BLD_ROAD_ACCESS_Y);
    if (U8(st, mb + BLD_U44) == 1 || GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, mb + BLD_X), S16(st, mb + BLD_Y), 3) != 0) {
        return id;
    }
    return 0;
}
