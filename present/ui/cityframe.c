/* Imhotep - the city view's remaining per-frame and input helpers (roadmap phase 7.6, package
 * frontier7).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Functions of window 1's per-frame chain (0x004D75D0) and input chain (0x004D8310) that no window
 * package owned: the construction cost shown at the hovered tile (0x0043E1A0), the tutorial checks
 * (0x00417B40), the scrolling direction (0x004CF660, rate-limited on the clock by 0x004DDD60), the
 * animation-object debug step (0x004E61F0), the cheat input window 0x55 (draw 0x0041D210, input
 * 0x0041D310 -> 0x0041D3F0 -> 0x0041D050), the panel 0x00414C40, the build menu helpers
 * 0x00403F10 and 0x00405170, and the overlay level images of overlays 0x21..0x2A (0x00434AA0,
 * x87: reproduced on integers, core/city/day_x87.h). */
#include <string.h>

#include "ui.h"

#include "city/day_x87.h"
#include "city/month_message.h"
#include "city/monument_rating_x87.h"
#include "frame/frame.h"
#include "frame/view.h"
#include "present_layout.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "sim/x87_float.h"
#include "state/ported.h"


/* ---- the construction cost at the hovered tile ---- */

void city_cost_draw(game_state *st)
{
    /* @bin 0x0043E1A0 with a hovered tile (0x00BCA300), no scrolling (0x00E38DCC) and a cost
     * 0x00BC9724: the clip set to the city viewport (0x0070E430..), the cost drawn twice ("@"
     * prefix, small font) at the tile's screen point (0x004F0BA0) + (0x0070E458, 0): a black shadow
     * one pixel right and down, then the text, 0xFAC1 when the treasury (city +0x2C54 of the
     * current player, 0x00EA187C) covers it, else red 0xF800; the cost cleared, the clip back to the
     * whole screen */
    if (S32(st, G_HOVER_TILE) == 0 || S32(st, G_SCROLLING) != 0 || S32(st, G_PREVIEW_COST) == 0) {
        return;
    }
    int32_t x, y;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &x, &y);
    S32(st, G_CLIP_LEFT) = S32(st, G_CV_VIEW_LEFT);
    S32(st, G_CLIP_RIGHT) = S32(st, G_CV_VIEW_RIGHT);
    S32(st, G_CLIP_TOP) = S32(st, G_CV_VIEW_TOP);
    S32(st, G_CLIP_BOTTOM) = S32(st, G_CV_VIEW_BOTTOM);
    int32_t cost = S32(st, G_PREVIEW_COST);
    int32_t colour = cost <= S32(st, CITY_BASE + CITY_TREASURY + (uint32_t) (S32(st, G_PLAYER) * 0x49D8)) ? 0xFAC1 : 0xF800;
    int32_t ty = (int32_t) ((uint32_t) y + (uint32_t) S32(st, G_TILE_H));
    int32_t off = S32(st, G_TILE_H);
    GAME_CALL(st, FN_NUMBER_DRAW, cost, 0x40, (int32_t) G_NUMBER_SUFFIX, S32(st, G_U70E458) + x + 1, ty - off + 1,
              S32(st, G_FONT_SMALL), 0);
    GAME_CALL(st, FN_NUMBER_DRAW, S32(st, G_PREVIEW_COST), 0x40, (int32_t) G_NUMBER_SUFFIX, S32(st, G_U70E458) + x,
              ty - S32(st, G_TILE_H), S32(st, G_FONT_SMALL), colour);
    S32(st, G_PREVIEW_COST) = 0;
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_RIGHT) = S32(st, G_SCREEN_W);
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

/* ---- the tutorial ---- */

