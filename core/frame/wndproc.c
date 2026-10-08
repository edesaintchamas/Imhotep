/* Imhotep - the window procedure 0x00414CD0: the state effects of the window messages.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * docs/presentation.md 5.1 and 5.3 (read from the disassembly: Ghidra did not make the WndProc a
 * function). The platform calls frame_wndproc once per window message, before the frame, as Windows
 * does with the original. The input globals (mouse position and buttons, key flags), the pause P
 * 0x00E38E60, the speed keys, the overlay and advisor keys, the formation keys of window 0x1E and the
 * ship and legion order keys are written here; the commands they reach are called through GAME_CALL
 * (core/command and others). What only drives Windows (window position, surfaces, the cursor, posted
 * messages, the quit message) has no state effect and no platform request yet: it is left out and
 * reported. WM_MOVE / WM_SIZE and WM_ACTIVATEAPP are core/frame/display.c's (the client rectangle,
 * the minimised and foreground states from the platform: sim_platform.window_state). The Bink
 * video's message path is not ported (state_unported). */
#include "frame/display.h"
#include "frame/frame.h"

#include "sim/call.h"
#include "state/ported.h"

#define WINDOW 0x00E38DDAu
#define REDRAW 0x00E38DCAu
#define PARAM 0x00D3B198u      /* the button / command parameter */
#define CTRL 0x00E92E20u       /* i32 Ctrl held (WM_KEYDOWN / WM_KEYUP) */
#define SHIFT 0x00E92E24u
#define DEBUG_OFF 0x00578C68u  /* u8 1 in the exe: the debug keys are off */
#define FORMATION 0x00BCB9C0u  /* i32 the current formation; records of 0x90 bytes from here */
#define PROGRESS 0x006FDD08u   /* the campaign progress object (thiscall) */
#define SOUND 0x00E92BF8u
#define PLAYER_NAME 0x00E38E74u
#define BUTTONS 0x00E38E07u    /* u8 bit 0 left, bit 1 right */

static uint32_t formation_rec(int32_t f)
{
    return FORMATION + (uint32_t) f * 0x90u;
}

static int is_char(uint32_t c, char upper)
{
    return c == (uint32_t) upper || c == (uint32_t) upper + 0x20;
}

/* ---- the helpers right after the WndProc ---- */

