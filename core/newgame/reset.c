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
#include "state/ported.h"

#define CITY_INDEX 0x00EAAC28u  /* the current city record (0x00EA1870 + 0x49D8 * index) */
#define CITY_BASE 0x00EA1870u
#define CITY_SIZE 0x49D8u
#define EDITOR 0x00605A3Cu      /* 1: the scenario editor */
#define DIFFICULTY 0x00E39080u
#define PLAYER_NAME 0x00E38E74u /* char[32] */
#define SCREEN_W 0x00E38DB0u    /* the screen width (640, 800, 1024) */

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
    zero(st, 0x007999D8u, 0x101D00u);
    U32(st, 0x0078D78Cu) = 0;
    U32(st, 0x0078D7B4u) = 0;
}

static void rec4a_clear(game_state *st)
{
    /* @bin 0x0046A260 the ten 0x1C-byte records at 0x009783C0: in-use byte and building id */
    for (uint32_t a = 0x009783D8u; a < 0x009784F0u; a += 0x1Cu) {
        U8(st, a - 0x18u) = 0;
        U32(st, a) = 0;
    }
}

static void storage_clear(game_state *st)
{
    zero(st, 0x007900B8u, 0x9920u); /* @bin 0x0046B0E0 storage records */
}

static void rec_eaaf68_clear(game_state *st)
{
    zero(st, 0x00EAAF68u, 0x2BC0u); /* @bin 0x0046EFB0 the 0xE0-byte records at 0x00EAAF68 */
}

static void rec_9784d8_clear(game_state *st)
{
    zero(st, 0x009784D8u, 0x898u); /* @bin 0x0046F120 the 0x2C-byte records at 0x009784D8 */
}

static void figures_clear(game_state *st)
{
    /* @bin 0x004B4760 the 2000 figure records (0x184 bytes each, one memset per record), the
     * counters 0x00BCD5F0 / 0x00BCD5F4 */
    for (uint32_t a = 0x00BCD5F8u; a < 0x00C8AD38u; a += 0x184u) {
        zero(st, a, 0x184u);
    }
    U32(st, 0x00BCD5F0u) = 0;
    U32(st, 0x00BCD5F4u) = 0;
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
    uint32_t rec = events_next(st, 0x00785D70u, 0, 1);
    while (rec != 0) {
        event_fields_clear(st, rec);
        rec = events_next(st, 0x00785D70u, rec, 1);
    }
    U32(st, 0x00D0563Cu) = 0;
    U32(st, 0x00D05638u) = 0;
    U32(st, 0x00D05634u) = 0;
    U32(st, 0x00D05630u) = 0;
    U32(st, 0x00D0562Cu) = 0;
}

static void figure_name_counters(game_state *st)
{
    /* @bin 0x004B58D0 41 counters 0x00D056E0 down to 0x00D05640 := CRT rand() & 15, in this order */
    for (uint32_t a = 0x00D056E0u; a >= 0x00D05640u; a -= 4u) {
        U32(st, a) = (uint32_t) crt_rand(st) & 0xFu;
    }
}

static void formations_clear(game_state *st)
{
    /* @bin 0x004B8290 the formation records (0x90 bytes each) and their three counters */
    for (uint32_t a = 0x00BCB9C8u; a < 0x00BCD5E8u; a += 0x90u) {
        zero(st, a, 0x90u);
    }
    U32(st, 0x00BCB9BCu) = 0;
    U32(st, 0x00BCB9B8u) = 0;
    U32(st, 0x00BCB9B4u) = 0;
}

static void rec_c8ad38_clear(game_state *st)
{
    /* @bin 0x004B32A0 0x1E848 dwords at 0x00C8AD38, 0x1F4 dwords at 0x00D04E58 */
    zero(st, 0x00C8AD38u, 0x1E848u * 4u);
    zero(st, 0x00D04E58u, 0x1F4u * 4u);
}

static void rec_605a6c_clear(game_state *st)
{
    /* @bin 0x0040F250 0x33 dwords at 0x00605A6C, 0x33 bytes at 0x00605B38 */
    zero(st, 0x00605A6Cu, 0x33u * 4u);
    zero(st, 0x00605B38u, 0x33u);
}

static void build_menu_enable_all(game_state *st)
{
    fill32(st, 0x0070A8DCu, 0x474u, 1); /* @bin 0x00404330 0x474 dwords := 1 */
}