void tutorial_checks(game_state *st)
{
    /* @bin 0x00417B40 outside the editor and the custom games (0x00E38EBC = 0): the tutorial steps
     * 0x0F..0x19 not done yet (u8 0x007027C7 + k) whose mission (i32 0x00579654 + 0x1C k) is the
     * current one (i8 0x00E38EA8) (0x00417AE0); then once (0x006FDCB4): the message 0x83 on the
     * goods asked for (three u8 flags 0x006FDCC4 / D0 / DC with their i16 0x00784D2A / C / E) */
    if (S32(st, G_CUSTOM) != 0) {
        return;
    }
    for (uint32_t k = 0; k < 11; k++) {
        if (U8(st, G_TUTORIAL_DONE + k) == 0 && S32(st, G_TUTORIAL_MISSIONS + 0x1Cu * k) == (int8_t) U8(st, G_MISSION)) {
            GAME_CALL(st, FN_TUTORIAL_STEP, (int32_t) (0x0F + k));
        }
    }
    if (U8(st, G_FORMATIONS_KEEP) != 0) {
        return;
    }
    int a = U8(st, G_CARRY_GROUP_0) != 0 && S16(st, G_ALLOWED_BUILDINGS + 2u * 0x25u) != 0;
    int b = U8(st, G_CARRY_GROUP_1) != 0 && S16(st, G_ALLOWED_BUILDINGS + 2u * 0x26u) != 0;
    int c = U8(st, G_CARRY_GROUP_2) != 0 && S16(st, G_ALLOWED_BUILDINGS + 2u * 0x27u) != 0;
    if (!a && !b && !c) {
        U8(st, G_FORMATIONS_KEEP) = 1;
        return;
    }
    /* @bin 0x00417D31 the message's 30-byte block (its phrases, i16 each: +0 the body, +2 the title,
     * +4 the subtitle, ...) is a buffer of the original's frame: 0x249 at +0, 0x248 at +2, the goods'
     * text at +0x10 */
    uint8_t block[30];
    /* @unverified UNINIT-417B40 the other 24 bytes are never written: the original sends what its
     * stack held (in the oracle +4 is 0x2BDC or 1 by the state, the low half of a heap address), so
     * the subtitle phrase +4 is arbitrary; here they are -1, the block's "no phrase" (as 0x004E0CF0
     * writes for a message without a block) */
    memset(block, 0xFF, sizeof(block));
    int16_t phrase;
    /* @bin 0x00417D3F the goods' text: all three 0x250; first and second 0x24D; first and third 0x24E;
     * first 0x24A; second and third 0x24F; second 0x24B; third 0x24C */
    if (a) {
        phrase = b ? (c ? 0x250 : 0x24D) : (c ? 0x24E : 0x24A);
    } else if (b && c) {
        phrase = 0x24F;
    } else {
        phrase = b ? 0x24B : 0x24C;
    }
    const int16_t body = 0x249, title = 0x248;
    memcpy(block, &body, 2);
    memcpy(block + 2, &title, 2);
    memcpy(block + 0x10, &phrase, 2);
    /* @bin 0x00417DA8 0x00E950DC := 1, the message 0x83 posted (0x004E0CF0(0x83, 0, 0, block, 0)),
     * then once only (0x006FDCB4 := 1) */
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x83, 0, 0, block, 0);
    U8(st, G_FORMATIONS_KEEP) = 1;
}

/* ---- scrolling ---- */

int32_t scroll_due(game_state *st)
{
    /* @bin 0x004DDD60 the clock (timeGetTime) since the last scroll step 0x00E94DB4 (negative:
     * 10000) against ((100 - scroll speed 0x00E38E70) / 10) * 12 + 2 ms: 1 (the time kept) when due
     * and the speed step is below 10 */
    uint32_t now = platform_clock_ms(st);
    int32_t dt = (int32_t) (now - U32(st, G_SCROLL_TIME));
    if (dt < 0) {
        dt = 10000;
    }
    int32_t i = (100 - S32(st, G_SCROLL_SPEED)) / 10;
    if (i < 10 && dt >= i * 12 + 2) {
        U32(st, G_SCROLL_TIME) = now;
        return 1;
    }
    return 0;
}

