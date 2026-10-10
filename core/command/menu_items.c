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
#include "state/layout.h"
#include "state/ported.h"


void chunk175_defaults(game_state *st)
{
    /* @bin 0x004D3C40 three records of 12 bytes at 0x006FDCBC and three at 0x006FDCE0 (the first
     * dword of 0x006FDCE0 := 0 instead): +0 u8 := 0xFF, +4 i32 := -1, +8 u8 := 0; in the original's
     * order of writes */
    U32(st, G_CARRY_COPY) = 0;
    U32(st, G_CARRY_COPY_GROUPS + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_COPY_GROUPS + 8u) = 0;
    U8(st, G_CARRY_COPY_GROUPS) = 0xFF;
    U32(st, G_CARRY_COPY_GROUPS + 12u + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_COPY_GROUPS + 12u + 8u) = 0;
    U8(st, G_CARRY_COPY_GROUPS + 12u) = 0xFF;
    U32(st, G_CARRY_COPY_GROUPS + 24u + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_COPY_GROUPS + 24u + 8u) = 0;
    U8(st, G_CARRY_COPY_GROUPS + 24u) = 0xFF;
    U32(st, G_CARRY_GROUPS + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_GROUPS + 8u) = 0;
    U8(st, G_CARRY_GROUPS) = 0xFF;
    U32(st, G_CARRY_GROUPS + 12u + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_GROUPS + 12u + 8u) = 0;
    U8(st, G_CARRY_GROUPS + 12u) = 0xFF;
    U32(st, G_CARRY_GROUPS + 24u + 4u) = 0xFFFFFFFFu;
    U8(st, G_CARRY_GROUPS + 24u + 8u) = 0;
    U8(st, G_CARRY_GROUPS + 24u) = 0xFF;
}

static void menu_exit_to_main(game_state *st)
{
    /* @bin 0x004023A0 File 1: the chunk 175 defaults (0x004D3C40); 0x0078D7B0, 0x0078D7A8,
     * 0x0078D7A4, 0x0078D7A0, 0x006059E8, 0x00BC9694, 0x00E94E0C, 0x00E94E10 := 0 */
    chunk175_defaults(st);
    U32(st, G_BLOOD_RIVER_LEFT) = 0;
    U32(st, G_FROGS_COUNTDOWN) = 0;
    U32(st, G_PYRAMID_CURSE_COUNTDOWN) = 0;
    U32(st, G_HAIL_TIMER) = 0;
    U32(st, G_CONSTRUCTION_UI_TYPE) = 0;
    U32(st, G_UNDO_POSSIBLE) = 0;
    U32(st, G_OVERLAY) = 0;
    U32(st, G_OVERLAY_PREV) = 0;
    if (S32(st, G_EDITOR) == 1) {
        /* @bin 0x004023E7 the editor: window 0, redraw */
        U8(st, G_WINDOW) = 0;
        U8(st, G_REDRAW) = 1;
    } else {
        /* @bin 0x00402400 by the game mode 0x007027B4 (u8; jump table 0x0040243C): 1 the family
         * list (0x00516BB0, 0x00516990); 0, 2, 3 the main menu (0x004CB6A0) */
        uint8_t mode = U8(st, G_CAMPAIGN_KIND);
        if (mode == 1) {
            GAME_CALL0(st, FN_FAMILY_LIST_OPEN);
            GAME_CALL0(st, FN_FAMILY_DONE);
        } else if (mode <= 3) {
            GAME_CALL0(st, FN_MAIN_MENU_OPEN);
        }
    }
    /* @bin 0x00402418 the main menu's music when music is on (0x00E38E64 u8) and 0x00F55B88 */
    if (U8(st, G_MUSIC_ON) != 0 && S32(st, G_SOUND_ON) != 0) {
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, G_STR_SETUP_MUSIC, 0, 0);
    }
}

static void menu_exit_game(game_state *st)
{
    /* @bin 0x00402B90 File 6: PostMessageA(0x00E39140, WM_KEYDOWN, VK_ESCAPE, 0) */
    platform_post_message(st, 0x100, 0x1B, 0);
}

static void open_window(game_state *st, uint8_t w)
{
    U8(st, G_WINDOW) = w;
}

static void menu_display_options(game_state *st)
{
    /* @bin 0x00402BC0 Options 1: window 10, 0x00F53F18 := 0, redraw */
    open_window(st, 0x0A);
    U32(st, G_UF53F18) = 0;
    U8(st, G_REDRAW) = 1;
}

static void menu_difficulty_options(game_state *st)
{
    /* @bin 0x00402BE0 Options 4: window 0x56, 0x006DE510 := 0x00E38EF0, 0x00F53F4C := 0, redraw */
    uint32_t v = U32(st, G_MESSAGE_SETTINGS);
    open_window(st, 0x56);
    U32(st, G_MESSAGE_OPTIONS_EDIT) = v;
    U32(st, G_UF53F4C) = 0;
    U8(st, G_REDRAW) = 1;
}

