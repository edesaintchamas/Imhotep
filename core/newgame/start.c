/* Imhotep - starting a new city (package start, roadmap phase 6, docs/newgame.md section 3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every path of the menus that starts a city ends in 0x005405B0(0x00E37078, flag). Its inputs are
 * globals the menus set (docs/newgame.md 3.1): the map name 0x00E37078, the custom flag
 * 0x00E38EBC, the mission 0x00E38EA8, the campaign kind 0x007027B4, the difficulty 0x00E39080, the
 * player name 0x00E38E74 and the carry-over block of a linked mission (0x006FDCB8.., 0x0070278C..).
 * The loader (0x004D58B0), the new-game reset (0x00540A10) and what they call belong to other
 * packages and are called through GAME_CALL.
 *
 * The original builds "maps\<name>" in its own stack frame and passes its address to 0x005409D0
 * and 0x00540A10. 0x005409D0 is ported here on a C buffer; 0x00540A10 hands the address to
 * 0x004DE820, which never reads it (0x004DE600 builds its path from 0x00E37078 again), so the port
 * passes 0 for it. */
#include "newgame/start.h"

#include <string.h>

#include "building/building_create.h"
#include "building/map_add.h"
#include "city/text_lookup.h"
#include "command/menu.h"
#include "city/day_basic.h"
#include "command/orders.h"
#include "newgame/map_load.h"
#include "sim/call.h"
#include "sim/math.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/loader.h"
#include "state/records.h"

#define MAP_NAME 0x00E37078u      /* char[65] */
#define PLAYER_NAME 0x00E38E74u   /* char[32] */
#define CITY_PLAYER_NAME 0x00EA184Cu
#define MISSION 0x00E38EA8u       /* i8: record of mission1.pak (0x7F: none) */
#define PERIOD 0x00E38EA9u        /* u8 */
#define STEP_PREV 0x00E38EAAu     /* u8 */
#define STEP 0x00E38EABu          /* u8 */
#define KINGDOM_START 0x00E38EB0u /* i32 */
#define KINGDOM_CARRY 0x00E38EB4u /* i32 */
#define PLAYED 0x00E38EB8u        /* i32 (only its low byte survives the start) */
#define CUSTOM 0x00E38EBCu        /* i32: != 0 custom scenario */
#define DIFFICULTY 0x00E39080u    /* i32 0..4 */
#define CAMPAIGN_KIND 0x007027B4u /* u8: 0 custom, 1 Pharaoh, 2 Cleopatra, 3 mission list */
#define CAMPAIGN_TABLE 0x006FDD08u
#define FORMATIONS_KEEP 0x006FDCB4u /* u8 */
#define CARRY 0x006FDCB8u         /* 10 dwords: the carry-over of the previous mission */
#define CARRY_COPY 0x006FDCE0u    /* 10 dwords: its copy read by 0x00418BF0 */
#define SCREEN 0x00E38DDAu
#define REDRAW 0x00E38DCAu
#define EXT 0x00E38AECu           /* char[4]: the current file extension */
#define TREASURY_PCT 0x005D1960u  /* i32[5] by difficulty */
#define KINGDOM_BY_DIFF 0x005D1988u /* i32[5] by difficulty */

/* ---- 0x004DE140, 0x004E1770, 0x004E1830, 0x00489B40 ---- */

void newgame_map_points_reset(game_state *st)
{
    /* @bin 0x004DE140 4 pairs of i32 at 0x00E94DDC .. 0x00E94DFC := -1 */
    for (uint32_t a = 0x00E94DDCu; a < 0x00E94DFCu; a += 4) {
        S32(st, a) = -1;
    }
}

void newgame_message_timer_reset(game_state *st)
{
    /* @bin 0x004E1830 */
    U32(st, 0x00E951D8u) = 0;
    U32(st, 0x00E951D4u) = 0;
    /* @bin 0x004E183C timeGetTime() - 10000: the wall clock (core/sim/platform.h) */
    U32(st, 0x00E951D0u) = platform_clock_ms(st) - 0x2710u;
}

void newgame_messages_reset(game_state *st)
{
    /* @bin 0x004E1770 the message list: 1000 records of 0x30 bytes from 0x00E95232 (+0 i16 := 0,
     * +6 i16 := -1) */
    for (uint32_t a = 0x00E95238u; a < 0x00EA0DB8u; a += 0x30) {
        U16(st, a - 6) = 0;
        U16(st, a) = 0xFFFF;
    }
    U32(st, 0x00E951C4u) = 0;
    U32(st, 0x00E951C8u) = 0;
    U32(st, 0x00EA0DA8u) = 0;
    memset(st->mem + (0x00E95174u - STATE_BASE), 0, 0x14 * 4);
    memset(st->mem + (0x00E95124u - STATE_BASE), 0, 0x14 * 4);
    memset(st->mem + (0x00E95080u - STATE_BASE), 0, 0x14 * 4);
    U16(st, 0x00E951CCu) = 0;
    U32(st, 0x00E951F0u) = 0;
    U32(st, 0x00E951ECu) = 0;
    S32(st, 0x00E951E8u) = -1;
    U32(st, 0x00E951E4u) = 0;
    U32(st, 0x00E9511Cu) = 0;
    U32(st, 0x00E95120u) = 0;
    U32(st, 0x00E950E0u) = 1;
    U32(st, 0x00E950F0u) = 0;
    U32(st, 0x00E950ECu) = 0;
    U32(st, 0x00E950E8u) = 0;
    U32(st, 0x00E950E4u) = 0;
    newgame_message_timer_reset(st);
    U32(st, 0x00E950D4u) = 0;
    U32(st, 0x00E950D0u) = 0;
}

void newgame_profiler_reset(game_state *st)
{
    /* @bin 0x00489B46 QueryPerformanceFrequency(0x00BCA848): no performance counter in core (the
     * oracle's stub writes 0 and returns 0): the 64-bit frequency := 0, 0x00D05710 := (result != 0) */
    U32(st, 0x00BCA848u) = 0;
    U32(st, 0x00BCA84Cu) = 0;
    U8(st, 0x00D05710u) = 0;
    /* @bin 0x00489B56 the profiler's timers: 0x370 dwords at 0x00BCA850 := 0 */
    memset(st->mem + (0x00BCA850u - STATE_BASE), 0, 0x370 * 4);
}

