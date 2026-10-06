/* Imhotep - cheat codes: the table run by 0x0041D010 and its functions.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.9. Table 0x0057C3C8: 34 entries {name XOR 3, function, flag}; the
 * names are read from the exe only to identify the entries (comments). */
#include "command/orders.h"

#include "random/random.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CHEAT_TABLE 0x0057C3C8u  /* 12-byte entries; +4 the function */
#define CHEAT_COUNT 0x22
#define MESSAGE_FORCED 0x00E950DCu
#define MOUSE_X 0x00BCA2C8u
#define MOUSE_Y 0x00BCA2CCu
#define MAP_START 0x00BCA2D8u
#define TERRAIN_GRID 0x00AF598Cu
#define DIRECTIONS 0x005CD978u   /* 8 (dx, dy) pairs of i32 */

int32_t cheat_run(game_state *st, int32_t index)
{
    /* @bin 0x0041D010 city +0x44A0 counts the cheats used; the entry's function through the table */
    if (index < 0 || index >= CHEAT_COUNT) {
        return 0;
    }
    U32(st, CURRENT_CITY(st) + 0x44A0) += 1u;
    GAME_CALL0(st, U32(st, CHEAT_TABLE + 4 + 12u * (uint32_t) index));
    return 1;
}

static void cheat_tomb(game_state *st)
{
    /* @bin 0x0041CFC0 "Pharaohs Tomb" */
    U8(st, 0x00E38DD8u) = 1;
    U8(st, 0x00E38DD7u) = 1;
}

static void cheat_treasure(game_state *st)
{
    /* @bin 0x0041CFD0 "Treasure Chest": below 15000, treasury and +0x44A0 += 1000 */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x0C) < 0x3A98) {
        U32(st, c + 0x0C) += 0x3E8u;
        U32(st, c + 0x44A0) += 0x3E8u;
    }
}

static void cheat_side_show(game_state *st)
{
    /* @bin 0x0041CCF0 "Side Show": u8 0x007027E1 toggled */
    U8(st, 0x007027E1u) = U8(st, 0x007027E1u) == 0;
}

static void cheat_hippo_stomp(game_state *st)
{
    /* @bin 0x0041CD00 "Hippo Stomp", in the city view (window 1): a herd formation (type 0x54) at
     * the tile under the mouse and 8 animals around it */
    if (U8(st, UI_WINDOW) != 1) {
        return;
    }
    int32_t f = GAME_CALL(st, 0x004B6FF0u, 0x54, 5, 0, S32(st, MOUSE_X), S32(st, MOUSE_Y));
    if (f <= 0) {
        return;
    }
    uint32_t fo = 0x00BCB9C8u + 0x90u * (uint32_t) f;
    U8(st, fo + 0x69) = 1;
    S16(st, fo + 0x4E) = 0x18;
    U8(st, fo + 0x29) = 8;
    for (int32_t k = 0; k < 8; k++) {
        random_next_state(st);
        /* @bin 0x0041CE17 the first of the 8 directions (from k) whose tile 0x004EC410 refuses (low
         * byte 0) ends the search; the index used after it is the next one. The tile offset adds
         * the mouse x and y (0x00BCA2C8, 0x00BCA2CC) both unscaled, as the original does */
        int32_t dir = k;
        for (int tries = 0; tries < 8; tries++) {
            int32_t dx = S32(st, DIRECTIONS + 8u * (uint32_t) dir);
            int32_t dy = S32(st, DIRECTIONS + 4 + 8u * (uint32_t) dir);
            uint32_t off = (uint32_t) (dx + dy * 0xE4) + U32(st, MAP_START) + U32(st, MOUSE_X) + U32(st, MOUSE_Y);
            uint8_t ok = (uint8_t) GAME_CALL(st, 0x004EC410u, (int32_t) U32(st, TERRAIN_GRID + 4u * off), 0);
            dir = dir + 1 >= 8 ? 0 : dir + 1;
            if (ok == 0) {
                break;
            }
        }
        random_next_state(st);
        int32_t x = S32(st, DIRECTIONS + 8u * (uint32_t) dir) + S32(st, MOUSE_X);
        int32_t y = S32(st, DIRECTIONS + 4 + 8u * (uint32_t) dir) + S32(st, MOUSE_Y);
        int32_t fig = GAME_CALL(st, 0x004B4800u, 1, 0x54, x, y, (int32_t) (U32(st, 0x00D3B384u) & 7));
        uint32_t p = FIGURE_ADDR(fig);
        U8(st, p + 0x38) = 0x19;
        S16(st, p + 0x60) = (int16_t) f;
        S16(st, p + 0x36) = 0x3E8;
        U8(st, p + 0x62) = (uint8_t) k;
        S32(st, p + 0x144) = 7;
        GAME_CALL(st, 0x004EC990u, fig);
    }
}

