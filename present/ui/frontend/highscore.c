/* Imhotep - the high scores (window 0x53, the file save\highscore.jas), the victory scoring (0x0041C380,
 * 0x004192B0) and the window 0x51 (0x57: 0x00520AF0, package boot) (roadmap phase 7.6, package frontier7).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The high score table is 100 records of 0x4C bytes at 0x00D38708 (0x1DB0 bytes, the file's whole
 * contents): +0 the score, +4 the mission, +8 the player's name, +0x28 the rating culture, +0x2C
 * prosperity, +0x30 kingdom (0x00EA5B48), +0x34 population, +0x38 treasury, +0x3C the months, +0x40
 * the difficulty (city +0x47A8), +0x44 the monuments done, +0x48 in use. The campaign table keeps a
 * record of the same layout by mission at 0x007009DC (the family's best, saved in its progress file).
 * Window 0x53 (from the main menu, 0x00417FF0 parameter 3 after 0x004C5920): full redraw and per
 * frame 0x004C5660, input 0x004C57D0 (the scroll bar), buttons 0x005E01D0. Window 0x42 (input only,
 * 0x004D8310): 0x004C5A40 records the won mission's score, then 0x0041AA20(2, 0x33, 1000). */
#include "frontend.h"

#include <string.h>

#include "city/day_x87.h"
#include "city/monument_rating.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sim/call.h"
#include "sim/math.h"
#include "sim/platform.h"
#include "state/ported.h"

#define TABLE_SIZE 0x1DB0u

