/* Imhotep - cheat codes: the table run by 0x0041D010 and its functions.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.9. Table 0x0057C3C8: 34 entries {name XOR 3, function, flag}; the
 * names are read from the exe only to identify the entries (comments). */
#include "command/orders.h"

#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CHEAT_COUNT 0x22

int32_t cheat_run(game_state *st, int32_t index)
{
    /* @bin 0x0041D010 city +0x44A0 counts the cheats used; the entry's function through the table */
    if (index < 0 || index >= CHEAT_COUNT) {
        return 0;
    }
    U32(st, CURRENT_CITY(st) + CITY_U44A0) += 1u;
    GAME_CALL0(st, U32(st, G_CHEAT_TABLE + 4 + 12u * (uint32_t) index));
    return 1;
}

static void cheat_tomb(game_state *st)
{
    /* @bin 0x0041CFC0 "Pharaohs Tomb" */
    U8(st, G_CHEAT_WIN) = 1;
    U8(st, G_CHEATED) = 1;
}

static void cheat_treasure(game_state *st)
{
    /* @bin 0x0041CFD0 "Treasure Chest": below 15000, treasury and +0x44A0 += 1000 */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + CITY_TREASURY) < 0x3A98) {
        U32(st, c + CITY_TREASURY) += 0x3E8u;
        U32(st, c + CITY_U44A0) += 0x3E8u;
    }
}

static void cheat_side_show(game_state *st)
{
    /* @bin 0x0041CCF0 "Side Show": u8 0x007027E1 toggled */
    U8(st, G_SIDE_SHOW) = U8(st, G_SIDE_SHOW) == 0;
}

static void cheat_hippo_stomp(game_state *st)
{
    /* @bin 0x0041CD00 "Hippo Stomp", in the city view (window 1): a herd formation (type 0x54) at
     * the tile under the mouse and 8 animals around it */
    if (U8(st, G_WINDOW) != 1) {
        return;
    }
    int32_t f = GAME_CALL(st, FN_FORMATION_CREATE_0X4B6FF0, 0x54, 5, 0, S32(st, G_MOUSE_TILE_X), S32(st, G_MOUSE_TILE_Y));
    if (f <= 0) {
        return;
    }
    uint32_t fo = G_FORMATION_BASE + 0x90u * (uint32_t) f;
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
            int32_t dx = S32(st, G_DIRECTIONS + 8u * (uint32_t) dir);
            int32_t dy = S32(st, G_DIRECTIONS + 4 + 8u * (uint32_t) dir);
            uint32_t off = (uint32_t) (dx + dy * 0xE4) + U32(st, G_MAP_START) + U32(st, G_MOUSE_TILE_X) + U32(st, G_MOUSE_TILE_Y);
            uint8_t ok = (uint8_t) GAME_CALL(st, FN_INVADER_TILE_OK, (int32_t) U32(st, G_GRID_TERRAIN + 4u * off), 0);
            dir = dir + 1 >= 8 ? 0 : dir + 1;
            if (ok == 0) {
                break;
            }
        }
        random_next_state(st);
        int32_t x = S32(st, G_DIRECTIONS + 8u * (uint32_t) dir) + S32(st, G_MOUSE_TILE_X);
        int32_t y = S32(st, G_DIRECTIONS + 4 + 8u * (uint32_t) dir) + S32(st, G_MOUSE_TILE_Y);
        int32_t fig = GAME_CALL(st, FN_FIGURE_CREATE, 1, 0x54, x, y, (int32_t) (U32(st, G_R_IV1) & 7));
        uint32_t p = FIGURE_ADDR(fig);
        U8(st, p + FIG_ACTION) = 0x19;
        S16(st, p + FIG_FORMATION) = (int16_t) f;
        S16(st, p + FIG_WAIT_TICKS) = 0x3E8;
        U8(st, p + FIG_U62) = (uint8_t) k;
        S32(st, p + FIG_FRAME_PERIOD) = 7;
        GAME_CALL(st, FN_ANIMAL_SEEK_TARGET, fig);
    }
}

/* @bin 0x00446530 / 0x004465A0 "mockattack1" / "mockattack2": an army (0x00446A30) and its message
 * (0x7A, or 0x139 from scenario kind 12) */
static void mock_attack(game_state *st, int32_t a, int32_t b)
{
    int32_t r = GAME_CALL(st, FN_EVENT_TROOPS, S16(st, G_SCENARIO_ENEMY), a, b, 0, 0x8C);
    if (r == 0) {
        return;
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    int32_t kind = S16(st, G_SCENARIO_ENEMY) < 0xC ? 0x7A : 0x139;
    GAME_CALL(st, FN_MESSAGE_POST, kind, S16(st, G_INVASION_COUNTER), r, 0, 0);
}

static void cheat_mock_attack_1(game_state *st) { mock_attack(st, 0xF, 8); }
static void cheat_mock_attack_2(game_state *st) { mock_attack(st, 0x10, 9); }

/* @bin 0x0041CBC0 .. 0x0041CC50 a message (forced) then a plague */
static void plague_message(game_state *st, int32_t text)
{
    S32(st, G_MESSAGE_POPUP) = 1;
    GAME_CALL(st, FN_MESSAGE_POST, text, 0, 0, 0, 0);
}

static void cheat_crop_busters(game_state *st)
{
    plague_message(st, 0x1E3);
    GAME_CALL0(st, FN_PLAGUE_FROGS_END);
}

static void cheat_amphibious(game_state *st)
{
    plague_message(st, 0x1E4);
    GAME_CALL(st, FN_PLAGUE_FROGS, 6);
}

static void cheat_hail(game_state *st)
{
    plague_message(st, 0x1E5);
    GAME_CALL0(st, FN_PLAGUE_HAIL);
}

static void cheat_crimson_tide(game_state *st)
{
    plague_message(st, 0x1E6);
    GAME_CALL(st, FN_PLAGUE_FLIES, 5);
}

static void cheat_astronauts(game_state *st)
{
    /* @bin 0x0041CC80 "Ancient Astronauts" */
    if (GAME_CALL0(st, FN_PYRAMID_PROGRESS_4F5340) != 0) {
        GAME_CALL(st, FN_PYRAMID_SPEEDUP_4F3AA0, 2);
    }
}

static void cheat_jail_break(game_state *st)
{
    /* @bin 0x0041CCA0 "Jail Break" */
    GAME_CALL(st, FN_PLAGUE_FIRE, 3);
}

static void cheat_mummys_revenge(game_state *st)
{
    /* @bin 0x0041CCB0 "Mummys Revenge": outside the editor (0x00605A3C) message 0x1F0 and
     * 0x004E7AC0(10); u8 0x00EA137C := 1 */
    if (S32(st, G_EDITOR) != 1) {
        plague_message(st, 0x1F0);
        GAME_CALL(st, FN_PLAGUE_TOMB_ROBBERS, 0xA);
    }
    U8(st, G_UEA137C) = 1;
}

/* @bin 0x0041CA80 .. 0x0041CBB0 entries 7..26: god_act(god, 1, kind) for gods 0..4, kinds 1..4 */
#define GOD(n, god, kind) \
    static int32_t run_god_##n(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        GAME_CALL(st, FN_GOD_ACT, god, 1, kind); \
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