/* ---- 0x0041B9D0 ---- */

int32_t campaign_step_of_mission(game_state *st, uint32_t self, int32_t mission)
{
    /* @bin 0x0041B9D0 the first of the [this + 0x1900] steps (0x40 bytes, mission at +0xC) */
    int32_t n = (int32_t) P32(st, self + 0x1900);
    for (int32_t i = 0; i < n; i++) {
        if ((int32_t) P32(st, self + 0xC + 0x40u * (uint32_t) i) == mission) {
            return i;
        }
    }
    return -1;
}

/* ---- 0x005409D0 ---- */

static void strip_ext(char *path)
{
    /* as 0x004CE3A0: cut at the first '.' */
    char *dot = strchr(path, '.');
    if (dot) {
        *dot = 0;
    }
}

static void add_ext(game_state *st, char *path)
{
    /* as 0x004CE3C0: "." and the three bytes of 0x00E38AEC */
    size_t n = strlen(path);
    path[n] = '.';
    path[n + 1] = (char) U8(st, EXT);
    path[n + 2] = (char) U8(st, EXT + 1);
    path[n + 3] = (char) U8(st, EXT + 2);
    path[n + 4] = 0;
}

/* strcmp(state string at a, state string at b) == 0 */
static int state_str_eq(game_state *st, uint32_t a, uint32_t b)
{
    for (uint32_t i = 0;; i++) {
        uint8_t x = U8(st, a + i);
        if (x != U8(st, b + i)) {
            return 0;
        }
        if (x == 0) {
            return 1;
        }
    }
}

static void ext_of_path(game_state *st, const char *path)
{
    /* @bin 0x004CE330 the three bytes after the first '.' (if any) into 0x00E38AEC, then 0 */
    const char *dot = strchr(path, '.');
    if (dot) {
        U8(st, EXT) = (uint8_t) dot[1];
        U8(st, EXT + 1) = (uint8_t) dot[2];
        U8(st, EXT + 2) = (uint8_t) dot[3];
        U8(st, EXT + 3) = 0;
    }
}

static void upper_case(game_state *st, uint32_t a)
{
    /* @bin 0x004CBEC0 'a'..'z' -> 'A'..'Z' in place */
    for (; P8(st, a) != 0; a++) {
        uint8_t c = P8(st, a);
        if (c >= 'a' && c <= 'z') {
            P8(st, a) = (uint8_t) (c - 0x20);
        }
    }
}

/* @bin 0x004CE100 a file that did not open: only videos, sounds and graphics (extension "WAV",
 * "SMK", "555") are looked for on the CD drive: 0x00E36BF8 := "c:\" (0x005E06DC) with the drive
 * letter 0x00E38E62, and _chdir there. The directory change is the platform's: core takes it as
 * done (the oracle's _chdir succeeds), 1; 0 for other extensions */
static int32_t file_cd_retry(game_state *st, const char *path)
{
    ext_of_path(st, path);
    upper_case(st, EXT);
    if (!state_str_eq(st, 0x005E06E8u, EXT) && !state_str_eq(st, 0x005E06E4u, EXT) &&
        !state_str_eq(st, 0x005E06E0u, EXT)) {
        return 0;
    }
    memset(st->mem + (0x00E36BF8u - STATE_BASE), 0, 40);
    if (state_str_eq(st, 0x005E06E8u, EXT) || state_str_eq(st, 0x005E06E4u, EXT) ||
        state_str_eq(st, 0x005E06E0u, EXT)) {
        text_copy(st, 0x005E06DCu, 0x00E36BF8u, 10);
    }
    U8(st, 0x00E36BF8u) = U8(st, 0x00E38E62u);
    return 1; /* @bin 0x004CE2B3 _chdir(0x00E36BF8) == 0 */
}

/* @bin 0x004CE3F0 does the file open (_open, _close: a size query to the platform); if not,
 * 0x004CE100 (the CD drive), a second try, and 0x004CE2D0 (back to the game's directory) */
static int32_t file_exists(game_state *st, const char *path)
{
    if (platform_file_read(st, path, 0, NULL, 0) >= 0) {
        return 1;
    }
    file_cd_retry(st, path);
    int32_t ok = platform_file_read(st, path, 0, NULL, 0) >= 0 ? 1 : 0;
    GAME_CALL0(st, 0x004CE2D0u);
    return ok;
}

int32_t newgame_map_file_exists(game_state *st, char *path)
{
    /* @bin 0x005409D7 0x004CE3A0(path); 0x004CBEA0("map" 0x00576274, 0x00E38AEC, 3);
     * 0x004CE3C0(path) */
    strip_ext(path);
    text_copy(st, 0x00576274u, EXT, 3);
    add_ext(st, path);
    int32_t ok = file_exists(st, path);
    /* @bin 0x00540A02 the extension is cut again */
    strip_ext(path);
    return ok;
}

/* ---- the carried monuments of 0x00418BF0 ---- */

static void monument_parts_complete_4df2c0(game_state *st, int32_t id)
{
    /* @bin 0x004DF2C0 from the main building (id - [id +0x6 i16]): 116 parts, one per entry of the
     * part table 0x005E2AE4 (0x34 bytes, until 0x005E4274): +0 := 0xFFFF when the entry's low
     * byte < 4, else 2; +0xC := 0xFF; +8 := 3; +0x1C u8 := 1; -0x42 i16 := -1 */
    int32_t base = id - P16(st, 0x007999EAu + 0x108u * (uint32_t) id);
    uint32_t a = 0x00799A30u + 0x108u * (uint32_t) base;
    for (uint32_t e = 0x005E2AE4u; e < 0x005E4274u; e += 0x34, a += 0x108) {
        uint32_t lo = U32(st, e) & 0xFF;
        P32(st, a) = lo < 4 ? 0xFFFFu : 2u;
        P32(st, a + 0xC) = 0xFF;
        P32(st, a + 8) = 3;
        P8(st, a + 0x1C) = 1;
        P16(st, a - 0x42) = -1;
    }
    GAME_CALL(st, 0x004E0C80u, id);
}