int32_t scroll_direction(game_state *st)
{
    /* @bin 0x004CF660 8 (none) unless a step is due (0x004DDD60); the cursor (0x00E38DC4,
     * 0x00E38DC0) on an edge of the screen (<= 0, >= size - 1: 0x00E38DB8, 0x00E38DB4) or the arrow
     * flags (left 0x00E92E30, right 0x00E92E2C, up 0x00E92E38, down 0x00E92E34, cleared) set the
     * scrolling flag 0x00E38DCC; the direction: 7 up-left, 5 down-left, 1 up-right, 3 down-right, 6
     * left, 0 up, 4 down, 2 right, 8 none */
    if (scroll_due(st) == 0) {
        return 8;
    }
    int32_t cx = S32(st, G_CURSOR_POS_X);
    int left = 0, right = 0, up = 0, down = 0;
    S32(st, G_SCROLLING) = 0;
    if (cx <= 0) {
        left = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (cx >= S32(st, G_DESKTOP_W) - 1) {
        right = 1;
        S32(st, G_SCROLLING) = 1;
    }
    int32_t cy = S32(st, G_CURSOR_POS_Y);
    if (cy <= 0) {
        up = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (cy >= S32(st, G_DESKTOP_H) - 1) {
        down = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (S32(st, G_ARROW_KEY_LEFT) != 0) {
        left = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (S32(st, G_ARROW_KEY_RIGHT) != 0) {
        right = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (S32(st, G_ARROW_KEY_UP) != 0) {
        up = 1;
        S32(st, G_SCROLLING) = 1;
    }
    if (S32(st, G_ARROW_KEY_DOWN) != 0) {
        down = 1;
        S32(st, G_SCROLLING) = 1;
    }
    S32(st, G_ARROW_KEY_RIGHT) = 0;
    S32(st, G_ARROW_KEY_LEFT) = 0;
    S32(st, G_ARROW_KEY_DOWN) = 0;
    S32(st, G_ARROW_KEY_UP) = 0;
    if (left) {
        if (up) {
            return 7;
        }
        if (down) {
            return 5;
        }
    }
    if (right) {
        if (up) {
            return 1;
        }
        if (down) {
            return 3;
        }
    }
    if (left) {
        return 6;
    }
    if (up) {
        return 0;
    }
    if (down) {
        return 4;
    }
    return right ? 2 : 8;
}

/* ---- the animation-object debug step ---- */

void anim_debug_step(game_state *st)
{
    /* @bin 0x004E61F0 the clock (timeGetTime) into 0x012651A4, its step since the last frame into
     * 0x012651A0, kept in 0x012651A8. With the debug switch 0x006044B4: F2 toggles 0x006044B0; F5 /
     * F6 lower / raise the float 0x006044AC by the float 0x0056F484 (x87, stored as a float), back to
     * 10.0 below the float 0x0056F6E4 / to 80.0 above 0x0056F6E0. Then the objects 0x00EA136C (its
     * step 0x0055D6D0 and its method +0x18 (0x15E, 300)) and 0x00EA1370 (step, then its current
     * frame __ftol(float +0x48) of the list +0x44 drawn at (0x1C2 + frame height, 300) with the RLE
     * blitters) */
    uint32_t now = platform_clock_ms(st);
    U32(st, G_FRAME_NOW_MS) = now;
    U32(st, G_FRAME_STEP_MS) = now - U32(st, G_FRAME_LAST_MS);
    U32(st, G_FRAME_LAST_MS) = now;
    if (S32(st, G_ANIM_DEBUG) != 0) {
        if ((frame_key_state(st, 0x71) & 0x8001) == 0x8001) {
            S32(st, G_ANIM_DEBUG_TOGGLE) = S32(st, G_ANIM_DEBUG_TOGGLE) == 0;
        }
        if ((frame_key_state(st, 0x74) & 0x8001) == 0x8001) {
            x87_val v = x87_sub(x87_from_float(U32(st, G_ANIM_DEBUG_VALUE)), x87_from_float(U32(st, G_FLT_1)));
            U32(st, G_ANIM_DEBUG_VALUE) = x87_to_float_bits(v);
            if (x87_cmp(v, x87_from_float(U32(st, G_FLT_10))) < 0) {
                U32(st, G_ANIM_DEBUG_VALUE) = 0x41200000u; /* 10.0f */
            }
        }
        if ((frame_key_state(st, 0x75) & 0x8001) == 0x8001) {
            x87_val v = x87_add(x87_from_float(U32(st, G_ANIM_DEBUG_VALUE)), x87_from_float(U32(st, G_FLT_1)));
            U32(st, G_ANIM_DEBUG_VALUE) = x87_to_float_bits(v);
            if (x87_cmp(v, x87_from_float(U32(st, G_FLT_80))) > 0) {
                U32(st, G_ANIM_DEBUG_VALUE) = 0x42A00000u; /* 80.0f */
            }
        }
    }
    uint32_t a = U32(st, G_DEBUG_ANIM1);
    if (a != 0) {
        GAME_CALLT0(st, a, 0x0055D6D0u);
        a = U32(st, G_DEBUG_ANIM1);
        GAME_CALLT(st, a, P32(st, P32(st, a) + 0x18), 0x15E, 0x12C);
    }
    uint32_t o = U32(st, G_DEBUG_ANIM2);
    if (o == 0 || P32(st, o + 0x38) == 0 || P32(st, P32(st, o + 0x38) + 8) == 0) {
        return;
    }
    GAME_CALLT0(st, o, 0x0055D6D0u);
    o = U32(st, G_DEBUG_ANIM2);
    int16_t frame = (int16_t) x87_ftol(x87_from_float(P32(st, o + 0x48)));
    uint32_t f = (uint32_t) ((int32_t) frame << 4) + P32(st, P32(st, o + 0x44) + 4);
    int16_t h = P8(st, f + 1) != 0 ? (int16_t) P16(st, P32(st, f + 8) + 2) : (int16_t) P16(st, f + 8);
    U16(st, G_UD3B5BE) = P16(st, f + 2);
    U16(st, G_UD3B5BC) = P16(st, f + 4);
    S32(st, G_BLIT_X) = h + 0x1C2;
    S32(st, G_BLIT_Y) = 0x12C;
    GAME_CALL0(st, FN_CLIP_HORIZONTAL);
    GAME_CALL0(st, FN_CLIP_VERTICAL);
    if (U8(st, G_CLIP_Y) == 5) {
        return;
    }
    if (U8(st, G_CLIP_X) == 1) {
        GAME_CALL(st, FN_BLIT_RLE_LEFT, (int32_t) P32(st, f + 0xC));
    } else if (U8(st, G_CLIP_X) == 2) {
        GAME_CALL(st, FN_BLIT_RLE_RIGHT, (int32_t) P32(st, f + 0xC));
    } else {
        GAME_CALL(st, FN_BLIT_RLE, (int32_t) P32(st, f + 0xC));
    }
}

/* ---- the cheat input (window 0x55) ---- */

void cheat_window_draw(game_state *st)
{
    /* @bin 0x0041D210 the panel (24 x 8), the OK button (0x00536C30, lit when hovered 0x007027E4 =
     * 1) and its text (group 0xBF, 1), the field's frame (20 x 2), the typed text 0x007027EC (font
     * 0x00F56F9C, after 0x004CC540) and the caret */
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_LAYOUT_DX) + 0xA, S32(st, G_LAYOUT_DY) + 0xA0, 0x18, 8);
    int32_t lit = S32(st, G_CHEAT_HOVER) == 1;
    GAME_CALL(st, FN_PANEL_DRAW_FRAME, 0, S32(st, G_LAYOUT_DX) + 0x32, S32(st, G_LAYOUT_DY) + 0xF6, 0x96, 0x1E, lit);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0xBF, 1, S32(st, G_LAYOUT_DX) + 0x37, S32(st, G_LAYOUT_DY) + 0x100, S32(st, G_FONT_LARGE), lit);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x2A, S32(st, G_LAYOUT_DY) + 0xD0, 0x14, 2);
    GAME_CALL0(st, FN_TEXT_RUN_RESET);
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) G_CHEAT_TEXT, S32(st, G_LAYOUT_DX) + 0x3A, S32(st, G_LAYOUT_DY) + 0xD8, S32(st, G_FONT_9C), 0);
    GAME_CALL(st, FN_TEXT_CARET_DRAW, S32(st, G_LAYOUT_DX) + 0x3A, S32(st, G_LAYOUT_DY) + 0xD9, 0xFFFF);
}

