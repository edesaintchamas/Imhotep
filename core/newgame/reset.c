/* Imhotep - the new-game reset of a custom scenario start (docs/newgame.md section 3.3): 0x00540A10,
 * the view and UI reset 0x00540B50 and the record clears they call.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "newgame/reset.h"

#include <string.h>

#include "city/month.h"
#include "command/orders.h"
#include "map/terrain_rings.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

/* @bin 0x0040F8F0 (the game's memset, called by most of the clears below) */
static void zero(game_state *st, uint32_t address, uint32_t len)
{
    memset(state_ptr(st, address, len), 0, len);
}

static void fill32(game_state *st, uint32_t address, uint32_t count, uint32_t value)
{
    for (uint32_t i = 0; i < count; i++) {
        U32(st, address + 4u * i) = value;
    }
}

/* ---- record clears ---- */

static void buildings_clear(game_state *st)
{
    /* @bin 0x0046A230 the building records (0x007999D8, 0x101D00 bytes), their count and the
     * highest id */
    zero(st, BUILDING_BASE, BUILDING_COUNT * BUILDING_REC_SIZE);
    U32(st, G_BUILDING_HIGHEST_EVER) = 0;
    U32(st, G_BUILDING_SEQUENCE) = 0;
}

static void rec4a_clear(game_state *st)
{
    /* @bin 0x0046A260 the ten 0x1C-byte records at 0x009783C0: in-use byte and building id */
    for (uint32_t a = G_SHIPWRIGHT_QUEUES + 0x18u; a < G_SHIPWRIGHT_QUEUES + 10u * G_SHIPWRIGHT_QUEUE_STRIDE + 0x18u;
         a += G_SHIPWRIGHT_QUEUE_STRIDE) {
        U8(st, a - 0x18u) = 0;
        U32(st, a) = 0;
    }
}

static void storage_clear(game_state *st)
{
    zero(st, G_STORAGE, 0x9920u); /* @bin 0x0046B0E0 storage records */
}

static void rec_eaaf68_clear(game_state *st)
{
    zero(st, G_FERRY_QUEUES, 0x2BC0u); /* @bin 0x0046EFB0 the 0xE0-byte records at 0x00EAAF68 */
}

static void rec_9784d8_clear(game_state *st)
{
    zero(st, G_FERRY_SLOTS, 0x898u); /* @bin 0x0046F120 the 0x2C-byte records at 0x009784D8 */
}

static void figures_clear(game_state *st)
{
    /* @bin 0x004B4760 the 2000 figure records (0x184 bytes each, one memset per record), the
     * counters 0x00BCD5F0 / 0x00BCD5F4 */
    for (uint32_t a = FIGURE_BASE; a < FIGURE_ADDR(FIGURE_COUNT); a += FIGURE_SIZE) {
        zero(st, a, FIGURE_SIZE);
    }
    U32(st, G_FIGURE_HIGHEST) = 0;
    U32(st, G_UBCD5F4) = 0;
}

static void event_fields_clear(game_state *st, uint32_t rec)
{
    /* @bin 0x00448A60 (thiscall) words +0x3A..+0x46 and +0x4A := 0 */
    for (uint32_t o = 0x3Au; o <= 0x46u; o += 2u) {
        P16(st, rec + o) = 0;
    }
    P16(st, rec + 0x4Au) = 0;
}

static void events_release(game_state *st)
{
    /* @bin 0x004B8C90 every record of the list 0x00785D70 (0x0044BB90 with create = 1) gets
     * 0x00448A60; then 0x00D0562C..0x00D0563C := 0 */
    uint32_t rec = events_next(st, G_EVENTS, 0, 1);
    while (rec != 0) {
        event_fields_clear(st, rec);
        rec = events_next(st, G_EVENTS, rec, 1);
    }
    U32(st, G_ENEMY_FORMATIONS) = 0;
    U32(st, G_ENEMY_SOLDIERS) = 0;
    U32(st, G_OWN_FORMATIONS) = 0;
    U32(st, G_OWN_STRENGTH) = 0;
    U32(st, G_ENEMY_COUNTER) = 0;
}

