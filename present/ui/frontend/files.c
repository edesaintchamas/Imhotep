/* Imhotep - the file dialogs: load / save / delete a saved game (window 0x0D) and the scenario
 * editor's map dialog (0x10) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The dialog's file list is the object 0x006057C8 (core/newgame/flow2.h: +4 the offsets 0x006057CC,
 * +8 the count 0x006057D0, +0x10 the names 0x006057D8), filled by 0x004CDFF0 from the family's
 * directory (Save\<family>\*.sav) or the maps directory (maps\*.map) through the platform's listing.
 * 0x006059F8 the rows, 0x006059F4 the first shown, 0x006059F0 the clicked one; the typed name is the
 * edit field 2 (0x00E37100, saved games) or 4 (0x00E37078, maps); the mode 0x00D3B1C0: 0 save, 1
 * load, 2 delete; 0x00D3B1BC the window to go back to; 0x00D3B1B3 the frames left of the "no such
 * file" title. Window 0x0D: per frame and full redraw 0x00532340; input (0x004D8310): field 2,
 * Enter = 0x00402720(1), a right click back, 0x004261D0 on the rows 0x005C3118 (0x00402A80), the
 * buttons 0x005C79F0 (0x00402720 OK / cancel, 0x00402980 arrows), the scroll bar 0x004029D0. Window
 * 0x10: 0x005325F0, buttons 0x005C7A80 (0x004099B0). */
#include "frontend.h"

#include <string.h>

#include "city/text_lookup.h"
#include "draw/text.h"
#include "frame/frame.h"
#include "newgame/flow.h"
#include "newgame/flow2.h"
#include "present_layout.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/loader.h"
#include "state/ported.h"


void text_caret_draw(game_state *st, int32_t x, int32_t y, int32_t colour);

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

static uint32_t list_row(game_state *st, int32_t i)
{
    /* the name of row i of the list (0 out of range) */
    if (i < 0 || i >= S32(st, G_FILES_LIST_COUNT)) {
        return 0;
    }
    return P32(st, U32(st, G_LIST_ROWS) + 4u * (uint32_t) i) + U32(st, G_LIST_NAMES);
}

static void state_strcpy(game_state *st, uint32_t dst, uint32_t src)
{
    uint32_t i = 0;
    do {
        P8(st, dst + i) = P8(st, src + i);
    } while (P8(st, src + i++) != 0);
}

static void host_strcpy(game_state *st, char *dst, uint32_t src, size_t size)
{
    size_t i = 0;
    for (; i + 1 < size && P8(st, src + (uint32_t) i) != 0; i++) {
        dst[i] = (char) P8(st, src + (uint32_t) i);
    }
    dst[i] = 0;
}

static void strip_ext_host(char *path)
{
    /* as 0x004CE3A0: cut at the first '.' */
    char *dot = strchr(path, '.');
    if (dot) {
        *dot = 0;
    }
}

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

static int32_t fe_file_exists(game_state *st, const char *path)
{
    /* @bin 0x004CE3F0 on a path of the caller's frame (the port of core/newgame/start.c is reached
     * with state paths only): the file opens (a size query to the platform); if not, 0x004CE100 (the
     * extension into 0x00E38AEC, upper-cased; for "WAV" / "SMK" / "555" the CD drive's directory
     * 0x00E36BF8), a second try, and 0x004CE2D0 */
    if (platform_file_read(st, path, 0, NULL, 0) >= 0) {
        return 1;
    }
    const char *dot = strchr(path, '.');
    if (dot) {
        U8(st, G_START_EXT) = (uint8_t) dot[1];
        U8(st, G_START_EXT + 1) = (uint8_t) dot[2];
        U8(st, G_START_EXT + 2) = (uint8_t) dot[3];
        U8(st, G_START_EXT + 3) = 0;
    }
    for (uint32_t a = G_START_EXT; P8(st, a) != 0; a++) {
        uint8_t c = P8(st, a);
        if (c >= 'a' && c <= 'z') {
            P8(st, a) = (uint8_t) (c - 0x20);
        }
    }
    if (state_str_eq(st, G_STR_WAV, G_START_EXT) || state_str_eq(st, G_STR_SMK, G_START_EXT) || state_str_eq(st, G_STR_555, G_START_EXT)) {
        for (uint32_t i = 0; i < 40; i++) {
            U8(st, G_CD_DIR + i) = 0;
        }
        text_copy(st, G_STR_C_ROOT, G_CD_DIR, 10);
        U8(st, G_CD_DIR) = U8(st, G_DRIVE);
    }
    int32_t ok = platform_file_read(st, path, 0, NULL, 0) >= 0 ? 1 : 0;
    GAME_CALL0(st, FN_WORKING_DIRECTORY);
    return ok;
}