static void monument_complete_40d7f0(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x0040D7F0 (thiscall) buildings id .. id + 24: +0x8 := 0x15, +0x10 := 1, +0x18 u8 := 1,
     * -0x42 i16 := -1, then the object's method +0x18 on each */
    for (int32_t i = id; i < id + 0x19; i++) {
        uint32_t a = 0x00799A38u + 0x108u * (uint32_t) i;
        P32(st, a - 8) = 0x15;
        P32(st, a) = 1;
        P8(st, a + 8) = 1;
        P16(st, a - 0x4A) = -1;
        GAME_CALLT(st, self, P32(st, P32(st, self) + 0x18), i);
    }
}

static void monument_complete_4e65a0(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x004E65A0 (thiscall) one building: +0 := 9, +0xC := 1, +8 := 3, +0x10 u8 := 1,
     * -0x42 i16 := -1, then the object's method +0x18 */
    uint32_t a = 0x00799A30u + 0x108u * (uint32_t) id;
    P32(st, a) = 9;
    P32(st, a + 0xC) = 1;
    P32(st, a + 8) = 3;
    P8(st, a + 0x10) = 1;
    P16(st, a - 0x42) = -1;
    GAME_CALLT(st, self, P32(st, P32(st, self) + 0x18), id);
}

static void monument_complete_547ad0(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x00547AD0 (thiscall) from the main building: the object's method +0x3C gives the part
     * count; main +0x40 := 400, +0 := 1; each part: +8 := 4 unless +0x10 == 0x11, -0x42 i16 := -1,
     * +0x45 u8 := 1; then the method +0x18 on each */
    int32_t base = id - P16(st, 0x007999EAu + 0x108u * (uint32_t) id);
    int32_t n = GAME_CALLT0(st, self, P32(st, P32(st, self) + 0x3C));
    int32_t end = n + base;
    uint32_t m = 0x108u * (uint32_t) base;
    P32(st, m + 0x00799A70u) = 0x190;
    P32(st, m + 0x00799A30u) = 1;
    if (base < end) {
        uint32_t a = m + 0x00799A38u;
        for (int32_t k = end - base; k != 0; k--, a += 0x108) {
            if (P32(st, a + 8) != 0x11) {
                P32(st, a) = 4;
            }
            P16(st, a - 0x4A) = -1;
            P8(st, a + 0x3D) = 1;
        }
        for (int32_t i = base; i < end; i++) {
            GAME_CALLT(st, self, P32(st, P32(st, self) + 0x18), i);
        }
    }
}

/* @bin 0x00418C48 a carried monument: (x, y) i16 at `at`, created and put on the map complete */
static int32_t carried(game_state *st, uint32_t at, int32_t type, int32_t mask_x)
{
    int32_t x = mask_x ? (int32_t) (U32(st, at) & 0x1FFF) : S16(st, at);
    int32_t id = building_create(st, S32(st, CURRENT_PLAYER_ADDR), type, x, S16(st, at + 2), 0, 0);
    x = mask_x ? (int32_t) (U32(st, at) & 0x1FFF) : S16(st, at);
    building_map_add(st, S32(st, CURRENT_PLAYER_ADDR), type, id, x, S16(st, at + 2), 1, 1, 1);
    return id;
}

void newgame_mission_buildings(game_state *st)
{
    int32_t mission = S8(st, MISSION);
    /* @bin 0x00418BFA the carry-over block is copied (10 dwords) */
    memmove(st->mem + (CARRY_COPY - STATE_BASE), st->mem + (CARRY - STATE_BASE), 40);
    /* @bin 0x00418C11 the mission's flag string: text group 0x133 */
    text_seek(st, 0x133, mission);
    uint32_t t = U32(st, TEXT_CURSOR);
    uint32_t carry = U32(st, CARRY_COPY);
    U32(st, CARRY_COPY) = 0;
    /* @bin 0x00418C26 flag 0 == '1': the kingdom rating is carried */
    U32(st, KINGDOM_CARRY) = P8(st, t) == '1' ? carry : 0;
    uint8_t kind = P8(st, t + 4);
    if (kind == 'A') {
        /* @bin 0x00418C48 'A': pyramid 0xDE (x masked to 13 bits) */
        if (U16(st, 0x00702798u) != 0 && U16(st, 0x0070279Au) != 0) {
            int32_t id = carried(st, 0x00702798u, 0xDE, 1);
            monument_parts_complete_4df2c0(st, id);
        }
        /* @bin 0x00418CB1 0xE1 on mission 0x33 */
        if (U16(st, 0x0070279Cu) != 0 && U16(st, 0x0070279Eu) != 0 && S8(st, MISSION) == 0x33) {
            int32_t id = carried(st, 0x0070279Cu, 0xE1, 0);
            monument_complete_40d7f0(st, 0x006FD5D8u, id);
        }
        /* @bin 0x00418D24 0xE4 on mission 0x33 */
        if (U16(st, 0x007027A0u) != 0 && U16(st, 0x007027A2u) != 0 && S8(st, MISSION) == 0x33) {
            int32_t id = carried(st, 0x007027A0u, 0xE4, 0);
            monument_complete_4e65a0(st, 0x006DE548u, id);
        }
    } else if (kind == 'V') {
        /* @bin 0x00418DAF 'V': 0xE5; 0xEA after mission 0x27; 0xEB after mission 0x28 */
        if (U16(st, 0x0070278Cu) != 0 && U16(st, 0x0070278Eu) != 0) {
            int32_t id = carried(st, 0x0070278Cu, 0xE5, 0);
            monument_complete_547ad0(st, 0x006F9320u, id);
        }
        if (U16(st, 0x00702790u) != 0 && U16(st, 0x00702792u) != 0 && S8(st, MISSION) > 0x27) {
            int32_t id = carried(st, 0x00702790u, 0xEA, 0);
            monument_complete_547ad0(st, 0x006F5648u, id);
        }
        if (U16(st, 0x00702794u) != 0 && U16(st, 0x00702796u) != 0 && S8(st, MISSION) > 0x28) {
            int32_t id = carried(st, 0x00702794u, 0xEB, 0);
            monument_complete_547ad0(st, 0x006DE9E0u, id);
        }
    }
    /* @bin 0x00418EFF flag 2 == '1': the carried formations are kept */
    if (P8(st, U32(st, TEXT_CURSOR) + 2) == '1') {
        U8(st, FORMATIONS_KEEP) = 0;
    } else {
        for (uint32_t k = 0; k < 3; k++) {
            S32(st, 0x006FDCE8u + 0xC * k) = -1;
            U8(st, 0x006FDCECu + 0xC * k) = 0;
            U8(st, 0x006FDCE4u + 0xC * k) = 0xFF;
        }
        U8(st, FORMATIONS_KEEP) = 1;
    }
    GAME_CALL0(st, 0x004DEA50u); /* terrain rebuild */
}