static void figure_name_counters(game_state *st)
{
    /* @bin 0x004B58D0 41 counters 0x00D056E0 down to 0x00D05640 := CRT rand() & 15, in this order */
    for (uint32_t a = G_NAME_COUNTER_FE; a >= G_NAME_COUNTER_12F; a -= 4u) {
        U32(st, a) = (uint32_t) crt_rand(st) & 0xFu;
    }
}

static void formations_clear(game_state *st)
{
    /* @bin 0x004B8290 the formation records (0x90 bytes each) and their three counters */
    for (uint32_t a = G_FORMATION_BASE; a < G_FORMATION_BASE + 50u * 0x90u; a += 0x90u) {
        zero(st, a, 0x90u);
    }
    U32(st, G_FORMATION_HIGHEST) = 0;
    U32(st, G_LEGION_LAST) = 0;
    U32(st, G_LEGIONS) = 0;
}

static void rec_c8ad38_clear(game_state *st)
{
    /* @bin 0x004B32A0 0x1E848 dwords at 0x00C8AD38, 0x1F4 dwords at 0x00D04E58 */
    zero(st, G_FIGURE_PATHS, 0x1E848u * 4u);
    zero(st, G_ROUTE_OWNERS, 0x1F4u * 4u);
}

static void rec_605a6c_clear(game_state *st)
{
    /* @bin 0x0040F250 0x33 dwords at 0x00605A6C, 0x33 bytes at 0x00605B38 */
    zero(st, G_ANIM_TIMERS, 0x33u * 4u);
    zero(st, G_ANIM_FLAGS, 0x33u);
}

static void build_menu_enable_all(game_state *st)
{
    fill32(st, G_MENU_AVAIL, 0x474u, 1); /* @bin 0x00404330 0x474 dwords := 1 */
}

static void counters_0x0078d5c8_clear(game_state *st)
{
    zero(st, G_U78D5C8, 10u * 4u); /* @bin 0x0046A080 0x0078D5C8..0x0078D5EC := 0 */
}

static void date_reset(game_state *st)
{
    /* @bin 0x00451BE0 date and counters; year 0x0078D7BC := 0x832 */
    U32(st, G_U78D7E0) = 0;
    U32(st, G_TICK) = 0;
    U32(st, G_TOTAL_DAYS) = 0;
    U32(st, G_TICK_DAY) = 0;
    U32(st, G_MONTH) = 0;
    U32(st, G_YEAR) = 0x832u;
    U32(st, G_BLOOD_RIVER_LEFT) = 0;
    U32(st, G_FROGS_COUNTDOWN) = 0;
    U32(st, G_PYRAMID_CURSE_COUNTDOWN) = 0;
    U32(st, G_HAIL_TIMER) = 0;
    U32(st, G_BUILDINGS_REPAIRED) = 0;
    U32(st, G_BUILDINGS_NOT_FOUND) = 0;
    U32(st, G_BUILDING_HIGHEST_EVER) = 0;
    counters_0x0078d5c8_clear(st);
    zero(st, G_U78D3EC, 0x33u * 4u);
    zero(st, G_U78D4B8, 0x33u * 4u);
}

/* ---- city records ---- */

void city_records_init_9ee0(game_state *st)
{
    /* @bin 0x004E9EE0 five entries: bytes 0x32 / 0x32 / 0 x5, dword 0; then fixed fields */
    for (uint32_t i = 0; i < 5u; i++) {
        U8(st, CITY_ADDR(1) + CITY_GOD_TARGET_MOOD + i) = 0x32;
        U8(st, CITY_ADDR(1) + CITY_GOD_MOOD + i) = 0x32;
        U8(st, CITY_ADDR(1) + CITY_GOD_WRATH + i) = 0;
        U8(st, CITY_ADDR(1) + CITY_GOD_BLESSING + i) = 0;
        U8(st, CITY_ADDR(1) + CITY_U43C2 + i) = 0;
        U8(st, CITY_ADDR(1) + CITY_U43CC + i) = 0;
        U8(st, CITY_ADDR(1) + CITY_U43D6 + i) = 0;
        U32(st, CITY_ADDR(1) + CITY_GOD_MONTHS + 4u * i) = 0;
    }
    U16(st, CITY_ADDR(1) + CITY_U4984) = 0xFFFFu;
    U16(st, CITY_ADDR(1) + CITY_U4986) = 0xFFFFu;
    U16(st, CITY_ADDR(1) + CITY_U4988) = 0xFFFFu;
    U32(st, CITY_ADDR(1) + CITY_U45CC) = 0xFFFFFFFFu;
    U32(st, CITY_ADDR(1) + CITY_U47B4) = 0xFFFFFFFFu;
    U32(st, CITY_ADDR(1) + CITY_U45D8) = 0;
}

