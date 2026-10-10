/* Imhotep - the campaign flow, continued (package flow2, docs/newgame.md section 2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The family's progress file read (0x0041B680), the family list and the new family (0x00516990 ..
 * 0x00516C80), the game's file lists (0x0041C740: FindFirstFileA / FindNextFileA become
 * platform_file_list), the period choice (0x0041BB70, 0x00417DD0) and the "continue" button
 * 0x00418220. Files go through the platform (core/sim/platform.h). */
#include "newgame/flow2.h"

#include <stdlib.h>
#include <string.h>

#include "city/rest_gods.h"
#include "newgame/flow.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

/* ---- strings ---- */

static const char *state_str(game_state *st, uint32_t a)
{
    const char *s = (const char *) state_ptr(st, a, 1);
    for (uint32_t n = 0; s && n < 4096; n++) {
        const uint8_t *c = state_ptr(st, a + n, 1);
        if (!c) {
            return NULL;
        }
        if (*c == 0) {
            return s;
        }
    }
    return NULL;
}

/* the original's inline strcpy (repne scasb, rep movs): strlen + 1 bytes */
static void str_copy(game_state *st, uint32_t dst, uint32_t src)
{
    uint8_t c;
    do {
        c = P8(st, src++);
        P8(st, dst++) = c;
    } while (c != 0);
}

/* ---- the progress file ---- */

int32_t campaign_progress_read(game_state *st, uint32_t self, uint32_t path)
{
    /* @bin 0x0041B680 a table on the stack (0x0041AC50: its steps initialised, which the read
     * overwrites); the 12 words +0x4A84 .. +0x4A9A cleared */
    uint8_t buf[CAMPAIGN_TABLE_SIZE] = {0};
    for (uint32_t k = 0; k < 12; k++) {
        P16(st, self + 0x4A84u + 2 * k) = 0;
    }
    /* @bin 0x0041B6FC fopen(path, "rb") (0x005763BC) */
    const char *p = state_str(st, path);
    int32_t size = p ? platform_file_read(st, p, 0, NULL, 0) : -1;
    if (size < 0) {
        return 0;
    }
    /* @bin 0x0041B71B fread(buf, 0x4A9C, 1): a shorter file fills buf as far as it goes, then
     * @bin 0x0041B72C fseek(0) and fread(buf, 0x4A6C, 1) (the older size, without the words). The
     * rest of buf is the original's stack: 0 here (the check clears the stack) */
    platform_file_read(st, p, 0, buf, size < (int32_t) CAMPAIGN_TABLE_SIZE ? (uint32_t) size : CAMPAIGN_TABLE_SIZE);
    int32_t ok;
    if (size >= (int32_t) CAMPAIGN_TABLE_SIZE) {
        /* @bin 0x0041B74D the words from the file */
        memcpy(state_ptr(st, self + 0x4A84u, 24), buf + 0x4A84, 24);
        ok = 1;
    } else {
        ok = size >= 0x4A6C;
    }
    /* @bin 0x0041B801 fclose */
    if (!ok) {
        return 0;
    }
    /* @bin 0x0041B80F strcpy(+0x2C90) from the file's, 0x76C dwords from +0x2CD4 (the mission
     * records), then each step's won flag +0x3C := the record's +0x48 != 0, for the name count */
    size_t n = 0;
    while (0x2C90 + n < sizeof(buf) && buf[0x2C90 + n] != 0) {
        n++;
    }
    if (0x2C90 + n >= sizeof(buf)) {
        state_unported(st, FN_CAMPAIGN_PROGRESS_READ); /* no end in the file: the copy runs into the stack */
        return 0;
    }
    memcpy(state_ptr(st, self + 0x2C90u, (uint32_t) n + 1), buf + 0x2C90, n + 1);
    memcpy(state_ptr(st, self + 0x2CD4u, 0x76Cu * 4), buf + 0x2CD4, 0x76Cu * 4);
    for (int32_t i = 0; i < S32(st, self + 0x2C8Cu); i++) {
        P8(st, self + 0x3Cu + 0x40u * (uint32_t) i) = P32(st, self + 0x2D1Cu + 0x4Cu * (uint32_t) i) != 0;
    }
    return 1;
}

/* ---- file lists ---- */