void file_dialog_list(game_state *st, int32_t maps)
{
    /* @bin 0x004CDFF0 the list filled (0x0041C740 on 0x006057C8, emptied first) from maps\*.map, or
     * from the family's directory + "\*.sav" (0x005761C4; the first argument is not read); the
     * extensions cut (0x004CE3A0), sorted with _stricmp (0x00562370); the rows, the first row, the
     * clicked row, the title's count, 0x006059EC and 0x00E36EFC cleared */
    if ((uint8_t) maps != 0) {
        file_list_fill(st, G_FILES_LIST, G_STR_MAPS_PATTERN, 0);
    } else {
        /* the original copies the directory and the pattern into its frame: here the pattern is put
         * after the directory's string in its buffer for the call, and those bytes restored */
        uint32_t dir = family_dir_path(st);
        uint32_t end = dir;
        while (P8(st, end) != 0) {
            end++;
        }
        uint8_t saved[8];
        for (uint32_t i = 0; i < 8; i++) {
            saved[i] = P8(st, end + i);
        }
        state_strcpy(st, end, G_STR_SAV_PATTERN);
        file_list_fill(st, G_FILES_LIST, dir, 0);
        for (uint32_t i = 0; i < 8; i++) {
            P8(st, end + i) = saved[i];
        }
    }
    S32(st, G_FILES_ROWS) = S32(st, G_FILES_LIST_COUNT);
    for (int32_t i = 0; i < S32(st, G_FILES_ROWS); i++) {
        GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) list_row(st, i));
    }
    file_list_sort(st, G_FILES_LIST, FN_CRT_STRCMPI);
    S32(st, G_FIRST) = 0;
    S32(st, G_CLICKED) = 0;
    U8(st, G_NOT_FOUND) = 0;
    S32(st, G_BIG_LIST) = 0;
    S32(st, G_UE36EFC) = 0;
}

static void dialog_open(game_state *st, int32_t mode)
{
    /* the list of the family's saved games, window 0x0D (back to the city), the mode, the name field
     * 2 holding the last saved game's name */
    family_dir_path(st);
    file_dialog_list(st, 0);
    S32(st, G_UE20998) = 0;
    U8(st, G_WINDOW) = 0x0D;
    S32(st, G_BACK_WINDOW) = 1;
    U8(st, G_FILES_MODE) = (uint8_t) mode;
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_FILES_NAME, 0x40, 0);
    state_strcpy(st, G_FILES_NAME, G_LAST_NAME);
    GAME_CALL(st, FN_TEXT_BOX_FOCUS, 2);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_START);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_END);
    U8(st, G_REDRAW) = 1;
}

void load_dialog_open(game_state *st)
{
    /* @bin 0x00402450 the load dialog (mode 1); the chosen building type 0x006059E8 := 0 */
    S32(st, G_CONSTRUCTION_UI_TYPE) = 0;
    dialog_open(st, 1);
}

void save_dialog_open(game_state *st)
{
    /* @bin 0x00402540 the save dialog (mode 0) */
    dialog_open(st, 0);
}

void delete_dialog_open(game_state *st)
{
    /* @bin 0x00402630 the delete dialog (mode 2) */
    dialog_open(st, 2);
}