/* ---- 0x005405B0 ---- */

static void copy_player_name(game_state *st)
{
    /* @bin 0x005406A7 strcpy(0x00EA184C, 0x00E38E74) */
    uint32_t i = 0;
    uint8_t c;
    do {
        c = U8(st, PLAYER_NAME + i);
        U8(st, CITY_PLAYER_NAME + i) = c;
        i++;
    } while (c != 0);
}

void newgame_start(game_state *st, uint32_t name, int32_t flag)
{
    /* @bin 0x005405B7 local := "maps\" (0x0057626C) + name */
    char path[512];
    size_t n = 0;
    for (uint32_t i = 0; U8(st, 0x0057626Cu + i) != 0; i++) {
        path[n++] = (char) U8(st, 0x0057626Cu + i);
    }
    for (uint32_t i = 0; n < sizeof(path) - 8; i++) {
        uint8_t c = P8(st, name + i);
        if (c == 0) {
            break;
        }
        path[n++] = (char) c;
    }
    path[n] = 0;
    /* @bin 0x005405CB the campaign globals the loader overwrites: kept and restored below */
    int32_t carry = S32(st, KINGDOM_CARRY);
    int8_t mission = S8(st, MISSION);
    uint8_t period = U8(st, PERIOD);
    uint8_t kind = U8(st, CAMPAIGN_KIND);
    uint8_t played = U8(st, PLAYED);
    uint8_t step = U8(st, STEP);
    uint8_t step_prev = U8(st, STEP_PREV);
    U8(st, FORMATIONS_KEEP) = 1;
    newgame_map_points_reset(st);
    if (S32(st, CUSTOM) != 0 && (uint8_t) flag == 0) {
        /* @bin 0x0054066E custom scenario: the map file must exist, else back to screen 1 */
        if (newgame_map_file_exists(st, path) == 0) {
            U8(st, SCREEN) = 1;
            U8(st, REDRAW) = 1;
            return;
        }
        /* @bin 0x0054069A the new-game reset and setup (the argument, the local path, is unused
         * down the call: 0x004DE820 does not read it) */
        GAME_CALL(st, 0x00540A10u, 0);
    } else {
        /* @bin 0x005406A7 campaign record */
        copy_player_name(st);
        if (GAME_CALL(st, 0x004D58B0u, 0, 0) == 0) {
            return;
        }
        uint32_t city = CURRENT_CITY(st);
        int32_t pct = S32(st, TREASURY_PCT + 4u * U32(st, DIFFICULTY));
        U32(st, CUSTOM) = 0;
        int32_t treasury = calc_percentage(S32(st, city + 0x0C), pct);
        S32(st, CURRENT_CITY(st) + 0x0C) = treasury;
        copy_player_name(st);
    }
    /* @bin 0x00540747 the kept globals back */
    U32(st, PLAYED) = played;
    S8(st, MISSION) = mission;
    U8(st, PERIOD) = period;
    U8(st, CAMPAIGN_KIND) = kind;
    S32(st, KINGDOM_CARRY) = carry;
    int8_t m = mission;
    if (kind == 3) {
        /* @bin 0x00540781 mission list: the step of the mission in the campaign table */
        int8_t s = (int8_t) campaign_step_of_mission(st, CAMPAIGN_TABLE, mission);
        U8(st, STEP) = (uint8_t) (s < 0 ? 0 : s);
        m = S8(st, MISSION);
        U8(st, STEP_PREV) = 0xFF;
    } else {
        U8(st, STEP) = step;
        U8(st, STEP_PREV) = step_prev;
    }
    /* @bin 0x005407C1 the kingdom rating at start by difficulty; the city fields */
    int32_t custom = S32(st, CUSTOM);
    int32_t kingdom = S32(st, KINGDOM_BY_DIFF + 4u * U32(st, DIFFICULTY));
    uint32_t city = CURRENT_CITY(st);
    S32(st, KINGDOM_START) = kingdom;
    S32(st, city + 0x42D8) = kingdom;
    S32(st, city + 0x4450) = S32(st, KINGDOM_CARRY);
    if (custom != 0) {
        S32(st, city + 0x4450) = 0;
    }
    int32_t rank = S16(st, 0x00784B62u);
    S32(st, city + 0x444C) = rank;
    S32(st, city + 0x2C9C) = rank;
    if (rank > 10) {
        S32(st, city + 0x2C9C) = 10;
    }
    int32_t r = S32(st, city + 0x2C9C);
    U32(st, 0x0078D830u) = 0;
    U32(st, 0x0078D828u) = 0;
    S32(st, city + 0x2C98) = S32(st, 0x005D0320u + 4u * (uint32_t) r);
    S32(st, city + 0x4720) = 1;
    U32(st, 0x0078D824u) = 1;
    U32(st, 0x0078D82Cu) = 1;
    U32(st, 0x0078D820u) = 1;
    U32(st, 0x0078D818u) = 1;
    U32(st, 0x0078D81Cu) = 1;
    U32(st, 0x0078D814u) = 1;
    U32(st, 0x0078D810u) = 1;
    S32(st, city + 0x4724) = 1;
    if (custom == 0) {
        /* @bin 0x00540882 the first missions' flags */
        if (m == 0) {
            U32(st, 0x0078D830u) = 1;
            U32(st, 0x0078D828u) = 1;
        } else if (m <= 4) {
            U32(st, 0x0078D828u) = 1;
        }
    }
    if (m <= 10 && custom == 0) {
        /* @bin 0x005408A2 building availability of the first campaign missions: 26 lists of
         * 0x1C bytes at 0x005794B0 (+0 the last mission without it, +0xC count, +0x10 the indices
         * into 0x006FDC88): enabled from the mission after +0 on */
        memset(st->mem + (0x006FDC88u - STATE_BASE), 0, 15);
        uint32_t i = 0;
        for (uint32_t e = 0x005794B0u; e + 0xC < 0x00579794u; e += 0x1C, i++) {
            uint8_t on = (int32_t) m > S32(st, e) ? 1 : 0;
            int32_t cnt = S32(st, e + 0xC);
            U8(st, 0x007027B8u + i) = on;
            uint32_t list = U32(st, e + 0x10);
            for (int32_t k = 0; k < cnt; k++) {
                P8(st, 0x006FDC88u + P32(st, list + 4u * (uint32_t) k)) = on;
            }
        }
    } else {
        /* @bin 0x0054091A everything available: 26 bytes at 0x007027B8, 15 at 0x006FDC88 */
        memset(st->mem + (0x007027B8u - STATE_BASE), 1, 26);
        memset(st->mem + (0x006FDC88u - STATE_BASE), 1, 15);
    }
    build_menu_update(st);    /* 0x00404350 */
    build_buttons_update(st); /* 0x004051A0 */
    newgame_messages_reset(st);
    newgame_mission_buildings(st);
    city = CURRENT_CITY(st);
    if (S32(st, CUSTOM) == 0) {
        /* @bin 0x00540974 the campaign's kingdom ratings again */
        S32(st, city + 0x42D8) = S32(st, KINGDOM_START);
        S32(st, city + 0x4450) = S32(st, KINGDOM_CARRY);
    }
    S32(st, city + 0x47A8) = S32(st, DIFFICULTY);
    /* @bin 0x005409A5 */
    U8(st, 0x00784CBDu) = U8(st, 0x00784CBDu) == 1;
    newgame_profiler_reset(st);
}


