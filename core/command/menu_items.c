/* Imhotep - the items of the top menu bar (File, Options, Help) (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The game's menu bar 0x005C91F0 (four menus of 0x10 bytes: +8 the item table, +0xC the item
 * count) is read by the menu input 0x00426030 (present/sidebar/input.c), which calls the chosen
 * item's function (item entries of 0x10 bytes, the function at +4). File: 0x005C8F30 (6 items),
 * Options: 0x005C8F90 (8), Help: 0x005C9010 (4), Advisors: 0x005C9080 (13, 0x0040A0E0). The items
 * already ported elsewhere: 0x00418510 (core/newgame/start.c), 0x00402450 / 0x00402540 /
 * 0x00402630 (present/ui/frontend/files.c). Every item takes no argument and returns nothing.
 * Also 0x004D3C40 (the defaults of chunk 175), called by the first item and by the loader. */
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define WINDOW 0x00E38DDAu    /* the current window id (u8) */
#define REDRAW 0x00E38DCAu    /* the redraw flag (u8) */
#define MENU_REDRAW 0x0070571Cu /* the menu bar's redraw (i32) */

void chunk175_defaults(game_state *st)
{
    /* @bin 0x004D3C40 three records of 12 bytes at 0x006FDCBC and three at 0x006FDCE0 (the first
     * dword of 0x006FDCE0 := 0 instead): +0 u8 := 0xFF, +4 i32 := -1, +8 u8 := 0; in the original's
     * order of writes */
    U32(st, 0x006FDCE0u) = 0;
    U32(st, 0x006FDCE8u) = 0xFFFFFFFFu;
    U8(st, 0x006FDCECu) = 0;
    U8(st, 0x006FDCE4u) = 0xFF;
    U32(st, 0x006FDCF4u) = 0xFFFFFFFFu;
    U8(st, 0x006FDCF8u) = 0;
    U8(st, 0x006FDCF0u) = 0xFF;
    U32(st, 0x006FDD00u) = 0xFFFFFFFFu;
    U8(st, 0x006FDD04u) = 0;
    U8(st, 0x006FDCFCu) = 0xFF;
    U32(st, 0x006FDCC0u) = 0xFFFFFFFFu;
    U8(st, 0x006FDCC4u) = 0;
    U8(st, 0x006FDCBCu) = 0xFF;
    U32(st, 0x006FDCCCu) = 0xFFFFFFFFu;
    U8(st, 0x006FDCD0u) = 0;
    U8(st, 0x006FDCC8u) = 0xFF;
    U32(st, 0x006FDCD8u) = 0xFFFFFFFFu;
    U8(st, 0x006FDCDCu) = 0;
    U8(st, 0x006FDCD4u) = 0xFF;
}

static void menu_exit_to_main(game_state *st)
{
    /* @bin 0x004023A0 File 1: the chunk 175 defaults (0x004D3C40); 0x0078D7B0, 0x0078D7A8,
     * 0x0078D7A4, 0x0078D7A0, 0x006059E8, 0x00BC9694, 0x00E94E0C, 0x00E94E10 := 0 */
    chunk175_defaults(st);
    U32(st, 0x0078D7B0u) = 0;
    U32(st, 0x0078D7A8u) = 0;
    U32(st, 0x0078D7A4u) = 0;
    U32(st, 0x0078D7A0u) = 0;
    U32(st, 0x006059E8u) = 0;
    U32(st, 0x00BC9694u) = 0;
    U32(st, 0x00E94E0Cu) = 0;
    U32(st, 0x00E94E10u) = 0;
    if (S32(st, 0x00605A3Cu) == 1) {
        /* @bin 0x004023E7 the editor: window 0, redraw */
        U8(st, WINDOW) = 0;
        U8(st, REDRAW) = 1;
    } else {
        /* @bin 0x00402400 by the game mode 0x007027B4 (u8; jump table 0x0040243C): 1 the family
         * list (0x00516BB0, 0x00516990); 0, 2, 3 the main menu (0x004CB6A0) */
        uint8_t mode = U8(st, 0x007027B4u);
        if (mode == 1) {
            GAME_CALL0(st, 0x00516BB0u);
            GAME_CALL0(st, 0x00516990u);
        } else if (mode <= 3) {
            GAME_CALL0(st, 0x004CB6A0u);
        }
    }
    /* @bin 0x00402418 the main menu's music when music is on (0x00E38E64 u8) and 0x00F55B88 */
    if (U8(st, 0x00E38E64u) != 0 && S32(st, 0x00F55B88u) != 0) {
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F449u, 0x005761AC, 0, 0);
    }
}

