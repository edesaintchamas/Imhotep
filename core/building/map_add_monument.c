/* Imhotep - the monuments' parts created by 0x00470990 for a new monument (package mapadd):
 * pyramids, mastabas, obelisks, the sun temple, the Cleopatra tombs and their objects, and the
 * monument slots of the scenario (0x004F4F80).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/map_add.h"

#include "building/rest_redraw.h"
#include "figure/actions_b.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)
#define TERRAIN 0x00AF598Cu
#define BUILDING_GRID 0x00B4E6FCu
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define ORIENT 0x00BCA2E0u
#define CURSOR 0x00BC9768u
#define PLAYER S32(st, 0x00EAAC28u)
#define CITY CURRENT_CITY(st)
#define SLOTS 0x00784DE0u /* i16[3] the scenario's monument kinds (entries of 0x005D19C8) */
#define KINDS 0x005D19C8u /* 16 bytes: +0 i16 class, +2 i16 kind, +4 i16 variant */

static int32_t create(game_state *st, int32_t player, int32_t type, int32_t x, int32_t y, int32_t n, int32_t id)
{
    return GAME_CALL(st, 0x0046A400u, player, type, x, y, n, id);
}

static void keep_removed(game_state *st, int32_t id)
{
    GAME_CALL(st, 0x00485100u, id);
}

static void tile_of(game_state *st, uint32_t r, int32_t x, int32_t y)
{
    S16(st, r + 6) = (int16_t) x;
    S16(st, r + 8) = (int16_t) y;
    S32(st, r + 0xC) = S32(st, MAP_START) + y * GRID_SIZE + x;
}

static void link_part(game_state *st, int32_t prev, int32_t b)
{
    S16(st, B(b) + 0x38) = (int16_t) prev;
    S16(st, B(prev) + 0x3A) = (int16_t) b;
    S16(st, B(b) + 0x3A) = 0;
}

/* ---- the scenario's monument slots ------------------------------------------------------------ */

static int32_t kinds_count_426b30(void)
{
    /* @bin 0x00426B30 the number of entries of 0x005D19C8 */
    return 0x26;
}

static int32_t pyramid_kind_4f44e0(game_state *st)
{
    /* @bin 0x004F44E0 the first scenario slot of class 1: its kind, else 4 */
    for (uint32_t p = SLOTS; p < SLOTS + 6; p += 2) {
        int32_t v = S16(st, p);
        if (v != 0 && S16(st, KINDS + 16u * (uint32_t) v) == 1) {
            return S16(st, KINDS + 2 + 16u * (uint32_t) v);
        }
    }
    return 4;
}

static int32_t mastaba_variant_53d930(game_state *st)
{
    /* @bin 0x0053D930 the first of two scenario slots of class 2 kind 4: its variant, else 0 */
    for (uint32_t p = SLOTS; p < SLOTS + 4; p += 2) {
        int32_t v = S16(st, p);
        if (v != 0 && S16(st, KINDS + 16u * (uint32_t) v) == 2 && S16(st, KINDS + 2 + 16u * (uint32_t) v) == 4) {
            return S16(st, KINDS + 4 + 16u * (uint32_t) v);
        }
    }
    return 0;
}

static int32_t find_kind(game_state *st, int32_t kind, int with_variant, int32_t variant)
{
    for (int32_t k = 0; k < kinds_count_426b30(); k++) {
        uint32_t e = KINDS + 16u * (uint32_t) k;
        if (S16(st, e) == 2 && S16(st, e + 2) == kind && (!with_variant || S16(st, e + 4) == variant)) {
            return k;
        }
    }
    return -1;
}

int32_t monument_kind_4f4b70(game_state *st, int32_t id)
{
    /* @bin 0x004F4B70 jump table 0x004F4F04 the monument's entry of 0x005D19C8, or -1 */
    uint32_t r = B(id);
    switch (S16(st, r + 0x10)) {
    case 0xB7:
        for (int32_t k = 0; k < kinds_count_426b30(); k++) {
            uint32_t e = KINDS + 16u * (uint32_t) k;
            if (S16(st, e) == 1 && S16(st, e + 2) == pyramid_kind_4f44e0(st) && U16(st, e + 4) == U8(st, r + 0x54)) {
                return k;
            }
        }
        return -1;
    case 0xCF: return find_kind(st, 2, 1, 0);
    case 0xD0: return find_kind(st, 2, 1, 1);
    case 0xD2: return find_kind(st, 1, 0, 0);
    case 0xD5: return find_kind(st, 3, 0, 0);
    case 0xDE:
        for (int32_t k = 0; k < kinds_count_426b30(); k++) {
            uint32_t e = KINDS + 16u * (uint32_t) k;
            if (S16(st, e) == 2 && S16(st, e + 2) == 4 && S16(st, e + 4) == mastaba_variant_53d930(st)) {
                return k;
            }
        }
        /* @bin 0x004F4E17 falls through to the case of 0xE5 */
        return find_kind(st, 0xA, 0, 0);
    case 0xE5: return find_kind(st, 0xA, 0, 0);
    case 0xE1: return find_kind(st, 6, 0, 0);
    case 0xE3: return find_kind(st, 7, 0, 0);
    case 0xE4: return find_kind(st, 5, 0, 0);
    case 0xE6: return find_kind(st, 0xE, 0, 0);
    case 0xEA: return find_kind(st, 0xB, 0, 0);
    case 0xEB: return find_kind(st, 0xC, 0, 0);
    case 0xEC: return find_kind(st, 0xD, 0, 0);
    default: return -1;
    }
}