/* a file list: in the state (addr) or on the original's stack (the fields here) */
typedef struct {
    game_state *st;
    uint32_t addr;
    uint32_t f[7];
} flist;

static int32_t fl_get(const flist *l, uint32_t off)
{
    return l->addr ? (int32_t) P32(l->st, l->addr + off) : (int32_t) l->f[off / 4];
}

static void fl_set(flist *l, uint32_t off, int32_t v)
{
    if (l->addr) {
        P32(l->st, l->addr + off) = (uint32_t) v;
    } else {
        l->f[off / 4] = (uint32_t) v;
    }
}

/* @bin 0x0041C8F0 (thiscall) FindFirstFileA(pattern) .. FindNextFileA .. FindClose: every name
 * counted (*count, *bytes with its 0); added when an offset is free (+8 < +0xC) and the name fits
 * with room to spare (+0x18 + len + 1 < +0x14); 1 when they all were added, 0 when one was not or
 * nothing was found */
static int scan(flist *l, const char *pattern, int32_t *bytes, int32_t *count)
{
    game_state *st = l->st;
    char name[264];
    *bytes = 0;
    *count = 0;
    int ok = 1;
    if (platform_file_list(st, pattern, 0, name, sizeof(name)) < 0) {
        return 0;
    }
    for (int32_t i = 0; platform_file_list(st, pattern, i, name, sizeof(name)) >= 0; i++) {
        int32_t len = (int32_t) strlen(name);
        *count += 1;
        *bytes += len + 1;
        if (fl_get(l, 8) < fl_get(l, 0xC) && fl_get(l, 0x18) + len + 1 < fl_get(l, 0x14)) {
            P32(st, (uint32_t) fl_get(l, 4) + 4u * (uint32_t) fl_get(l, 8)) = (uint32_t) fl_get(l, 0x18);
            uint32_t dst = P32(st, (uint32_t) fl_get(l, 4) + 4u * (uint32_t) fl_get(l, 8)) + (uint32_t) fl_get(l, 0x10);
            for (int32_t k = 0; k <= len; k++) {
                P8(st, dst + (uint32_t) k) = (uint8_t) name[k];
            }
            fl_set(l, 8, fl_get(l, 8) + 1);
            fl_set(l, 0x18, fl_get(l, 0x18) + len + 1);
        } else {
            ok = 0;
        }
    }
    return ok;
}

static int32_t fill(flist *l, const char *pattern, int32_t keep)
{
    /* @bin 0x0041C740 */
    game_state *st = l->st;
    uint8_t k = (uint8_t) keep;
    if (k == 0) {
        fl_set(l, 8, 0);
        fl_set(l, 0x18, 0);
    }
    int32_t saved_count = fl_get(l, 8), saved_used = fl_get(l, 0x18);
    int32_t bytes, count;
    if (scan(l, pattern, &bytes, &count)) {
        return 1;
    }
    for (;;) {
        /* @bin 0x0041C78F what did not fit is dropped; the buffers grow (operator new / delete) */
        fl_set(l, 8, saved_count);
        fl_set(l, 0x18, saved_used);
        if (bytes <= fl_get(l, 0x14) && count <= fl_get(l, 0xC)) {
            return 0;
        }
        if (bytes + saved_used > fl_get(l, 0x14)) {
            if (k == 0) {
                if (fl_get(l, 0x10) != 0) {
                    state_dealloc(st, (uint32_t) fl_get(l, 0x10));
                    fl_set(l, 0x10, 0);
                }
                fl_set(l, 0x14, 0);
            }
            int32_t cap = bytes + fl_get(l, 0x14) + 1000;
            if (cap <= 0) {
                return 0;
            }
            uint32_t p = state_alloc(st, (uint32_t) cap);
            if (p == 0) {
                return 0;
            }
            if (fl_get(l, 0x10) != 0) {
                uint32_t old = (uint32_t) fl_get(l, 0x10);
                for (uint32_t i = 0; i < (uint32_t) fl_get(l, 0x18); i++) {
                    P8(st, p + i) = P8(st, old + i);
                }
                state_dealloc(st, old);
            }
            fl_set(l, 0x10, (int32_t) p);
            fl_set(l, 0x14, cap);
        }
        if (fl_get(l, 8) + count > fl_get(l, 0xC)) {
            /* @bin 0x0041C849 emptied: the old offsets are released but their pointer kept (copied
             * from, 0 of them, and released again below) */
            if (k == 0) {
                if (fl_get(l, 4) != 0) {
                    state_dealloc(st, (uint32_t) fl_get(l, 4));
                }
                fl_set(l, 0xC, 0);
            }
            int32_t cap = count + fl_get(l, 0xC) + 100;
            uint32_t p = state_alloc(st, (uint32_t) cap * 4u);
            if (p == 0) {
                return 0;
            }
            if (fl_get(l, 4) != 0) {
                uint32_t old = (uint32_t) fl_get(l, 4);
                for (int32_t i = 0; i < fl_get(l, 8); i++) {
                    P32(st, p + 4u * (uint32_t) i) = P32(st, old + 4u * (uint32_t) i);
                }
                state_dealloc(st, old);
            }
            fl_set(l, 4, (int32_t) p);
            fl_set(l, 0xC, cap);
        }
        if (scan(l, pattern, &bytes, &count)) {
            return 1;
        }
    }
}