static void menu_exit_game(game_state *st)
{
    /* @bin 0x00402B90 File 6: PostMessageA(0x00E39140, WM_KEYDOWN, VK_ESCAPE, 0) */
    platform_post_message(st, 0x100, 0x1B, 0);
}

static void open_window(game_state *st, uint8_t w)
{
    U8(st, WINDOW) = w;
}

static void menu_display_options(game_state *st)
{
    /* @bin 0x00402BC0 Options 1: window 10, 0x00F53F18 := 0, redraw */
    open_window(st, 0x0A);
    U32(st, 0x00F53F18u) = 0;
    U8(st, REDRAW) = 1;
}

static void menu_difficulty_options(game_state *st)
{
    /* @bin 0x00402BE0 Options 4: window 0x56, 0x006DE510 := 0x00E38EF0, 0x00F53F4C := 0, redraw */
    uint32_t v = U32(st, 0x00E38EF0u);
    open_window(st, 0x56);
    U32(st, 0x006DE510u) = v;
    U32(st, 0x00F53F4Cu) = 0;
    U8(st, REDRAW) = 1;
}

static void menu_window_45(game_state *st)
{
    /* @bin 0x00402E30 Options 7: window 0x45, 0x00F53F18 := 0, redraw */
    open_window(st, 0x45);
    U32(st, 0x00F53F18u) = 0;
    U8(st, REDRAW) = 1;
}

static void menu_sound_options(game_state *st)
{
    /* @bin 0x00402EE0 Options 2: the sound settings kept to cancel (0x006057A4..0x006057C0 from
     * 0x00E38ED0, 0x00E38EC0 low byte, 0x00E38EC4, 0x00E38E63 u8, 0x00E38E64 second byte,
     * 0x00E38ECC, 0x00E38E64 low byte, 0x00E38EC8), window 0x0B, 0x00F53F1C := 0, redraw */
    uint32_t e64 = U32(st, 0x00E38E64u);
    U32(st, 0x006057BCu) = U32(st, 0x00E38EC8u);
    U32(st, 0x006057C0u) = e64 & 0xFF;
    U32(st, 0x006057B4u) = U32(st, 0x00E38ECCu);
    U32(st, 0x006057B8u) = (e64 >> 8) & 0xFF;
    U32(st, 0x006057B0u) = U8(st, 0x00E38E63u);
    U32(st, 0x006057ACu) = U32(st, 0x00E38EC4u);
    U32(st, 0x006057A8u) = U32(st, 0x00E38EC0u) & 0xFF;
    U32(st, 0x006057A4u) = U32(st, 0x00E38ED0u);
    open_window(st, 0x0B);
    U32(st, 0x00F53F1Cu) = 0;
    U8(st, REDRAW) = 1;
}

static void menu_speed_options(game_state *st)
{
    /* @bin 0x004032C0 Options 3: the speeds kept to cancel (0x006057A0 := 0x00E38E6C, 0x0060579C :=
     * 0x00E38E70), window 0x0C, 0x00F53F20 := 0, redraw */
    U32(st, 0x006057A0u) = U32(st, 0x00E38E6Cu);
    U32(st, 0x0060579Cu) = U32(st, 0x00E38E70u);
    open_window(st, 0x0C);
    U32(st, 0x00F53F20u) = 0;
    U8(st, REDRAW) = 1;
}

static void menu_help(game_state *st)
{
    GAME_CALL(st, 0x004C5BF0u, 10, 0, 0); /* @bin 0x004033F0 Help 1: the help window, topic 10 */
}

static void menu_about(game_state *st)
{
    GAME_CALL(st, 0x004C5BF0u, 0, 0, 0); /* @bin 0x00403400 Help 4: the help window, topic 0 */
}