uint32_t crt_strncpy(game_state *st, uint32_t dst, uint32_t src, uint32_t n)
{
    /* @bin 0x00562240 strncpy: at most n bytes of src, the rest of the n bytes zeros; returns dst */
    uint32_t i = 0;
    for (; i < n && P8(st, src + i) != 0; i++) {
        P8(st, dst + i) = P8(st, src + i);
    }
    for (; i < n; i++) {
        P8(st, dst + i) = 0;
    }
    return dst;
}

int32_t cheat_lookup(game_state *st, uint32_t text)
{
    /* @bin 0x0041D050 the text (strncpy 0x00562240 into 100 bytes of the frame, the last one 0) with every byte XOR 3,
     * looked for in the table 0x0057C3C8 (34 entries of 12 bytes: the coded name, the function, a
     * flag; with the gate 0x00578C68 set only the flagged entries count): its index, the count 0x22
     * when none matches; -1 before the first cheat (0x007027E0 = 0) */
    uint8_t s[100];
    uint32_t i = 0;
    for (; i < 100 && P8(st, text + i) != 0; i++) {
        s[i] = P8(st, text + i);
    }
    for (; i < 100; i++) {
        s[i] = 0;
    }
    s[99] = 0;
    for (i = 0; s[i] != 0; i++) {
        s[i] ^= 3;
    }
    if (U8(st, G_CHEATS_ON) == 0) {
        return -1;
    }
    int32_t k = 0;
    for (uint32_t e = G_CHEAT_TABLE; e < G_CHEAT_TABLE + 34u * 12u; e += 12, k++) {
        if (U8(st, G_DEBUG_OFF) != 0 && U8(st, e + 8) == 0) {
            continue;
        }
        uint32_t name = U32(st, e);
        uint32_t j = 0;
        while (s[j] == P8(st, name + j) && s[j] != 0) {
            j++;
        }
        if (s[j] == P8(st, name + j)) {
            return k;
        }
    }
    return k;
}

