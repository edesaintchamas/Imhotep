/* Imhotep - the list popup's set-up, and the replay check of a lost city (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include <string.h>

#include "newgame/flow.h"
#include "newgame/start.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

void list_popup_open(game_state *st, const int32_t *a)
{
    /* @bin 0x00403C00 (15 arguments) a list popup of n = a3 items (at most 0x4E), per = a14
     * per column (0x1A when not positive): the parameters into 0x00605954 (a1), 0x0060594C (a2),
     * 0x00605944 (a4), 0x00605940 (a5), 0x00605930 (a6), 0x00605934 (a7), 0x00605938 (a8),
     * 0x0060593C (a9), 0x0060592C (a10), 0x00605A50 (a12), 0x006057F0 (a15, u8); 0x00605948 :=
     * n, 0x00605950 := per, 0x006057EC := the columns (n + per - 1) / per; unless a11 (u8), the
     * order 0x006057F4[i] := i; the previous window into 0x00605958, 0x00E38DDC := 0x14, window
     * 0x48 when a13 (u8) else 0x0F; the n buttons at 0x00704DC8 (0x1C bytes): x 200 * column + 5
     * .. + 0xC9, y 20 * row + 0x0E .. + 0x20 (i16), functions 0x00403950 / 0x0051F780, bytes 0, 0,
     * 0, 3, parameter i, 0 */
    int32_t n = a[2];
    if (n > 0x4E) {
        n = 0x4E;
    }
    int32_t per = a[13];
    if (per <= 0) {
        per = 0x1A;
    }
    S32(st, G_LIST_A1) = a[0];
    S32(st, G_LIST_A8) = a[7];
    S32(st, G_LIST_A2) = a[1];
    S32(st, G_LIST_A6) = a[5];
    S32(st, G_LIST_A7) = a[6];
    S32(st, G_LIST_TEXT_GROUP) = a[3];
    S32(st, G_LIST_A10) = a[9];
    S32(st, G_LIST_A9) = a[8];
    S32(st, G_LIST_TEXT_FIRST) = a[4];
    U8(st, G_LIST_A15) = (uint8_t) a[14];
    S32(st, G_LIST_A12) = a[11];
    S32(st, G_LIST_COUNT) = n;
    S32(st, G_LIST_PER_COLUMN) = per;
    S32(st, G_LIST_COLUMNS) = (n + per - 1) / per;
    if ((uint8_t) a[10] == 0) {
        for (int32_t i = 0; i < n; i++) {
            S32(st, G_LIST_ORDER + 4u * (uint32_t) i) = i;
        }
    }
    S32(st, G_LIST_PREV_WINDOW) = U8(st, G_WINDOW);
    S32(st, G_UE38DDC) = 0x14;
    U8(st, G_WINDOW) = (uint8_t) a[12] != 0 ? 0x48 : 0x0F;
    for (int32_t i = 0; i < n; i++) {
        uint32_t e = G_LIST_BUTTONS + 0x1Cu * (uint32_t) i;
        int32_t col = i / per, row = i % per;
        S16(st, e) = (int16_t) (col * 200 + 5);
        S16(st, e + 2) = (int16_t) (row * 20 + 0x0E);
        S16(st, e + 4) = (int16_t) (col * 200 + 0xC9);
        S16(st, e + 6) = (int16_t) (row * 20 + 0x20);
        U32(st, e + 8) = 0x00403950u;
        U32(st, e + 0x0C) = FN_NOTHING;
        U8(st, e + 0x10) = 0;
        U8(st, e + 0x11) = 0;
        U8(st, e + 0x12) = 0;
        U8(st, e + 0x13) = 3;
        S32(st, e + 0x14) = i;
        S32(st, e + 0x18) = 0;
    }
}

static void cat_state(char *dst, size_t size, game_state *st, uint32_t s)
{
    size_t n = strlen(dst);
    for (uint32_t i = 0; n + 1 < size; i++) {
        char c = (char) P8(st, s + i);
        if (c == 0) {
            break;
        }
        dst[n++] = c;
    }
    dst[n] = 0;
}

uint8_t replay_possible(game_state *st)
{
    /* @bin 0x004183D0 a custom map (0x00E38EBC != 0): "maps\" (0x0057626C) + the map's name
     * 0x00E37078 exists (0x005409D0). A campaign of the family list (0x007027B4 = 1): the family's
     * directory (0x005169C0) + "\" (0x005761D0) + "autosave_replay.sav" (0x0057BFC4) opens with
     * fopen "rb" (0x005763BC; then fclose). Otherwise 1 */
    char path[300] = "";
    if (S32(st, G_CUSTOM) != 0) {
        cat_state(path, sizeof(path), st, G_STR_MAPS_DIR);
        cat_state(path, sizeof(path), st, G_MAP_NAME);
        return newgame_map_file_exists(st, path) != 0;
    }
    if (U8(st, G_CAMPAIGN_KIND) != 1) {
        return 1;
    }
    cat_state(path, sizeof(path), st, family_dir_path(st));
    cat_state(path, sizeof(path), st, G_STR_BACKSLASH);
    cat_state(path, sizeof(path), st, G_STR_AUTOSAVE_REPLAY);
    return platform_file_read(st, path, 0, NULL, 0) >= 0;
}

static int32_t run_403c00(game_state *st, const int32_t *a)
{
    list_popup_open(st, a);
    return 0;
}

static int32_t run_4183d0(game_state *st, const int32_t *a)
{
    (void) a;
    return replay_possible(st);
}

const ported_function list_popup_ports[] = {
    {0x00403C00u, "list_popup_open", 15, 0, 0, run_403c00, 0},
    {0x004183D0u, "replay_possible", 0, 8, 0, run_4183d0, 0},
};
const int list_popup_ports_count = sizeof(list_popup_ports) / sizeof(list_popup_ports[0]);
