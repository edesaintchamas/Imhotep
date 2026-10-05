/* Imhotep - figure helpers left over by the earlier packages (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21); the original addresses its fields from
 * 0x00BCD600 = FIGURE_ADDR(0) + 8, the fields below are named by FIGURE_ADDR offsets. */
#include "figure/last_figure.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define PLAYER 0x00EAAC28u
#define FIG(id, off) (FIGURE_ADDR(id) + (off))

int32_t figure_unload_to_works(game_state *st, int32_t building, int32_t good, int32_t route, int32_t figure)
{
    /* @bin 0x00445E50 (building, good, route, figure) a building of type 0x48 (word +0x10) takes
     * loads of `good` from the figure: 0x0045D510(building, good) (signed) >= 100 and the figure's
     * load count (byte +0x6F) < 12: n = stock / 100, 4 from 400 on; limited by the current city's
     * amount of the good (+0x48F0 + 4 good) above its keep level: word +0x286E + 2 good * 100
     * when its trade mode (word +0x2826 + 2 good) is 2, else 0x00775810 + 4 good. Each load:
     * count + 1, 0x0045B570(player, building, good, 100), the good's value (0x005D2C4C + 8 good;
     * 150 % through 0x004CDE40 when the city's +0x45CC > 0) added to the city's +0x0C and
     * +0x2C64; stops at 12 loads, else the route count 0x0077592C [route byte (0x0077DE04 +
     * 106 route) * 36 + good] + 1 and 0x00444BD0(figure, good). Returns 1 when n != 0, else 0. */
    if (S16(st, BUILDING_ADDR(building) + 0x10) != 0x48) {
        return 0;
    }
    int32_t stock = GAME_CALL(st, 0x0045D510u, building, good);
    uint32_t count = FIG(figure, 0x6F);
    if (U8(st, count) >= 0xC || stock < 100) {
        return 0;
    }
    int32_t n = stock < 400 ? stock / 100 : 4;
    uint32_t p = U32(st, PLAYER);
    int32_t amount = S32(st, 0x00EA6160u + 4u * (p * 0x1276u + (uint32_t) good));
    if (S16(st, 0x00EA4096u + 2u * (p * 0x24ECu + (uint32_t) good)) == 2) {
        int32_t keep = (int32_t) S16(st, 0x00EA40DEu + 2u * (p * 0x24ECu + (uint32_t) good)) * 100;
        if (amount - n * 100 < keep) {
            n = (amount - keep) / 100;
        }
    } else {
        int32_t keep = S32(st, 0x00775810u + 4u * (uint32_t) good);
        if (amount - n * 100 < keep) {
            n = (amount - keep) / 100;
        }
    }
    if (n == 0) {
        return 0;
    }
    for (int32_t i = 0; i < n; i++) {
        U8(st, count) += 1;
        GAME_CALL(st, 0x0045B570u, S32(st, PLAYER), building, good, 100);
        uint32_t city = CITY_ADDR(U32(st, PLAYER));
        if (S32(st, city + 0x45CC) > 0) {
            int32_t v = GAME_CALL(st, 0x004CDE40u, S32(st, 0x005D2C4Cu + 8u * (uint32_t) good), 0x96);
            S32(st, city + 0x0C) += v;
            city = CITY_ADDR(U32(st, PLAYER));
            v = GAME_CALL(st, 0x004CDE40u, S32(st, 0x005D2C4Cu + 8u * (uint32_t) good), 0x96);
            S32(st, city + 0x2C64) += v;
        } else {
            int32_t v = S32(st, 0x005D2C4Cu + 8u * (uint32_t) good);
            S32(st, city + 0x0C) += v;
            S32(st, city + 0x2C64) += v;
        }
        if (U8(st, count) >= 0xC) {
            break;
        }
        uint32_t r = U8(st, 0x0077DE04u + 106u * (uint32_t) route);
        S32(st, 0x0077592Cu + 4u * ((uint32_t) good + 36u * r)) += 1;
        GAME_CALL(st, 0x00444BD0u, figure, good);
    }
    return 1;
}

int32_t building_nearest_0x458c30(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00458C30 (x, y) the building (1 .. 0x0078D790, re-read each step) in use (byte +0 1)
     * with byte +5 set, words +0x1E > 0 and +0x20 > 0 and +0x2E == 0, at the least Chebyshev
     * distance from (x, y) (its words +6, +8; |dx| -> 0x00D3B4FC, |dy| -> 0x00D3B4F8): the first
     * such, else 0 */
    int32_t best = 1000, found = 0;
    for (int32_t id = 1; id <= S32(st, 0x0078D790u); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || U8(st, b + 5) == 0 || S16(st, b + 0x20) <= 0 || S16(st, b + 0x1E) <= 0 ||
            S16(st, b + 0x2E) != 0) {
            continue;
        }
        int32_t bx = S16(st, b + 6), by = S16(st, b + 8);
        int32_t dx = x > bx ? x - bx : (x < bx ? bx - x : 0);
        S32(st, 0x00D3B4FCu) = dx;
        int32_t dy = y > by ? y - by : (y < by ? by - y : 0);
        S32(st, 0x00D3B4F8u) = dy;
        int32_t d = dx >= dy ? dx : dy;
        if (d < best) {
            best = d;
            found = id;
        }
    }
    return found;
}