void cheat_enter(game_state *st)
{
    /* @bin 0x0041D3F0 back to the window 0x007027E8, the typed cheat run (0x0041D010) */
    U8(st, G_WINDOW) = U8(st, G_CHEAT_BACK);
    GAME_CALL(st, FN_CHEAT_RUN, cheat_lookup(st, G_CHEAT_TEXT));
}

void cheat_window_input(game_state *st)
{
    /* @bin 0x0041D310 the field 7 edited (0x004CC500); the OK button hovered (0x00426450) and
     * clicked (0x004261D0, table 0x0057C560); Enter (0x00E20998) without a click: the notice 0xD9
     * the first time, cheats on, the cheat entered */
    GAME_CALL(st, FN_TEXT_BOX_FOCUS, 7);
    S32(st, G_CHEAT_HOVER) = GAME_CALL(st, FN_BUTTONS_HOVERED, S32(st, G_LAYOUT_DX) + 0x32, S32(st, G_LAYOUT_DY) + 0xF6, (int32_t) G_CHEAT_OK_RECT, 1);
    if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_LAYOUT_DX) + 0x32, S32(st, G_LAYOUT_DY) + 0xF6, (int32_t) G_CHEAT_OK_RECT, 1) != 0) {
        return;
    }
    if (S32(st, G_UE20998) == 0) {
        return;
    }
    if (U8(st, G_CHEATS_ON) == 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xD9, 0, 0);
    }
    U8(st, G_CHEATS_ON) = 1;
    cheat_enter(st);
}

/* ---- a panel of two texts ---- */

void notice_panel_draw(game_state *st)
{
    /* @bin 0x00414C40 the panel (24 x 7) and the texts (group 0xB, 5 large title font, 6 in
     * 0x00F56F94) centred in 0x180 pixels */
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0xA0, 0x18, 7);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0xB, 5, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0xBC, 0x180, S32(st, G_FONT_TITLE), 0);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0xB, 6, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0xDC, 0x180, S32(st, G_FONT_ROW), 0);
}

/* ---- the build menus ---- */