/* @bin 0x00446530 / 0x004465A0 "mockattack1" / "mockattack2": an army (0x00446A30) and its message
 * (0x7A, or 0x139 from scenario kind 12) */
static void mock_attack(game_state *st, int32_t a, int32_t b)
{
    int32_t r = GAME_CALL(st, 0x00446A30u, S16(st, 0x007848FCu), a, b, 0, 0x8C);
    if (r == 0) {
        return;
    }
    S32(st, MESSAGE_FORCED) = 1;
    int32_t kind = S16(st, 0x007848FCu) < 0xC ? 0x7A : 0x139;
    GAME_CALL(st, 0x004E0CF0u, kind, S16(st, 0x00775900u), r, 0, 0);
}

static void cheat_mock_attack_1(game_state *st) { mock_attack(st, 0xF, 8); }
static void cheat_mock_attack_2(game_state *st) { mock_attack(st, 0x10, 9); }

/* @bin 0x0041CBC0 .. 0x0041CC50 a message (forced) then a plague */
static void plague_message(game_state *st, int32_t text)
{
    S32(st, MESSAGE_FORCED) = 1;
    GAME_CALL(st, 0x004E0CF0u, text, 0, 0, 0, 0);
}

static void cheat_crop_busters(game_state *st)
{
    plague_message(st, 0x1E3);
    GAME_CALL0(st, 0x004E6EB0u);
}

static void cheat_amphibious(game_state *st)
{
    plague_message(st, 0x1E4);
    GAME_CALL(st, 0x004E70B0u, 6);
}

static void cheat_hail(game_state *st)
{
    plague_message(st, 0x1E5);
    GAME_CALL0(st, 0x004E6ED0u);
}

static void cheat_crimson_tide(game_state *st)
{
    plague_message(st, 0x1E6);
    GAME_CALL(st, 0x004E7200u, 5);
}

static void cheat_astronauts(game_state *st)
{
    /* @bin 0x0041CC80 "Ancient Astronauts" */
    if (GAME_CALL0(st, 0x004F5340u) != 0) {
        GAME_CALL(st, 0x004F3AA0u, 2);
    }
}

static void cheat_jail_break(game_state *st)
{
    /* @bin 0x0041CCA0 "Jail Break" */
    GAME_CALL(st, 0x00469C60u, 3);
}

static void cheat_mummys_revenge(game_state *st)
{
    /* @bin 0x0041CCB0 "Mummys Revenge": outside the editor (0x00605A3C) message 0x1F0 and
     * 0x004E7AC0(10); u8 0x00EA137C := 1 */
    if (S32(st, 0x00605A3Cu) != 1) {
        plague_message(st, 0x1F0);
        GAME_CALL(st, 0x004E7AC0u, 0xA);
    }
    U8(st, 0x00EA137Cu) = 1;
}

/* @bin 0x0041CA80 .. 0x0041CBB0 entries 7..26: god_act(god, 1, kind) for gods 0..4, kinds 1..4 */
#define GOD(n, god, kind) \
    static int32_t run_god_##n(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        GAME_CALL(st, 0x004C47A0u, god, 1, kind); \
        return 0; \
    }
GOD(7, 0, 1) GOD(8, 0, 2) GOD(9, 0, 3) GOD(10, 0, 4)
GOD(11, 1, 1) GOD(12, 1, 2) GOD(13, 1, 3) GOD(14, 1, 4)
GOD(15, 2, 1) GOD(16, 2, 2) GOD(17, 2, 3) GOD(18, 2, 4)
GOD(19, 3, 1) GOD(20, 3, 2) GOD(21, 3, 3) GOD(22, 3, 4)
GOD(23, 4, 1) GOD(24, 4, 2) GOD(25, 4, 3) GOD(26, 4, 4)

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_tomb, cheat_tomb)
V(run_treasure, cheat_treasure)
V(run_side_show, cheat_side_show)
V(run_hippo, cheat_hippo_stomp)
V(run_mock_1, cheat_mock_attack_1)
V(run_mock_2, cheat_mock_attack_2)
V(run_crop, cheat_crop_busters)
V(run_amphibious, cheat_amphibious)
V(run_hail, cheat_hail)
V(run_crimson, cheat_crimson_tide)
V(run_astronauts, cheat_astronauts)
V(run_jail, cheat_jail_break)
V(run_mummy, cheat_mummys_revenge)