void figure_hit_target(game_state *st, int32_t attacker, int32_t target)
{
    /* @bin 0x004BC4A0 (attacker, target) a target not of the current player (byte +0x0F): its
     * damage (byte +0x1D) grows by the attacker's type attack (0x007090D4 + 56 type; x4 while the
     * attacker's +0x134 is 6 .. 19) * ((30 - defence 0x007090D8 + 56 type) / 20), 0 when negative;
     * at most its type's hit points (0x007090D0 + 56 type): 0x004B5210(attacker); else killed:
     * unless its type is an enemy (0x004EA9C0), an animal (0x004EA9A0) or 0x17, an attacker of
     * the current player raises its formation's (word +0x60) word 0x00BCBA42 + 0x90 f by 1, at
     * most 100; byte +0x38 := 4, word +0x36 := 0, 0x004B5050(target), 0x004B7AB0(target's
     * formation) */
    int32_t fa = S16(st, FIG(attacker, 0x60));
    int32_t ft = S16(st, FIG(target, 0x60));
    if (attacker == target) {
        return;
    }
    if ((uint32_t) U8(st, FIG(target, 0x0F)) == U32(st, PLAYER)) {
        return;
    }
    uint32_t ta = U8(st, FIG(attacker, 0x0A));
    int32_t attack = S32(st, 0x007090D4u + 56u * ta);
    int32_t defence = S32(st, 0x007090D8u + 56u * ta);
    int32_t k = S32(st, FIG(attacker, 0x134));
    if (k > 5 && k < 0x14) {
        attack = (int32_t) ((uint32_t) attack << 2);
    }
    int32_t dmg = 0;
    if (attack != 0) {
        dmg = (int32_t) ((uint32_t) ((30 - defence) / 20) * (uint32_t) attack);
        if (dmg < 0) {
            dmg = 0;
        }
    }
    uint8_t hp = (uint8_t) (U8(st, FIG(target, 0x1D)) + (uint8_t) dmg);
    U8(st, FIG(target, 0x1D)) = hp;
    if ((int32_t) hp <= S32(st, 0x007090D0u + 56u * (uint32_t) U8(st, FIG(target, 0x0A)))) {
        GAME_CALL(st, 0x004B5210u, attacker);
        return;
    }
    if ((uint8_t) GAME_CALL(st, 0x004EA9C0u, (int32_t) U8(st, FIG(target, 0x0A))) == 0 &&
        (uint8_t) GAME_CALL(st, 0x004EA9A0u, (int32_t) U8(st, FIG(target, 0x0A))) == 0 &&
        U8(st, FIG(target, 0x0A)) != 0x17 && (uint32_t) U8(st, FIG(attacker, 0x0F)) == U32(st, PLAYER)) {
        uint32_t f = 0x00BCBA42u + 0x90u * (uint32_t) fa;
        int32_t v = S16(st, f) + 1;
        if (v >= 100) {
            v = 100;
        }
        S16(st, f) = (int16_t) v;
    }
    U8(st, FIG(target, 0x38)) = 4;
    S16(st, FIG(target, 0x36)) = 0;
    GAME_CALL(st, 0x004B5050u, target);
    GAME_CALL(st, 0x004B7AB0u, ft);
}

int32_t city_list_index_0x480860(game_state *st, int32_t value)
{
    /* @bin 0x00480860 (value) when 0x00705C1C > 0: the index of `value` among the 10 words at
     * the current city's +0x4460, else -1 */
    if (S32(st, 0x00705C1Cu) <= 0) {
        return -1;
    }
    uint32_t base = CITY_ADDR(U32(st, PLAYER)) + 0x4460u;
    for (int32_t i = 0; i < 10; i++) {
        if (value == S16(st, base + 2u * (uint32_t) i)) {
            return i;
        }
    }
    return -1;
}

static int32_t run_unload(game_state *st, const int32_t *a)
{
    return figure_unload_to_works(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_nearest(game_state *st, const int32_t *a)
{
    return building_nearest_0x458c30(st, a[0], a[1]);
}

static int32_t run_hit(game_state *st, const int32_t *a)
{
    figure_hit_target(st, a[0], a[1]);
    return 0;
}

static int32_t run_index(game_state *st, const int32_t *a)
{
    return city_list_index_0x480860(st, a[0]);
}

const ported_function last_figure_ports[] = {
    {0x00445E50u, "figure_unload_to_works", 4, 32, 0, run_unload, 0},
    {0x00458C30u, "building_nearest_0x458c30", 2, 32, 0, run_nearest, 0},
    {0x004BC4A0u, "figure_hit_target", 2, 0, 0, run_hit, 0},
    {0x00480860u, "city_list_index_0x480860", 1, 32, 0, run_index, 0},
};
const int last_figure_ports_count = sizeof(last_figure_ports) / sizeof(last_figure_ports[0]);