void city_records_init(game_state *st)
{
    /* @bin 0x004E7D10 both city records (0x00EA1870, 0x93B0 bytes) cleared, then 0x004E7E20
     * (0x00EA1820 := byte 0x00EA624B != 0) and the defaults of record 1 */
    zero(st, CITY_BASE, 2u * CITY_REC_SIZE);
    U32(st, G_UEA1820) = U8(st, CITY_ADDR(1) + CITY_U03) != 0 ? 1u : 0u; /* @bin 0x004E7E20 */
    U16(st, G_UEA186C) = 0;
    U32(st, CITY_ADDR(1) + CITY_HEALTH_TARGET) = 0x32;
    U32(st, CITY_ADDR(1) + CITY_HEALTH) = 0x32;
    U32(st, CITY_ADDR(1) + CITY_SENTIMENT) = 0x3C;
    U32(st, CITY_ADDR(1) + CITY_WAGE_KINGDOM) = 0x1E;
    U32(st, CITY_ADDR(1) + CITY_WAGE_RATE) = 0x1E;
    U32(st, CITY_ADDR(1) + CITY_U20) = 3;
    U32(st, CITY_ADDR(1) + CITY_TAX_RATE) = 9;
    U32(st, CITY_ADDR(1) + CITY_U42C4) = 1;
    U32(st, CITY_ADDR(1) + CITY_U42C8) = 1;
    U32(st, CITY_ADDR(1) + CITY_U25BC) = 0;
    U32(st, CITY_ADDR(1) + CITY_U25C0) = 0;
    U32(st, CITY_ADDR(1) + CITY_U438C) = 1;
    U32(st, CITY_ADDR(1) + CITY_U44F4) = 1;
    U32(st, CITY_ADDR(1) + CITY_U4574) = 0;
    U32(st, CITY_ADDR(1) + CITY_U4578) = 0;
    U32(st, CITY_ADDR(1) + CITY_U457C) = 0;
    city_records_init_9ee0(st);
}

/* ---- ambient sound slots ---- */

/* @bin 0x0041D730 the sound of slots 1..63 (+0x28) */
static const uint8_t AMBIENT_SOUND[63] = {
    30, 34, 38, 42, 46, 50, 51, 52, 53, 54, 55, 56, 57, 58, 62, 63, 64, 65, 66, 67, 68,
    69, 70, 74, 78, 79, 80, 81, 82, 83, 87, 89, 91, 93, 95, 97, 98, 99, 100, 101, 102, 106,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 132, 133,
};

void ambient_slots_reset(game_state *st)
{
    /* @bin 0x0041D730 0x007028FC := timeGetTime(), the wall clock (core/sim/platform.h); the 70 slots of 0x80 bytes at 0x00702900 (save record 92) cleared,
     * slots 1..69 +0x50 := that time, slots 1..63: +0x08 := 200, +0x24 := 1, +0x28 := sound,
     * +0x48 := 1, +0x54 := 30000 */
    uint32_t now = platform_clock_ms(st);
    U32(st, G_SOUND_LAST_TIME) = now;
    zero(st, G_SOUND_SLOTS, 0x2300u);
    for (uint32_t a = G_SOUND_SLOTS + 0x80u + 0x50u; a < G_SOUND_SLOTS + 70u * 0x80u + 0x50u; a += 0x80u) {
        U32(st, a) = now;
    }
    for (uint32_t i = 1; i <= 63u; i++) {
        uint32_t s = G_SOUND_SLOTS + 0x80u * i;
        U32(st, s + 0x08u) = 200;
        U32(st, s + 0x24u) = 1;
        U32(st, s + 0x28u) = AMBIENT_SOUND[i - 1u];
        U32(st, s + 0x48u) = 1;
        U32(st, s + 0x54u) = 30000;
    }
}