static void speed_change(game_state *st)
{
    /* @bin 0x00403320 speed 0x00E38E6C -10 (param 1) / +10 (param 0), kept in 10..100 */
    if (S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    int32_t p = S32(st, PARAM);
    if (p == 1) {
        S32(st, 0x00E38E6Cu) -= 10;
    } else if (p == 0) {
        S32(st, 0x00E38E6Cu) += 10;
    }
    if (S32(st, 0x00E38E6Cu) < 10) {
        S32(st, 0x00E38E6Cu) = 10;
        U8(st, REDRAW) = 1;
        return;
    }
    if (S32(st, 0x00E38E6Cu) > 100) {
        S32(st, 0x00E38E6Cu) = 100;
    }
    U8(st, REDRAW) = 1;
}

static void speed_key(game_state *st, int32_t slower)
{
    /* @bin 0x00416BA0 '[' / ']' in the city view */
    if (U8(st, WINDOW) == 1) {
        S32(st, PARAM) = slower;
        speed_change(st);
    }
}

/* the order windows (formation 0x1E, 0x49, ship 0x1F, 0x20) go back to the city view; 1 when the
 * window is (now) the city view */
static int back_to_city(game_state *st)
{
    uint8_t w = U8(st, WINDOW);
    if (w == 0x1E || w == 0x49 || w == 0x1F || w == 0x20) {
        U8(st, WINDOW) = 1;
        return 1;
    }
    return w == 1;
}

static void overlay_key(game_state *st, int32_t overlay)
{
    /* @bin 0x004168E0 an overlay key: from the city view (or an order window, which closes; windows
     * 0x12 / 0x23 when 0x00D3A6C4 = 1) the overlay 0x00E94E0C is set, or cleared when it is the current
     * one (the previous one kept in 0x00E94E10) */
    if (!back_to_city(st)) {
        uint8_t w = U8(st, WINDOW);
        if ((w != 0x12 && w != 0x23) || S32(st, 0x00D3A6C4u) != 1) {
            return;
        }
    }
    if (S32(st, 0x00E94E0Cu) == overlay) {
        S32(st, 0x00E94E10u) = overlay;
        S32(st, 0x00E94E0Cu) = 0;
    } else {
        S32(st, 0x00E94E10u) = 0;
        S32(st, 0x00E94E0Cu) = overlay;
    }
    U8(st, REDRAW) = 1;
    GAME_CALL0(st, 0x00547A60u);
}

static void advisor_key(game_state *st, int32_t advisor)
{
    /* @bin 0x00416990 an advisor key: from the city view (an order window closes) the advisor opens
     * (0x0040A0E0 with the parameter); in the advisors' window 7 the open one closes it (city flag
     * +0x9CC := 0) or another one opens */
    if (!back_to_city(st)) {
        if (U8(st, WINDOW) != 7) {
            return;
        }
        if (S32(st, 0x00605A44u) == advisor) {
            U8(st, WINDOW) = 1;
            U8(st, REDRAW) = 1;
            U8(st, 0x00EA623Cu + U32(st, 0x00EAAC28u) * 0x49D8u) = 0;
            return;
        }
    }
    S32(st, PARAM) = advisor;
    GAME_CALL0(st, 0x0040A0E0u);
}

static void legion_next(game_state *st)
{
    /* @bin 0x00416A20 'L' in the city view: the next legion (formations 1..6 after 0x00578C6C,
     * wrapping) that is the player's (+8 = 1), in use (+0x4A) and not in the fort (+0x71 = 0) gets the
     * camera (0x00430CD0 on its tile) */
    if (U8(st, WINDOW) != 1) {
        return;
    }
    int32_t k = S32(st, 0x00578C6Cu);
    int32_t found = 0;
    S32(st, 0x00578C6Cu) = 0;
    for (int i = 0; i < 6; i++) {
        k++;
        if (k > 6) {
            k = 1;
        }
        uint32_t r = formation_rec(k);
        if (U8(st, r + 8) == 1 && U8(st, r + 0x71) == 0 && U8(st, r + 0x4A) != 0 && found == 0) {
            found = k;
        }
    }
    S32(st, 0x00578C6Cu) = found;
    if (found <= 0) {
        return;
    }
    uint32_t r = formation_rec(found);
    int32_t y = (int16_t) S16(st, r + 0x38);
    int32_t x = (int16_t) S16(st, r + 0x36);
    GAME_CALL(st, 0x00430CD0u, S32(st, 0x00BCA2D8u) + x + y * 228);
    U8(st, REDRAW) = 1;
}

static void bookmark_go(game_state *st, int32_t n)
{
    /* @bin 0x00416AC0 F1..F4: the camera to bookmark n (0x00E94DDC, 8 bytes each) when set */
    uint8_t w = U8(st, WINDOW);
    if (w == 0x1E || w == 0x49) {
        U8(st, WINDOW) = 1;
    } else if (w != 1) {
        return;
    }
    if (n < 0 || n > 4) {
        return;
    }
    int32_t t = S32(st, 0x00E94DDCu + 8u * (uint32_t) n);
    if (t <= -1) {
        return;
    }
    GAME_CALL(st, 0x00430CD0u, t);
    U8(st, REDRAW) = 1;
}

static void bookmark_set(game_state *st, int32_t n)
{
    /* @bin 0x00416B10 Ctrl+F1..F4: bookmark n := the view-to-tile entry at the view's centre */
    if (!back_to_city(st)) {
        return;
    }
    if (n < 0 || n > 4) {
        return;
    }
    int32_t row = S32(st, 0x0070E460u) / 2 + S32(st, 0x00BCA2C0u);
    int32_t col = S32(st, 0x0070E464u) / 2 + S32(st, 0x00BCA2BCu);
    S32(st, 0x00E94DDCu + 8u * (uint32_t) n) = S32(st, 0x0070E494u + 4u * (uint32_t) (row * 0xE7 + col));
}

/* @bin 0x0041D110 the city view's Alt hot keys, before the WndProc's switch: Alt+1 / Alt+2 save the
 * screen (shot1.bmp / shot2.bmp, 0x004CEED0), Ctrl+Alt+C the cheat input (0x0041D3D0), Ctrl+Alt+F
 * the campaign progress written. Alt is read with GetAsyncKeyState(VK_MENU). Returns 1 when handled */
static int32_t hot_keys(game_state *st, uint32_t msg, uint32_t wparam)
{
    int16_t alt = (int16_t) ((uint16_t) frame_key_state(st, 0x12) & 0xFF00u);
    if (U8(st, WINDOW) != 1) {
        return 0;
    }
    if (msg == 0x104) {
        if (wparam == 0x31) {
            GAME_CALL(st, 0x004CEED0u, (int32_t) 0x0057C768u);
        } else if (wparam == 0x32) {
            GAME_CALL(st, 0x004CEED0u, (int32_t) 0x0057C774u);
        }
    } else if (msg != 0x100) {
        return 0;
    }
    if (is_char(wparam, 'C')) {
        if (S32(st, CTRL) != 0 && alt != 0) {
            GAME_CALL0(st, 0x0041D3D0u);
            return 1;
        }
    } else if (is_char(wparam, 'F')) {
        if (S32(st, CTRL) != 0 && alt != 0 && U8(st, PLAYER_NAME) != 0 && U8(st, WINDOW) != 0x36) {
            GAME_CALLT0(st, PROGRESS, 0x0041BAE0u);
            int32_t p = GAME_CALL0(st, 0x00516A20u);
            GAME_CALLT(st, PROGRESS, 0x0041B8A0u, p);
            return 1;
        }
    }
    return 0;
}

/* ---- WM_CHAR ---- */

/* a ship order: param := k around the call (0x00406B40 transports, 0x00406C90 warships; W: the
 * return 0x00406E80 / 0x00406DE0 with 4) */
static void ship_order(game_state *st, uint32_t fn, int32_t k)
{
    int32_t saved = S32(st, PARAM);
    S32(st, PARAM) = k;
    GAME_CALL0(st, fn);
    S32(st, PARAM) = saved;
}

static void transport_keys(game_state *st, uint32_t ch)
{
    /* @bin 0x004164A4 (window 0x1F), 0x004165E4 (window 9, kind 6): H E K R W */
    if (is_char(ch, 'H')) {
        ship_order(st, 0x00406B40u, 0);
    } else if (is_char(ch, 'E')) {
        ship_order(st, 0x00406B40u, 1);
    } else if (is_char(ch, 'K')) {
        ship_order(st, 0x00406B40u, 2);
    } else if (is_char(ch, 'R')) {
        ship_order(st, 0x00406B40u, 3);
    } else if (is_char(ch, 'W')) {
        ship_order(st, 0x00406E80u, 4);
    }
}

static void warship_keys(game_state *st, uint32_t ch)
{
    /* @bin 0x004164F0 (window 0x20), 0x00416594 (window 9, kind 5): H N A R W */
    if (is_char(ch, 'H')) {
        ship_order(st, 0x00406C90u, 0);
    } else if (is_char(ch, 'N')) {
        ship_order(st, 0x00406C90u, 1);
    } else if (is_char(ch, 'A')) {
        ship_order(st, 0x00406C90u, 2);
    } else if (is_char(ch, 'R')) {
        ship_order(st, 0x00406C90u, 3);
    } else if (is_char(ch, 'W')) {
        ship_order(st, 0x00406DE0u, 4);
    }
}

static void formation_keys(game_state *st, uint32_t ch, int32_t f)
{
    /* @bin 0x00416630 window 0x1E: the formation's layout +0x32 written directly (T: 1, or 2 for a
     * legion with +0x7F; L: 3 / 4 unless the formation's kind +0x0C is 0xD; N: 0xF; M: 6, the previous
     * one kept in +0x6A), R (0x00407110 with 0x00D3B194 = 1), F (0x00406FB0 on 0x00EADCCC), C (window
     * 0x49 for a kind 0xD with +0x86 = 0) */
    uint32_t r = formation_rec(f);
    if (is_char(ch, 'T')) {
        S16(st, r + 0x32) = U8(st, r + 0x7F) != 0 ? 2 : 1;
    } else if (is_char(ch, 'L')) {
        if (S16(st, r + 0x0C) != 0x0D) {
            S16(st, r + 0x32) = U8(st, r + 0x7F) != 0 ? 4 : 3;
        }
    } else if (is_char(ch, 'R')) {
        S32(st, 0x00D3B194u) = 1;
        GAME_CALL0(st, 0x00407110u);
    } else if (is_char(ch, 'N')) {
        S16(st, r + 0x32) = 0x0F;
    } else if (is_char(ch, 'M')) {
        S16(st, r + 0x6A) = S16(st, r + 0x32);
        S16(st, r + 0x32) = 6;
    } else if (is_char(ch, 'F')) {
        S32(st, 0x00EADCCCu) = f;
        GAME_CALL0(st, 0x00406FB0u);
    } else if (is_char(ch, 'C')) {
        if (S16(st, r + 0x0C) == 0x0D && S16(st, r + 0x86) == 0) {
            U8(st, WINDOW) = 0x49;
        }
    }
}

static void legion_order(game_state *st, int32_t k)
{
    S32(st, PARAM) = k;
    GAME_CALL0(st, 0x00406720u);
}

static void legion_keys(game_state *st, uint32_t ch)
{
    /* @bin 0x00416540 window 9 on a legion (0x00EADD50 = 4): the orders 0x00406720 by parameter */
    int32_t f = S32(st, FORMATION);
    uint32_t r = formation_rec(f);
    if (is_char(ch, 'T')) {
        legion_order(st, 0);
    } else if (is_char(ch, 'L')) {
        if (S16(st, r + 0x0C) != 0x0D) {
            legion_order(st, 1);
        }
    } else if (is_char(ch, 'N')) {
        legion_order(st, 2);
    } else if (is_char(ch, 'M')) {
        legion_order(st, 3);
        if (S16(st, r + 0x32) == 6) {
            U8(st, WINDOW) = 1;
        }
    } else if (is_char(ch, 'C')) {
        if (S16(st, r + 0x0C) == 0x0D && S16(st, r + 0x86) == 0) {
            legion_order(st, 1);
        }
    } else if (is_char(ch, 'R')) {
        GAME_CALL0(st, 0x00407110u);
    } else if (is_char(ch, 'F')) {
        GAME_CALL0(st, 0x00406FB0u);
    }
}

static void city_keys(game_state *st, uint32_t ch, uint8_t w)
{
    /* @bin 0x004151C8 */
    if (ch == 0x20) {
        /* Space: the overlay and the previous one are swapped (order windows close) */
        int swap = 0;
        if (w == 0x49 || w == 0x1F || w == 0x20) {
            U8(st, WINDOW) = 1;
            swap = 1;
        } else if (w == 1) {
            swap = 1;
        }
        if (swap) {
            int32_t prev = S32(st, 0x00E94E10u);
            S32(st, 0x00E94E10u) = S32(st, 0x00E94E0Cu);
            S32(st, 0x00E94E0Cu) = prev;
            U8(st, REDRAW) = 1;
            GAME_CALL0(st, 0x00547A60u);
        }
    }
    if (S32(st, 0x00605A3Cu) != 0) {
        if (is_char(ch, 'H')) { /* @bin 0x00415787 */
            U8(st, 0x006DE515u) = U8(st, 0x006DE515u) == 0;
        }
        return;
    }
    /* @bin 0x004155B9 the overlay keys */
    static const struct {
        char key;
        int32_t overlay;
    } overlays[] = {{'F', 8}, {'D', 9}, {'C', 0x0A}, {'T', 0x1F}, {'W', 0x2D}, {'H', 0x33}, {'X', 0x19},
                    {'Y', 0x30}, {'Z', 0x2B}};
    for (unsigned i = 0; i < sizeof(overlays) / sizeof(overlays[0]); i++) {
        if (is_char(ch, overlays[i].key)) {
            overlay_key(st, overlays[i].overlay);
        }
    }
    /* @bin 0x0041566D the advisor keys: ` 1..9 0 - = */
    static const struct {
        uint32_t key;
        int32_t advisor;
    } advisors[] = {{0x60, 1}, {0x31, 2}, {0x32, 4}, {0x33, 5}, {0x34, 6}, {0x35, 7}, {0x36, 8}, {0x37, 9},
                    {0x38, 0x0A}, {0x39, 0x0B}, {0x30, 0x0C}, {0x2D, 0x0D}, {0x3D, 0x0E}};
    for (unsigned i = 0; i < sizeof(advisors) / sizeof(advisors[0]); i++) {
        if (ch == advisors[i].key) {
            advisor_key(st, advisors[i].advisor);
            return;
        }
    }
    if (is_char(ch, 'L')) {
        legion_next(st);
    }
}

static void wm_char(game_state *st, uint32_t ch)
{
    /* @bin 0x004150C2 */
    GAME_CALL(st, 0x004CC250u, (int32_t) ch); /* the active text field */
    uint8_t w;
    if (ch == 0x5B) {
        speed_key(st, 1);
        w = U8(st, WINDOW);
    } else if (ch == 0x5D) {
        speed_key(st, 0);
        w = U8(st, WINDOW);
    } else {
        if (is_char(ch, 'P')) {
            /* @bin 0x0041510D the pause, from the city view or an order window (seen as the city view
             * during the toggle, then back) */
            uint8_t old = U8(st, WINDOW);
            if (back_to_city(st) && S32(st, 0x00605A3Cu) == 0) {
                U8(st, 0x00E38E60u) = U8(st, 0x00E38E60u) == 0;
                GAME_CALL0(st, 0x004E2AB0u);
            }
            U8(st, WINDOW) = old;
        }
        w = U8(st, WINDOW);
        if (is_char(ch, 'M') && S32(st, 0x00605A3Cu) == 0) {
            /* @bin 0x0041518E M freezes / unfreezes the hovered tile */
            S32(st, 0x006FDC50u) = 0;
            U8(st, 0x00605A5Cu) = U8(st, 0x00605A5Cu) == 0;
        }
    }
    /* @bin 0x004151A8 the order windows */
    int32_t f = S32(st, FORMATION);
    if (w == 0x1E) {
        formation_keys(st, ch, f);
        return;
    }
    if (w == 0x49) {
        if (f != 0) {
            U8(st, WINDOW) = 0x1E;
            formation_keys(st, ch, f);
            return;
        }
    } else if (w == 0x1F) {
        if (S32(st, 0x00D05708u) != 0) {
            S32(st, 0x00EADCC8u) = S32(st, 0x00D05700u);
            transport_keys(st, ch);
            return;
        }
    } else if (w == 0x20) {
        if (S32(st, 0x00D0570Cu) != 0) {
            S32(st, 0x00EADCC4u) = S32(st, 0x00D05704u);
            warship_keys(st, ch);
            return;
        }
    } else if (w == 9) {
        int32_t kind = S32(st, 0x00EADD50u);
        if (kind == 4) {
            legion_keys(st, ch);
            return;
        }
        if (kind == 5) {
            S32(st, 0x00EADCC4u) = S32(st, 0x00D05704u);
            warship_keys(st, ch);
            return;
        }
        if (kind == 6) {
            S32(st, 0x00EADCC4u) = S32(st, 0x00D05700u);
            transport_keys(st, ch);
            return;
        }
    } else if (w == 0x0E) {
        /* @bin 0x0041553B the digits in window 0x0E */
        if (ch >= 0x30 && ch <= 0x39) {
            S32(st, PARAM) = (int32_t) ch - 0x30;
            GAME_CALL0(st, 0x004035A0u);
        }
        return;
    }
    city_keys(st, ch, w);
}

/* ---- WM_KEYDOWN ---- */

static int32_t step(game_state *st)
{
    return S32(st, CTRL) != 0 ? 8 : 1;
}

static int is_storage(game_state *st)
{
    /* the info window's building (0x00EADD6C) is a storage yard or a granary (types 0x48, 0xC1, 0x49) */
    int16_t t = S16(st, 0x007999E8u + U32(st, 0x00EADD6Cu) * 0x108u);
    return t == 0x48 || t == 0xC1 || t == 0x49;
}

static void arrow_down_up(game_state *st, int32_t down)
{
    /* @bin 0x00415AB5 (down: param 1, scroll 4, flag 0x00E92E34) / 0x00415BBD (up: 0, 0, 0x00E92E38) */
    int32_t p = down ? 1 : 0;
    uint32_t flag = down ? 0x00E92E34u : 0x00E92E38u;
    uint8_t w = U8(st, WINDOW);
    if (w == 7) {
        if (S32(st, 0x00605A44u) == 6) {
            S32(st, PARAM) = p;
            GAME_CALL0(st, 0x0040C760u);
        }
    } else if (w == 9) {
        if (is_storage(st)) {
            S32(st, PARAM) = p;
            GAME_CALL0(st, S32(st, 0x00EADCC0u) != 0 ? 0x0040C6B0u : 0x0040C630u);
        }
    } else if (w == 0x0D || w == 0x10) {
        S32(st, PARAM) = p;
        GAME_CALL0(st, 0x00402980u);
    } else if (w == 0x3D) {
        S32(st, PARAM) = p;
        GAME_CALL0(st, 0x0041F460u);
    } else if (w == 0x2F) {
        S32(st, PARAM) = p;
        GAME_CALL0(st, 0x00516890u);
    } else if (down) {
        GAME_CALL0(st, 0x004CC390u);
        GAME_CALL(st, 0x00407C10u, 4, step(st));
    } else {
        GAME_CALL0(st, 0x004CC370u);
        GAME_CALL(st, 0x00407C10u, 0, step(st));
    }
    S32(st, flag) = 1;
}

static void repeat(game_state *st, uint32_t fn, int n)
{
    for (int i = 0; i < n; i++) {
        GAME_CALL0(st, fn);
    }
}

static void page_key(game_state *st, int32_t p)
{
    /* @bin 0x00415CD9 Page Up (p 0) / 0x00415DFC Page Down (p 1) */
    switch (U8(st, WINDOW)) {
    case 1:
        S32(st, PARAM) = p ? 1 : 2; /* the city view turns */
        GAME_CALL0(st, 0x00409EB0u);
        break;
    case 0x0D:
    case 0x10:
        S32(st, PARAM) = p;
        repeat(st, 0x00402980u, 12);
        U8(st, REDRAW) = 1;
        break;
    case 0x3D:
        S32(st, PARAM) = p;
        repeat(st, 0x0041F460u, 15);
        U8(st, REDRAW) = 1;
        break;
    case 0x2F:
        S32(st, PARAM) = p;
        repeat(st, 0x00516890u, 12);
        U8(st, REDRAW) = 1;
        break;
    case 7:
        if (S32(st, 0x00605A44u) == 6) {
            S32(st, PARAM) = p;
            repeat(st, 0x0040C760u, 12);
            U8(st, REDRAW) = 1;
        }
        break;
    case 9:
        if (is_storage(st)) {
            S32(st, PARAM) = p;
            /* @bin 0x00415DD0 one row per visible row (0x00502780, re-read each time) */
            int32_t i = 0;
            if (GAME_CALL0(st, 0x00502780u) > 0) {
                do {
                    GAME_CALL0(st, 0x0040C6B0u);
                    i++;
                } while (i < GAME_CALL0(st, 0x00502780u));
            }
            U8(st, REDRAW) = 1;
        }
        break;
    default:
        break;
    }
}

static void resolution_key(game_state *st, int32_t mode)
{
    /* @bin 0x004161DC F7 / F8 / F9: DirectDraw closed (0x004D01E0), the mode set (0x004CFDC0), the
     * layout redone (0x00414BD0, 0x004E2AB0), DirectDraw set up again (0x004D0220), 0x00402DA0 */
    S32(st, 0x005E06B0u) = 1;
    GAME_CALL(st, 0x004D01E0u, 1);
    GAME_CALL(st, 0x004CFDC0u, mode);
    GAME_CALL0(st, 0x00414BD0u);
    GAME_CALL0(st, 0x004E2AB0u);
    GAME_CALL0(st, 0x004D0220u);
    S32(st, 0x005E06B0u) = 0;
    GAME_CALL0(st, 0x00402DA0u);
    U8(st, REDRAW) = 1;
}

/* returns 1 when the message is answered without DefWindowProcA (Esc) */
static int wm_keydown(game_state *st, uint32_t vk)
{
    /* @bin 0x004158BA */
    U8(st, 0x00E38E4Eu) = 1;
    uint8_t w = U8(st, WINDOW);
    int debug = U8(st, DEBUG_OFF) == 0;
    switch (vk) {
    case 0x11:
        S32(st, CTRL) = 1;
        break;
    case 0x10:
        S32(st, SHIFT) = 1;
        break;
    case 0x7B: /* F12 */
        U8(st, 0x00775680u) = U8(st, 0x00775680u) == 0;
        break;
    case 0x47: case 0x67: /* debug: Ctrl+G, Ctrl+H, Ctrl+J cycle three values, Ctrl+K uses them */
        if (debug && S32(st, CTRL) != 0 && w == 1 && ++S32(st, 0x00605A08u) > 4) {
            S32(st, 0x00605A08u) = 0;
        }
        break;
    case 0x48: case 0x68:
        if (debug && S32(st, CTRL) != 0 && w == 1 && ++S32(st, 0x00605A04u) > 3) {
            S32(st, 0x00605A04u) = 1;
        }
        break;
    case 0x4A: case 0x6A:
        if (debug && S32(st, CTRL) != 0 && w == 1 && ++S32(st, 0x00605A00u) > 4) {
            S32(st, 0x00605A00u) = 0;
        }
        break;
    case 0x4B: case 0x6B:
        if (debug && S32(st, CTRL) != 0 && w == 1 && S32(st, 0x00605A00u) != 0) {
            GAME_CALL(st, 0x004C47A0u, S32(st, 0x00605A08u), S32(st, 0x00605A04u), S32(st, 0x00605A00u));
        }
        break;
    case 0x24: /* Home */
        GAME_CALL0(st, 0x004CC3C0u);
        if (U8(st, WINDOW) == 1) {
            S32(st, PARAM) = 0;
            GAME_CALL0(st, 0x00409EB0u);
        }
        break;
    case 0x23: /* End */
        GAME_CALL0(st, 0x004CC3E0u);
        break;
    case 0x27: /* Right */
        GAME_CALL0(st, 0x004CC390u);
        GAME_CALL(st, 0x00407C10u, 2, step(st));
        S32(st, 0x00E92E2Cu) = 1;
        break;
    case 0x25: /* Left */
        GAME_CALL0(st, 0x004CC370u);
        GAME_CALL(st, 0x00407C10u, 6, step(st));
        S32(st, 0x00E92E30u) = 1;
        break;
    case 0x28: /* Down */
        arrow_down_up(st, 1);
        break;
    case 0x26: /* Up */
        arrow_down_up(st, 0);
        break;
    case 0x21: /* Page Up */
        page_key(st, 0);
        break;
    case 0x22: /* Page Down */
        page_key(st, 1);
        break;
    case 0x2D: /* Insert */
        GAME_CALL0(st, 0x004CC360u);
        break;
    case 0x2E: /* Delete */
        GAME_CALL0(st, U8(st, WINDOW) == 0x0E ? 0x00403770u : 0x004CC350u);
        break;
    case 0x08: /* Backspace */
        GAME_CALL0(st, 0x004CC330u);
        break;
    case 0x0D: /* Enter */
        GAME_CALL0(st, U8(st, WINDOW) == 0x0E ? 0x004035F0u : 0x004CC350u);
        GAME_CALL0(st, 0x004CC320u);
        break;
    case 0x41: case 0x61: /* debug: Ctrl+A free building (0x00605A0C), the menus updated, the progress */
        if (!debug) {
            break;
        }
        if (S32(st, CTRL) != 0 && U8(st, WINDOW) == 1) {
            S32(st, 0x00605A0Cu) ^= 1;
        }
        GAME_CALL0(st, 0x00404350u);
        GAME_CALL0(st, 0x004051A0u);
        if (S32(st, CTRL) != 0 && U8(st, PLAYER_NAME) != 0 && U8(st, WINDOW) != 0x36) {
            GAME_CALLT0(st, PROGRESS, 0x0041BAE0u);
            int32_t p = GAME_CALL0(st, 0x00516A20u);
            GAME_CALLT(st, PROGRESS, 0x0041B8A0u, p);
        }
        break;
    case 0x62: /* debug */
        if (debug && --S32(st, 0x00E38E94u) < 0) {
            S32(st, 0x00E38E94u) = 0x1B;
        }
        break;
    case 0x63: /* only with the debug byte set */
        if (U8(st, WINDOW) == 0x0E && !debug) {
            S32(st, PARAM) = 3;
            GAME_CALL0(st, 0x004035A0u);
        }
        break;
    case 0x70: case 0x71: case 0x72: case 0x73: /* F1..F4: bookmarks, set with Ctrl */
        if (S32(st, CTRL) != 0) {
            bookmark_set(st, (int32_t) vk - 0x70);
        } else {
            bookmark_go(st, (int32_t) vk - 0x70);
        }
        break;
    case 0x74: /* F5: a windowed game's window centred (Windows only) */
        break;
    case 0x75: { /* F6: fullscreen toggled */
        int32_t full = S32(st, 0x00E38E5Cu);
        S32(st, 0x005E06B0u) = 1;
        S32(st, 0x00E38E5Cu) = full == 0;
        GAME_CALL(st, 0x004D01E0u, 1);
        GAME_CALL0(st, 0x004D0220u);
        S32(st, 0x005E06B0u) = 0;
        /* ShowWindow, UpdateWindow, SetWindowPos: the platform's */
        U8(st, REDRAW) = 1;
        break;
    }
    case 0x76: /* F7..F9: the resolution */
        if (w != 0x0F && w != 0x48 && w != 0x15 && w != 0x0E) {
            resolution_key(st, 1);
        }
        break;
    case 0x77:
        if (w != 0x0F && w != 0x48 && w != 0x0E) {
            resolution_key(st, 2);
        }
        break;
    case 0x78:
        if (w != 0x0F && w != 0x48 && w != 0x0E) {
            resolution_key(st, 3);
        }
        break;
    case 0x1B: { /* Esc: the quit box */
        GAME_CALL0(st, 0x00539170u);
        GAME_CALL(st, 0x004CC110u, 5, 0);
        GAME_CALL(st, 0x004CC160u, 5, 1);
        int32_t r = GAME_CALL(st, 0x00538CD0u, S32(st, 0x00E20764u), S32(st, 0x00E20768u), 4);
        U8(st, 0x00E38E07u) = 0;
        if (r == 6) {
            U8(st, 0x00D3B43Du) = 1;
            U8(st, REDRAW) = 1;
        }
        return 1;
    }
    default:
        break;
    }
    return 0;
}

/* ---- the window procedure ---- */

int32_t frame_wndproc(game_state *st, uint32_t hwnd, uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    /* @bin 0x00414CD0 */
    if (S32(st, 0x006FDC40u) != 0) {
        /* @bin 0x00414CFC a Bink video plays: messages go to the player (0x00560330, 0x005602F0)
         * with GetMessageTime / GetMessagePos (package 7.8) */
        state_unported(st, 0x00414CD0u);
        return 0;
    }
    uint32_t bink = U32(st, 0x006FDC54u);
    if (bink != 0 && P8(st, bink + 4) == 0) {
        return 0; /* DefWindowProcA only */
    }
    if (U8(st, WINDOW) == 0x54 && msg != 0x20 && msg != 0x0F && msg != 0x200 && msg != 0x205 && msg != 0x204 &&
        msg != 0x206 && msg != 0x202 && msg != 0x201 && msg != 0x203) {
        return 0;
    }
    hot_keys(st, msg, wparam);
    switch (msg) {
    case 0x01: /* WM_CREATE */
        U32(st, 0x006FDC44u) = hwnd;
        break;
    case 0x02: /* WM_DESTROY: the sound and DirectDraw closed; PostQuitMessage is the platform's */
        GAME_CALL0(st, 0x005391E0u);
        U32(st, 0x00E39140u) = 0;
        GAME_CALL(st, 0x004D01E0u, 1);
        break;
    case 0x03: /* WM_MOVE, WM_SIZE: the client rectangle (core/frame/display.c) */
    case 0x05:
        display_window_moved(st);
        break;
    case 0x1C: /* WM_ACTIVATEAPP: active or not, surfaces restored (core/frame/display.c) */
        display_window_activated(st, wparam);
        break;
    case 0x06: /* WM_ACTIVATE */
        if ((wparam >> 16) != 0) { /* minimised: the three streams stopped */
            GAME_CALLT(st, SOUND, 0x0055F561u, 0);
            GAME_CALLT(st, SOUND, 0x0055F561u, 2);
            GAME_CALLT(st, SOUND, 0x0055F561u, 1);
            S32(st, 0x006FDC48u) = 1;
        } else if (S32(st, 0x006FDC48u) != 0 && S32(st, 0x00E38E5Cu) != 0) {
            /* ShowWindow, UpdateWindow, SetWindowPos: the platform's */
            S32(st, 0x006FDC48u) = 0;
        }
        break;
    case 0x0F: /* WM_PAINT (BeginPaint / EndPaint: the platform's) */
        frame_present(st, 1);
        U8(st, 0x00E38DC8u) = 1;
        U8(st, REDRAW) = 1;
        break;
    case 0x10: /* WM_CLOSE */
        U8(st, 0x00D3B43Du) = 3;
        break;
    case 0x7E: /* WM_DISPLAYCHANGE */
        S32(st, 0x00E38DBCu) = (int32_t) wparam;
        S32(st, 0x00E38DB8u) = (int32_t) (lparam & 0xFFFFu);
        S32(st, 0x00E38DB4u) = (int32_t) (lparam >> 16);
        break;
    case 0x100: /* WM_KEYDOWN */
        if (wm_keydown(st, wparam)) {
            return 0;
        }
        break;
    case 0x101: /* WM_KEYUP */
        if (wparam == 0x10) {
            S32(st, SHIFT) = 0;
        } else if (wparam == 0x11) {
            S32(st, CTRL) = 0;
        }
        break;
    case 0x102: /* WM_CHAR */
        wm_char(st, wparam);
        break;
    case 0x104: /* WM_SYSKEYDOWN: Alt+X posts Esc (the platform's), Alt+Z, Alt+D */
        if (is_char(wparam, 'Z')) {
            GAME_CALL0(st, 0x00406090u);
        } else if (is_char(wparam, 'D')) {
            S32(st, 0x006FDC50u) = S32(st, 0x006FDC50u) == 0;
        }
        break;
    case 0x200: /* WM_MOUSEMOVE */
    case 0x201: /* WM_LBUTTONDOWN */
    case 0x202:
    case 0x204:
    case 0x205:
        if (msg == 0x201) {
            U8(st, 0x00E38E02u) = 1; /* the click latch */
            U8(st, BUTTONS) |= 1;
        } else if (msg == 0x202) {
            U8(st, BUTTONS) &= 0xFE;
        } else if (msg == 0x204) {
            U8(st, BUTTONS) |= 2;
        } else if (msg == 0x205) {
            U8(st, BUTTONS) &= 0xFD;
        }
        S16(st, 0x00E38E0Au) = (int16_t) (lparam & 0xFFFFu);
        S16(st, 0x00E38E08u) = (int16_t) (lparam >> 16);
        break;
    default:
        break;
    }
    return 0; /* DefWindowProcA */
}

/* ---- the port table ---- */

static int32_t run_wndproc(game_state *st, const int32_t *a)
{
    return frame_wndproc(st, (uint32_t) a[0], (uint32_t) a[1], (uint32_t) a[2], (uint32_t) a[3]);
}

static int32_t run_hot_keys(game_state *st, const int32_t *a)
{
    return hot_keys(st, (uint32_t) a[0], (uint32_t) a[1]);
}

static int32_t run_overlay_key(game_state *st, const int32_t *a)
{
    overlay_key(st, a[0]);
    return 0;
}

static int32_t run_advisor_key(game_state *st, const int32_t *a)
{
    advisor_key(st, a[0]);
    return 0;
}

static int32_t run_legion_next(game_state *st, const int32_t *a)
{
    (void) a;
    legion_next(st);
    return 0;
}

static int32_t run_bookmark_go(game_state *st, const int32_t *a)
{
    bookmark_go(st, a[0]);
    return 0;
}

static int32_t run_bookmark_set(game_state *st, const int32_t *a)
{
    bookmark_set(st, a[0]);
    return 0;
}

static int32_t run_speed_key(game_state *st, const int32_t *a)
{
    speed_key(st, a[0]);
    return 0;
}

static int32_t run_speed_change(game_state *st, const int32_t *a)
{
    (void) a;
    speed_change(st);
    return 0;
}

const ported_function frame_wndproc_ports[] = {
    {0x00414CD0u, "frame_wndproc", 4, 32, 0, run_wndproc, 0},
    {0x0041D110u, "wndproc_hot_keys", 2, 8, 0, run_hot_keys, 0},
    {0x004168E0u, "wndproc_overlay_key", 1, 0, 0, run_overlay_key, 0},
    {0x00416990u, "wndproc_advisor_key", 1, 0, 0, run_advisor_key, 0},
    {0x00416A20u, "wndproc_legion_next", 0, 0, 0, run_legion_next, 0},
    {0x00416AC0u, "wndproc_bookmark_go", 1, 0, 0, run_bookmark_go, 0},
    {0x00416B10u, "wndproc_bookmark_set", 1, 0, 0, run_bookmark_set, 0},
    {0x00416BA0u, "wndproc_speed_key", 1, 0, 0, run_speed_key, 0},
    {0x00403320u, "speed_change", 0, 0, 0, run_speed_change, 0},
};
const int frame_wndproc_ports_count = sizeof(frame_wndproc_ports) / sizeof(frame_wndproc_ports[0]);