static void slot_set_4f4a60(game_state *st, int32_t b, int32_t i)
{
    /* @bin 0x004F4A60 city +0x473C[i] := the monument's building */
    S32(st, CITY + 0x473C + 4u * (uint32_t) i) = b;
}

static int32_t slot_of_4f4aa0(game_state *st, int32_t b)
{
    /* @bin 0x004F4AA0 the slot (0-2) whose city +0x473C is b, or -1 */
    uint32_t c = CITY;
    for (int32_t i = 0; i < 3; i++) {
        if (S32(st, c + 0x473C + 4u * (uint32_t) i) == b) {
            return i;
        }
    }
    return -1;
}

int32_t monument_slot_4f4f80(game_state *st, int32_t id)
{
    /* @bin 0x004F4F80 a main part takes the first free scenario slot (0x00EAA978) of its kind;
     * returns the slot or -1 */
    if (S16(st, B(id) + 0x38) != 0) {
        return -1;
    }
    int32_t k = monument_kind_4f4b70(st, id);
    if (k < 0) {
        return -1;
    }
    for (int32_t i = 0; i < 3; i++) {
        if (S16(st, SLOTS + 2u * (uint32_t) i) == k && S32(st, 0x00EAA978u + 4u * (uint32_t) i) == 0) {
            S32(st, 0x00EAA978u + 4u * (uint32_t) i) = k;
            slot_set_4f4a60(st, id, i);
            return i;
        }
    }
    return -1;
}

/* ---- obelisks, mastabas, pyramids, sun temple ------------------------------------------------- */

static void obelisk_anims_4e5b90(game_state *st, int32_t id)
{
    /* @bin 0x004E5B90 the obelisk's animation objects restarted (10 of 0x00586B08 for 0xCF,
     * 18 of 0x0058FE48 otherwise) */
    uint32_t tp = B(id) + 0x10;
    for (int32_t k = 0;; k++) {
        int is_cf = U16(st, tp) == 0xCF;
        if (k >= (is_cf ? 10 : 18)) {
            break;
        }
        uint32_t e = (is_cf ? 0x00586B08u : 0x0058FE48u) + 0x10u * (uint32_t) k;
        P16(st, U32(st, e + 8) + 0x12) = 0;
        GAME_CALLT(st, U32(st, e + 8), 0x0041FCA0u, 3);
        P32(st, U32(st, e + 8) + 0xC) = U32(st, e + 0xC);
    }
}

int32_t obelisk_add_4e57f0(game_state *st, int32_t id)
{
    /* @bin 0x004E57F0 a new obelisk: its slot, +0x12 the slot, +0x55 = 0x21, its animations */
    monument_slot_4f4f80(st, id);
    S16(st, B(id) + 0x12) = (int16_t) slot_of_4f4aa0(st, id);
    U8(st, B(id) + 0x55) = 0x21;
    obelisk_anims_4e5b90(st, id);
    return 1;
}

