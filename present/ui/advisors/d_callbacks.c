/* Imhotep - the advisors, group d: the trade advisor's button callbacks (page 6 rectangle table
 * 0x006032E8, the dialog's close button 0x00603604) and the empire window's opening.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Callbacks are cdecl without arguments; the clicked button's parameter is 0x00D3B198. The trade
 * dialog's state-changing buttons are the phase-5 commands of core/command/trade.c. */
#include "advisors.h"
#include "d.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define LIST_COUNT 0x00F53E6Cu
#define TRADE_RESOURCE 0x0060598Cu

void trade_cb_select(game_state *st)
{
    /* @bin 0x00559820 a list row (parameter: the row, 0..14): its resource (list entry scroll + row)
     * becomes the dialog's 0x0060598C, checked by 0x005599C0, and window 0x28 opens */
    int32_t k = S32(st, CURRENT_CITY(st) + 0x4834u) + S32(st, ADV_PARAM1);
    int32_t r = S32(st, LIST_COUNT + 4u * (uint32_t) k);
    S32(st, TRADE_RESOURCE) = r;
    GAME_CALL(st, 0x005599C0u, r);
    U8(st, ADV_WINDOW) = 0x28;
    U8(st, ADV_REDRAW) = 1;
}

void trade_cb_toggle_prices(game_state *st)
{
    /* @bin 0x00559860 prices / stock and orders: CITY+0x49CC := !CITY+0x49CC */
    U8(st, ADV_REDRAW) = 1;
    uint32_t p = CURRENT_CITY(st) + 0x49CCu;
    U8(st, p) = U8(st, p) == 0;
}

void empire_window_open(game_state *st)
{
    /* @bin 0x00409FA0 open the empire window: in the editor (0x00605A3C = 1) window 0x15 (or a
     * message box, group 5 0x7B / 0x7C, when 0x00E38E68 = 1); in a game window 0x14, or a notice
     * 0x0D when 0x00E38EBC = 0 and 0x0078D828 is set; the tutorial step 0x0E (window 1) when
     * 0x00E38EBC = 0, 0x007027C6 = 0 and the mission 0x00E38EA8 is 0x00579638 */
    int32_t editor = S32(st, 0x00605A3Cu);
    S32(st, 0x005D2E88u) = -1;
    S32(st, 0x00F53FDCu) = 0;
    if (editor == 1) {
        if (S32(st, 0x00E38E68u) == 1) {
            GAME_CALL(st, 0x004CC110u, 5, 0x7B);
            GAME_CALL(st, 0x004CC160u, 5, 0x7C);
            GAME_CALL(st, 0x00538CD0u, (int32_t) U32(st, 0x00E20768u), (int32_t) U32(st, 0x00E20764u), 0);
            U8(st, ADV_REDRAW) = 1;
            return;
        }
        U8(st, ADV_WINDOW) = 0x15;
        S32(st, 0x00775928u) = 4;
        S32(st, 0x00775920u) = 1;
        GAME_CALL0(st, 0x00442800u);
        U8(st, ADV_REDRAW) = 1;
        return;
    }
    int32_t intro = S32(st, 0x00E38EBCu);
    if (intro == 0 && S32(st, 0x0078D828u) != 0) {
        GAME_CALL(st, 0x004E2970u, 0x0D, 0, 0);
        return;
    }
    U8(st, ADV_WINDOW) = 0x14;
    if (intro == 0 && U8(st, 0x007027C6u) == 0 && (int32_t) S8(st, 0x00E38EA8u) == S32(st, 0x00579638u)) {
        U8(st, ADV_WINDOW) = 1;
        GAME_CALL(st, 0x00417AE0u, 0x0E);
        S32(st, 0x00D3B17Cu) = 0x14;
    }
    U8(st, ADV_REDRAW) = 1;
}

void trade_cb_empire(game_state *st)
{
    /* @bin 0x00559890 the empire button */
    empire_window_open(st);
    U8(st, ADV_REDRAW) = 1;
}

void trade_cb_close(game_state *st)
{
    /* @bin 0x00559E00 the dialog's close button: back to the advisors (window 7) */
    U8(st, ADV_WINDOW) = 7;
    U8(st, ADV_REDRAW) = 1;
}