static uint32_t city(game_state *st)
{
    return (uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x49D8u;
}

static void state_copy_str(game_state *st, uint32_t dst, uint32_t src)
{
    /* repne scasb + rep movs: the string and its 0 */
    uint32_t i = 0;
    do {
        P8(st, dst + i) = P8(st, src + i);
    } while (P8(st, src + i++) != 0);
}

static int32_t months_played(game_state *st)
{
    /* @bin 0x004C5A5B |i16 0x007848D8 (the year) - 0x0078D7BC (the start year)| * 12 + 0x0078D7C0 */
    int32_t d = S16(st, G_START_YEAR) - S32(st, G_YEAR);
    if (d < 0) {
        d = -d;
    }
    return S32(st, G_MONTH) + d * 12;
}

/* ---- the file ---- */

int32_t highscore_read(game_state *st, uint32_t self, uint32_t path)
{
    /* @bin 0x004C5880 (thiscall on the table) fopen(path, "rb") (0x005763BC), fread(self, 0x1DB0, 1),
     * fclose: 1 when the whole table was read (a short file fills what it has), 0 without the file */
    char name[260];
    uint32_t i = 0;
    for (; i + 1 < sizeof(name) && P8(st, path + i) != 0; i++) {
        name[i] = (char) P8(st, path + i);
    }
    name[i] = 0;
    uint8_t buf[TABLE_SIZE];
    int32_t n = platform_file_read(st, name, 0, buf, TABLE_SIZE);
    if (n < 0) {
        return 0;
    }
    uint32_t got = (uint32_t) n < TABLE_SIZE ? (uint32_t) n : TABLE_SIZE;
    for (uint32_t k = 0; k < got; k++) {
        P8(st, self + k) = buf[k];
    }
    return got == TABLE_SIZE;
}

int32_t highscore_write(game_state *st, uint32_t self, uint32_t path)
{
    /* @bin 0x004C58D0 (thiscall on the table) fopen(path, "wb") (0x0057C280), fwrite(self, 0x1DB0, 1),
     * fclose: 1 when written */
    char name[260];
    uint32_t i = 0;
    for (; i + 1 < sizeof(name) && P8(st, path + i) != 0; i++) {
        name[i] = (char) P8(st, path + i);
    }
    name[i] = 0;
    uint8_t buf[TABLE_SIZE];
    for (uint32_t k = 0; k < TABLE_SIZE; k++) {
        buf[k] = P8(st, self + k);
    }
    return platform_file_write(st, name, buf, TABLE_SIZE) == (int32_t) TABLE_SIZE;
}

int32_t highscore_open(game_state *st)
{
    /* @bin 0x004C5920 the table read (0 without the file); the records in use among the campaign's
     * missions (0x00700994) counted into 0x00D3A4BC */
    S32(st, G_HIGHSCORE_ROWS) = 0;
    if ((uint8_t) highscore_read(st, G_HIGHSCORE_TABLE, G_FILE_NAME) == 0) {
        return 0;
    }
    int32_t n = S32(st, G_MISSION_COUNT);
    if (n > 0) {
        int32_t rows = S32(st, G_HIGHSCORE_ROWS);
        for (uint32_t e = G_HIGHSCORE_TABLE + 0x48u; n != 0; e += 0x4C, n--) {
            if (S32(st, e) != 0) {
                rows++;
            }
        }
        S32(st, G_HIGHSCORE_ROWS) = rows;
    }
    return 1;
}

/* @bin 0x004C5970 / 0x0041C600 a record (table base, at mission k): the ratings, difficulty,
 * treasury, population, kingdom, the mission, in use, the monuments, the name, prosperity, score,
 * months */
static void record_fill(game_state *st, uint32_t base, int32_t k, int32_t done, int32_t score, int32_t months)
{
    uint32_t c = city(st);
    uint32_t r = base + (uint32_t) k * 0x4Cu;
    S32(st, r + 0x28) = S32(st, CITY_BASE + CITY_RATING_CULTURE + c);
    S32(st, r + 0x40) = S32(st, CITY_BASE + CITY_DIFFICULTY + c);
    S32(st, r + 0x38) = S32(st, CITY_BASE + CITY_TREASURY + c);
    S32(st, r + 0x34) = S32(st, CITY_BASE + CITY_POPULATION + c);
    S32(st, r + 0x30) = S32(st, CITY_BASE + CITY_RATING_KINGDOM + c);
    S32(st, r + 4) = k;
    S32(st, r + 0x48) = 1;
    S32(st, r + 0x44) = done;
    state_copy_str(st, r + 8, G_PLAYER_NAME);
    S32(st, r + 0x2C) = S32(st, CITY_BASE + CITY_RATING_PROSPERITY + c);
    S32(st, r) = score;
    S32(st, r + 0x3C) = months;
}

void highscore_record(game_state *st, int32_t k, int32_t done, int32_t score, int32_t months)
{
    /* @bin 0x004C5970 mission k's high score record, the table written (0x004C58D0) */
    record_fill(st, G_HIGHSCORE_TABLE, k, done, score, months);
    highscore_write(st, G_HIGHSCORE_TABLE, G_FILE_NAME);
}

void campaign_best_record(game_state *st, int32_t k, int32_t done, int32_t score, int32_t months)
{
    /* @bin 0x0041C600 mission k's record of the campaign table (0x007009DC), the family's progress
     * file written (0x0041B8A0 on 0x00516A20) */
    record_fill(st, G_MISSION_RECORDS, k, done, score, months);
    uint32_t path = (uint32_t) GAME_CALL0(st, FN_CAMPAIGN_PROGRESS_PATH);
    GAME_CALLT(st, G_CAMPAIGN_TABLE, FN_CAMPAIGN_PROGRESS_WRITE, (int32_t) path);
}

/* ---- the victory scoring ---- */

/* a monument of the city (slot k of the three, its building b): its percent done, by its type (the
 * tables 0x0041C590 / 0x0041944C); -1 for the types that have none; 0x7FFFFFFF when left to the
 * original (the pyramid's status not ported for that case) */
static int32_t monument_done(game_state *st, int32_t b)
{
    switch (S16(st, BUILDING_ADDR(b) + BLD_TYPE)) {
    case 0xB7: {
        int32_t rec[0x1C];
        memset(rec, 0, sizeof(rec));
        pyramid_status(st, b, rec); /* @bin 0x004F1310 the record in the caller's frame: +0x34 */
        return rec[0xD];
    }
    case 0xCF:
    case 0xD0: return U8(st, BUILDING_ADDR(b) + BLD_U55);
    case 0xD2: return (int32_t) S16(st, CITY_BASE + CITY_U433A + city(st)) * 100 / 16;
    case 0xD5: return GAME_CALL(st, FN_MONUMENT_D5_STATUS, b, 0);
    case 0xDE: return GAME_CALL(st, FN_MONUMENT_DE_STATUS, b, 0);
    case 0xE1: return monument_progress_40d940(st, G_MONUMENT_E1, b, NULL);
    case 0xE3: return monument_progress_417050(st, G_MONUMENT_E3, b, NULL);
    case 0xE4: return monument_progress_4e6710(st, G_MONUMENT_E4, b, NULL);
    case 0xE5: return monument_progress_parts(st, G_MONUMENT_E5, b, 9, NULL);
    case 0xE6: return monument_progress_401540(st, b, NULL);
    case 0xEA: return monument_progress_parts(st, G_MONUMENT_EA, b, 0xA, NULL);
    case 0xEB: return monument_progress_parts(st, G_MONUMENT_EB, b, 0x15, NULL);
    case 0xEC: return monument_progress_parts(st, G_MONUMENT_EC, b, 0x1F, NULL);
    default: return -1;
    }
}

static x87_val dbl(game_state *st, uint32_t a)
{
    return x87_from_double((uint64_t) U32(st, a) | ((uint64_t) U32(st, a + 4) << 32));
}

static int x87_finite_ok;

static x87_val divv(x87_val a, x87_val b)
{
    /* x87 division with masked exceptions: by zero gives an infinity or a NaN, which __ftol turns
     * into the indefinite integer (low half 0) */
    if (b.m == 0) {
        x87_finite_ok = 0;
        return a;
    }
    return x87_divv(a, b);
}

int32_t victory_score(game_state *st, int32_t months)
{
    /* @bin 0x0041C380 the score: the three monuments' weights (i16 0x005D19D4 + 16 kind, kind i32
     * 0x00EA5FA0 + 4 k) of those done (100); then (x87, 53 bits) ((culture + prosperity + kingdom) *
     * (population * 0.002) + (weights / (months^2 * 0.00015625)) * (weights * 20.0) + treasury /
     * (months * (1/30))) * ((difficulty + 1.0) * (1/3)) (doubles 0x0056F540 .. 0x0056F518), __ftol,
     * minus the city's i32 0x00EA5D10 */
    int32_t weights = 0;
    for (uint32_t k = 0; k < 3; k++) {
        int32_t b = S32(st, CITY_BASE + CITY_MONUMENTS + ((uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x1276u + k) * 4u);
        if (b <= 0) {
            continue;
        }
        int32_t p = monument_done(st, b);
        if (st->unported) {
            return 0;
        }
        if (p == 100) {
            uint32_t kind = U32(st, CITY_BASE + CITY_MONUMENT_TYPES + ((uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x1276u + k) * 4u);
            weights += S16(st, G_MONUMENT_MODELS + 0x0Cu + (kind << 4));
        }
    }
    uint32_t c = city(st);
    int32_t ratings = (int32_t) (U32(st, CITY_BASE + CITY_RATING_KINGDOM + c) + U32(st, CITY_BASE + CITY_RATING_PROSPERITY + c) +
                                   U32(st, CITY_BASE + CITY_RATING_CULTURE + c));
    x87_finite_ok = 1;
    x87_val a = x87_mul(x87_from_int(ratings), x87_mul(x87_from_int(S32(st, CITY_BASE + CITY_POPULATION + c)), dbl(st, G_DBL_0_002)));
    x87_val nn = x87_mul(x87_from_int((int32_t) ((uint32_t) months * (uint32_t) months)), dbl(st, G_DBL_0_00015625));
    x87_val q = divv(x87_from_int(weights), nn);
    q = x87_mul(q, x87_mul(x87_from_int(weights), dbl(st, G_DBL_20)));
    x87_val s = x87_add(a, q);
    x87_val t = divv(x87_from_int(S32(st, CITY_BASE + CITY_TREASURY + c)), x87_mul(x87_from_int(months), dbl(st, G_DBL_1_30)));
    s = x87_add(s, t);
    x87_val d = x87_mul(x87_add(x87_from_int(S32(st, CITY_BASE + CITY_DIFFICULTY + c)), dbl(st, G_K_ONE)), dbl(st, G_DBL_1_3));
    s = x87_mul(s, d);
    int32_t r = x87_finite_ok ? x87_ftol(s) : 0;
    return r - S32(st, CITY_BASE + CITY_U44A0 + city(st));
}

int32_t victory_monuments_draw(game_state *st)
{
    /* @bin 0x004192B0 the monuments done (100): their names (group 0xC6, the kind) at the panel's x +
     * 0x113, 20 pixels apart from y + 0xC8; their number */
    int32_t n = 0, y = 0xB4;
    for (uint32_t k = 0; k < 3; k++) {
        int32_t b = S32(st, CITY_BASE + CITY_MONUMENTS + ((uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x1276u + k) * 4u);
        if (b <= 0) {
            continue;
        }
        int32_t p = monument_done(st, b);
        if (st->unported) {
            return 0;
        }
        if (p != 100) {
            continue;
        }
        y += 0x14;
        n++;
        int32_t kind = S32(st, CITY_BASE + CITY_MONUMENT_TYPES + ((uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x1276u + k) * 4u);
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0xC6, kind, S32(st, G_PANEL_X) + 0x113, S32(st, G_PANEL_Y) + y, S32(st, G_FONT_LARGE), 0);
    }
    return n;
}

int32_t mission_victory_text(game_state *st)
{
    /* @bin 0x004181A0 in a campaign, the mission step 0x00E38EAB's text (campaign table 0x006FDD08 +
     * 0x40 step + 0x10, steps 0x006FF608); else 0x25 */
    if (S32(st, G_CUSTOM) == 0) {
        int32_t s = (int8_t) U8(st, G_STEP);
        if (s >= 0 && s < S32(st, G_CAMPAIGN_TABLE + CAMPAIGN_STEP_COUNT)) {
            return S32(st, G_CAMPAIGN_TABLE + 0x10u + (uint32_t) s * 0x40u);
        }
    }
    return 0x25;
}

void highscore_update(game_state *st)
{
    /* @bin 0x004C5A40 (window 0x42) outside the custom games: the score (0x0041C380 of the months
     * played) and the monuments done (0x004192B0); a cheated game (0x00E38DD7) is not recorded (1);
     * else the table read and mission's record set: 2 new, 4 the same player's better score, 5 another
     * player's (its name kept in 0x00D386E0), 3 / 6 not better; the family's campaign record
     * (0x007009DC) when better (0x0041C600) */
    S32(st, G_HIGHSCORE_RESULT) = 0;
    if (S32(st, G_CUSTOM) != 0) {
        return;
    }
    int32_t months = months_played(st);
    int32_t score = victory_score(st, months);
    if (st->unported) {
        return;
    }
    int32_t done = victory_monuments_draw(st);
    if (st->unported) {
        return;
    }
    if (U8(st, G_CHEATED) != 0) {
        U8(st, G_CHEATED) = 0;
        S32(st, G_HIGHSCORE_RESULT) = 1;
        return;
    }
    highscore_read(st, G_HIGHSCORE_TABLE, G_FILE_NAME);
    int32_t k = (int8_t) U8(st, G_MISSION);
    if (S32(st, G_HIGHSCORE_TABLE + 0x48u + (uint32_t) k * 0x4Cu) == 0) {
        S32(st, G_HIGHSCORE_RESULT) = 2;
        highscore_record(st, k, done, score, months);
    } else if (GAME_CALL(st, FN_CRT_STRCMPI, (int32_t) (G_HIGHSCORE_TABLE + 8u + (uint32_t) k * 0x4Cu), (int32_t) G_PLAYER_NAME) == 0) {
        k = (int8_t) U8(st, G_MISSION);
        if (S32(st, G_HIGHSCORE_TABLE + (uint32_t) k * 0x4Cu) >= score) {
            S32(st, G_HIGHSCORE_RESULT) = 3;
        } else {
            S32(st, G_HIGHSCORE_RESULT) = 4;
            highscore_record(st, k, done, score, months);
        }
    } else {
        k = (int8_t) U8(st, G_MISSION);
        if (S32(st, G_HIGHSCORE_TABLE + (uint32_t) k * 0x4Cu) >= score) {
            S32(st, G_HIGHSCORE_RESULT) = 6;
        } else {
            S32(st, G_HIGHSCORE_RESULT) = 5;
            state_copy_str(st, G_HIGHSCORE_PREV_NAME, G_HIGHSCORE_TABLE + 8u + (uint32_t) k * 0x4Cu);
            highscore_record(st, k, done, score, months);
        }
    }
    k = (int8_t) U8(st, G_MISSION);
    if (S32(st, G_MISSION_RECORDS + (uint32_t) k * 0x4Cu) < score) {
        campaign_best_record(st, k, done, score, months);
    }
}

/* ---- window 0x53 ---- */

void highscore_thumb_draw(game_state *st)
{
    /* @bin 0x004C5450 with more than 20 records: the scroll bar's thumb (image 0x00F56494 + 0x27) at
     * 0x101 pixels x the first row's share (0x004CDE60 of the rows past 20; 0 / 100 at the ends) */
    int32_t extra = S32(st, G_HIGHSCORE_ROWS) - 0x14;
    if (extra <= 0) {
        return;
    }
    int32_t first = S32(st, G_HIGHSCORE_FIRST);
    int32_t pct;
    if (first <= 0) {
        pct = 0;
    } else if (first + 0x14 >= S32(st, G_HIGHSCORE_ROWS)) {
        pct = 100;
    } else {
        pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, first, extra);
    }
    int32_t dy = GAME_CALL(st, FN_CALC_PERCENTAGE, 0x101, pct);
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_SCROLL_IMAGES) + 0x27, S32(st, G_LAYOUT_DX) + 0x248,
               S32(st, G_LAYOUT_DY) + dy + 0x82);
}

void highscore_rows_draw(game_state *st)
{
    /* @bin 0x004C54C0 no records: text (0x128, 6); else from the first shown, the records in use (16
     * pixels apart, at most 20): the mission's name (0x006FF60C + 0x32 k), "by" (0x128, 5) and the
     * player, the difficulty (group 0x99), the score */
    if (S32(st, G_HIGHSCORE_ROWS) == 0) {
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x128, 6, S32(st, G_LAYOUT_DX) + 0x30, S32(st, G_LAYOUT_DY) + 0x68, S32(st, G_FONT_ROW), 0);
        return;
    }
    int32_t y = 0;
    for (int32_t i = 0; i < S32(st, G_MISSION_COUNT); i++) {
        int32_t k = S32(st, G_HIGHSCORE_FIRST) + i;
        uint32_t r = G_HIGHSCORE_TABLE + (uint32_t) k * 0x4Cu;
        if (S32(st, r + 0x48) != 0) {
            uint32_t font = G_FONT_SMALL;
            GAME_CALL(st, FN_TEXT_DRAW, (int32_t) (G_CAMPAIGN_MISSION_NAMES + (uint32_t) k * 0x32u), S32(st, G_LAYOUT_DX) + 0x30,
                      y + S32(st, G_LAYOUT_DY) + 0x68, S32(st, font), 0);
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x128, 5, S32(st, G_LAYOUT_DX) + 0xE0, y + S32(st, G_LAYOUT_DY) + 0x68, S32(st, font), 0);
            GAME_CALL(st, FN_TEXT_DRAW, (int32_t) (r + 8), S32(st, G_TEXT_X) + S32(st, G_LAYOUT_DX) + 0xE0,
                      y + S32(st, G_LAYOUT_DY) + 0x68, S32(st, font), 0);
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x99, S32(st, r + 0x40) + 1, S32(st, G_LAYOUT_DX) + 0x170, y + S32(st, G_LAYOUT_DY) + 0x68,
                      S32(st, font), 0);
            GAME_CALL(st, FN_NUMBER_DRAW, S32(st, r), 0x40, (int32_t) G_NUMBER_SUFFIX, S32(st, G_LAYOUT_DX) + 0x200,
                      y + S32(st, G_LAYOUT_DY) + 0x68, S32(st, font), 0);
            y += 0x10;
        }
        if (y >= 0x140) {
            return;
        }
    }
}