static void counters_0x0078d5c8_clear(game_state *st)
{
    zero(st, 0x0078D5C8u, 10u * 4u); /* @bin 0x0046A080 0x0078D5C8..0x0078D5EC := 0 */
}

static void date_reset(game_state *st)
{
    /* @bin 0x00451BE0 date and counters; year 0x0078D7BC := 0x832 */
    U32(st, 0x0078D7E0u) = 0;
    U32(st, 0x0078D7CCu) = 0;
    U32(st, 0x0078D7C8u) = 0;
    U32(st, 0x0078D7C4u) = 0;
    U32(st, 0x0078D7C0u) = 0;
    U32(st, 0x0078D7BCu) = 0x832u;
    U32(st, 0x0078D7B0u) = 0;
    U32(st, 0x0078D7A8u) = 0;
    U32(st, 0x0078D7A4u) = 0;
    U32(st, 0x0078D7A0u) = 0;
    U32(st, 0x0078D5C4u) = 0;
    U32(st, 0x0078D5C0u) = 0;
    U32(st, 0x0078D78Cu) = 0;
    counters_0x0078d5c8_clear(st);
    zero(st, 0x0078D3ECu, 0x33u * 4u);
    zero(st, 0x0078D4B8u, 0x33u * 4u);
}

/* ---- city records ---- */

void city_records_init_9ee0(game_state *st)
{
    /* @bin 0x004E9EE0 five entries: bytes 0x32 / 0x32 / 0 x5, dword 0; then fixed fields */
    for (uint32_t i = 0; i < 5u; i++) {
        U8(st, 0x00EAA5D8u + i) = 0x32;
        U8(st, 0x00EAA5E2u + i) = 0x32;
        U8(st, 0x00EAA5ECu + i) = 0;
        U8(st, 0x00EAAA5Eu + i) = 0;
        U8(st, 0x00EAA60Au + i) = 0;
        U8(st, 0x00EAA614u + i) = 0;
        U8(st, 0x00EAA61Eu + i) = 0;
        U32(st, 0x00EAA628u + 4u * i) = 0;
    }
    U16(st, 0x00EAABCCu) = 0xFFFFu;
    U16(st, 0x00EAABCEu) = 0xFFFFu;
    U16(st, 0x00EAABD0u) = 0xFFFFu;
    U32(st, 0x00EAA814u) = 0xFFFFFFFFu;
    U32(st, 0x00EAA9FCu) = 0xFFFFFFFFu;
    U32(st, 0x00EAA820u) = 0;
}