void mastaba_add_4df9d0(game_state *st, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    /* @bin 0x004DF9D0 a mastaba-like monument of 0x74 parts laid out by 0x004DED50 (direction:
     * the view, or bits 13-15 of 0x00702798 when flag2) */
    int32_t slot = 0, dir;
    if ((uint8_t) flag2 == 0) {
        slot = monument_slot_4f4f80(st, id);
        dir = S32(st, ORIENT);
    } else {
        dir = (int32_t) ((U32(st, 0x00702798u) & 0xFFFF) >> 13);
    }
    int32_t lay[13];
    monument_part_layout_4ded50(st, lay, 0, S32(st, ORIENT), dir);
    uint32_t r = B(id);
    tile_of(st, r, lay[0] + x, lay[1] + y);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = (uint8_t) lay[2];
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 0;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    S32(st, r + 0x68) = x;
    S32(st, r + 0x6C) = y;
    S32(st, r + 0x70) = dir;
    GAME_CALL(st, 0x004DFBB0u, id);
    keep_removed(st, id);
    int32_t prev = id;
    for (int32_t k = 1; k < 0x74; k++) {
        monument_part_layout_4ded50(st, lay, k, S32(st, ORIENT), dir);
        int32_t b = create(st, PLAYER, 0xDE, lay[0] + x, lay[1] + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = (uint8_t) lay[2];
        S32(st, br + 0x54) = slot;
        S32(st, br + 0x58) = 0;
        S32(st, br + 0x60) = 0;
        S32(st, br + 0x5C) = 0;
        S32(st, br + 0x68) = x;
        S32(st, br + 0x6C) = y;
        S32(st, br + 0x70) = dir;
        GAME_CALL(st, 0x004DFBB0u, b);
        keep_removed(st, b);
        prev = b;
    }
}

static void pyramid_footprint_4ee6c0(game_state *st, int32_t id)
{
    /* @bin 0x004EE6C0 a pyramid part's square (offsets 0x005CC8C0, rows of 0x30 bytes): terrain
     * |= 0x10000008, building grid := the part */
    uint32_t r = B(id);
    int32_t x = S16(st, r + 6), y = S16(st, r + 8);
    int32_t s = U8(st, r + 3);
    if (x < 0 || y < 0 || s + x - 1 >= S32(st, MAP_W) || s + y - 1 >= S32(st, MAP_H)) {
        return;
    }
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    S32(st, CURSOR) = base;
    for (int32_t row = 0; row < s; row++) {
        for (int32_t c = 0; c < s; c++) {
            int32_t t = S32(st, 0x005CC8C0u + 0x30u * (uint32_t) row + 8u * (uint32_t) c) + base;
            U32(st, TERRAIN + 4u * (uint32_t) t) |= 0x10000008u;
            S16(st, BUILDING_GRID + 2u * (uint32_t) t) = (int16_t) id;
        }
    }
}

static void pyramid_part_fields(game_state *st, uint32_t r, int32_t v, int32_t big, int32_t k)
{
    /* @bin 0x004EE7A0 0x004EE933 */
    U8(st, r + 0x54) = (uint8_t) v;
    U8(st, r + 0x55) = 2;
    U8(st, r + 0x56) = 0;
    S16(st, r + 0x64) = 0;
    U8(st, r + 0x57) = 0;
    U8(st, r + 0x58) = big ? U8(st, 0x005E657Cu + 8u * (uint32_t) (k + v * 0x24))
                           : U8(st, 0x005E4EFCu + 8u * (uint32_t) (v * 0x90 + k));
    U8(st, r + 0x59) = 0;
    U8(st, r + 0x5A) = 0;
    U8(st, r + 0x5B) = 0xF;
    S16(st, r + 0x5C) = 0;
    S16(st, r + 0x5E) = 0;
    S16(st, r + 0x60) = 0;
    S16(st, r + 0x62) = 0;
    S32(st, r + 0x68) = 0;
}

static void pyramid_stages_4f3c90(game_state *st, int32_t id)
{
    /* @bin 0x004F3C90 the pyramid's parts of its kind (+0x54) listed in 0x005F6CD0 / 0x005F6D58
     * (by 0x005E4EF4 == 5) drawn at stage 3 (0x004EDF50); then its ramp start (0x005F5D08 offset
     * from its tile) walked by 0x004ED360 when 0x004ECEA0 allows the kind */
    uint32_t r = B(id);
    int32_t v = U8(st, r + 0x54);
    int32_t o2 = S32(st, ORIENT) / 2;
    uint32_t tbl = S32(st, 0x005E4EF4u) == 5 ? 0x005F6D58u : 0x005F6CD0u;
    for (int32_t i = 0; i < (int32_t) P32(st, tbl + 8u * (uint32_t) v + 4); i++) {
        int32_t part = (int32_t) P32(st, P32(st, tbl + 8u * (uint32_t) v) + 8u * (uint32_t) i) + id;
        U8(st, B(part) + 0x55) = 3;
        U8(st, B(part) + 0x5B) = 0;
        GAME_CALL(st, 0x004EDF50u, part, o2, v);
    }
    if ((uint8_t) GAME_CALL(st, 0x004ECEA0u, v) == 0) {
        return;
    }
    int32_t off = S32(st, 0x005F5D08u + 4u * (uint32_t) v) + S32(st, r + 0xC) - S32(st, MAP_START);
    GAME_CALL(st, 0x004ED360u, off % GRID_SIZE, off / GRID_SIZE, 2, 0);
}

void pyramid_ramp_4f5f10(game_state *st, int32_t id)
{
    /* @bin 0x004F5F10 a pyramid's ramp: a building of type 0xD7 at the kind's offset
     * (0x005700D4 / 0x005700E8) from the main part's tile, size 0x005D17A8, +0x12 the pyramid */
    if (U16(st, B(id) + 0x10) != 0xB7) {
        return;
    }
    int32_t main = id - S16(st, B(id) + 0x12);
    uint32_t r = B(main);
    uint32_t v = U8(st, r + 0x54);
    int32_t d = S32(st, 0x005E4EF4u) == 5 ? S32(st, 0x005700E8u + 4u * v) : S32(st, 0x005700D4u + 4u * v);
    int32_t off = S32(st, r + 0xC) + d;
    int32_t x = off % GRID_SIZE - S32(st, 0x00BCA2E8u);
    int32_t y = off / GRID_SIZE - S32(st, 0x00BCA2ECu);
    int32_t size = S32(st, 0x005D17A8u);
    int32_t b = create(st, PLAYER, 0xD7, x, y, 0, 0);
    keep_removed(st, b);
    building_map_add(st, PLAYER, 0xD7, b, x, y, size, 0, 0);
    S16(st, B(b) + 0x12) = (int16_t) main;
}

void pyramid_add_4ee780(game_state *st, int32_t player, int32_t x, int32_t y, int32_t size, int32_t id)
{
    /* @bin 0x004EE780 a new pyramid of kind 0x005E6970: its parts at the offsets 0x0056F898 /
     * 0x0056FD18 (count 0x0056F7D8 / 0x0056F7EC; 0x005E4EF4 == 5 the second tables), each one's
     * square (0x004EE6C0); jump table 0x004EE9FC for +0xC8 */
    (void) size;
    uint32_t r = B(id);
    int32_t v = S32(st, 0x005E6970u);
    int32_t big = S32(st, 0x005E4EF4u);
    S16(st, r + 0x38) = 0;
    pyramid_part_fields(st, r, v, big == 5, 0);
    int32_t w;
    switch (big) {
    case 1: w = S32(st, 0x005F5C68u + 4u * (uint32_t) v); break;
    case 2: case 4: w = S32(st, 0x005F5C40u + 4u * (uint32_t) v); break;
    case 3: w = S32(st, 0x005F5C54u + 4u * (uint32_t) v); break;
    case 5: w = S32(st, 0x005F5C7Cu + 4u * (uint32_t) v); break;
    default: w = 0; break;
    }
    S16(st, r + 0xC8) = (int16_t) w;
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = 2;
    pyramid_footprint_4ee6c0(st, id);
    monument_slot_4f4f80(st, id);
    int32_t prev = id;
    for (int32_t k = 1;; k++) {
        int five = S32(st, 0x005E4EF4u) == 5;
        int32_t vv = S32(st, 0x005E6970u);
        int32_t n = five ? S32(st, 0x0056F7ECu + 4u * (uint32_t) vv) : S32(st, 0x0056F7D8u + 4u * (uint32_t) vv);
        if (k >= n) {
            break;
        }
        uint32_t o = 8u * (uint32_t) (k - 1);
        int32_t dx = five ? S32(st, 0x0056FD18u + o) : S32(st, 0x0056F898u + o);
        int32_t dy = five ? S32(st, 0x0056FD1Cu + o) : S32(st, 0x0056F89Cu + o);
        int32_t b = create(st, player, 0xB7, dx + x, dy + y, 0, prev + 1);
        keep_removed(st, b);
        uint32_t br = B(b);
        five = S32(st, 0x005E4EF4u) == 5;
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = 2;
        pyramid_part_fields(st, br, S32(st, 0x005E6970u), five, k);
        pyramid_footprint_4ee6c0(st, b);
        prev = b;
    }
    int32_t main = prev - S16(st, B(prev) + 0x12);
    pyramid_stages_4f3c90(st, main);
    pyramid_ramp_4f5f10(st, main);
}

void sun_temple_add_5417b0(game_state *st, int32_t x, int32_t y, int32_t id)
{
    /* @bin 0x005417B0 the sun temple's 100 parts (table 0x00601DD8, 0x34 bytes: x, y, size),
     * each drawn by 0x00541920 */
    int32_t slot = monument_slot_4f4f80(st, id);
    uint32_t r = B(id);
    int32_t dx = S32(st, 0x00601DD8u), dy = S32(st, 0x00601DDCu);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = U8(st, 0x00601DE0u);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 0;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    GAME_CALL(st, 0x00541920u, id);
    keep_removed(st, id);
    int32_t prev = id;
    int32_t k = 1;
    for (uint32_t e = 0x00601E10u; e < 0x0060322Cu; e += 0x34, k++) {
        int32_t b = create(st, PLAYER, 0xD5, S32(st, e - 4) + x, S32(st, e) + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = U8(st, e + 4);
        S32(st, br + 0x54) = slot;
        S32(st, br + 0x58) = 0;
        S32(st, br + 0x60) = 0;
        S32(st, br + 0x5C) = 0;
        GAME_CALL(st, 0x00541920u, b);
        keep_removed(st, b);
        prev = b;
    }
}

/* ---- the Cleopatra monuments' objects (thiscall, vtable at +0) -------------------------------- */

static void entry_401af0(game_state *st, int32_t id, int32_t *x, int32_t *y)
{
    /* @bin 0x00401AF0 as core/building/day_road.c ports it, into locals (the original writes its
     * caller's frame): the first part's corner + (6, -8) */
    uint32_t base = B(id - S16(st, B(id) + 0x12));
    *x = S16(st, base + 6) + 6;
    *y = S16(st, base + 8) - 8;
}

static void entry_40eb90(game_state *st, uint32_t self, int32_t id, int32_t *x, int32_t *y)
{
    /* @bin 0x0040EB90 as core/building/day_road.c ports it, into locals: before stage 3 the
     * part's corner plus half its size (the object's +0x54, 13 words a part), then the main
     * part's corner + (3, 11) */
    uint32_t b = B(id);
    if (S32(st, b + 0x58) < 3) {
        int32_t half = (int32_t) P32(st, self + 0x54 + 4u * (uint32_t) (13 * S16(st, b + 0x12))) / 2;
        *x = S16(st, b + 6) + half;
        *y = S16(st, b + 8) + half;
        return;
    }
    uint32_t m = B(id - S16(st, b + 0x12));
    *x = S16(st, m + 6) + 3;
    *y = S16(st, m + 8) + 0xB;
}

static void entry(game_state *st, uint32_t self, int32_t id, int32_t *x, int32_t *y)
{
    /* the object's method +0x20 (id, &x, &y): its entry tile */
    uint32_t fn = P32(st, P32(st, self) + 0x20);
    switch (fn) {
    case 0x00401AF0u: entry_401af0(st, id, x, y); break;
    case 0x0040EB90u: entry_40eb90(st, self, id, x, y); break;
    case 0x00545A30u: figb_tile_545a30(st, id, x, y); break;
    case 0x00558F00u: figb_tile_558f00(st, id, x, y); break;
    case 0x00556230u: figb_tile_556230(st, id, x, y); break;
    case 0x0054F8A0u: figb_tile_54f8a0(st, id, x, y); break;
    default: state_unported(st, fn); break;
    }
}

static void redraw(game_state *st, uint32_t self, int32_t id)
{
    /* the object's method +0x18 (id): the part drawn */
    GAME_CALLT(st, self, P32(st, P32(st, self) + 0x18), id);
}

static void zero_61(game_state *st, uint32_t r)
{
    /* @bin 0x0040166D three dwords at +0x61 (unaligned) */
    for (uint32_t i = 0; i < 12; i++) {
        U8(st, r + 0x61 + i) = 0;
    }
}

void monument_add_4015a0(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    /* @bin 0x004015A0 (thiscall on 0x006DE650) 16 parts (the object's +0x4C, 0x34 bytes: x, y,
     * size), each with its entry tile +0x26 / +0x28 */
    if ((uint8_t) flag2 == 0) {
        monument_slot_4f4f80(st, id);
    }
    int32_t dx = (int32_t) P32(st, self + 0x4C), dy = (int32_t) P32(st, self + 0x50);
    uint32_t r = B(id);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = P8(st, self + 0x54);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = 0;
    S32(st, r + 0x5C) = 0;
    S32(st, r + 0x58) = 0;
    int32_t ex = dx, ey = dy;
    entry(st, self, id, &ex, &ey);
    S16(st, r + 0x26) = (int16_t) ex;
    S16(st, r + 0x28) = (int16_t) ey;
    zero_61(st, r);
    redraw(st, self, id);
    keep_removed(st, id);
    P32(st, self + 8) = 0;
    int32_t prev = id;
    for (int32_t k = 1; k < 0x10; k++) {
        uint32_t e = self + 0x84 + 0x34u * (uint32_t) (k - 1);
        dx = (int32_t) P32(st, e - 4);
        dy = (int32_t) P32(st, e);
        int32_t b = create(st, PLAYER, 0xE6, dx + x, dy + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = P8(st, e + 4);
        S32(st, br + 0x54) = 0;
        S32(st, br + 0x5C) = 0;
        S32(st, br + 0x58) = 0;
        ex = dx, ey = dy;
        entry(st, self, b, &ex, &ey);
        S16(st, br + 0x26) = (int16_t) ex;
        S16(st, br + 0x28) = (int16_t) ey;
        zero_61(st, br);
        redraw(st, self, b);
        keep_removed(st, b);
        prev = b;
    }
    GAME_CALL0(st, 0x004DEA50u);
}

void monument_add_40da90(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    /* @bin 0x0040DA90 (thiscall on 0x006FD5D8) 25 parts, each with its entry tile */
    int32_t slot = 0;
    if ((uint8_t) flag2 == 0) {
        slot = monument_slot_4f4f80(st, id);
    }
    int32_t dx = (int32_t) P32(st, self + 0x4C), dy = (int32_t) P32(st, self + 0x50);
    uint32_t r = B(id);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = P8(st, self + 0x54);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 1;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    int32_t ex = dx, ey = dy;
    entry(st, self, id, &ex, &ey);
    S16(st, r + 0x26) = (int16_t) ex;
    S16(st, r + 0x28) = (int16_t) ey;
    redraw(st, self, id);
    keep_removed(st, id);
    P32(st, self + 8) = 0;
    int32_t prev = id;
    for (int32_t k = 1; k < 0x19; k++) {
        uint32_t e = self + 0x84 + 0x34u * (uint32_t) (k - 1);
        dx = (int32_t) P32(st, e - 4);
        dy = (int32_t) P32(st, e);
        int32_t b = create(st, PLAYER, 0xE1, dx + x, dy + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = P8(st, e + 4);
        S32(st, br + 0x54) = slot;
        S32(st, br + 0x58) = 1;
        S32(st, br + 0x60) = 1;
        S32(st, br + 0x5C) = 0;
        ex = dx, ey = dy;
        entry(st, self, b, &ex, &ey);
        S16(st, br + 0x26) = (int16_t) ex;
        S16(st, br + 0x28) = (int16_t) ey;
        redraw(st, self, b);
        keep_removed(st, b);
        prev = b;
    }
}

void monument_add_417180(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    /* @bin 0x00417180 (thiscall on 0x006FD368) 9 parts */
    int32_t slot = 0;
    if ((uint8_t) flag2 == 0) {
        slot = monument_slot_4f4f80(st, id);
    }
    int32_t dx = (int32_t) P32(st, self + 0x4C), dy = (int32_t) P32(st, self + 0x50);
    uint32_t r = B(id);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = P8(st, self + 0x54);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 0;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    redraw(st, self, id);
    keep_removed(st, id);
    P32(st, self + 8) = 0;
    int32_t prev = id;
    for (int32_t k = 1; k < 9; k++) {
        uint32_t e = self + 0x84 + 0x34u * (uint32_t) (k - 1);
        int32_t b = create(st, PLAYER, 0xE3, (int32_t) P32(st, e - 4) + x, (int32_t) P32(st, e) + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = (int16_t) k;
        U8(st, br + 3) = P8(st, e + 4);
        S32(st, br + 0x54) = slot;
        S32(st, br + 0x58) = 0;
        S32(st, br + 0x60) = 1;
        S32(st, br + 0x5C) = 0;
        redraw(st, self, b);
        keep_removed(st, b);
        prev = b;
    }
}

void monument_add_4e6840(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    /* @bin 0x004E6840 (thiscall on 0x006DE548) the main part and one more (+0x80) */
    int32_t slot = 0;
    if ((uint8_t) flag2 == 0) {
        slot = monument_slot_4f4f80(st, id);
    }
    int32_t dx = (int32_t) P32(st, self + 0x4C), dy = (int32_t) P32(st, self + 0x50);
    uint32_t r = B(id);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = 0;
    U8(st, r + 3) = P8(st, self + 0x54);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 0;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    keep_removed(st, id);
    redraw(st, self, id);
    int32_t b = create(st, PLAYER, 0xE4, (int32_t) P32(st, self + 0x80) + x, (int32_t) P32(st, self + 0x84) + y, 0, id + 1);
    uint32_t br = B(b);
    link_part(st, id, b);
    S16(st, br + 0x12) = P16(st, self + 0x8C);
    U8(st, br + 3) = P8(st, self + 0x88);
    S32(st, br + 0x54) = slot;
    S32(st, br + 0x58) = 0;
    S32(st, br + 0x60) = 0;
    S32(st, br + 0x5C) = 0;
    keep_removed(st, b);
    redraw(st, self, b);
    P32(st, self + 8) = 0;
    GAME_CALL0(st, 0x004DEA50u);
}

typedef struct {
    uint32_t fn;
    int32_t type, n;
    uint32_t pos, f90, f68, f94, f9c, fd, fc; /* offsets in the object */
} tomb_layout;

/* @bin 0x00545C30 0x0054FBA0 0x00556480 0x00559130 the objects' tables: parts at `pos` (0x34
 * bytes: x, y, size, +0xC i16 part number), per part dwords +0x90 / +0x68, bytes +0x94 / +0x9C,
 * 6 dwords for +0x6C, 3 dwords for +0x84 */
static const tomb_layout tombs[] = {
    {0x00545C30u, 0xE5, 0xA7, 0x1E5C, 0x58, 0x2F4, 0x1D0C, 0x1DB3, 0x590, 0x1538},
    {0x00559130u, 0xEA, 0x9E, 0x1CBC, 0x58, 0x2D0, 0x1B80, 0x1C1E, 0x548, 0x1418},
    {0x00556480u, 0xEB, 0x1B5, 0x4EE0, 0x58, 0x72C, 0x4B74, 0x4D29, 0xE00, 0x36F8},
    {0x0054FBA0u, 0xEC, 0x201, 0x5C88, 0x58, 0x85C, 0x5884, 0x5A85, 0x1060, 0x4078},
};

static void tomb_fields(game_state *st, const tomb_layout *t, uint32_t self, uint32_t r, int32_t k)
{
    S32(st, r + 0x90) = (int32_t) P32(st, self + t->f90 + 4u * (uint32_t) k);
    S32(st, r + 0x68) = (int32_t) P32(st, self + t->f68 + 4u * (uint32_t) k);
    U8(st, r + 0x94) = P8(st, self + t->f94 + (uint32_t) k);
    S32(st, r + 0x98) = 0;
    U8(st, r + 0x9C) = P8(st, self + t->f9c + (uint32_t) k);
    for (uint32_t i = 0; i < 6; i++) {
        S32(st, r + 0x6C + 4 * i) = (int32_t) P32(st, self + t->fd + 24u * (uint32_t) k + 4 * i);
    }
    for (uint32_t i = 0; i < 3; i++) {
        S32(st, r + 0x84 + 4 * i) = (int32_t) P32(st, self + t->fc + 12u * (uint32_t) k + 4 * i);
    }
}

void tomb_add(game_state *st, uint32_t fn, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2)
{
    const tomb_layout *t = NULL;
    for (unsigned i = 0; i < sizeof(tombs) / sizeof(tombs[0]); i++) {
        if (tombs[i].fn == fn) {
            t = &tombs[i];
        }
    }
    if (!t) {
        state_unported(st, fn);
        return;
    }
    int32_t slot = 0;
    if ((uint8_t) flag2 == 0) {
        slot = monument_slot_4f4f80(st, id);
    }
    uint32_t e = self + t->pos;
    int32_t dx = (int32_t) P32(st, e), dy = (int32_t) P32(st, e + 4);
    P8(st, self + 0x4C) = 0;
    P32(st, self + 0x54) = 1;
    uint32_t r = B(id);
    tile_of(st, r, x + dx, y + dy);
    S16(st, r + 0x12) = P16(st, e + 0xC);
    U8(st, r + 3) = P8(st, e + 8);
    S16(st, r + 0x38) = 0;
    S32(st, r + 0x54) = slot;
    S32(st, r + 0x58) = 0;
    S32(st, r + 0x60) = 1;
    S32(st, r + 0x5C) = 0;
    S32(st, r + 0x64) = 1;
    tomb_fields(st, t, self, r, 0);
    keep_removed(st, id);
    P32(st, self + 8) = 0;
    int32_t prev = id;
    for (int32_t k = 1; k < t->n; k++) {
        e = self + t->pos + 0x34u * (uint32_t) k;
        int32_t b = create(st, PLAYER, t->type, (int32_t) P32(st, e) + x, (int32_t) P32(st, e + 4) + y, 0, prev + 1);
        uint32_t br = B(b);
        link_part(st, prev, b);
        S16(st, br + 0x12) = P16(st, e + 0xC);
        U8(st, br + 3) = P8(st, e + 8);
        S32(st, br + 0x54) = slot;
        S32(st, br + 0x58) = 0;
        S32(st, br + 0x60) = 0;
        S32(st, br + 0x5C) = 0;
        S32(st, br + 0x64) = 0;
        tomb_fields(st, t, self, br, k);
        keep_removed(st, b);
        prev = b;
    }
    for (int32_t j = 0; j < t->n; j++) {
        redraw(st, self, id + j);
    }
    GAME_CALL0(st, 0x004DEA50u);
}

/* ---- the table -------------------------------------------------------------------------------- */


static int32_t run_slot(game_state *st, const int32_t *a) { return monument_slot_4f4f80(st, a[0]); }
static int32_t run_kind(game_state *st, const int32_t *a) { return monument_kind_4f4b70(st, a[0]); }
static int32_t run_kinds_count(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return kinds_count_426b30();
}
static int32_t run_pyramid_kind(game_state *st, const int32_t *a)
{
    (void) a;
    return pyramid_kind_4f44e0(st);
}
static int32_t run_mastaba_variant(game_state *st, const int32_t *a)
{
    (void) a;
    return mastaba_variant_53d930(st);
}
static int32_t run_slot_set(game_state *st, const int32_t *a)
{
    slot_set_4f4a60(st, a[0], a[1]);
    return 0;
}
static int32_t run_slot_of(game_state *st, const int32_t *a) { return slot_of_4f4aa0(st, a[0]); }
static int32_t run_obelisk_anims(game_state *st, const int32_t *a)
{
    obelisk_anims_4e5b90(st, a[0]);
    return 0;
}
static int32_t run_obelisk_add(game_state *st, const int32_t *a) { return obelisk_add_4e57f0(st, a[0]); }
static int32_t run_mastaba_add(game_state *st, const int32_t *a)
{
    mastaba_add_4df9d0(st, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_footprint(game_state *st, const int32_t *a)
{
    pyramid_footprint_4ee6c0(st, a[0]);
    return 0;
}
static int32_t run_stages(game_state *st, const int32_t *a)
{
    pyramid_stages_4f3c90(st, a[0]);
    return 0;
}
static int32_t run_ramp(game_state *st, const int32_t *a)
{
    pyramid_ramp_4f5f10(st, a[0]);
    return 0;
}
static int32_t run_pyramid_add(game_state *st, const int32_t *a)
{
    pyramid_add_4ee780(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}
static int32_t run_sun_temple(game_state *st, const int32_t *a)
{
    sun_temple_add_5417b0(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_4015a0(game_state *st, const int32_t *a)
{
    monument_add_4015a0(st, 0x006DE650u, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_40da90(game_state *st, const int32_t *a)
{
    monument_add_40da90(st, 0x006FD5D8u, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_417180(game_state *st, const int32_t *a)
{
    monument_add_417180(st, 0x006FD368u, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_4e6840(game_state *st, const int32_t *a)
{
    monument_add_4e6840(st, 0x006DE548u, a[0], a[1], a[2], a[3]);
    return 0;
}
#define TOMB(name, fn, self) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        tomb_add(st, fn, self, a[0], a[1], a[2], a[3]); \
        return 0; \
    }
TOMB(run_545c30, 0x00545C30u, 0x006F9320u)
TOMB(run_559130, 0x00559130u, 0x006F5648u)
TOMB(run_556480, 0x00556480u, 0x006DE9E0u)
TOMB(run_54fba0, 0x0054FBA0u, 0x006E9188u)

/* @bin 0x00545F60 0x00559460 0x005567B0 0x0054FED0 (thiscall, the tombs' method +0x38) constants 9, 10,
 * 0x15, 0x1F */
static int32_t run_tomb_kinds_e5(game_state *st, const int32_t *a) { (void) st; (void) a; return 9; }
static int32_t run_tomb_kinds_ea(game_state *st, const int32_t *a) { (void) st; (void) a; return 0xA; }
static int32_t run_tomb_kinds_eb(game_state *st, const int32_t *a) { (void) st; (void) a; return 0x15; }
static int32_t run_tomb_kinds_ec(game_state *st, const int32_t *a) { (void) st; (void) a; return 0x1F; }

const ported_function map_add_monument_ports[] = {
    {0x00545F60u, "tomb_kinds_545f60", 0, 32, 0x006F9320u, run_tomb_kinds_e5, 0},
    {0x00559460u, "tomb_kinds_559460", 0, 32, 0x006F5648u, run_tomb_kinds_ea, 0},
    {0x005567B0u, "tomb_kinds_5567b0", 0, 32, 0x006DE9E0u, run_tomb_kinds_eb, 0},
    {0x0054FED0u, "tomb_kinds_54fed0", 0, 32, 0x006E9188u, run_tomb_kinds_ec, 0},
    {0x004F4F80u, "monument_slot_4f4f80", 1, 32, 0, run_slot, 0},
    {0x004F4B70u, "monument_kind_4f4b70", 1, 32, 0, run_kind, 0},
    {0x00426B30u, "kinds_count_426b30", 0, 32, 0, run_kinds_count, 0},
    {0x004F44E0u, "pyramid_kind_4f44e0", 0, 32, 0, run_pyramid_kind, 0},
    {0x0053D930u, "mastaba_variant_53d930", 0, 32, 0, run_mastaba_variant, 0},
    {0x004F4A60u, "slot_set_4f4a60", 2, 0, 0, run_slot_set, 0},
    {0x004F4AA0u, "slot_of_4f4aa0", 1, 32, 0, run_slot_of, 0},
    {0x004E5B90u, "obelisk_anims_4e5b90", 1, 0, 0, run_obelisk_anims, 0},
    {0x004E57F0u, "obelisk_add_4e57f0", 1, 8, 0, run_obelisk_add, 0},
    {0x004DF9D0u, "mastaba_add_4df9d0", 4, 0, 0, run_mastaba_add, 0},
    {0x004EE6C0u, "pyramid_footprint_4ee6c0", 1, 0, 0, run_footprint, 0},
    {0x004F3C90u, "pyramid_stages_4f3c90", 1, 0, 0, run_stages, 0},
    {0x004F5F10u, "pyramid_ramp_4f5f10", 1, 0, 0, run_ramp, 0},
    {0x004EE780u, "pyramid_add_4ee780", 5, 0, 0, run_pyramid_add, 0},
    {0x005417B0u, "sun_temple_add_5417b0", 3, 0, 0, run_sun_temple, 0},
    {0x004015A0u, "monument_add_4015a0", 4, 0, 0x006DE650u, run_4015a0, 0},
    {0x0040DA90u, "monument_add_40da90", 4, 0, 0x006FD5D8u, run_40da90, 0},
    {0x00417180u, "monument_add_417180", 4, 0, 0x006FD368u, run_417180, 0},
    {0x004E6840u, "monument_add_4e6840", 4, 0, 0x006DE548u, run_4e6840, 0},
    {0x00545C30u, "tomb_add_545c30", 4, 0, 0x006F9320u, run_545c30, 0},
    {0x00559130u, "tomb_add_559130", 4, 0, 0x006F5648u, run_559130, 0},
    {0x00556480u, "tomb_add_556480", 4, 0, 0x006DE9E0u, run_556480, 0},
    {0x0054FBA0u, "tomb_add_54fba0", 4, 0, 0x006E9188u, run_54fba0, 0},
};
const int map_add_monument_ports_count = sizeof(map_add_monument_ports) / sizeof(map_add_monument_ports[0]);