/* ---- the menu handlers that write the start inputs (docs/newgame.md section 2) ---- */

#define LIST_OBJECT 0x00704C00u   /* the scenario / mission list of screen 0x3D */
#define LIST_ROW 0x00704DBCu      /* i32: the chosen row */

/* @bin 0x004188F0 the carry-over of a won mission into the next one: the city's kingdom rating
 * (0x006FDCB8), the best formation of each kind (strength i16 at formation +0x78, level u8 at
 * +0x26; kinds 0xC, 0xB and the others), and with flag 6 of the mission's string (text group
 * 0x133) == '1' the positions of its monuments; then the campaign progress file is written */
void newgame_carry_over(game_state *st)
{
    U32(st, CARRY) = U32(st, CURRENT_CITY(st) + 0x4450);
    int32_t best12 = -1, idx12 = -1, best11 = -1, idx11 = -1, best = -1, idx = -1;
    int32_t lvl12 = 0, lvl11 = 0, lvl = 0;
    /* @bin 0x0041892F formations 1 .. 49 (0x90 bytes; a = record + 0x78) */
    int32_t i = 1;
    for (uint32_t a = 0x00BCBAD2u; a < 0x00BCD662u; a += 0x90, i++) {
        if (U8(st, a - 0x7A) != 1 || U8(st, a - 0x38) == 0) {
            continue;
        }
        uint16_t kind = U16(st, a - 0x76);
        int32_t v = S16(st, a);
        int32_t l = U8(st, a - 0x52);
        if (kind == 0xC) {
            if (v > best12) {
                lvl12 = l;
                best12 = v;
                idx12 = i;
            } else if (v == best12 && l > lvl12) {
                lvl12 = l;
                idx12 = i;
            }
        } else if (kind == 0xB) {
            if (v > best11) {
                best11 = v;
                lvl11 = l;
                idx11 = i;
            } else if (v == best11 && l > lvl11) {
                lvl11 = l;
                idx11 = i;
            }
        } else {
            if (v > best) {
                best = v;
                lvl = l;
                idx = i;
            } else if (v == best && l > lvl) {
                lvl = l;
                idx = i;
            }
        }
    }
    /* @bin 0x004189FF per kind: strength, level, and the formation's byte +0x8 (0xFF: none) */
    S32(st, 0x006FDCC0u) = best12;
    U8(st, 0x006FDCC4u) = (uint8_t) lvl12;
    U8(st, 0x006FDCBCu) = idx12 != -1 ? U8(st, 0x00BCB9CAu + 0x90u * (uint32_t) idx12) : 0xFF;
    S32(st, 0x006FDCCCu) = best11;
    U8(st, 0x006FDCD0u) = (uint8_t) lvl11;
    U8(st, 0x006FDCC8u) = idx11 != -1 ? U8(st, 0x00BCB9CAu + 0x90u * (uint32_t) idx11) : 0xFF;
    S32(st, 0x006FDCD8u) = best;
    U8(st, 0x006FDCDCu) = (uint8_t) lvl;
    U8(st, 0x006FDCD4u) = idx != -1 ? U8(st, 0x00BCB9CAu + 0x90u * (uint32_t) idx) : 0xFF;
    text_seek(st, 0x133, S8(st, MISSION));
    if (P8(st, U32(st, TEXT_CURSOR) + 6) == '1') {
        /* @bin 0x00418ABC buildings 1 .. 3999 in use (+0 == 1): the monuments' positions */
        int32_t id = 1;
        for (uint32_t a = 0x00799AF0u; a < 0x0089B6E8u; a += 0x108, id++) {
            if (U8(st, a - 0x10) != 1) {
                continue;
            }
            uint16_t type = U16(st, a);
            if (type == 0xDE) {
                /* @bin 0x00418ADA x (+0x68) | (+0x70) << 13, y (+0x6C) */
                U16(st, 0x0070279Au) = U16(st, a + 0x5C);
                U16(st, 0x00702798u) = (uint16_t) (U16(st, a + 0x58) | (uint16_t) (U16(st, a + 0x60) << 13));
            }
            if (type == 0xE1) {
                U16(st, 0x0070279Cu) = (uint16_t) (U16(st, a - 0xA) - 4);
                U16(st, 0x0070279Eu) = (uint16_t) (U16(st, a - 8) + 2);
            }
            if (type == 0xE4) {
                /* @bin 0x00418B27 the main building (id - [+0x12]): its x, y */
                uint32_t m = 0x108u * (uint32_t) (id - S16(st, a + 2));
                U16(st, 0x007027A0u) = P16(st, m + 0x007999DEu);
                U16(st, 0x007027A2u) = P16(st, m + 0x007999E0u);
            }
            if (type == 0xE5) {
                U16(st, 0x0070278Cu) = (uint16_t) (U16(st, a - 0xA) - 5);
                U16(st, 0x0070278Eu) = (uint16_t) (U16(st, a - 8) + 6);
            }
            if (type == 0xEA) {
                U16(st, 0x00702790u) = (uint16_t) (U16(st, a - 0xA) - 6);
                U16(st, 0x00702792u) = (uint16_t) (U16(st, a - 8) + 8);
            }
            if (type == 0xEB) {
                U16(st, 0x00702794u) = (uint16_t) (U16(st, a - 0xA) + 1);
                U16(st, 0x00702796u) = (uint16_t) (U16(st, a - 8) + 0x11);
            }
        }
    }
    /* @bin 0x00418BD3 the campaign progress into the family's file (0x00516A20: its path) */
    int32_t path = GAME_CALL0(st, 0x00516A20u);
    GAME_CALLT(st, CAMPAIGN_TABLE, 0x0041B8A0u, path);
}