void highscore_draw(game_state *st)
{
    /* @bin 0x004C5660 window 0x53 (full redraw and per frame): the panel (40 x 30), the list's frame,
     * the title (0x128, 0), the scroll bar's frame, the column titles (0x128, 1..4), the rows, the
     * thumb, the note (0xD, 1); a full redraw next frame */
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_PANEL_DRAW, dx, dy, 0x28, 0x1E);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x20, S32(st, G_LAYOUT_DY) + 0x60, 0x22, 0x16);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x128, 0, S32(st, G_LAYOUT_DX) + 0x14, S32(st, G_LAYOUT_DY) + 0x20, 0x260, S32(st, G_FONT_TITLE), 0);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x243, S32(st, G_LAYOUT_DY) + 0x7B, 2, 0x13);
    static const int32_t xs[4] = {0x50, 0xE0, 0x170, 0x200};
    for (int32_t k = 0; k < 4; k++) {
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x128, k + 1, S32(st, G_LAYOUT_DX) + xs[k], S32(st, G_LAYOUT_DY) + 0x50, S32(st, G_FONT_LARGE), 0);
    }
    highscore_rows_draw(st);
    highscore_thumb_draw(st);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0xD, 1, S32(st, G_LAYOUT_DX) + 0x14, S32(st, G_LAYOUT_DY) + 0x1C4, 0x260, S32(st, G_FONT_LARGE), 0);
    U8(st, G_REDRAW) = 1;
}