static void ambient_clear_f0(game_state *st)
{
    /* @bin 0x0041DF60 */
    U32(st, G_U7028F4) = 0;
    U32(st, G_U7028F0) = 0;
}

/* ---- view and UI ---- */

void view_geometry_set(game_state *st, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4,
                       int32_t a5, int32_t a6, int32_t a7)
{
    /* @bin 0x004303A0 (halves: signed division by 2, truncating; products wrap) */
    int32_t h6 = a6 / 2;
    int32_t h7 = a7 / 2;
    S32(st, G_VIEW_ROWS) = a5;
    S32(st, G_U70E46C) = a1;
    S32(st, G_U70E48C) = a3;
    S32(st, G_TILE_W) = h6;
    S32(st, G_CORNER_W3) = (int32_t) ((uint32_t) h6 + (uint32_t) a6);
    S32(st, G_CORNER_W4) = (int32_t) ((uint32_t) a6 * 2u);
    S32(st, G_U70E458) = a6;
    S32(st, G_CORNER_H3) = (int32_t) ((uint32_t) h7 + (uint32_t) a7);
    S32(st, G_CORNER_H4) = (int32_t) ((uint32_t) a7 * 2u);
    S32(st, G_U70E468) = (int32_t) ((uint32_t) a0 - (uint32_t) a4);
    S32(st, G_CV_VIEW_RIGHT) = (int32_t) (((uint32_t) a6 + 2u) * (uint32_t) a4 + (uint32_t) a2 - 2u);
    S32(st, G_TILE_H) = h7;
    S32(st, G_CV_VIEW_BOTTOM) = (int32_t) (((uint32_t) a5 - 1u) * (uint32_t) h7 + (uint32_t) a3);
    S32(st, G_U70E470) = a0;
    S32(st, G_U70E448) = a7;
    S32(st, G_U70E478) = a0 / 2;
    S32(st, G_U70E490) = a2;
    S32(st, G_CV_VIEW_LEFT) = a2;
    S32(st, G_VIEW_COLS) = a4;
    S32(st, G_CV_VIEW_TOP) = a3;
    S32(st, G_U70E474) = a1;
}

static void view_geometry_a(game_state *st)
{
    /* @bin 0x004DDEC0 the geometry for the screen width (640 / 800 / 1024; others as 640's
     * fallback 8, 0x1F) */
    int32_t w = S32(st, G_SCREEN_W), x, y;
    if (w == 0x280) {
        x = 0xA, y = 0x1F;
    } else if (w == 0x320) {
        x = 0xC, y = 0x27;
    } else if (w == 0x400) {
        x = 0x10, y = 0x32;
    } else {
        x = 8, y = 0x1F;
    }
    view_geometry_set(st, 0xE4, 0xE4, 0, 0x1E, x, y, 0x3A, 0x1E);
}

static void view_geometry_b(game_state *st)
{
    /* @bin 0x004DDF20 as 0x004DDEC0 with other values (640 and others: 8, 0x1F) */
    int32_t w = S32(st, G_SCREEN_W), x, y;
    if (w == 0x320) {
        x = 0xA, y = 0x27;
    } else if (w == 0x400) {
        x = 0xE, y = 0x32;
    } else {
        x = 8, y = 0x1F;
    }
    view_geometry_set(st, 0xE4, 0xE4, 0, 0x1E, x, y, 0x3A, 0x1E);
}

void random_warm_up(game_state *st)
{
    /* @bin 0x004CDD60 100 steps of the generator (the same step as 0x004D7090) from pool index 0;
     * the index ends at 0 */
    random_state r;
    random_from_state(st, &r);
    r.pool_index = 0;
    for (int i = 0; i < 100; i++) {
        random_next(&r);
    }
    random_to_state(st, &r);
}