static void progress_save(game_state *st)
{
    int32_t path = GAME_CALL0(st, 0x00516A20u);
    GAME_CALLT(st, CAMPAIGN_TABLE, 0x0041B8A0u, path);
}

/* @bin 0x00418640 the victory screen's "continue": the mission marked won, the next campaign step,
 * the carry-over (0x004188F0), then the briefing, the mission choice or the next period */
void newgame_next_mission(game_state *st)
{
    uint32_t city = CURRENT_CITY(st);
    int32_t kingdom = S32(st, city + 0x42D8);
    int32_t carry = S32(st, city + 0x4450);
    int32_t custom = S32(st, CUSTOM);
    S32(st, KINGDOM_START) = kingdom;
    S32(st, KINGDOM_CARRY) = carry;
    U32(st, PLAYED) = U32(st, PLAYED) + 1;
    int choose = 0;
    int32_t next = -1;
    if (custom == 0) {
        int32_t s = S8(st, STEP);
        if (s >= 0 && s < S32(st, 0x006FF608u)) {
            /* @bin 0x004186A9 the step won (+0x3C) and saved */
            U8(st, CAMPAIGN_TABLE + 0x40u * (uint32_t) s + 0x3C) = 1;
            progress_save(st);
        }
        next = GAME_CALLT(st, CAMPAIGN_TABLE, 0x0041B920u, S8(st, PERIOD), S8(st, STEP), 0);
        uint8_t kind = U8(st, CAMPAIGN_KIND);
        if ((kind == 2 || kind == 1) && next >= 0) {
            newgame_carry_over(st);
            uint8_t cur = U8(st, STEP);
            int32_t c = (int8_t) cur;
            int32_t n = S32(st, 0x006FF608u);
            if (c >= 0 && c < n) {
                if (S32(st, CAMPAIGN_TABLE + 0x40u * (uint32_t) c + 0x38) >= 2) {
                    choose = 1; /* @bin 0x0041875B a choice of missions */
                } else {
                    U8(st, STEP_PREV) = cur;
                    U8(st, STEP) = (uint8_t) next;
                    if (next < n) {
                        U8(st, MISSION) = U8(st, CAMPAIGN_TABLE + 0x40u * (uint32_t) next + 0xC);
                    } else {
                        state_unported(st, 0x0041874Eu); /* reads address 0xC: the original faults */
                        return;
                    }
                }
            }
        } else if (S8(st, MISSION) == 0x28) {
            /* @bin 0x00418762 mission 0x28: the position of its 0xEB monument, saved */
            for (uint32_t a = 0x00799AE6u; a < 0x0089B6DEu; a += 0x108) {
                if (U16(st, a + 0xA) == 0xEB) {
                    U16(st, 0x00702794u) = (uint16_t) (U16(st, a) + 1);
                    U16(st, 0x00702796u) = (uint16_t) (U16(st, a + 2) + 0x11);
                }
            }
            progress_save(st);
        }
    }
    city = CURRENT_CITY(st);
    S32(st, city + 0x44D0) = 0;
    S32(st, city + 0x44D4) = 0;
    S32(st, city + 0x44D8) = 0;
    custom = S32(st, CUSTOM);
    int list = 0, period = 0;
    if (next < 0) {
        if (custom == 0) {
            period = 1;
        } else {
            list = 1;
        }
    } else if (custom != 0) {
        list = 1;
    } else if (U8(st, CAMPAIGN_KIND) == 3) {
        period = 1;
    } else if (choose) {
        U8(st, SCREEN) = 0x34;
        U8(st, 0x007027A4u) = 0;
    } else {
        GAME_CALL0(st, 0x004181E0u); /* the briefing */
    }
    if (list) {
        /* @bin 0x0041880E back to the scenario list */
        U32(st, 0x00605A3Cu) = 0;
        U8(st, 0x006DE515u) = 0;
        U32(st, 0x00704DB0u) = 0;
        U32(st, 0x00704DACu) = 3;
        GAME_CALL0(st, 0x0041F870u);
    } else if (period) {
        /* @bin 0x00418835 the end of a period: the next one (videos after the fifth) */
        if (U8(st, CAMPAIGN_KIND) == 1) {
            int32_t p = S8(st, PERIOD) + 1;
            S32(st, 0x007027B0u) = p;
            if (p != 5) {
                if (p < 9) {
                    S32(st, 0x007027ACu) = p;
                    U8(st, SCREEN) = 0x17;
                    U32(st, 0x006FDC98u) = 0;
                    goto done;
                }
            } else {
                int32_t w = S32(st, 0x00E38DACu), h = S32(st, 0x00E38DB0u);
                uint32_t video = S32(st, 0x00E38EDCu) < 0xE9 ? 0x0057BFF8u : 0x0057BFD8u;
                GAME_CALLT(st, U32(st, 0x006FDC54u), 0x00413580u, (int32_t) video, (h - 0x1E0) / 2, (w - 0x11E) / 2);
            }
        }
        GAME_CALL0(st, 0x004CB6A0u); /* the main menu */
    }
done:
    U32(st, 0x00BC9694u) = 0;
    U32(st, 0x00E94E0Cu) = 0;
    U8(st, REDRAW) = 1;
}