int32_t build_menu_next_item(game_state *st, int32_t menu, int32_t item)
{
    /* @bin 0x00403F10 the next item after `item` of build menu `menu` (i32 table 0x005CE3BC, 30 per
     * menu, ended by a value <= 0) that can be shown, or 0: by default when its flag 0x0070A8DC is
     * set; menu 6: monuments (0x30) with the gate 0x00403EF0 while placed < planned, item 0x61 of the
     * open menu 0x007056EC by the city's flag (i16 +0x2C10 of the current player: then 0x004C52D0 of
     * its building +0x2DC4 with 1 and 2) or its flag, others by their flag; menu 0x0A: item 4 with the
     * gate when a monument 0xB3, 0xB2 or 0xB1 can be opened (0x004F5470), others by their flag; menu
     * 0x16: with the gate, the monument (0x004F5470); menu 0x1C: with the gate, when the city's dword
     * +0x2DC8 + 4 item is 0; menu 0x24: when 0x004C52D0 of the building is 0 for item + 1 */
    int32_t i = item + 1;
    if (i >= 0x1E) {
        return 0;
    }
    int32_t next = i + 1;
    uint32_t at = (uint32_t) (i + menu * 30) * 4u;
    for (;;) {
        int32_t v = S32(st, G_MENU_TYPES_1 + at);
        if (v <= 0) {
            return 0;
        }
        uint32_t city = (uint32_t) S32(st, G_PLAYER);
        switch (menu) {
        case 6:
            if (v == 0x30) {
                if ((uint8_t) GAME_CALL0(st, FN_MENU_CHEAT_GATE) != 0) {
                    int32_t placed = GAME_CALL0(st, FN_MONUMENTS_PLACED);
                    if (placed < GAME_CALL0(st, FN_MONUMENTS_PLANNED)) {
                        return i;
                    }
                }
                break;
            } else {
                uint32_t open = (uint32_t) (i + S32(st, G_SUBMENU) * 30) * 4u;
                if (S32(st, G_MENU_TYPES_1 + open) != 0x61) {
                    if (S32(st, G_MENU_AVAIL + at) != 0) {
                        return i;
                    }
                    break;
                }
                if (S16(st, CITY_BASE + CITY_U4478 + city * 0x49D8u) == 0) {
                    if (S32(st, G_MENU_AVAIL + open) != 0) {
                        return i;
                    }
                    break;
                }
                if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CITY_BASE + CITY_U472C + city * 0x49D8u), 1) == 0) {
                    return i;
                }
                city = (uint32_t) S32(st, G_PLAYER);
                if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CITY_BASE + CITY_U472C + city * 0x49D8u), 2) == 0) {
                    return i;
                }
                break;
            }
        case 0x0A:
            if (v != 4) {
                if (S32(st, G_MENU_AVAIL + at) != 0) {
                    return i;
                }
                break;
            }
            if ((uint8_t) GAME_CALL0(st, FN_MENU_CHEAT_GATE) != 0) {
                if ((uint8_t) GAME_CALL(st, FN_MONUMENT_MENU_OPEN, 0xB3) != 0 || (uint8_t) GAME_CALL(st, FN_MONUMENT_MENU_OPEN, 0xB2) != 0 ||
                    (uint8_t) GAME_CALL(st, FN_MONUMENT_MENU_OPEN, 0xB1) != 0) {
                    return i;
                }
            }
            break;
        case 0x16:
            if ((uint8_t) GAME_CALL0(st, FN_MENU_CHEAT_GATE) != 0) {
                if ((uint8_t) GAME_CALL(st, FN_MONUMENT_MENU_OPEN, S32(st, G_MENU_TYPES_1 + at)) != 0) {
                    return i;
                }
            }
            break;
        case 0x1C:
            if ((uint8_t) GAME_CALL0(st, FN_MENU_CHEAT_GATE) != 0) {
                city = (uint32_t) S32(st, G_PLAYER);
                if (S32(st, CITY_BASE + CITY_MONUMENT_CODE + (city * 0x1276u + (uint32_t) i) * 4u) == 0) {
                    return i;
                }
            }
            break;
        case 0x24:
            if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CITY_BASE + CITY_U472C + city * 0x49D8u), next) == 0) {
                return i;
            }
            break;
        default:
            if (S32(st, G_MENU_AVAIL + at) != 0) {
                return i;
            }
            break;
        }
        i++;
        at += 4;
        next++;
        if (next >= 0x1F) {
            return 0;
        }
    }
}