void file_dialog_thumb(game_state *st)
{
    /* @bin 0x00532830 the scroll bar's thumb: 12 rows (bar of 0x82 at 0x1D8, 0x91) or, with
     * 0x006059EC, 15 rows (0xA4 at 0x11C, 0xF5) */
    int32_t rows = S32(st, G_FILES_ROWS), first = S32(st, G_FIRST);
    int32_t shown = S32(st, G_BIG_LIST) == 0 ? 12 : 15;
    if (rows - shown <= 0) {
        return;
    }
    int32_t pct;
    if (first < 1) {
        pct = 0;
    } else if (first + shown < rows) {
        pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, first, rows - shown);
    } else {
        pct = 100;
    }
    if (shown == 12) {
        int32_t d = GAME_CALL(st, FN_CALC_PERCENTAGE, 0x82, pct);
        image(st, S32(st, G_SCROLL_IMAGES) + 0x27, S32(st, G_LAYOUT_DX) + 0x1D8, S32(st, G_LAYOUT_DY) + 0x91 + d);
    } else {
        int32_t d = GAME_CALL(st, FN_CALC_PERCENTAGE, 0xA4, pct);
        image(st, S32(st, G_SCROLL_IMAGES) + 0x27, S32(st, G_LAYOUT_DX) + 0x11C, S32(st, G_LAYOUT_DY) + 0xF5 + d);
    }
}

static void rows_draw(game_state *st, int32_t strip)
{
    /* up to 12 rows from the first shown (lit font when hovered), each copied (and its extension cut
     * for the maps) */
    int32_t n = S32(st, G_FILES_LIST_COUNT) > 12 ? 12 : S32(st, G_FILES_LIST_COUNT);
    int32_t y = 0x10;
    for (int32_t k = 1; k <= n; k++, y += 0x10) {
        int32_t font = S32(st, G_HOVER) == k ? S32(st, G_FONT_9C) : S32(st, G_FONT_ROW);
        uint32_t row = list_row(st, S32(st, G_FIRST) - 1 + k);
        if (row == 0) {
            state_unported(st, FN_FILE_DIALOG_DRAW); /* the original's strcpy reads address 0 */
            return;
        }
        char name[260];
        host_strcpy(st, name, row, sizeof(name));
        if (strip) {
            strip_ext_host(name);
        }
        text_draw_host(st, name, S32(st, G_LAYOUT_DX) + 0xA0, S32(st, G_LAYOUT_DY) + 0x72 + y, font, 0);
    }
}

static void dialog_panels(game_state *st)
{
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_PANEL_DRAW, dx + 0x80, dy + 0x28, 0x18, 0x15);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, dx + 0x90, dy + 0x50, 0x14, 2, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, dx + 0x90, dy + 0x78, 0x14, 0x0D, dx, dy);
}

void file_dialog_draw(game_state *st)
{
    /* @bin 0x00532340 window 0x0D: the panel, the field's and the list's inner panels, the scroll
     * bar's (2 x 10); the title (group 0x2B: "not found" 2 in red while 0x00D3B1B3 counts down, else
     * the mode: save 0, load 1, delete 6), the label 5, the rows, the buttons 0x005C79F0, the typed
     * name and its caret, the thumb */
    U8(st, G_REDRAW_SIDEBAR) = 1;
    dialog_panels(st);
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, dx + 0x1D3, dy + 0x90, 2, 10, dx, dy);
    int32_t index, colour;
    if (U8(st, G_NOT_FOUND) != 0) {
        U8(st, G_NOT_FOUND) = (uint8_t) (U8(st, G_NOT_FOUND) - 1);
        index = 2;
        colour = 0xF800;
    } else {
        colour = 0xFFFF;
        index = U8(st, G_FILES_MODE) == 2 ? 6 : U8(st, G_FILES_MODE);
    }
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x2B, index, dx + 0xA0, dy + 0x32, 0x130, S32(st, G_FONT_TITLE), colour, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP, 0x2B, 5, dx + 0xE0, dy + 0x156, S32(st, G_FONT_LARGE), 0xF800, dx, dy);
    rows_draw(st, 0);
    GAME_CALL(st, FN_BUTTONS_DRAW, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY), G_FILE_DIALOG_BUTTONS, 4);
    GAME_CALL0(st, FN_TEXT_RUN_RESET);
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) G_FILES_NAME, S32(st, G_LAYOUT_DX) + 0xA0, S32(st, G_LAYOUT_DY) + 0x5A, S32(st, G_FONT_9C), 0);
    text_caret_draw(st, S32(st, G_LAYOUT_DX) + 0xA0, S32(st, G_LAYOUT_DY) + 0x5B, 0xFFFF);
    file_dialog_thumb(st);
}