static void menu_mouse_help(game_state *st)
{
    /* @bin 0x00403410 Help 2: the mouse help level 0x00E38EAC (i32) 2 -> 0 -> 1 -> 2; the item's
     * text (words 0x005C9022 and 0x005C9062) 2, 3, 4 for 0, 1, 2; the menu redrawn */
    int32_t v = S32(st, 0x00E38EACu);
    if (v == 2) {
        S32(st, 0x00E38EACu) = 0;
        U16(st, 0x005C9022u) = 2;
        U16(st, 0x005C9062u) = 2;
    } else if (v == 0) {
        S32(st, 0x00E38EACu) = 1;
        U16(st, 0x005C9022u) = 3;
        U16(st, 0x005C9062u) = 3;
    } else {
        S32(st, 0x00E38EACu) = 2;
        U16(st, 0x005C9022u) = 4;
        U16(st, 0x005C9062u) = 4;
    }
    S32(st, MENU_REDRAW) = 1;
}

/* an on / off item: the flag (u8 or i32), the item's text word and its two values (off, on) */
static void toggle8(game_state *st, uint32_t flag, uint32_t text, uint16_t off, uint16_t on)
{
    if (U8(st, flag) != 0) {
        U8(st, flag) = 0;
        U16(st, text) = off;
    } else {
        U16(st, text) = on;
        U8(st, flag) = 1;
    }
    S32(st, MENU_REDRAW) = 1;
}

static void menu_warnings(game_state *st)
{
    toggle8(st, 0x00E38EC1u, 0x005C9032u, 5, 6); /* @bin 0x00403490 Help 3: the warnings */
}

static void menu_toggle_eec(game_state *st)
{
    /* @bin 0x00403500 Options 8: 0x00E38EEC (i32) on / off, text 0x005C8FF2 9 / 10 */
    if (S32(st, 0x00E38EECu) != 0) {
        S32(st, 0x00E38EECu) = 0;
        U16(st, 0x005C8FF2u) = 9;
    } else {
        U16(st, 0x005C8FF2u) = 10;
        S32(st, 0x00E38EECu) = 1;
    }
    S32(st, MENU_REDRAW) = 1;
}

static void menu_toggle_ec3(game_state *st)
{
    toggle8(st, 0x00E38EC3u, 0x005C9002u, 4, 5); /* @bin 0x00403540 Options 9: 0x00E38EC3 */
}

static void menu_toggle_9088(game_state *st)
{
    toggle8(st, 0x00E39088u, 0x005C8FD2u, 7, 8); /* @bin 0x0040C7D0 Options 6: 0x00E39088 */
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_175, chunk175_defaults)
V(run_exit_main, menu_exit_to_main)
V(run_exit_game, menu_exit_game)
V(run_display, menu_display_options)
V(run_difficulty, menu_difficulty_options)
V(run_45, menu_window_45)
V(run_sound, menu_sound_options)
V(run_speed, menu_speed_options)
V(run_help, menu_help)
V(run_about, menu_about)
V(run_mouse, menu_mouse_help)
V(run_warnings, menu_warnings)
V(run_eec, menu_toggle_eec)
V(run_ec3, menu_toggle_ec3)
V(run_9088, menu_toggle_9088)
#undef V

const ported_function menu_item_ports[] = {
    {0x004D3C40u, "chunk175_defaults", 0, 0, 0, run_175, 0},
    {0x004023A0u, "menu_exit_to_main", 0, 0, 0, run_exit_main, 0},
    {0x00402B90u, "menu_exit_game", 0, 0, 0, run_exit_game, 0},
    {0x00402BC0u, "menu_display_options", 0, 0, 0, run_display, 0},
    {0x00402BE0u, "menu_difficulty_options", 0, 0, 0, run_difficulty, 0},
    {0x00402E30u, "menu_window_45", 0, 0, 0, run_45, 0},
    {0x00402EE0u, "menu_sound_options", 0, 0, 0, run_sound, 0},
    {0x004032C0u, "menu_speed_options", 0, 0, 0, run_speed, 0},
    {0x004033F0u, "menu_help", 0, 0, 0, run_help, 0},
    {0x00403400u, "menu_about", 0, 0, 0, run_about, 0},
    {0x00403410u, "menu_mouse_help", 0, 0, 0, run_mouse, 0},
    {0x00403490u, "menu_warnings", 0, 0, 0, run_warnings, 0},
    {0x00403500u, "menu_toggle_eec", 0, 0, 0, run_eec, 0},
    {0x00403540u, "menu_toggle_ec3", 0, 0, 0, run_ec3, 0},
    {0x0040C7D0u, "menu_toggle_9088", 0, 0, 0, run_9088, 0},
};
const int menu_item_ports_count = sizeof(menu_item_ports) / sizeof(menu_item_ports[0]);