int32_t file_list_fill(game_state *st, uint32_t self, uint32_t pattern, int32_t keep)
{
    flist l = {st, self, {0}};
    const char *p = state_str(st, pattern);
    if (!p) {
        state_unported(st, FN_FILE_LIST_FILL);
        return 0;
    }
    return fill(&l, p, keep);
}

int32_t file_list_count(game_state *st, const char *pattern)
{
    /* @bin 0x0041C6D0 the list constructed empty (its vtable 0x0056F548 aside) */
    flist l = {st, 0, {G_FILE_LIST_VTABLE, 0, 0, 0, 0, 0, 0}};
    fill(&l, pattern, 0);
    int32_t n = fl_get(&l, 8);
    /* @bin 0x0041C710 the destructor: the names, then the offsets released */
    if (fl_get(&l, 0x10) != 0) {
        state_dealloc(st, (uint32_t) fl_get(&l, 0x10));
    }
    if (fl_get(&l, 4) != 0) {
        state_dealloc(st, (uint32_t) fl_get(&l, 4));
    }
    return n;
}

int32_t file_list_names(game_state *st, const char *pattern, char *out, size_t size)
{
    /* @bin 0x0041C6D0 the list constructed empty, filled (0x0041C740), its names read (offset +4[i]
     * into the names +0x10), destroyed (0x0041C710) */
    flist l = {st, 0, {G_FILE_LIST_VTABLE, 0, 0, 0, 0, 0, 0}};
    fill(&l, pattern, 0);
    int32_t n = fl_get(&l, 8);
    size_t at = 0;
    for (int32_t i = 0; i < n; i++) {
        uint32_t s = P32(st, (uint32_t) fl_get(&l, 4) + 4u * (uint32_t) i) + (uint32_t) fl_get(&l, 0x10);
        for (uint32_t k = 0;; k++) {
            uint8_t c = P8(st, s + k);
            if (at < size) {
                out[at++] = (char) c;
            }
            if (c == 0) {
                break;
            }
        }
    }
    if (fl_get(&l, 0x10) != 0) {
        state_dealloc(st, (uint32_t) fl_get(&l, 0x10));
    }
    if (fl_get(&l, 4) != 0) {
        state_dealloc(st, (uint32_t) fl_get(&l, 4));
    }
    return n;
}

static int compare_offsets(game_state *st, int32_t a, int32_t b);

void file_list_sort(game_state *st, uint32_t self, uint32_t cmp)
{
    /* @bin 0x0041CA10 with both buffers and 2 names or more: qsort (0x00562069) of the offsets with
     * 0x0041CA50, which compares the names through the comparator kept in 0x007027D8 (names base
     * 0x007027DC) */
    if (P32(st, self + 4) == 0 || P32(st, self + 0x10) == 0 || (int32_t) P32(st, self + 8) < 2) {
        return;
    }
    U32(st, G_SORT_COMPARE) = cmp;
    U32(st, G_SORT_NAMES) = P32(st, self + 0x10);
    /* the offsets (in the heap) sorted as values, then written back */
    uint32_t n = P32(st, self + 8), at = P32(st, self + 4);
    int32_t *v = malloc(sizeof(int32_t) * n);
    if (!v) {
        state_unported(st, FN_FILE_LIST_SORT);
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        v[i] = (int32_t) P32(st, at + 4 * i);
    }
    crt_qsort4(st, v, n, compare_offsets);
    for (uint32_t i = 0; i < n; i++) {
        P32(st, at + 4 * i) = (uint32_t) v[i];
    }
    free(v);
}