static void menu_window_45(game_state *st)
{
    /* @bin 0x00402E30 Options 7: window 0x45, 0x00F53F18 := 0, redraw */
    open_window(st, 0x45);
    U32(st, G_UF53F18) = 0;
    U8(st, G_REDRAW) = 1;
}

static void menu_sound_options(game_state *st)
{
    /* @bin 0x00402EE0 Options 2: the sound settings kept to cancel (0x006057A4..0x006057C0 from
     * 0x00E38ED0, 0x00E38EC0 low byte, 0x00E38EC4, 0x00E38E63 u8, 0x00E38E64 second byte,
     * 0x00E38ECC, 0x00E38E64 low byte, 0x00E38EC8), window 0x0B, 0x00F53F1C := 0, redraw */
    uint32_t e64 = U32(st, G_MUSIC_ON);
    U32(st, G_SAVED_MUSIC_VOLUME) = U32(st, G_MUSIC_VOLUME);
    U32(st, G_SAVED_MUSIC_ON) = e64 & 0xFF;
    U32(st, G_SAVED_SPEECH_VOLUME) = U32(st, G_SPEECH_VOLUME);
    U32(st, G_SAVED_E38E65) = (e64 >> 8) & 0xFF;
    U32(st, G_SAVED_EFFECTS_ON) = U8(st, G_SOUND_EFFECTS_ON);
    U32(st, G_SAVED_EFFECTS_VOLUME) = U32(st, G_EFFECTS_VOLUME);
    U32(st, G_SAVED_CITY_SOUNDS) = U32(st, G_CITY_SOUNDS_ON) & 0xFF;
    U32(st, G_SAVED_CITY_VOLUME) = U32(st, G_CITY_VOLUME);
    open_window(st, 0x0B);
    U32(st, G_UF53F1C) = 0;
    U8(st, G_REDRAW) = 1;
}

static void menu_speed_options(game_state *st)
{
    /* @bin 0x004032C0 Options 3: the speeds kept to cancel (0x006057A0 := 0x00E38E6C, 0x0060579C :=
     * 0x00E38E70), window 0x0C, 0x00F53F20 := 0, redraw */
    U32(st, G_SAVED_GAME_SPEED) = U32(st, G_GAME_SPEED);
    U32(st, G_SAVED_SCROLL_SPEED) = U32(st, G_SCROLL_SPEED);
    open_window(st, 0x0C);
    U32(st, G_UF53F20) = 0;
    U8(st, G_REDRAW) = 1;
}

static void menu_help(game_state *st)
{
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 10, 0, 0); /* @bin 0x004033F0 Help 1: the help window, topic 10 */
}

static void menu_about(game_state *st)
{
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0, 0, 0); /* @bin 0x00403400 Help 4: the help window, topic 0 */
}

static void menu_mouse_help(game_state *st)
{
    /* @bin 0x00403410 Help 2: the mouse help level 0x00E38EAC (i32) 2 -> 0 -> 1 -> 2; the item's
     * text (words 0x005C9022 and 0x005C9062) 2, 3, 4 for 0, 1, 2; the menu redrawn */
    int32_t v = S32(st, G_TIPS_ON);
    if (v == 2) {
        S32(st, G_TIPS_ON) = 0;
        U16(st, G_MENU_TEXT_TIPS) = 2;
        U16(st, G_MENU_TEXT_TIPS2) = 2;
    } else if (v == 0) {
        S32(st, G_TIPS_ON) = 1;
        U16(st, G_MENU_TEXT_TIPS) = 3;
        U16(st, G_MENU_TEXT_TIPS2) = 3;
    } else {
        S32(st, G_TIPS_ON) = 2;
        U16(st, G_MENU_TEXT_TIPS) = 4;
        U16(st, G_MENU_TEXT_TIPS2) = 4;
    }
    S32(st, G_MENU_REDRAW) = 1;
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
    S32(st, G_MENU_REDRAW) = 1;
}

static void menu_warnings(game_state *st)
{
    toggle8(st, G_NOTICES_ON, G_MENU_TEXT_WARNINGS, 5, 6); /* @bin 0x00403490 Help 3: the warnings */
}

static void menu_toggle_eec(game_state *st)
{
    /* @bin 0x00403500 Options 8: 0x00E38EEC (i32) on / off, text 0x005C8FF2 9 / 10 */
    if (S32(st, G_UE38EEC) != 0) {
        S32(st, G_UE38EEC) = 0;
        U16(st, G_MENU_TEXT_EEC) = 9;
    } else {
        U16(st, G_MENU_TEXT_EEC) = 10;
        S32(st, G_UE38EEC) = 1;
    }
    S32(st, G_MENU_REDRAW) = 1;
}

static void menu_toggle_ec3(game_state *st)
{
    toggle8(st, G_UE38EC3, G_MENU_TEXT_EC3, 4, 5); /* @bin 0x00403540 Options 9: 0x00E38EC3 */
}

static void menu_toggle_9088(game_state *st)
{
    toggle8(st, G_EMPIRE_LANGUAGE, G_MENU_TEXT_LANGUAGE, 7, 8); /* @bin 0x0040C7D0 Options 6: 0x00E39088 */
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