/* @bin 0x00418510 "replay mission": a custom scenario starts again; a Cleopatra or list mission
 * goes back to its briefing; a Pharaoh campaign mission loads Save\<player>\autosave_replay.sav
 * (written by 0x004D5C30 on the mission's first frame) keeping the difficulty, or opens the main
 * menu when there is none */
void newgame_replay(game_state *st)
{
    U32(st, 0x006059E8u) = 0;
    if (S32(st, CUSTOM) != 0) {
        U8(st, CAMPAIGN_KIND) = 0;
        newgame_start(st, MAP_NAME, 0);
        U8(st, SCREEN) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    if (U8(st, CAMPAIGN_KIND) != 1) {
        U8(st, FORMATIONS_KEEP) = 0;
        GAME_CALL0(st, 0x004181E0u);
        return;
    }
    /* @bin 0x00418539 the path in the original's frame (char[100]): 0x005169C0 (the family's
     * directory "save\" + player) + "\" (0x005761D0) + "autosave_replay.sav" (0x0057BFC4) */
    char path[300] = "";
    uint32_t parts[3] = {(uint32_t) GAME_CALL0(st, 0x005169C0u), 0x005761D0u, 0x0057BFC4u};
    for (int k = 0; k < 3; k++) {
        size_t n = strlen(path);
        for (uint32_t i = 0; n < sizeof(path) - 1 && P8(st, parts[k] + i) != 0; i++) {
            path[n++] = (char) P8(st, parts[k] + i);
        }
        path[n] = 0;
    }
    /* @bin 0x004185C5 fopen(path, "rb") (0x005763BC) and fclose: does it open */
    if (platform_file_read(st, path, 0, NULL, 0) < 0) {
        GAME_CALL0(st, 0x004CB6A0u); /* the main menu */
        return;
    }
    U32(st, 0x00605A3Cu) = 0;
    U8(st, 0x006DE515u) = 0;
    loader_load_save_path(st, path, 1); /* 0x004D51B0 */
    U8(st, SCREEN) = 1;
    U8(st, REDRAW) = 1;
}

/* @bin 0x0041F5C0 the scenario list's start button: row kind 0 (a .map) or 1 (a mission) */
void newgame_list_start(game_state *st)
{
    uint8_t flag = 0;
    sound_stop_stream(st, 0x00E92BF8u, 0);
    int32_t r = GAME_CALLT(st, LIST_OBJECT, 0x0041DFE0u, S32(st, LIST_ROW));
    if (r == 0) {
        U8(st, CAMPAIGN_KIND) = 0;
    } else if (r == 1) {
        flag = 1;
        U8(st, CAMPAIGN_KIND) = 3;
    } else {
        return;
    }
    GAME_CALL(st, 0x0051F780u, 1); /* the empty function */
    newgame_start(st, MAP_NAME, flag);
    if (U8(st, CAMPAIGN_KIND) == 3) {
        GAME_CALL0(st, 0x004181E0u); /* the briefing */
        U8(st, REDRAW) = 1;
        return;
    }
    U8(st, SCREEN) = 1;
    U8(st, 0x00E38DD6u) = 2;
    U8(st, REDRAW) = 1;
}

static void strcpy_state(game_state *st, uint32_t dst, uint32_t src)
{
    uint8_t c;
    do {
        c = P8(st, src++);
        P8(st, dst++) = c;
    } while (c != 0);
}

/* @bin 0x0041F670 a row of the scenario list clicked (row 0x00D3B198 + first row 0x00704DB8):
 * the map name; a .map is loaded for the preview (chunks only), a mission's record without the
 * scenario start */
void newgame_list_row(game_state *st)
{
    int32_t row = S32(st, 0x00D3B198u);
    if (row >= S32(st, 0x00704DB4u)) {
        return;
    }
    row += S32(st, 0x00704DB8u);
    S32(st, LIST_ROW) = row;
    uint32_t name = (uint32_t) GAME_CALLT(st, LIST_OBJECT, 0x0041E010u, row);
    if (name == 0) {
        return;
    }
    GAME_CALL(st, 0x0040F8F0u, (int32_t) MAP_NAME, 0x40, 0);
    strcpy_state(st, MAP_NAME, name);
    file_name_strip_extension(st, MAP_NAME); /* 0x004CE3A0 */
    int32_t r = GAME_CALLT(st, LIST_OBJECT, 0x0041DFE0u, S32(st, LIST_ROW));
    if (r == 0) {
        /* @bin 0x0041F772 maps\<name>.map, preview */
        text_copy(st, 0x00576274u, EXT, 3);
        file_name_add_extension(st, MAP_NAME); /* 0x004CE3C0 */
        char path[512];
        size_t n = 0;
        for (uint32_t i = 0; U8(st, 0x0057626Cu + i) != 0; i++) {
            path[n++] = (char) U8(st, 0x0057626Cu + i);
        }
        for (uint32_t i = 0; n < sizeof(path) - 1 && U8(st, MAP_NAME + i) != 0; i++) {
            path[n++] = (char) U8(st, MAP_NAME + i);
        }
        path[n] = 0;
        map_file_load(st, path, 1);
        file_name_strip_extension(st, MAP_NAME);
        U8(st, REDRAW) = 1;
        return;
    }
    if (r < 0 || r > 2) {
        return;
    }
    /* @bin 0x0041F70C a mission: its record loaded without the scenario start */
    int8_t m = (int8_t) GAME_CALLT(st, LIST_OBJECT, 0x0041DFC0u, S32(st, LIST_ROW));
    S8(st, MISSION) = m;
    GAME_CALL(st, 0x004D58B0u, 0, 1);
    S8(st, MISSION) = m;
    strcpy_state(st, MAP_NAME, name);
    file_name_strip_extension(st, MAP_NAME);
    U8(st, REDRAW) = 1;
}

/* ---- the table ---- */

#define VOID_RUN(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }

static int32_t run_start(game_state *st, const int32_t *a)
{
    newgame_start(st, (uint32_t) a[0], a[1]);
    return 0;
}

/* a path in the state, copied out (with slack for the extension and the reads past its end) */
static int path_in(game_state *st, uint32_t a, char *path, uint32_t cap, uint32_t *len)
{
    memset(path, 0, cap);
    for (uint32_t n = 0; n < cap - 8; n++) {
        const uint8_t *c = state_ptr(st, a + n, 1);
        if (!c) {
            return -1;
        }
        path[n] = (char) *c;
        if (*c == 0) {
            *len = n;
            return 0;
        }
    }
    return -1;
}

static int32_t run_file_exists(game_state *st, const int32_t *a)
{
    char path[512];
    uint32_t n;
    if (path_in(st, (uint32_t) a[0], path, sizeof(path), &n) != 0) {
        state_unported(st, 0x004CE3F0u);
        return 0;
    }
    return file_exists(st, path);
}

static int32_t run_file_cd_retry(game_state *st, const int32_t *a)
{
    char path[512];
    uint32_t n;
    if (path_in(st, (uint32_t) a[0], path, sizeof(path), &n) != 0) {
        state_unported(st, 0x004CE100u);
        return 0;
    }
    return file_cd_retry(st, path);
}

static int32_t run_ext_of_path(game_state *st, const int32_t *a)
{
    char path[512];
    uint32_t n;
    if (path_in(st, (uint32_t) a[0], path, sizeof(path), &n) != 0) {
        state_unported(st, 0x004CE330u);
        return 0;
    }
    ext_of_path(st, path);
    return 0;
}

static int32_t run_upper_case(game_state *st, const int32_t *a)
{
    upper_case(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_map_file_exists(game_state *st, const int32_t *a)
{
    /* the path in the state: checked, then the bytes the original leaves written back (the path
     * up to its first '.', 0, "map", 0) */
    char path[512];
    uint32_t n;
    if (path_in(st, (uint32_t) a[0], path, sizeof(path), &n) != 0) {
        state_unported(st, 0x005409D0u);
        return 0;
    }
    int32_t r = newgame_map_file_exists(st, path);
    size_t len = strlen(path);
    for (size_t i = 0; i <= len + 4; i++) {
        P8(st, (uint32_t) a[0] + (uint32_t) i) = (uint8_t) path[i];
    }
    return r;
}

static int32_t run_points(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_map_points_reset(st);
    return 0;
}

static int32_t run_messages(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_messages_reset(st);
    return 0;
}

static int32_t run_timer(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_message_timer_reset(st);
    return 0;
}

static int32_t run_buildings(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_mission_buildings(st);
    return 0;
}

static int32_t run_profiler(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_profiler_reset(st);
    return 0;
}

static int32_t run_step(game_state *st, const int32_t *a)
{
    return campaign_step_of_mission(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

VOID_RUN(run_carry_over, newgame_carry_over)
VOID_RUN(run_next_mission, newgame_next_mission)
VOID_RUN(run_replay, newgame_replay)
VOID_RUN(run_list_start, newgame_list_start)
VOID_RUN(run_list_row, newgame_list_row)

static int32_t run_4df2c0(game_state *st, const int32_t *a)
{
    monument_parts_complete_4df2c0(st, a[0]);
    return 0;
}

static int32_t run_40d7f0(game_state *st, const int32_t *a)
{
    monument_complete_40d7f0(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

static int32_t run_4e65a0(game_state *st, const int32_t *a)
{
    monument_complete_4e65a0(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

static int32_t run_547ad0(game_state *st, const int32_t *a)
{
    monument_complete_547ad0(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

const ported_function newgame_start_ports[] = {
    {0x005405B0u, "newgame_start", 2, 0, 0, run_start, 0},
    {0x005409D0u, "newgame_map_file_exists", 1, 32, 0, run_map_file_exists, 0},
    {0x004DE140u, "newgame_map_points_reset", 0, 0, 0, run_points, 0},
    {0x004E1770u, "newgame_messages_reset", 0, 0, 0, run_messages, 0},
    {0x004E1830u, "newgame_message_timer_reset", 0, 0, 0, run_timer, 0},
    {0x00418BF0u, "newgame_mission_buildings", 0, 0, 0, run_buildings, 0},
    {0x00489B40u, "newgame_profiler_reset", 0, 0, 0, run_profiler, 0},
    {0x004CE3F0u, "file_exists", 1, 32, 0, run_file_exists, 0},
    {0x004CE100u, "file_cd_retry", 1, 32, 0, run_file_cd_retry, 0},
    {0x004CE330u, "file_ext_of_path", 1, 0, 0, run_ext_of_path, 0},
    {0x004CBEC0u, "text_upper_case", 1, 0, 0, run_upper_case, 0},
    {0x0041B9D0u, "campaign_step_of_mission", 1, 32, 0x006FDD08u, run_step, 0},
    {0x004188F0u, "newgame_carry_over", 0, 0, 0, run_carry_over, 0},
    {0x00418640u, "newgame_next_mission", 0, 0, 0, run_next_mission, 0},
    {0x00418510u, "newgame_replay", 0, 0, 0, run_replay, 0},
    {0x0041F5C0u, "newgame_list_start", 0, 0, 0, run_list_start, 0},
    {0x0041F670u, "newgame_list_row", 0, 0, 0, run_list_row, 0},
    {0x004DF2C0u, "monument_parts_complete_4df2c0", 1, 0, 0, run_4df2c0, 0},
    {0x0040D7F0u, "monument_complete_40d7f0", 1, 0, 0x006FD5D8u, run_40d7f0, 0},
    {0x004E65A0u, "monument_complete_4e65a0", 1, 0, 0x006DE548u, run_4e65a0, 0},
    {0x00547AD0u, "monument_complete_547ad0", 1, 0, 0, run_547ad0, 0},
};
const int newgame_start_ports_count = sizeof(newgame_start_ports) / sizeof(newgame_start_ports[0]);