void map_dialog_draw(game_state *st)
{
    /* @bin 0x005325F0 window 0x10 (the scenario editor's maps): as 0x00532340 without the scroll
     * bar's panel; title group 0x2B, 3 + mode at x 0x80; the rows without their extension; the
     * buttons 0x005C7A80; the map's name (field 4) */
    U8(st, G_REDRAW_SIDEBAR) = 1;
    dialog_panels(st);
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x2B, U8(st, G_FILES_MODE) + 3, dx + 0x80, dy + 0x32, 0x130, S32(st, G_FONT_TITLE), 0xFFFF, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP, 0x2B, 5, dx + 0xE0, dy + 0x156, S32(st, G_FONT_LARGE), 0xF800, dx, dy);
    rows_draw(st, 1);
    GAME_CALL(st, FN_BUTTONS_DRAW, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY), G_MAP_DIALOG_BUTTONS, 4);
    GAME_CALL0(st, FN_TEXT_RUN_RESET);
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) G_MAP_NAME, S32(st, G_LAYOUT_DX) + 0xA0, S32(st, G_LAYOUT_DY) + 0x5A, S32(st, G_FONT_9C), 0);
    text_caret_draw(st, S32(st, G_LAYOUT_DX) + 0xA0, S32(st, G_LAYOUT_DY) + 0x5B, 0xFFFF);
    file_dialog_thumb(st);
}

void file_dialog_arrows(game_state *st)
{
    /* @bin 0x00402980 the arrows (0x00D3B198: 0 up, 1 down) of a list of more than 12 rows; the
     * "not found" title cleared */
    int32_t rows = S32(st, G_FILES_ROWS);
    if (rows <= 12) {
        return;
    }
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        if (S32(st, G_FIRST) > 0) {
            U8(st, G_NOT_FOUND) = 0;
            S32(st, G_FIRST) = S32(st, G_FIRST) - 1;
        }
        return;
    }
    if (p == 1) {
        if (S32(st, G_FIRST) + 12 >= rows) {
            return;
        }
        S32(st, G_FIRST) = S32(st, G_FIRST) + 1;
    }
    U8(st, G_NOT_FOUND) = 0;
}

int32_t file_dialog_drag(game_state *st)
{
    /* @bin 0x004029D0 the left button held on the scroll bar (x 0x1D0..0x1F0, y 0x91..0x12C) of a
     * list of more than 12: the first row by the place; a full redraw; 1 when it took the input */
    int32_t rows = S32(st, G_FILES_ROWS);
    if (rows <= 12 || U8(st, G_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t range = rows - 12;
    int32_t mx = S32(st, G_MOUSE_X), dx = S32(st, G_LAYOUT_DX);
    if (mx < dx + 0x1D0 || mx > dx + 0x1F0) {
        return 0;
    }
    int32_t my = S32(st, G_MOUSE_Y), dy = S32(st, G_LAYOUT_DY);
    if (my < dy + 0x91 || my > dy + 0x12C) {
        return 0;
    }
    int32_t pos = my - dy - 0x91;
    if (pos > 0x82) {
        pos = 0x82;
    }
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, pos, 0x82);
    S32(st, G_FIRST) = GAME_CALL(st, FN_CALC_PERCENTAGE, range, pct);
    U8(st, G_REDRAW) = 1;
    return 1;
}

void file_dialog_row_click(game_state *st)
{
    /* @bin 0x00402A80 a row (0x00D3B198 < rows): clicked; its name into the field (window 0x0D: the
     * saved game's 0x00E37100, field 2; else the map's 0x00E37078, field 4), the caret at its end; the
     * "not found" title cleared */
    int32_t p = S32(st, G_PARAM1);
    if (p >= S32(st, G_FILES_ROWS)) {
        return;
    }
    S32(st, G_CLICKED) = p;
    uint32_t dst;
    int32_t field;
    uint32_t row;
    if (U8(st, G_WINDOW) == 0x0D) {
        dst = G_FILES_NAME;
        field = 2;
        GAME_CALL(st, FN_FILL_BYTES, (int32_t) dst, 0x40, 0);
        row = list_row(st, S32(st, G_CLICKED) + S32(st, G_FIRST));
    } else {
        dst = G_MAP_NAME;
        field = 4;
        GAME_CALL(st, FN_FILL_BYTES, (int32_t) dst, 0x40, 0);
        row = list_row(st, S32(st, G_FIRST) + S32(st, G_CLICKED));
    }
    if (row == 0) {
        state_unported(st, FN_FILE_DIALOG_ROW_CLICK); /* the original's strcpy reads address 0 */
        return;
    }
    state_strcpy(st, dst, row);
    GAME_CALL(st, FN_TEXT_BOX_FOCUS, field);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_START);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_END);
    U8(st, G_NOT_FOUND) = 0;
}

