/* Imhotep - the troops of an invasion or a military request event (0x00446A30) (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/events_troops.h"

#include "figure/actions_c2.h"
#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

/* the formations: records of 0x90 bytes, record 1 at 0x00BCBA58 (+0 in use) */
#define FORMATION_ADDR(i) (0x00BCB9C8u + 0x90u * (uint32_t) (i))

void figure_init_military(game_state *st, int32_t f)
{
    /* @bin 0x00446650 the figure's +0x124.. from the table 0x007090D0 (0x38 bytes by figure type
     * +0xA); +0x158 := 20 percent of its +4; city +0x49A0 += 1 */
    uint32_t r = FIGURE_ADDR(f);
    S32(st, r + 0x124) = 0;
    uint32_t t = 0x007090D0u + 0x38u * U8(st, r + 0xA);
    S32(st, r + 0x128) = S32(st, t);
    S32(st, r + 0x12C) = S32(st, 0x007090D0u + 0x38u * U8(st, r + 0xA));
    U8(st, r + 0x130) = 0;
    S32(st, r + 0x138) = 0;
    S32(st, r + 0x134) = 0;
    S32(st, r + 0x13C) = S32(st, 0x007090ECu + 0x38u * U8(st, r + 0xA));
    S32(st, r + 0x140) = S32(st, 0x007090F4u + 0x38u * U8(st, r + 0xA));
    S32(st, r + 0x144) = S32(st, 0x007090FCu + 0x38u * U8(st, r + 0xA));
    S32(st, r + 0x148) = S32(st, 0x00709100u + 0x38u * U8(st, r + 0xA));
    S32(st, r + 0x150) = S32(st, 0x007090D4u + 0x38u * U8(st, r + 0xA));
    U8(st, r + 0x154) = 0;
    S32(st, r + 0x158) = calc_percentage(S32(st, 0x007090D4u + 0x38u * U8(st, r + 0xA)), 0x14);
    S32(st, CURRENT_CITY(st) + 0x49A0) += 1;
}

static int32_t entry_dir(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00446B63 the troops face the map: 4 at the top edge, 0 at the bottom, 2 at the left, 4
     * or 6 */
    if (y == 0) {
        return 4;
    }
    if (y >= S32(st, 0x00784908u) - 1) {
        return 0;
    }
    if (x == 0) {
        return 2;
    }
    return x < S32(st, 0x00784904u) - 1 ? 4 : 6;
}

static void split(int32_t n, int32_t *count, int32_t *sizes)
{
    /* @bin 0x00446FB6 up to 16: one group; up to 32: two (n / 2); more: three (n / 3 twice) */
    if (n <= 0) {
        return;
    }
    if (n <= 0x10) {
        *count = 1;
    } else if (n <= 0x20) {
        *count = 2;
        sizes[1] = n / 2;
        n -= n / 2;
    } else {
        *count = 3;
        int32_t t = n / 3;
        sizes[2] = t;
        sizes[1] = t;
        n -= 2 * t;
    }
    sizes[0] = n;
}