void text_box_set(game_state *st, int32_t box, int32_t text, int32_t a2, int32_t a3, int32_t a4,
                  int32_t a5)
{
    /* @bin 0x004CC560 */
    uint32_t b = 4u * (uint32_t) box;
    S32(st, G_TEXTEDIT_MAX + b) = a2;
    S32(st, G_MAX_W + b) = a3;
    S32(st, G_FONT + b) = a5;
    U8(st, G_NO_PUNCT + (uint32_t) box) = (uint8_t) a4;
    S32(st, G_TEXTEDIT_END + b) = 0;
    S32(st, G_CARET_POS + b) = 0;
    S32(st, G_TEXTEDIT_TEXT + b) = text;
    S32(st, G_TEXTEDIT_RESULT) = 0;
    S32(st, G_CARET_Y) = 0;
}

static void view_rows_reset(game_state *st)
{
    /* @bin 0x004E2AB0 (screen width - 180) / 2 (signed, truncating) into five 8-byte entries of
     * 0x005E44A8 and five 0x78-byte records from 0x00E94E2A (word, +2 := the entry's +4, +4 := 0);
     * 0x00F53F34..0x00F53F44 := 0 */
    int32_t v = (S32(st, G_SCREEN_W) - 0xB4) / 2;
    U32(st, G_UF53F34) = 0;
    U32(st, G_UF53F34 + 4u) = 0;
    U32(st, G_UF53F34 + 8u) = 0;
    U32(st, G_UF53F34 + 0xCu) = 0;
    U32(st, G_UF53F34 + 0x10u) = 0;
    uint32_t e = G_U5E44A8, r = G_NOTICES + 4u;
    for (; e < G_U5E44A8 + 5u * 8u; e += 8u, r += NOTICE_SIZE) {
        S32(st, e) = v;
        U16(st, r - 2u) = (uint16_t) v;
        U16(st, r) = U16(st, e + 4u);
        U16(st, r + 2u) = 0;
    }
}

void newgame_view_reset(game_state *st)
{
    /* @bin 0x00540B50 */
    U8(st, G_UE38DD9) = 0;
    U32(st, G_UE38E50) = 0;
    U32(st, G_UE38E54) = 0;
    U32(st, G_DEBUG_EVOLVE) = 0;
    U32(st, G_DEBUG_TILE) = 0;
    U32(st, G_DEBUG_FIGURE) = 0;
    ring_offsets_build(st);
    U32(st, G_UE94E04) = 0;
    view_table_build(st, 0); /* pushed (0, 1); the callee reads one argument */
    /* @bin 0x00540B92 0x00F53F78 == 1 chooses the geometry */
    if (S32(st, G_SIDEBAR_COLLAPSED) == 1) {
        view_geometry_a(st);
    } else {
        view_geometry_b(st);
    }
    U32(st, G_CAMERA_X) = 0x6D;
    U32(st, G_CAMERA_Y) = 0xDA;
    U32(st, G_VIEW_ORIENTATION) = 0;
    GAME_CALL0(st, FN_VIEW_BOUNDS); /* view_bounds (core/command/rotate.c) */
    camera_clamp(st);
    random_warm_up(st);
    U32(st, G_CARET_FIELD) = 0;
    /* @bin 0x00540BCC the six text boxes: (box, text buffer, ..., colour 0x00F56F84 / 0x00F56F9C) */
    text_box_set(st, 1, (int32_t) G_PLAYER_NAME, 0x1F, 0x68, 1, S32(st, G_FONT_SMALL));
    text_box_set(st, 2, (int32_t) G_FILES_NAME, 0x20, 0x118, 1, S32(st, G_FONT_9C));
    text_box_set(st, 4, (int32_t) G_MAP_NAME, 0x20, 0x118, 1, S32(st, G_FONT_9C));
    text_box_set(st, 5, (int32_t) G_SCENARIO_TITLE, 0x18, 0x118, 0, S32(st, G_FONT_SMALL));
    text_box_set(st, 6, (int32_t) G_U784954, 0x1F4, 0x190, 0, S32(st, G_FONT_SMALL));
    text_box_set(st, 7, (int32_t) G_CHEAT_TEXT, 0x100, 0x118, 0, S32(st, G_FONT_SMALL));
    view_rows_reset(st);
}