/* @bin 0x0041CA50 the qsort comparator on the offsets themselves */
static int compare_offsets(game_state *st, int32_t a, int32_t b)
{
    uint32_t base = U32(st, G_SORT_NAMES);
    return GAME_CALL(st, U32(st, G_SORT_COMPARE), (int32_t) ((uint32_t) a + base), (int32_t) ((uint32_t) b + base));
}

/* @bin 0x0041CA50 the qsort comparator: [0x007027D8](names + *a, names + *b) */
static int32_t list_compare(game_state *st, uint32_t a, uint32_t b)
{
    return compare_offsets(st, (int32_t) P32(st, a), (int32_t) P32(st, b));
}

/* ---- families ---- */

void name_trim(game_state *st, uint32_t s)
{
    /* @bin 0x0041ACD0 leading bytes 1..0x20 shifted out, then the string cut after its last byte
     * above 0x20 */
    while (P8(st, s) != 0 && P8(st, s) <= 0x20) {
        uint32_t a = s;
        uint8_t c;
        do {
            c = P8(st, a + 1);
            P8(st, a) = c;
            a++;
        } while (c != 0);
    }
    uint32_t last = 0;
    for (uint32_t a = s; P8(st, a) != 0; a++) {
        if (P8(st, a) > 0x20) {
            last = a;
        }
    }
    if (last != 0) {
        P8(st, last + 1) = 0;
    }
}

/* the name of row i of the family list (0 for no such row) */
static uint32_t family_row(game_state *st, int32_t i)
{
    if (i < 0 || i >= S32(st, G_FAMILY_LIST + 8)) {
        return 0;
    }
    return P32(st, U32(st, G_FAMILY_LIST + 4) + 4u * (uint32_t) i) + U32(st, G_FAMILY_LIST + 0x10);
}

void family_list_load(game_state *st, uint32_t pattern)
{
    /* @bin 0x00516B50 the files, their names without the extension (0x004CE3A0), sorted with
     * _stricmp (0x00562370) */
    file_list_fill(st, G_FAMILY_LIST, pattern, 0);
    for (int32_t i = 0; i < S32(st, G_FAMILY_LIST + 8); i++) {
        GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) family_row(st, i));
    }
    file_list_sort(st, G_FAMILY_LIST, FN_CRT_STRCMPI);
}

void family_select(game_state *st)
{
    /* @bin 0x00516AB0 */
    int32_t row = S32(st, G_PARAM1), count = S32(st, G_FAMILY_LIST + 8);
    if (row >= count) {
        return;
    }
    int32_t i = row + S32(st, G_FAMILY_SCROLL);
    S32(st, G_FAMILY_CHOSEN) = i;
    if (i < 0 || i >= count) {
        return;
    }
    uint32_t name = P32(st, U32(st, G_FAMILY_LIST + 4) + 4u * (uint32_t) i) + U32(st, G_FAMILY_LIST + 0x10);
    if (name == 0) {
        return;
    }
    memset(state_ptr(st, G_PLAYER_NAME, 32), 0, 32);
    str_copy(st, G_PLAYER_NAME, name);
    GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_PLAYER_NAME);
    campaign_progress_read(st, G_CAMPAIGN_TABLE, campaign_progress_path(st));
    U8(st, G_REDRAW) = 1;
}

void family_name_open(game_state *st)
{
    /* @bin 0x00516C10 the text input (0x004CC500, 0x004CC3C0, 0x004CC3E0) on the name */
    memset(state_ptr(st, G_PLAYER_NAME, 32), 0, 32);
    U8(st, G_WINDOW) = 0x36;
    U32(st, G_UE20998) = 0;
    GAME_CALL(st, FN_TEXT_BOX_FOCUS, 1);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_START);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_END);
    U32(st, G_UE94DC0) = 0x2F;
    U8(st, G_REDRAW) = 1;
}