int32_t event_troops(game_state *st, int32_t kind, int32_t amount, int32_t where, int32_t arg4, int32_t ev)
{
    /* @bin 0x00446A30 (cdecl) the troops of event `ev` (its +0x32 := 1): `kind` 12 / 13 (the
     * scenario's 0x007848FC otherwise) at entry point `where` (1..8 land, 9..16 sea). Land: amount *
     * difficulty percent (0x005D19B0 by 0x00E39080), at most 150, split by the kind's shares into
     * three ranks of formations (0x004B6FF0) of figures; sea (not kind 13): the event's +0x36 ships
     * (amount / 4 for ev 0x8C) and the amount in boats of 16. Returns the entry tile, or -1 */
    int32_t a = amount;
    uint32_t rec = 0;
    if (ev >= 0 && ev < S32(st, 0x00785D70u)) {
        rec = 0x00785D74u + 0x7Cu * (uint32_t) ev;
        U8(st, rec + 0x32) = 1;
    }
    if (where >= 9 && where <= 0x10 && kind != 0xD) {
        /* @bin 0x00446A9D by sea */
        if (a >= 0x96) {
            a = 0x96;
        }
        int16_t seq = (int16_t) (U16(st, 0x00775900u) + 1);
        S16(st, 0x00775900u) = seq > 0x7D00 ? 1 : seq;
        int32_t ax[3], ay[3], n = 0;
        for (uint32_t p = 0x00784DC4u; p < 0x00784DD0u; p += 4) {
            if (S32(st, p) != -1) {
                ax[n] = S32(st, p);
                ay[n] = S32(st, p + 0xC);
                n++;
            }
        }
        int32_t idx = where - 1;
        for (int k = 8; k < 0x10 && S16(st, 0x00784BC0u + 2u * (uint32_t) idx) == -1; k++) {
            idx++;
            if (idx >= 0x10) {
                idx = 8;
            }
        }
        int32_t x = S16(st, 0x00784BC0u + 2u * (uint32_t) idx), y = S16(st, 0x00784BE0u + 2u * (uint32_t) idx);
        if (x == -1 || y == -1) {
            x = S16(st, 0x00784C68u);
            y = S16(st, 0x00784C6Au);
        }
        int32_t dir = entry_dir(st, x, y);
        int32_t tile = (int32_t) (U32(st, 0x00BCA2D8u) + (uint32_t) y * 0xE4u + (uint32_t) x);
        int32_t ships;
        if (ev == 0x8C) {
            ships = a / 4;
        } else if (rec != 0) {
            ships = U8(st, rec + 0x36);
        } else {
            state_unported(st, 0x00446A30u); /* the original reads through a null record */
            return 0;
        }
        for (int32_t i = 0; i < ships; i++) {
            int32_t f;
            if (kind == 0xC) {
                f = GAME_CALL(st, 0x004B4800u, 0, 0x64, x, y, dir);
            } else {
                int32_t px = x, py = y;
                spiral_free_water(st, &px, &py);
                f = GAME_CALL(st, 0x004B4800u, 0, 0x5D, px, py, dir);
            }
            uint32_t r = FIGURE_ADDR(f);
            U8(st, r + 0xD) = 0;
            U8(st, r + 0x38) = 8;
            S16(st, r + 0x36) = (int16_t) (i * 25 + 10);
            GAME_CALL(st, 0x004B5AF0u, f);
            U8(st, r + 0x65) = 1;
            U8(st, r + 0x76) = 1;
            S32(st, r + 0x180) = 0;
            figure_init_military(st, f);
            U8(st, r + 0x80) = U8(st, 0x0078D7C0u);
        }
        int32_t pick = 0;
        if (n > 0) {
            random_next_state(st);
            pick = (int32_t) (U32(st, 0x00D3B370u) & 0x7FFF) % n;
        }
        if (a != 0 && n == 0) {
            state_unported(st, 0x00446A30u); /* the landing point comes from uninitialized locals */
            return 0;
        }
        for (int32_t k = 0; a != 0; k++) {
            int32_t batch;
            if (a >= 0x10) {
                batch = 0x10;
                a -= 0x10;
            } else {
                batch = a;
                a = 0;
            }
            int32_t px = x, py = y;
            spiral_free_water(st, &px, &py);
            int32_t f = GAME_CALL(st, 0x004B4800u, 0, kind == 0xC ? 0x65 : 0x5C, px, py, dir);
            uint32_t r = FIGURE_ADDR(f);
            S16(st, r + 0x30) = (int16_t) ax[pick];
            S16(st, r + 0x32) = (int16_t) ay[pick];
            U8(st, r + 0xD) = 0;
            U8(st, r + 0x38) = 8;
            S16(st, r + 0x36) = (int16_t) (k * 40 + 10);
            GAME_CALL(st, 0x004B5AF0u, f);
            U8(st, r + 0x65) = 1;
            U8(st, r + 0x76) = 1;
            S32(st, r + 0x180) = 0;
            GAME_CALL(st, 0x004620C0u, f);
            U8(st, r + 0x130) = 1;
            S32(st, r + 0x16C) = batch;
            S32(st, r + 0x170) = S16(st, 0x00775900u);
            S32(st, r + 0x174) = kind;
            S32(st, r + 0x178) = arg4;
            S32(st, r + 0x17C) = ev;
        }
        return tile;
    }
    /* @bin 0x00446EF9 by land */
    int32_t n0 = calc_percentage(a, S32(st, 0x005D19B0u + 4u * U32(st, 0x00E39080u)));
    if (n0 >= 0x96) {
        n0 = 0x96;
    }
    int16_t seq = (int16_t) (U16(st, 0x00775900u) + 1);
    S16(st, 0x00775900u) = seq > 0x7D00 ? 1 : seq;
    int32_t p0, p1, p2;
    if (kind == 0xC) {
        p0 = S32(st, 0x00709CC0u);
        p1 = S32(st, 0x00709CF8u);
        p2 = S32(st, 0x00709D30u);
    } else if (kind == 0xD) {
        p0 = S32(st, 0x0070A698u);
        p1 = 0;
        p2 = 0;
    } else {
        p0 = S32(st, 0x00709A90u);
        p1 = S32(st, 0x00709A58u);
        p2 = S32(st, 0x00709AC8u);
    }
    (void) p0; /* @bin 0x00446F83 its percent is computed and not used */
    int32_t second = calc_percentage(n0, p1);
    int32_t third = calc_percentage(n0, p2);
    int32_t first = n0 - third - second;
    int32_t groups[3] = {0, 0, 0}, sizes[3][4] = {{0}};
    split(first, &groups[0], sizes[0]);
    split(second, &groups[1], sizes[1]);
    split(third, &groups[2], sizes[2]);
    int32_t x, y;
    if (kind == 0xB) {
        x = S16(st, 0x00784C64u);
        y = S16(st, 0x00784C66u);
    } else {
        int32_t cnt = 0;
        for (uint32_t p = 0x00784BC0u; p < 0x00784BD0u; p += 2) {
            if (S16(st, p) != -1) {
                cnt++;
            }
        }
        int32_t idx;
        if (where == 8) {
            uint32_t r7 = U32(st, 0x00D3B374u);
            idx = (int32_t) (cnt <= 2 ? r7 & 1 : cnt <= 4 ? r7 & 3 : r7 & 7);
        } else {
            idx = where - 1;
        }
        for (int k = 0; k < 8 && S16(st, 0x00784BC0u + 2u * (uint32_t) idx) == -1; k++) {
            idx++;
            if (idx >= 8) {
                idx = 0;
            }
        }
        x = S16(st, 0x00784BC0u + 2u * (uint32_t) idx);
        y = S16(st, 0x00784BE0u + 2u * (uint32_t) idx);
    }
    if (x == -1 || y == -1) {
        x = S16(st, 0x00784C68u);
        y = S16(st, 0x00784C6Au);
    }
    int32_t dir = entry_dir(st, x, y);
    int32_t tile = (int32_t) (U32(st, 0x00BCA2D8u) + (uint32_t) y * 0xE4u + (uint32_t) x);
    uint32_t fl = U32(st, 0x00AF598Cu + 4u * (uint32_t) tile);
    if (fl & 0x203) {
        return -1;
    }
    if (fl & 4) {
        if (!(fl & 0x40)) {
            return -1;
        }
    } else if (fl & 0xC108) {
        GAME_CALL(st, 0x00469140u, x, y, tile);
    }
    int32_t spawned = 0;
    for (int g = 0; g < 3; g++) {
        if (groups[g] <= 0) {
            continue;
        }
        int32_t t, m = 0xA;
        if (kind == 0xC) {
            t = g == 1 ? 0x37 : g == 2 ? 0x38 : 0x36;
        } else if (kind == 0xD) {
            t = 0x63;
            m = 8;
        } else {
            t = g == 1 ? 0x2B : g == 2 ? 0x2D : 0x2C;
        }
        for (int32_t j = 0; j < groups[g]; j++) {
            int32_t fm = GAME_CALL(st, 0x004B6FF0u, t, m, dir, x, y);
            if (fm <= 0) {
                continue;
            }
            uint32_t fr = FORMATION_ADDR(fm);
            S16(st, fr + 0x44) = (int16_t) arg4;
            U8(st, fr + 0x71) = (uint8_t) dir;
            U8(st, fr + 0x6A) = (uint8_t) kind;
            S16(st, fr + 0x78) = (int16_t) ev;
            S16(st, fr + 0x88) = S16(st, 0x00775900u);
            int32_t size = sizes[g][j];
            for (int32_t i = 0; i < size; i++) {
                int32_t f = GAME_CALL(st, 0x004B4800u, 0, t, x, y, dir);
                uint32_t r = FIGURE_ADDR(f);
                U8(st, r + 0xD) = 0;
                U8(st, r + 0x38) = 9;
                S16(st, r + 0x36) = (int16_t) ((i + 20 * spawned + 1) * 10);
                S16(st, r + 0x60) = (int16_t) fm;
                GAME_CALL(st, 0x004B5AF0u, f);
                U8(st, r + 0x65) = 1;
            }
            spawned++;
        }
    }
    return tile;
}

static int32_t run_troops(game_state *st, const int32_t *a)
{
    return event_troops(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_init_military(game_state *st, const int32_t *a)
{
    figure_init_military(st, a[0]);
    return 0;
}

const ported_function month2_troops_ports[] = {
    {0x00446A30u, "event_troops", 5, 32, 0, run_troops, 0},
    {0x00446650u, "figure_init_military", 1, 0, 0, run_init_military, 0},
};
const int month2_troops_ports_count = sizeof(month2_troops_ports) / sizeof(month2_troops_ports[0]);