static int32_t run_cheat(game_state *st, const int32_t *a) { return cheat_run(st, a[0]); }

const ported_function cheats_ports[] = {
    {0x0041D010u, "cheat_run", 1, 8, 0, run_cheat, 0},
    {0x0041CFC0u, "cheat_tomb", 0, 0, 0, run_tomb, 0},
    {0x0041CFD0u, "cheat_treasure", 0, 0, 0, run_treasure, 0},
    {0x0041CCF0u, "cheat_side_show", 0, 0, 0, run_side_show, 0},
    {0x0041CD00u, "cheat_hippo_stomp", 0, 0, 0, run_hippo, 0},
    {0x00446530u, "cheat_mock_attack_1", 0, 0, 0, run_mock_1, 0},
    {0x004465A0u, "cheat_mock_attack_2", 0, 0, 0, run_mock_2, 0},
    {0x0041CA80u, "cheat_god_0_1", 0, 0, 0, run_god_7, 0},
    {0x0041CA90u, "cheat_god_0_2", 0, 0, 0, run_god_8, 0},
    {0x0041CAA0u, "cheat_god_0_3", 0, 0, 0, run_god_9, 0},
    {0x0041CAB0u, "cheat_god_0_4", 0, 0, 0, run_god_10, 0},
    {0x0041CAC0u, "cheat_god_1_1", 0, 0, 0, run_god_11, 0},
    {0x0041CAD0u, "cheat_god_1_2", 0, 0, 0, run_god_12, 0},
    {0x0041CAE0u, "cheat_god_1_3", 0, 0, 0, run_god_13, 0},
    {0x0041CAF0u, "cheat_god_1_4", 0, 0, 0, run_god_14, 0},
    {0x0041CB00u, "cheat_god_2_1", 0, 0, 0, run_god_15, 0},
    {0x0041CB10u, "cheat_god_2_2", 0, 0, 0, run_god_16, 0},
    {0x0041CB20u, "cheat_god_2_3", 0, 0, 0, run_god_17, 0},
    {0x0041CB30u, "cheat_god_2_4", 0, 0, 0, run_god_18, 0},
    {0x0041CB40u, "cheat_god_3_1", 0, 0, 0, run_god_19, 0},
    {0x0041CB50u, "cheat_god_3_2", 0, 0, 0, run_god_20, 0},
    {0x0041CB60u, "cheat_god_3_3", 0, 0, 0, run_god_21, 0},
    {0x0041CB70u, "cheat_god_3_4", 0, 0, 0, run_god_22, 0},
    {0x0041CB80u, "cheat_god_4_1", 0, 0, 0, run_god_23, 0},
    {0x0041CB90u, "cheat_god_4_2", 0, 0, 0, run_god_24, 0},
    {0x0041CBA0u, "cheat_god_4_3", 0, 0, 0, run_god_25, 0},
    {0x0041CBB0u, "cheat_god_4_4", 0, 0, 0, run_god_26, 0},
    {0x0041CBC0u, "cheat_crop_busters", 0, 0, 0, run_crop, 0},
    {0x0041CBF0u, "cheat_amphibious_assault", 0, 0, 0, run_amphibious, 0},
    {0x0041CC20u, "cheat_hail_to_the_chief", 0, 0, 0, run_hail, 0},
    {0x0041CC50u, "cheat_crimson_tide", 0, 0, 0, run_crimson, 0},
    {0x0041CC80u, "cheat_ancient_astronauts", 0, 0, 0, run_astronauts, 0},
    {0x0041CCA0u, "cheat_jail_break", 0, 0, 0, run_jail, 0},
    {0x0041CCB0u, "cheat_mummys_revenge", 0, 0, 0, run_mummy, 0},
};
const int cheats_ports_count = sizeof(cheats_ports) / sizeof(cheats_ports[0]);