void family_list_open(game_state *st)
{
    /* @bin 0x00516BB0 */
    memset(state_ptr(st, G_PLAYER_NAME, 32), 0, 32);
    family_list_load(st, G_STR_FAMILY_PATTERN);
    int32_t chosen = S32(st, G_FAMILY_CHOSEN);
    U8(st, G_WINDOW) = 0x2F;
    U8(st, G_NOT_FOUND) = 0;
    S32(st, G_FAMILY_SCROLL) = 0;
    S32(st, G_PARAM1) = chosen;
    family_select(st);
    U8(st, G_REDRAW) = 1;
    if (S32(st, G_FAMILY_LIST + 8) == 0) {
        family_name_open(st);
    }
}

void family_done(game_state *st)
{
    /* @bin 0x00516990 a family: 0x0051F780(1) (nothing) and the main menu; none: message 0x5E */
    if (U8(st, G_PLAYER_NAME) == 0) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x5E, FN_NOTHING, 1);
        return;
    }
    GAME_CALL(st, FN_NOTHING, 1);
    main_menu_open(st);
    U8(st, G_REDRAW) = 1;
}

void family_create(game_state *st)
{
    /* @bin 0x00516C80 */
    name_trim(st, G_PLAYER_NAME);
    if (U8(st, G_PLAYER_NAME) == 0) {
        family_list_open(st);
        return;
    }
    /* @bin 0x00516CA2 fopen(progress, "rb"): the family exists, message 0x5C (then 0x00516C10) */
    const char *path = state_str(st, campaign_progress_path(st));
    if (path && platform_file_read(st, path, 0, NULL, 0) >= 0) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x5C, FN_FAMILY_NAME_OPEN, 1);
        return;
    }
    campaign_load(st, G_CAMPAIGN_TABLE, G_STR_CAMPAIGN);
    /* the write is a platform request (0x0041B8A0, flow.c): through sim_call */
    GAME_CALLT(st, G_CAMPAIGN_TABLE, FN_CAMPAIGN_PROGRESS_WRITE, (int32_t) campaign_progress_path(st));
    /* @bin 0x00516CFE CreateDirectoryA(Save\<name>, 0) (0x0056F078): the platform creates directories
     * when it writes into them; nothing lists a family's directory before a save is written there */
    family_dir_path(st);
    /* @bin 0x00516D04 the name kept in a char[32] of the frame while the list is reloaded */
    const char *typed = state_str(st, G_PLAYER_NAME);
    char name[32];
    if (!typed || strlen(typed) >= sizeof(name)) {
        state_unported(st, FN_FAMILY_CREATE); /* longer: the copy runs over the original's frame */
        return;
    }
    strcpy(name, typed);
    family_list_open(st);
    /* @bin 0x00516D2F the row whose name is the typed one (strcmp) becomes the chosen one */
    int32_t count = S32(st, G_FAMILY_LIST + 8);
    for (int32_t i = 0; i < count; i++) {
        const char *row = state_str(st, family_row(st, i));
        if (row && strcmp(row, name) == 0) {
            S32(st, G_FAMILY_CHOSEN) = i;
        }
    }
    for (size_t k = 0; k <= strlen(name); k++) {
        P8(st, G_PLAYER_NAME + (uint32_t) k) = (uint8_t) name[k];
    }
    family_list_open(st);
    family_done(st);
}

/* ---- the period and mission choices ---- */

void campaign_period_first(game_state *st, int32_t period)
{
    /* @bin 0x00417DD0 the kingdom rating of the difficulty (0x005D1988), counters cleared, the first
     * step of the period */
    uint32_t city = CURRENT_CITY(st);
    U8(st, G_FLOW2_PERIOD) = (uint8_t) period;
    U32(st, G_KINGDOM_START) = U32(st, G_KINGDOM_BY_DIFF + 4u * U32(st, G_DIFFICULTY));
    U32(st, G_KINGDOM_CARRY) = 0;
    U32(st, G_PLAYED) = 0;
    U32(st, city + CITY_PERSONAL_SAVINGS) = 0;
    int8_t step = (int8_t) campaign_next_step(st, G_CAMPAIGN_TABLE, (int8_t) period, -1, 0);
    U8(st, G_STEP) = (uint8_t) step;
    if (step >= 0 && step < S32(st, G_CAMPAIGN_TABLE + CAMPAIGN_STEP_COUNT)) {
        U8(st, G_CHOICE) = 0;
        U8(st, G_MISSION) = U8(st, G_CAMPAIGN_TABLE + ((uint32_t) step << 6) + 0xC);
    } else {
        U8(st, G_MISSION) = 0;
        U8(st, G_CHOICE) = 0;
    }
    U8(st, G_STEP_PREV) = 0xFF;
}