/* ---- the reset ---- */

int32_t newgame_reset(game_state *st, uint32_t path)
{
    /* @bin 0x00540A10 */
    U8(st, G_UE38DD9) = 0;
    U8(st, G_CHEAT_WIN) = 0;
    U8(st, G_CHEATED) = 0;
    U32(st, G_CONSTRUCTION_UI_TYPE) = 0;
    city_records_init(st);
    U32(st, G_UEA1820) = 0; /* @bin 0x004E7E10 */
    U32(st, CURRENT_PLAYER_ADDR) = 1;
    U8(st, CITY_ADDR(1) + CITY_U03) = 1;
    U8(st, CITY_ADDR(1) + CITY_ACTIVE) = 1;
    GAME_CALL0(st, FN_NEWGAME_MESSAGES_RESET);
    newgame_view_reset(st);
    rec_605a6c_clear(st);
    ambient_slots_reset(st);
    ambient_clear_f0(st);
    build_menu_enable_all(st);
    buildings_clear(st);
    storage_clear(st);
    rec_eaaf68_clear(st);
    rec_9784d8_clear(st);
    rec4a_clear(st);
    figures_clear(st);
    events_release(st);
    figure_name_counters(st);
    formations_clear(st);
    rec_c8ad38_clear(st);
    date_reset(st);
    if (S32(st, G_EDITOR) == 1) {
        /* @bin 0x00540AAE the scenario editor's branch */
        U32(st, G_EMPIRE_CURRENT) = 0;
        GAME_CALL(st, 0x004DE430u, 3);
        GAME_CALL(st, 0x00443EA0u, 1);
        GAME_CALL0(st, FN_EMPIRE_CITIES_SETUP);
        GAME_CALL0(st, FN_EMPIRE_0X77AF00_CLEAR);
        GAME_CALL0(st, 0x004BB090u);
    }
    GAME_CALL(st, FN_SCENARIO_SETUP, (int32_t) path);
    U32(st, CURRENT_PLAYER_ADDR) = 1;
    /* @bin 0x00540ADD strcpy(0x00EA184C, player name) */
    for (uint32_t i = 0;; i++) {
        uint8_t c = U8(st, G_PLAYER_NAME + i);
        U8(st, G_CITY_PLAYER_NAME + i) = c;
        if (c == 0) {
            break;
        }
    }
    U8(st, CITY_ADDR(1) + CITY_U03) = 1;
    U8(st, CITY_ADDR(1) + CITY_ACTIVE) = 1;
    U8(st, G_UEA186C + 1u) = 1;
    /* @bin 0x00540B0E the treasury of city record 1 (0x00EA6254) scaled by the difficulty's
     * percentage 0x005D1960[d], stored into the current record */
    int32_t d = S32(st, G_DIFFICULTY);
    int32_t t = calc_percentage(S32(st, CITY_ADDR(1) + CITY_TREASURY), S32(st, G_TREASURY_PCT + 4u * (uint32_t) d));
    S32(st, CURRENT_CITY(st) + CITY_TREASURY) = t;
    return t;
}

/* ---- table ---- */

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_view_reset, newgame_view_reset)
VOID0(run_records_init, city_records_init)
VOID0(run_records_9ee0, city_records_init_9ee0)
VOID0(run_ambient, ambient_slots_reset)
VOID0(run_ambient_f0, ambient_clear_f0)
VOID0(run_buildings, buildings_clear)
VOID0(run_rec4a, rec4a_clear)
VOID0(run_storage, storage_clear)
VOID0(run_eaaf68, rec_eaaf68_clear)
VOID0(run_9784d8, rec_9784d8_clear)
VOID0(run_figures, figures_clear)
VOID0(run_events, events_release)
VOID0(run_names, figure_name_counters)
VOID0(run_formations, formations_clear)
VOID0(run_c8ad38, rec_c8ad38_clear)
VOID0(run_605a6c, rec_605a6c_clear)
VOID0(run_menu, build_menu_enable_all)
VOID0(run_5c8, counters_0x0078d5c8_clear)
VOID0(run_date, date_reset)
VOID0(run_geom_a, view_geometry_a)
VOID0(run_geom_b, view_geometry_b)
VOID0(run_warm_up, random_warm_up)
VOID0(run_rows, view_rows_reset)