int32_t build_menu_items(game_state *st, int32_t menu)
{
    /* @bin 0x00405170 the number of items of build menu `menu` (i32 table 0x005CF75C, 30 per menu)
     * before the first <= 0 */
    int32_t n = 0;
    for (uint32_t k = 0; k < 30; k++) {
        if (S32(st, G_OVL_ITEMS + (uint32_t) menu * 0x78u + 4u * k) <= 0) {
            return n;
        }
        n++;
    }
    return n;
}

/* ---- the overlay levels (overlays 0x21 .. 0x2A) ---- */

static int32_t level_of(game_state *st, int32_t v, int house)
{
    /* @bin 0x00434C81 fild v (house: fmul 4.0f 0x0056F550), fmul 0x0056F558, fmul 0x0056F554
     * (-9.0f), __ftol: 4 - it, at most 9 */
    x87_val x = x87_from_int(v);
    if (house) {
        x = x87_mul(x, x87_from_float(U32(st, G_FLT_4)));
    }
    x = x87_mul(x, x87_from_float(U32(st, G_FLT_0_0003125)));
    x = x87_mul(x, x87_from_float(U32(st, G_FLT_MINUS_9)));
    int32_t l = 4 - x87_ftol(x);
    return l > 9 ? 9 : l;
}

void overlay_level_draw(game_state *st, int32_t x, int32_t y, int32_t good, int32_t good2)
{
    /* @bin 0x00434AA0 the tile of the walk 0x0070E488: on terrain kinds 0x26341677 that are not a
     * building, a ground image (0x00B7482C) only on flagged tiles (0x00B67D1C & 0x40), shifted by the
     * footprint part (0x00B285CC & 0xF: the column / row offsets 0x0070E440..0x0070E45C); kinds 0x4100:
     * the image 0x00F56B8C + (0x00ACF85C & 7); other non-buildings: their ground image; buildings:
     * the level image 0x00F56CA4 + a level from the building's goods: a storage yard (0x46 / 0xBE)
     * its i16 +0x56 + 2 good2, a granary (0x47 / 0xBF) +0x56 + 2 good, a market (0x48 / 0xC1) or a
     * dock (0x49, its main part 0x00475B50) 0x0045D510 of good, a house +0x54 + 2 good2 x 4; others 0 */
    uint32_t t = U32(st, G_U70E488);
    uint32_t kind = U32(st, G_GRID_TERRAIN + 4u * t);
    int32_t id = S16(st, G_GRID_BUILDING + 2u * t);
    int32_t img;
    if ((kind & 0x26341677u) != 0 && (kind & 8) == 0) {
        if ((U8(st, G_GRID_EDGE + t) & 0x40) == 0) {
            return;
        }
        img = S32(st, G_GRID_IMAGE + 4u * t);
        S32(st, G_GLYPH_LAST) = img;
        switch (U8(st, G_GRID_BITFIELDS + t) & 0xF) {
        case 0: GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, x, y); return;
        case 1:
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, S32(st, G_TILE_W) + x + 1, y - S32(st, G_TILE_H));
            return;
        case 2:
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, S32(st, G_U70E458) + x + 2, y - S32(st, G_U70E448));
            return;
        case 3:
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, S32(st, G_CORNER_W3) + x + 3, y - S32(st, G_CORNER_H3));
            return;
        case 4:
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, S32(st, G_CORNER_W4) + x + 4, y - S32(st, G_CORNER_H4));
            return;
        case 5: {
            int32_t yy = y - S32(st, G_CORNER_H4) - S32(st, G_TILE_H);
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, S32(st, G_CORNER_W4) + S32(st, G_TILE_W) + x + 5, yy);
            return;
        }
        default: return;
        }
    }
    if ((kind & 0x4100u) != 0) {
        img = (int32_t) (U8(st, G_GRID_RANDOM + t) & 7) + S32(st, G_IMG_4100);
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, x, y);
        return;
    }
    if ((kind & 8) == 0) {
        img = S32(st, G_GRID_IMAGE + 4u * t);
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, x, y);
        return;
    }
    uint32_t b = (uint32_t) id * 0x108u;
    int32_t level;
    switch (S16(st, BUILDING_BASE + BLD_TYPE + b)) {
    case 0x46:
    case 0xBE:
        level = level_of(st, S16(st, BUILDING_BASE + BLD_INVENTORY + 2u * (uint32_t) (good2 + id * 0x84)), 0);
        break;
    case 0x47:
    case 0xBF:
        level = level_of(st, S16(st, BUILDING_BASE + BLD_INVENTORY + 2u * (uint32_t) (good + id * 0x84)), 0);
        break;
    case 0x48:
    case 0xC1:
        level = level_of(st, GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, good), 0);
        break;
    case 0x49: {
        int32_t main = GAME_CALL(st, FN_BUILDING_MAIN_PART, id);
        level = level_of(st, GAME_CALL(st, FN_BUILDING_PARTS_STOCK, main, good), 0);
        break;
    }
    default:
        if (U8(st, BUILDING_BASE + BLD_HOUSE_SIZE + b) != 0) {
            level = level_of(st, S16(st, BUILDING_BASE + BLD_HOUSE_INVENTORY + 2u * (uint32_t) (good2 + id * 0x84)), 1);
        } else {
            level = 0;
        }
        break;
    }
    img = S32(st, G_IMG_OVERLAY_LEVELS) + level;
    S32(st, G_GLYPH_LAST) = img;
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, img, x, y);
}