void campaign_period_start(game_state *st)
{
    /* @bin 0x0041BB70 a Pharaoh period (0x006FDC98 == 0) only when it is the one open (0x007027B0) */
    uint32_t cleopatra = U32(st, G_CAMPAIGN_CAMPAIGN_KIND);
    int32_t period = S32(st, G_CAMPAIGN_PERIOD);
    if (cleopatra == 0 && period != S32(st, G_PERIOD_REACHED)) {
        return;
    }
    U8(st, G_MISSION) = 0;
    U32(st, G_EDITOR) = 0;
    U8(st, G_NO_GROUND) = 0;
    U32(st, G_CUSTOM) = 0;
    U32(st, G_U5D2A2C) = 1;
    U8(st, G_REDRAW) = 1;
    U8(st, G_CAMPAIGN_KIND) = (uint8_t) (1 + (cleopatra != 0)); /* campaign kind: 1 Pharaoh, 2 Cleopatra */
    if ((uint32_t) period <= 4) {
        /* @bin 0x0041BBC8 the period's video (table 0x0041BDF4): "Binks\high\..." from a screen height
         * 0x00E38EDC of 0xE9 on, else "Binks\low\..."; centred (0x00413580, thiscall on [0x006FDC54]) */
        static const uint32_t videos[5][2] = {{G_STR_BINK_PRE_DYNASTIC_SMALL, G_STR_BINK_PRE_DYNASTIC_BIG},
                                              {G_STR_BINK_ARCHAIC_SMALL, G_STR_BINK_ARCHAIC_BIG},
                                              {G_STR_BINK_OLD_KINGDOM_SMALL, G_STR_BINK_OLD_KINGDOM_BIG},
                                              {G_STR_BINK_MIDDLE_KINGDOM_SMALL, G_STR_BINK_MIDDLE_KINGDOM_BIG},
                                              {G_STR_BINK_NEW_KING_SMALL, G_STR_BINK_NEW_KING_BIG}};
        uint32_t video = videos[period][S32(st, G_CPU) >= 0xE9];
        int32_t x = (int32_t) (U32(st, G_SCREEN_W) - 0x1E0u) / 2;
        int32_t y = (int32_t) (U32(st, G_SCREEN_H) - 0x11Eu) / 2;
        GAME_CALLT(st, U32(st, G_BINK_PLAYER), FN_PLATFORM_VIDEO_BINK, (int32_t) video, x, y);
    }
    campaign_period_first(st, S32(st, G_CAMPAIGN_PERIOD));
    if (U8(st, G_MISSION) == 0x7F) {
        U8(st, G_WINDOW) = 0x34;
        return;
    }
    briefing_open(st);
}

void flow_continue(game_state *st)
{
    /* @bin 0x00418220 */
    uint8_t screen = U8(st, G_WINDOW);
    U32(st, G_HOVER_2) = 0;
    U32(st, G_HOVER) = 0;
    U32(st, G_HOVER_BUTTON) = 0;
    U32(st, G_HOVER_3) = 0;
    U32(st, G_OVERLAY_PREV) = 0;
    if (screen == 0x36) {
        family_create(st);
        U8(st, G_REDRAW) = 1;
        return;
    }
    if (screen == 0x34) {
        /* @bin 0x00418260 the mission chosen: the carry-over of the city won (0x004188F0), the step
         * after the choice */
        if (U8(st, G_CHOICE) != 0) {
            GAME_CALL0(st, FN_NEWGAME_CARRY_OVER);
            int32_t choice = (int32_t) (U32(st, G_CHOICE) & 0xFFu) - 1;
            uint8_t from = U8(st, G_STEP);
            U8(st, G_STEP_PREV) = from;
            int8_t step = (int8_t) campaign_next_step(st, G_CAMPAIGN_TABLE, S8(st, G_FLOW2_PERIOD), (int8_t) from, choice);
            U8(st, G_STEP) = (uint8_t) step;
            if (step < 0 || step >= S32(st, G_CAMPAIGN_TABLE + CAMPAIGN_STEP_COUNT)) {
                state_unported(st, FN_FLOW_CONTINUE); /* @bin 0x004182D2 reads address 0xC: a crash */
                return;
            }
            U8(st, G_MISSION) = U8(st, G_CAMPAIGN_TABLE + ((uint32_t) step << 6) + 0xC);
            briefing_open(st);
        }
        U8(st, G_REDRAW) = 1;
        return;
    }
    if (screen == 0x35) {
        U32(st, CURRENT_CITY(st) + CITY_U471C) = 0;
    }
    /* @bin 0x00418301 into the city: window 1, the music stream faded out (0x0053B690), the city's
     * difficulty at most the one chosen */
    U8(st, G_WINDOW) = 1;
    U8(st, G_UE38DD6) = 2;
    GAME_CALL(st, FN_MUSIC_FADE_OUT, 1);
    uint32_t city = CURRENT_CITY(st);
    int32_t d = S32(st, G_DIFFICULTY);
    U32(st, G_UF55B94) = 0;
    if (d < S32(st, city + CITY_DIFFICULTY)) {
        S32(st, city + CITY_DIFFICULTY) = d;
    }
    U8(st, G_REDRAW) = 1;
}