/* a += b, truncated to cap - 1 characters (strncat(a, b, cap - strlen(a) - 1)) */
static void append_path(char *a, size_t cap, const char *b)
{
    size_t l = strlen(a);
    while (*b && l + 1 < cap) {
        a[l++] = *b++;
    }
    a[l] = 0;
}

static void typed_path(game_state *st, uint32_t name, uint32_t ext, uint32_t dir_part, int family, char *out, size_t size)
{
    /* the typed name with its extension replaced (0x004CE3A0, 0x004CBEA0 into 0x00E38AEC, 0x004CE3C0);
     * the path: the family's directory + "\" + name, or "maps\" + name */
    GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) name);
    GAME_CALL(st, FN_TEXT_COPY, (int32_t) ext, (int32_t) G_START_EXT, 3);
    GAME_CALL(st, FN_FILE_NAME_ADD_EXTENSION, (int32_t) name);
    char a[260], b[260];
    if (family) {
        host_strcpy(st, a, family_dir_path(st), sizeof(a));
    } else {
        a[0] = 0;
    }
    host_strcpy(st, b, dir_part, sizeof(b));
    append_path(a, sizeof(a), b);
    host_strcpy(st, b, name, sizeof(b));
    append_path(a, sizeof(a), b);
    strncpy(out, a, size - 1);
    out[size - 1] = 0;
}

void crt_dosmaperr(game_state *st, uint32_t error)
{
    /* @bin 0x005672C8 _dosmaperr: _doserrno 0x01266434 := the Win32 error; errno 0x01266430 from the
     * table 0x00604F30 (0x2D pairs error, errno), else EACCES (0xD) for 0x13..0x24, ENOEXEC (8) for
     * 0xBC..0xCA, EINVAL (0x16) */
    U32(st, G_CRT_DOSERRNO) = error;
    for (uint32_t e = G_CRT_ERRNO_TABLE; e < G_CRT_ERRNO_TABLE + 0x2Du * 8u; e += 8) {
        if (U32(st, e) == error) {
            U32(st, G_CRT_ERRNO) = U32(st, e + 4);
            return;
        }
    }
    if (error >= 0x13 && error <= 0x24) {
        U32(st, G_CRT_ERRNO) = 0xD;
    } else if (error >= 0xBC && error <= 0xCA) {
        U32(st, G_CRT_ERRNO) = 8;
    } else {
        U32(st, G_CRT_ERRNO) = 0x16;
    }
}

int32_t crt_remove(game_state *st, const char *path)
{
    /* @bin 0x00562BB5 remove: DeleteFileA (the platform's file_delete), 0; on failure _dosmaperr of
     * GetLastError, -1. The platform reports a failure without its Win32 error: taken as
     * ERROR_FILE_NOT_FOUND (2), the error of the paths the game deletes (files it listed or tested
     * before; the oracle's DeleteFileA gives the same, tools/oracle/game.py) */
    if (platform_file_delete(st, path) == 0) {
        return 0;
    }
    crt_dosmaperr(st, 2);
    return -1;
}