#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(city_cost_draw)
RUN0(tutorial_checks)
RUN0(anim_debug_step)
RUN0(cheat_window_draw)
RUN0(cheat_enter)
RUN0(cheat_window_input)
RUN0(notice_panel_draw)
static int32_t run_scroll_due(game_state *st, const int32_t *a) { (void) a; return scroll_due(st); }
static int32_t run_scroll_direction(game_state *st, const int32_t *a) { (void) a; return scroll_direction(st); }
static int32_t run_cheat_lookup(game_state *st, const int32_t *a) { return cheat_lookup(st, (uint32_t) a[0]); }
static int32_t run_strncpy(game_state *st, const int32_t *a)
{
    return (int32_t) crt_strncpy(st, (uint32_t) a[0], (uint32_t) a[1], (uint32_t) a[2]);
}
static int32_t run_next_item(game_state *st, const int32_t *a) { return build_menu_next_item(st, a[0], a[1]); }
static int32_t run_count(game_state *st, const int32_t *a) { return build_menu_items(st, a[0]); }
static int32_t run_level(game_state *st, const int32_t *a)
{
    overlay_level_draw(st, a[0], a[1], a[2], a[3]);
    return 0;
}

const ported_function ui_cityframe_ports[] = {
    {0x0043E1A0u, "city_cost_draw", 0, 0, 0, run_city_cost_draw, 0},
    {0x00417B40u, "tutorial_checks", 0, 0, 0, run_tutorial_checks, 0},
    {0x004DDD60u, "scroll_due", 0, 32, 0, run_scroll_due, 0},
    {0x004CF660u, "scroll_direction", 0, 32, 0, run_scroll_direction, 0},
    {0x004E61F0u, "anim_debug_step", 0, 0, 0, run_anim_debug_step, 0},
    {0x0041D210u, "cheat_window_draw", 0, 0, 0, run_cheat_window_draw, 0},
    {0x0041D050u, "cheat_lookup", 1, 32, 0, run_cheat_lookup, 0},
    {0x00562240u, "crt_strncpy", 3, 32, 0, run_strncpy, 0},
    {0x0041D3F0u, "cheat_enter", 0, 0, 0, run_cheat_enter, 0},
    {0x0041D310u, "cheat_window_input", 0, 0, 0, run_cheat_window_input, 0},
    {0x00414C40u, "notice_panel_draw", 0, 0, 0, run_notice_panel_draw, 0},
    {0x00403F10u, "build_menu_next_item", 2, 32, 0, run_next_item, 0},
    {0x00405170u, "build_menu_items", 1, 32, 0, run_count, 0},
    {0x00434AA0u, "overlay_level_draw", 4, 0, 0, run_level, 0},
};
const int ui_cityframe_ports_count = sizeof(ui_cityframe_ports) / sizeof(ui_cityframe_ports[0]);