/* ---- text boxes, the music stream ---- */


void text_box_focus(game_state *st, int32_t box)
{
    /* @bin 0x004CC500 box `box` edited: its length (+0x00E208A0) := its text's (pointer 0x00E2079C)
     * up to its maximum + 1 (0x00E208F0; none when negative) */
    uint32_t b = 4u * (uint32_t) box;
    int32_t max = S32(st, G_TEXTEDIT_MAX + b);
    U32(st, G_CARET_FIELD) = (uint32_t) box;
    S32(st, G_TEXTEDIT_END + b) = 0;
    if (max < 0) {
        return;
    }
    uint32_t text = U32(st, G_TEXTEDIT_TEXT + b);
    for (int32_t i = 0; P8(st, text + (uint32_t) i) != 0;) {
        S32(st, G_TEXTEDIT_END + b) += 1;
        i++;
        if (i > max) {
            break;
        }
    }
}

void text_box_cursor_start(game_state *st)
{
    /* @bin 0x004CC3C0 the cursor (0x00E207EC) of the box being edited at its start */
    S32(st, G_CARET_POS + 4u * U32(st, G_CARET_FIELD)) = 0;
}

void text_box_cursor_end(game_state *st)
{
    /* @bin 0x004CC3E0 the cursor at the end of its text */
    uint32_t b = 4u * U32(st, G_CARET_FIELD);
    S32(st, G_CARET_POS + b) = S32(st, G_TEXTEDIT_END + b);
}

void music_fade_out(game_state *st, int32_t channel)
{
    /* @bin 0x0053B690 with the sound system on (0x00F55B88) and the channel's stream opened (its name
     * slot 0x00E92DD0): while it plays (0x0055F5A1), its volume (0x0055F5F4) down by 5 to 0 (0x0055F639),
     * then it is closed (0x0055F561) */
    if (S32(st, G_SOUND_ON) == 0 || S32(st, G_STREAM_NAMES + 4u * (uint32_t) channel) == 0) {
        return;
    }
    if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, channel) == 0) {
        return;
    }
    int32_t v = GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME, channel);
    while (v > 0) {
        v -= 5;
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_SET, channel, v);
    }
    GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, channel);
}

/* @bin 0x0055F5F4 a stream's volume (AIL_stream_volume, 0x0056F28C), @bin 0x0055F639 set it
 * (AIL_set_stream_volume, 0x0056F29C): both only call the sound library (thiscall on 0x00E92BF8, they
 * read the stream handles, write nothing): the platform's requests (core/sim/platform.c) */
static int32_t run_stream_volume(game_state *st, const int32_t *a)
{
    return platform_stream_volume(st, a[0]);
}

static int32_t run_stream_set_volume(game_state *st, const int32_t *a)
{
    platform_stream_set_volume(st, a[0], a[1]);
    return 0;
}

/* ---- the table ---- */

static int32_t run_progress_read(game_state *st, const int32_t *a)
{
    return campaign_progress_read(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]);
}

static int32_t run_list_fill(game_state *st, const int32_t *a)
{
    return file_list_fill(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0], a[1]);
}

static int32_t run_list_sort(game_state *st, const int32_t *a)
{
    file_list_sort(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]);
    return 0;
}