int32_t saved_game_delete(game_state *st, const char *path)
{
    /* @bin 0x004D4D80 remove the file (CRT remove 0x00562BB5): 1 when it fails; then the family's
     * directory (0x005169C0) + "\*.sav" (0x005761C4) into the frame, the list again (0x004CDFF0, which
     * builds its own pattern), 0 */
    if (crt_remove(st, path) == -1) {
        return 1;
    }
    family_dir_path(st);
    file_dialog_list(st, 0);
    return 0;
}

void file_dialog_ok(game_state *st)
{
    /* @bin 0x00402720 the dialog's buttons (0x00D3B198): 0 cancel (back to 0x00D3B1BC); 1 OK: the
     * path Save\<family>\<name>.sav; load and delete need the file (else the "not found" title for 100
     * frames); load: the music stopped, the game loaded (0x004D5100); delete: 0x004D4D80; save: an
     * existing file is confirmed first (message 0x6E / 0x6F, 7 = no), the game saved (0x004D4E20), in
     * the Pharaoh campaign its path kept (0x00700998) and the progress file written (0x0041B8A0); the
     * name kept as the last one (0x00E370BC); the city unless deleting */
    int32_t p = S32(st, G_PARAM1);
    S32(st, G_HOVER_2) = 0;
    S32(st, G_HOVER) = 0;
    S32(st, G_HOVER_BUTTON) = 0;
    S32(st, G_HOVER_3) = 0;
    if (p == 0) {
        U8(st, G_WINDOW) = (uint8_t) S32(st, G_BACK_WINDOW);
        U8(st, G_REDRAW) = 1;
        return;
    }
    if (p != 1) {
        return;
    }
    char path[100];
    typed_path(st, G_FILES_NAME, G_STR_SAV, G_STR_BACKSLASH, 1, path, sizeof(path));
    uint8_t mode = U8(st, G_FILES_MODE);
    if (mode >= 1) {
        if (fe_file_exists(st, path) == 0) {
            GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_FILES_NAME);
            U8(st, G_NOT_FOUND) = 100;
            return;
        }
        if (U8(st, G_FILES_MODE) == 1) {
            GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 0);
            loader_load_save_path(st, path, 0);
            goto done;
        }
        mode = U8(st, G_FILES_MODE);
    }
    if (mode == 2) {
        saved_game_delete(st, path);
    } else {
        if (platform_file_read(st, path, 0, NULL, 0) >= 0) {
            /* @bin 0x0040288F fopen(path, "r") (0x005761CC) succeeded: fclose, the question */
            GAME_CALL(st, FN_TEXT_SEEK, 5, 0x6E);
            GAME_CALL(st, FN_TEXT_SEEK2, 5, 0x6F);
            if (GAME_CALL(st, FN_PLATFORM_MESSAGE_BOX, S32(st, G_TEXT_CURSOR), S32(st, G_TEXT_PTR2), 4) == 7) {
                GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_FILES_NAME);
                return;
            }
        }
        frame_save_game(st, path);
        if (U8(st, G_CAMPAIGN_KIND) == 1) {
            size_t n = strlen(path);
            for (size_t i = 0; i <= n; i++) {
                P8(st, G_LAST_SAVE + (uint32_t) i) = (uint8_t) path[i];
            }
            GAME_CALLT(st, G_CAMPAIGN_TABLE, FN_CAMPAIGN_PROGRESS_WRITE, (int32_t) campaign_progress_path(st));
        }
    }
done:
    GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_FILES_NAME);
    state_strcpy(st, G_LAST_NAME, G_FILES_NAME);
    if (U8(st, G_FILES_MODE) < 2) {
        U8(st, G_WINDOW) = 1;
    }
    U8(st, G_REDRAW) = 1;
}