static int32_t run_reset(game_state *st, const int32_t *a)
{
    return newgame_reset(st, (uint32_t) a[0]);
}

static int32_t run_e7e10(game_state *st, const int32_t *a)
{
    (void) a;
    U32(st, G_UEA1820) = 0; /* @bin 0x004E7E10 */
    return 0;
}

static int32_t run_e7e20(game_state *st, const int32_t *a)
{
    (void) a;
    U32(st, G_UEA1820) = U8(st, CITY_ADDR(1) + CITY_U03) != 0 ? 1u : 0u; /* @bin 0x004E7E20 */
    return 0;
}

static int32_t run_event_fields(game_state *st, const int32_t *a)
{
    event_fields_clear(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_geometry(game_state *st, const int32_t *a)
{
    view_geometry_set(st, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    return 0;
}

static int32_t run_text_box(game_state *st, const int32_t *a)
{
    text_box_set(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

const ported_function reset_ports[] = {
    {0x00540A10u, "newgame_reset", 1, 32, 0, run_reset, 0},
    {0x00540B50u, "newgame_view_reset", 0, 0, 0, run_view_reset, 0},
    {0x004E7D10u, "city_records_init", 0, 0, 0, run_records_init, 0},
    {0x004E9EE0u, "city_records_init_9ee0", 0, 0, 0, run_records_9ee0, 0},
    {0x004E7E10u, "city_flag_clear", 0, 0, 0, run_e7e10, 0},
    {0x004E7E20u, "city_flag_set", 0, 0, 0, run_e7e20, 0},
    {0x0041D730u, "ambient_slots_reset", 0, 0, 0, run_ambient, 0},
    {0x0041DF60u, "ambient_clear_f0", 0, 0, 0, run_ambient_f0, 0},
    {0x0046A230u, "buildings_clear", 0, 0, 0, run_buildings, 0},
    {0x0046A260u, "rec4a_clear", 0, 0, 0, run_rec4a, 0},
    {0x0046B0E0u, "storage_clear", 0, 0, 0, run_storage, 0},
    {0x0046EFB0u, "rec_eaaf68_clear", 0, 0, 0, run_eaaf68, 0},
    {0x0046F120u, "rec_9784d8_clear", 0, 0, 0, run_9784d8, 0},
    {0x004B4760u, "figures_clear", 0, 0, 0, run_figures, 0},
    {0x004B8C90u, "events_release", 0, 0, 0, run_events, 0},
    {0x00448A60u, "event_fields_clear", 0, 0, 0, run_event_fields, 0},
    {0x004B58D0u, "figure_name_counters", 0, 0, 0, run_names, 0},
    {0x004B8290u, "formations_clear", 0, 0, 0, run_formations, 0},
    {0x004B32A0u, "rec_c8ad38_clear", 0, 0, 0, run_c8ad38, 0},
    {0x0040F250u, "rec_605a6c_clear", 0, 0, 0, run_605a6c, 0},
    {0x00404330u, "build_menu_enable_all", 0, 0, 0, run_menu, 0},
    {0x0046A080u, "counters_0x0078d5c8_clear", 0, 0, 0, run_5c8, 0},
    {0x00451BE0u, "date_reset", 0, 0, 0, run_date, 0},
    {0x004DDEC0u, "view_geometry_a", 0, 0, 0, run_geom_a, 0},
    {0x004DDF20u, "view_geometry_b", 0, 0, 0, run_geom_b, 0},
    {0x004303A0u, "view_geometry_set", 8, 0, 0, run_geometry, 0},
    {0x004CDD60u, "random_warm_up", 0, 0, 0, run_warm_up, 0},
    {0x004CC560u, "text_box_set", 6, 0, 0, run_text_box, 0},
    {0x004E2AB0u, "view_rows_reset", 0, 0, 0, run_rows, 0},
};
const int reset_ports_count = sizeof(reset_ports) / sizeof(reset_ports[0]);