static int32_t run_list_compare(game_state *st, const int32_t *a)
{
    return list_compare(st, (uint32_t) a[0], (uint32_t) a[1]);
}

static int32_t run_trim(game_state *st, const int32_t *a)
{
    name_trim(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_family_list_load(game_state *st, const int32_t *a)
{
    family_list_load(st, (uint32_t) a[0]);
    return 0;
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_family_select, family_select)
VOID0(run_cursor_start, text_box_cursor_start)
VOID0(run_cursor_end, text_box_cursor_end)

static int32_t run_text_box_focus(game_state *st, const int32_t *a)
{
    text_box_focus(st, a[0]);
    return 0;
}

static int32_t run_music_fade_out(game_state *st, const int32_t *a)
{
    music_fade_out(st, a[0]);
    return 0;
}

VOID0(run_family_list_open, family_list_open)
VOID0(run_family_name_open, family_name_open)
VOID0(run_family_create, family_create)
VOID0(run_family_done, family_done)
VOID0(run_period_start, campaign_period_start)
VOID0(run_continue, flow_continue)

static int32_t run_period_first(game_state *st, const int32_t *a)
{
    campaign_period_first(st, a[0]);
    return 0;
}

const ported_function newgame_flow2_ports[] = {
    {0x0041B680u, "campaign_progress_read", 1, 8, G_CAMPAIGN_TABLE, run_progress_read, 0},
    {0x0041C740u, "file_list_fill", 2, 8, G_LIST_OBJECT, run_list_fill, 0},
    {0x0041CA10u, "file_list_sort", 1, 0, G_LIST_OBJECT, run_list_sort, 0},
    {0x0041CA50u, "file_list_compare", 2, 32, 0, run_list_compare, 0},
    {0x0041ACD0u, "name_trim", 1, 0, 0, run_trim, 0},
    {0x00516B50u, "family_list_load", 1, 0, 0, run_family_list_load, 0},
    {0x00516AB0u, "family_select", 0, 0, 0, run_family_select, 0},
    {0x00516BB0u, "family_list_open", 0, 0, 0, run_family_list_open, 0},
    {0x00516C10u, "family_name_open", 0, 0, 0, run_family_name_open, 0},
    {0x00516C80u, "family_create", 0, 0, 0, run_family_create, 0},
    {0x00516990u, "family_done", 0, 0, 0, run_family_done, 0},
    {0x00417DD0u, "campaign_period_first", 1, 0, 0, run_period_first, 0},
    {0x0041BB70u, "campaign_period_start", 0, 0, 0, run_period_start, 0},
    {0x00418220u, "flow_continue", 0, 0, 0, run_continue, 0},
    {0x004CC500u, "text_box_focus", 1, 0, 0, run_text_box_focus, 0},
    {0x004CC3C0u, "text_box_cursor_start", 0, 0, 0, run_cursor_start, 0},
    {0x004CC3E0u, "text_box_cursor_end", 0, 0, 0, run_cursor_end, 0},
    {0x0053B690u, "music_fade_out", 1, 0, 0, run_music_fade_out, 0},
    {0x0055F5F4u, "platform stream volume (AIL)", 1, 32, G_SOUND, run_stream_volume, PORT_PLATFORM},
    {0x0055F639u, "platform stream volume set (AIL)", 2, 0, G_SOUND, run_stream_set_volume, PORT_PLATFORM},
    /* inside the ports above: the table on campaign_progress_read's frame, the list on the main menu's
     * frame (file_list_count), the scan writing its caller's frame */
    {0x0041AC50u, "campaign table constructor (campaign_progress_read)", 0, 32, 0, NULL, 0},
    {0x0041C6D0u, "file list constructor (file_list_count)", 0, 32, 0, NULL, 0},
    {0x0041C710u, "file list destructor (file_list_count)", 0, 0, 0, NULL, 0},
    {0x0041C8F0u, "file list scan, FindFirstFileA (file_list_fill: platform_file_list)", 3, 8, 0, NULL, 0},
    {0x00561CA1u, "CRT fread (platform_file_read)", 4, 32, 0, NULL, 0},
    {0x00561EA4u, "CRT fseek (platform_file_read)", 3, 32, 0, NULL, 0},
};
const int newgame_flow2_ports_count = sizeof(newgame_flow2_ports) / sizeof(newgame_flow2_ports[0]);