void map_dialog_ok(game_state *st)
{
    /* @bin 0x004099B0 the map dialog's buttons: 0 cancel (the city); 1 OK: the path maps\<name>.map;
     * open (mode 1) needs the file (else "not found"), then 0x004DE600 loads it; else the map is saved
     * (0x004D5520 on the path in the caller's frame: not ported); the name's extension cut, the city */
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        U8(st, G_WINDOW) = 1;
        U8(st, G_REDRAW) = 1;
        return;
    }
    if (p != 1) {
        return;
    }
    char path[100];
    typed_path(st, G_MAP_NAME, G_STR_MAP, G_STR_MAPS_DIR, 0, path, sizeof(path));
    if (U8(st, G_FILES_MODE) == 1) {
        if (fe_file_exists(st, path) == 0) {
            GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_MAP_NAME);
            U8(st, G_NOT_FOUND) = 100;
            return;
        }
        if (U8(st, G_FILES_MODE) == 1) {
            GAME_CALL0(st, FN_SCENARIO_MAP_LOAD);
        } else {
            state_unported(st, FN_MAP_DIALOG_OK);
            return;
        }
    } else {
        state_unported(st, FN_MAP_DIALOG_OK); /* 0x004D5520(path): the map's save */
        return;
    }
    GAME_CALL(st, FN_FILE_NAME_STRIP_EXTENSION, (int32_t) G_MAP_NAME);
    U8(st, G_WINDOW) = 1;
    U8(st, G_REDRAW) = 1;
}

static int32_t run_list(game_state *st, const int32_t *a) { file_dialog_list(st, a[1]); return 0; }
static int32_t run_delete(game_state *st, const int32_t *a)
{
    char path[260];
    host_strcpy(st, path, (uint32_t) a[0], sizeof(path));
    return saved_game_delete(st, path);
}
static int32_t run_remove(game_state *st, const int32_t *a)
{
    char path[260];
    host_strcpy(st, path, (uint32_t) a[0], sizeof(path));
    return crt_remove(st, path);
}
static int32_t run_dosmaperr(game_state *st, const int32_t *a) { crt_dosmaperr(st, (uint32_t) a[0]); return 0; }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return file_dialog_drag(st); }
#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(load_dialog_open)
RUN0(save_dialog_open)
RUN0(delete_dialog_open)
RUN0(file_dialog_thumb)
RUN0(file_dialog_draw)
RUN0(map_dialog_draw)
RUN0(file_dialog_arrows)
RUN0(file_dialog_row_click)
RUN0(file_dialog_ok)
RUN0(map_dialog_ok)

/* 0x004099B0 is not marked partial although its map-save path is not ported (state_unported): a partial port is recorded by the replay as one unit, and these reach
 * unported functions on their ported paths (the loader's, the saver's), which the replay must see
 * one by one */
const ported_function frontend_files_ports[] = {
    {0x004CDFF0u, "file_dialog_list", 2, 0, 0, run_list, 0},
    {0x00402450u, "load_dialog_open", 0, 0, 0, run_load_dialog_open, 0},
    {0x00402540u, "save_dialog_open", 0, 0, 0, run_save_dialog_open, 0},
    {0x00402630u, "delete_dialog_open", 0, 0, 0, run_delete_dialog_open, 0},
    {0x00532830u, "file_dialog_thumb", 0, 0, 0, run_file_dialog_thumb, 0},
    {0x00532340u, "file_dialog_draw", 0, 0, 0, run_file_dialog_draw, 0},
    {0x005325F0u, "map_dialog_draw", 0, 0, 0, run_map_dialog_draw, 0},
    {0x00402980u, "file_dialog_arrows", 0, 0, 0, run_file_dialog_arrows, 0},
    {0x004029D0u, "file_dialog_drag", 0, 32, 0, run_drag, 0},
    {0x00402A80u, "file_dialog_row_click", 0, 0, 0, run_file_dialog_row_click, 0},
    {0x00402720u, "file_dialog_ok", 0, 0, 0, run_file_dialog_ok, 0},
    {0x004D4D80u, "saved_game_delete", 1, 32, 0, run_delete, 0},
    {0x00562BB5u, "crt_remove", 1, 32, 0, run_remove, 0},
    {0x00562BDFu, "crt_unlink (crt_remove)", 1, 32, 0, run_remove, 0},
    {0x005672C8u, "crt_dosmaperr", 1, 0, 0, run_dosmaperr, 0},
    {0x004099B0u, "map_dialog_ok (map save: state_unported)", 0, 0, 0, run_map_dialog_ok, 0},
};
const int frontend_files_ports_count = sizeof(frontend_files_ports) / sizeof(frontend_files_ports[0]);