void city_records_init(game_state *st)
{
    /* @bin 0x004E7D10 both city records (0x00EA1870, 0x93B0 bytes) cleared, then 0x004E7E20
     * (0x00EA1820 := byte 0x00EA624B != 0) and the defaults of record 1 */
    zero(st, CITY_BASE, 0x93B0u);
    U32(st, 0x00EA1820u) = U8(st, 0x00EA624Bu) != 0 ? 1u : 0u; /* @bin 0x004E7E20 */
    U16(st, 0x00EA186Cu) = 0;
    U32(st, 0x00EA625Cu) = 0x32;
    U32(st, 0x00EA6260u) = 0x32;
    U32(st, 0x00EA6258u) = 0x3C;
    U32(st, 0x00EA8E54u) = 0x1E;
    U32(st, 0x00EA8E50u) = 0x1E;
    U32(st, 0x00EA6268u) = 3;
    U32(st, 0x00EA6250u) = 9;
    U32(st, 0x00EAA50Cu) = 1;
    U32(st, 0x00EAA510u) = 1;
    U32(st, 0x00EA8804u) = 0;
    U32(st, 0x00EA8808u) = 0;
    U32(st, 0x00EAA5D4u) = 1;
    U32(st, 0x00EAA73Cu) = 1;
    U32(st, 0x00EAA7BCu) = 0;
    U32(st, 0x00EAA7C0u) = 0;
    U32(st, 0x00EAA7C4u) = 0;
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
    U32(st, 0x007028FCu) = now;
    zero(st, 0x00702900u, 0x2300u);
    for (uint32_t a = 0x007029D0u; a < 0x00704C50u; a += 0x80u) {
        U32(st, a) = now;
    }
    for (uint32_t i = 1; i <= 63u; i++) {
        uint32_t s = 0x00702900u + 0x80u * i;
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
    U32(st, 0x007028F4u) = 0;
    U32(st, 0x007028F0u) = 0;
}

/* ---- view and UI ---- */

void view_geometry_set(game_state *st, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4,
                       int32_t a5, int32_t a6, int32_t a7)
{
    /* @bin 0x004303A0 (halves: signed division by 2, truncating; products wrap) */
    int32_t h6 = a6 / 2;
    int32_t h7 = a7 / 2;
    S32(st, 0x0070E460u) = a5;
    S32(st, 0x0070E46Cu) = a1;
    S32(st, 0x0070E48Cu) = a3;
    S32(st, 0x0070E45Cu) = h6;
    S32(st, 0x0070E454u) = (int32_t) ((uint32_t) h6 + (uint32_t) a6);
    S32(st, 0x0070E450u) = (int32_t) ((uint32_t) a6 * 2u);
    S32(st, 0x0070E458u) = a6;
    S32(st, 0x0070E444u) = (int32_t) ((uint32_t) h7 + (uint32_t) a7);
    S32(st, 0x0070E440u) = (int32_t) ((uint32_t) a7 * 2u);
    S32(st, 0x0070E468u) = (int32_t) ((uint32_t) a0 - (uint32_t) a4);
    S32(st, 0x0070E438u) = (int32_t) (((uint32_t) a6 + 2u) * (uint32_t) a4 + (uint32_t) a2 - 2u);
    S32(st, 0x0070E44Cu) = h7;
    S32(st, 0x0070E430u) = (int32_t) (((uint32_t) a5 - 1u) * (uint32_t) h7 + (uint32_t) a3);
    S32(st, 0x0070E470u) = a0;
    S32(st, 0x0070E448u) = a7;
    S32(st, 0x0070E478u) = a0 / 2;
    S32(st, 0x0070E490u) = a2;
    S32(st, 0x0070E43Cu) = a2;
    S32(st, 0x0070E464u) = a4;
    S32(st, 0x0070E434u) = a3;
    S32(st, 0x0070E474u) = a1;
}

static void view_geometry_a(game_state *st)
{
    /* @bin 0x004DDEC0 the geometry for the screen width (640 / 800 / 1024; others as 640's
     * fallback 8, 0x1F) */
    int32_t w = S32(st, SCREEN_W), x, y;
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
    int32_t w = S32(st, SCREEN_W), x, y;
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
    S32(st, 0x00E208F0u + b) = a2;
    S32(st, 0x00E20850u + b) = a3;
    S32(st, 0x00E20940u + b) = a5;
    U8(st, 0x00E2083Cu + (uint32_t) box) = (uint8_t) a4;
    S32(st, 0x00E208A0u + b) = 0;
    S32(st, 0x00E207ECu + b) = 0;
    S32(st, 0x00E2079Cu + b) = text;
    S32(st, 0x00E20994u) = 0;
    S32(st, 0x00E20794u) = 0;
}

static void view_rows_reset(game_state *st)
{
    /* @bin 0x004E2AB0 (screen width - 180) / 2 (signed, truncating) into five 8-byte entries of
     * 0x005E44A8 and five 0x78-byte records from 0x00E94E2A (word, +2 := the entry's +4, +4 := 0);
     * 0x00F53F34..0x00F53F44 := 0 */
    int32_t v = (S32(st, SCREEN_W) - 0xB4) / 2;
    U32(st, 0x00F53F34u) = 0;
    U32(st, 0x00F53F38u) = 0;
    U32(st, 0x00F53F3Cu) = 0;
    U32(st, 0x00F53F40u) = 0;
    U32(st, 0x00F53F44u) = 0;
    uint32_t e = 0x005E44A8u, r = 0x00E94E2Cu;
    for (; e < 0x005E44D0u; e += 8u, r += 0x78u) {
        S32(st, e) = v;
        U16(st, r - 2u) = (uint16_t) v;
        U16(st, r) = U16(st, e + 4u);
        U16(st, r + 2u) = 0;
    }
}

void newgame_view_reset(game_state *st)
{
    /* @bin 0x00540B50 */
    U8(st, 0x00E38DD9u) = 0;
    U32(st, 0x00E38E50u) = 0;
    U32(st, 0x00E38E54u) = 0;
    U32(st, 0x0078D7DCu) = 0;
    U32(st, 0x0078D7D8u) = 0;
    U32(st, 0x0078D7D4u) = 0;
    ring_offsets_build(st);
    U32(st, 0x00E94E04u) = 0;
    view_table_build(st, 0); /* pushed (0, 1); the callee reads one argument */
    /* @bin 0x00540B92 0x00F53F78 == 1 chooses the geometry */
    if (S32(st, 0x00F53F78u) == 1) {
        view_geometry_a(st);
    } else {
        view_geometry_b(st);
    }
    U32(st, 0x00BCA2BCu) = 0x6D;
    U32(st, 0x00BCA2C0u) = 0xDA;
    U32(st, 0x00BCA2E0u) = 0;
    GAME_CALL0(st, 0x004DDF90u); /* view_bounds (core/command/rotate.c) */
    camera_clamp(st);
    random_warm_up(st);
    U32(st, 0x00E20990u) = 0;
    /* @bin 0x00540BCC the six text boxes: (box, text buffer, ..., colour 0x00F56F84 / 0x00F56F9C) */
    text_box_set(st, 1, (int32_t) PLAYER_NAME, 0x1F, 0x68, 1, S32(st, 0x00F56F84u));
    text_box_set(st, 2, 0x00E37100, 0x20, 0x118, 1, S32(st, 0x00F56F9Cu));
    text_box_set(st, 4, 0x00E37078, 0x20, 0x118, 1, S32(st, 0x00F56F9Cu));
    text_box_set(st, 5, 0x00784914, 0x18, 0x118, 0, S32(st, 0x00F56F84u));
    text_box_set(st, 6, 0x00784954, 0x1F4, 0x190, 0, S32(st, 0x00F56F84u));
    text_box_set(st, 7, 0x007027EC, 0x100, 0x118, 0, S32(st, 0x00F56F84u));
    view_rows_reset(st);
}

/* ---- the reset ---- */

int32_t newgame_reset(game_state *st, uint32_t path)
{
    /* @bin 0x00540A10 */
    U8(st, 0x00E38DD9u) = 0;
    U8(st, 0x00E38DD8u) = 0;
    U8(st, 0x00E38DD7u) = 0;
    U32(st, 0x006059E8u) = 0;
    city_records_init(st);
    U32(st, 0x00EA1820u) = 0; /* @bin 0x004E7E10 */
    U32(st, CITY_INDEX) = 1;
    U8(st, 0x00EA624Bu) = 1;
    U8(st, 0x00EA624Au) = 1;
    GAME_CALL0(st, 0x004E1770u);
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
    if (S32(st, EDITOR) == 1) {
        /* @bin 0x00540AAE the scenario editor's branch */
        U32(st, 0x00784F10u) = 0;
        GAME_CALL(st, 0x004DE430u, 3);
        GAME_CALL(st, 0x00443EA0u, 1);
        GAME_CALL0(st, 0x00444720u);
        GAME_CALL0(st, 0x00444B10u);
        GAME_CALL0(st, 0x004BB090u);
    }
    GAME_CALL(st, 0x004DE820u, (int32_t) path);
    U32(st, CITY_INDEX) = 1;
    /* @bin 0x00540ADD strcpy(0x00EA184C, player name) */
    for (uint32_t i = 0;; i++) {
        uint8_t c = U8(st, PLAYER_NAME + i);
        U8(st, 0x00EA184Cu + i) = c;
        if (c == 0) {
            break;
        }
    }
    U8(st, 0x00EA624Bu) = 1;
    U8(st, 0x00EA624Au) = 1;
    U8(st, 0x00EA186Du) = 1;
    /* @bin 0x00540B0E the treasury of city record 1 (0x00EA6254) scaled by the difficulty's
     * percentage 0x005D1960[d], stored into the current record */
    int32_t d = S32(st, DIFFICULTY);
    int32_t t = calc_percentage(S32(st, 0x00EA6254u), S32(st, 0x005D1960u + 4u * (uint32_t) d));
    S32(st, CITY_BASE + CITY_SIZE * U32(st, CITY_INDEX) + 0x0Cu) = t;
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
    U32(st, 0x00EA1820u) = 0; /* @bin 0x004E7E10 */
    return 0;
}

static int32_t run_e7e20(game_state *st, const int32_t *a)
{
    (void) a;
    U32(st, 0x00EA1820u) = U8(st, 0x00EA624Bu) != 0 ? 1u : 0u; /* @bin 0x004E7E20 */
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