int32_t highscore_drag(game_state *st)
{
    /* @bin 0x004C57D0 with more than 20 records and the left button held over the scroll bar (x
     * 0x238..0x258, y 0x80..0x191 of the layout): the first row from the cursor's place (0x101
     * pixels), a redraw of the buttons (2); 1 */
    int32_t rows = S32(st, G_HIGHSCORE_ROWS);
    if (rows <= 0x14 || U8(st, G_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t mx = S32(st, G_MOUSE_X), dx = S32(st, G_LAYOUT_DX);
    if (mx < dx + 0x238 || mx > dx + 0x258) {
        return 0;
    }
    int32_t my = S32(st, G_MOUSE_Y), dy = S32(st, G_LAYOUT_DY);
    if (my < dy + 0x80 || my > dy + 0x191) {
        return 0;
    }
    int32_t v = my - dy - 0x80;
    if (v > 0x101) {
        v = 0x101;
    }
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, v, 0x101);
    S32(st, G_HIGHSCORE_FIRST) = GAME_CALL(st, FN_CALC_PERCENTAGE, rows - 0x14, pct);
    U8(st, G_REDRAW) = 2;
    return 1;
}

/* ---- window 0x51 ---- */

void choice_window_draw(game_state *st)
{
    /* @bin 0x00520D00 window 0x51 (full redraw and per frame): the panel (26 x 28) by the resolution
     * 0x00E38E68 (1: 0x6E, 0x1E; 2: 0xBE, 0x5A; else 0x12E, 0xAE), its 43 rows (rectangles 0x005C4358,
     * text group 0xC5 by row; the first and the 22nd rows plain, else lit 0x00F56FAC when hovered
     * (0x007056C8 = row + 1), 0x00F56FA4 for the chosen 0x006059FC), the button (0x005C47F0, lit when
     * the hover is 0x2B) and its text (0x12, 3) */
    int32_t x, y;
    if (S32(st, G_RESOLUTION) == 1) {
        x = 0x6E;
        y = 0x1E;
    } else if (S32(st, G_RESOLUTION) == 2) {
        x = 0xBE;
        y = 0x5A;
    } else {
        x = 0x12E;
        y = 0xAE;
    }
    GAME_CALL(st, FN_PANEL_DRAW, x, y, 0x1A, 0x1C);
    int32_t i = 0;
    for (uint32_t r = G_NAME_ROWS; r < G_NAME_BACK; r += 0x1C, i++) {
        int32_t font;
        if (r == G_NAME_ROWS || r == G_NAME_ROWS + 21u * 0x1Cu) {
            font = S32(st, G_FONT_LARGE);
        } else if (i + 1 == S32(st, G_HOVER)) {
            font = S32(st, G_FONT_LINK);
        } else if (i == S32(st, G_NAME_PICK)) {
            font = S32(st, G_FONT_RED);
        } else {
            font = S32(st, G_FONT_LARGE);
        }
        GAME_CALL(st, FN_LAYOUT_GROUP_AT, 0xC5, i, (int32_t) r, font, 0, 0x73, 0x1E, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) G_NAME_BACK, S32(st, G_HOVER) == 0x2B, 0x6E, 0x1E);
    GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_RECT, 0x12, 3, (int32_t) G_NAME_BACK, S32(st, G_FONT_LARGE), 0, 0x72, 0x22, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
}

static int32_t run_read(game_state *st, const int32_t *a) { return highscore_read(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]); }
static int32_t run_write(game_state *st, const int32_t *a) { return highscore_write(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]); }
static int32_t run_open(game_state *st, const int32_t *a) { (void) a; return highscore_open(st); }
static int32_t run_record(game_state *st, const int32_t *a) { highscore_record(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_best(game_state *st, const int32_t *a) { campaign_best_record(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_score(game_state *st, const int32_t *a) { return victory_score(st, a[0]); }
static int32_t run_monuments(game_state *st, const int32_t *a) { (void) a; return victory_monuments_draw(st); }
static int32_t run_text(game_state *st, const int32_t *a) { (void) a; return mission_victory_text(st); }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return highscore_drag(st); }
#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(highscore_update)
RUN0(highscore_thumb_draw)
RUN0(highscore_rows_draw)
RUN0(highscore_draw)
RUN0(choice_window_draw)

const ported_function frontend_highscore_ports[] = {
    {0x004C5880u, "highscore_read", 1, 8, G_HIGHSCORE_TABLE, run_read, 0},
    {0x004C58D0u, "highscore_write", 1, 8, G_HIGHSCORE_TABLE, run_write, 0},
    {0x004C5920u, "highscore_open", 0, 8, 0, run_open, 0},
    {0x004C5970u, "highscore_record", 4, 0, 0, run_record, 0},
    {0x0041C600u, "campaign_best_record", 4, 0, 0, run_best, 0},
    {0x0041C380u, "victory_score", 1, 32, 0, run_score, 0},
    {0x004192B0u, "victory_monuments_draw", 0, 32, 0, run_monuments, 0},
    {0x004181A0u, "mission_victory_text", 0, 32, 0, run_text, 0},
    {0x004C5A40u, "highscore_update", 0, 0, 0, run_highscore_update, 0},
    {0x004C5450u, "highscore_thumb_draw", 0, 0, 0, run_highscore_thumb_draw, 0},
    {0x004C54C0u, "highscore_rows_draw", 0, 0, 0, run_highscore_rows_draw, 0},
    {0x004C5660u, "highscore_draw", 0, 0, 0, run_highscore_draw, 0},
    {0x004C57D0u, "highscore_drag", 0, 32, 0, run_drag, 0},
    {0x00520D00u, "choice_window_draw", 0, 0, 0, run_choice_window_draw, 0},
};
const int frontend_highscore_ports_count = sizeof(frontend_highscore_ports) / sizeof(frontend_highscore_ports[0]);
