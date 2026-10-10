/* Imhotep - the advisors, group d: the trade advisor's button callbacks (page 6 rectangle table
 * 0x006032E8, the dialog's close button 0x00603604) and the empire window's opening.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Callbacks are cdecl without arguments; the clicked button's parameter is 0x00D3B198. The trade
 * dialog's state-changing buttons are the phase-5 commands of core/command/trade.c. */
#include "advisors.h"
#include "d.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

void trade_cb_select(game_state *st)
{
    /* @bin 0x00559820 a list row (parameter: the row, 0..14): its resource (list entry scroll + row)
     * becomes the dialog's 0x0060598C, checked by 0x005599C0, and window 0x28 opens */
    int32_t k = S32(st, CURRENT_CITY(st) + CITY_TRADE_SCROLL) + S32(st, G_PARAM1);
    int32_t r = S32(st, G_STORAGE_LIST + 4u * (uint32_t) k);
    S32(st, G_TRADE_RESOURCE) = r;
    GAME_CALL(st, FN_TRADE_ORDER_CHECK, r);
    U8(st, G_WINDOW) = 0x28;
    U8(st, G_REDRAW) = 1;
}

void trade_cb_toggle_prices(game_state *st)
{
    /* @bin 0x00559860 prices / stock and orders: CITY+0x49CC := !CITY+0x49CC */
    U8(st, G_REDRAW) = 1;
    uint32_t p = CURRENT_CITY(st) + CITY_U49CC;
    U8(st, p) = U8(st, p) == 0;
}

void empire_window_open(game_state *st)
{
    /* @bin 0x00409FA0 open the empire window: in the editor (0x00605A3C = 1) window 0x15 (or a
     * message box, group 5 0x7B / 0x7C, when 0x00E38E68 = 1); in a game window 0x14, or a notice
     * 0x0D when 0x00E38EBC = 0 and 0x0078D828 is set; the tutorial step 0x0E (window 1) when
     * 0x00E38EBC = 0, 0x007027C6 = 0 and the mission 0x00E38EA8 is 0x00579638 */
    int32_t editor = S32(st, G_EDITOR);
    S32(st, G_ARMY_SEL) = -1;
    S32(st, G_MARKER_COUNT) = 0;
    if (editor == 1) {
        if (S32(st, G_RESOLUTION) == 1) {
            GAME_CALL(st, FN_TEXT_SEEK, 5, 0x7B);
            GAME_CALL(st, FN_TEXT_SEEK2, 5, 0x7C);
            GAME_CALL(st, FN_PLATFORM_MESSAGE_BOX, (int32_t) U32(st, G_TEXT_CURSOR), (int32_t) U32(st, G_TEXT_PTR2), 0);
            U8(st, G_REDRAW) = 1;
            return;
        }
        U8(st, G_WINDOW) = 0x15;
        S32(st, G_EDITOR_MODE) = 4;
        S32(st, G_PICTURE_KIND) = 1;
        GAME_CALL0(st, FN_EMPIRE_SCROLL_CLAMP);
        U8(st, G_REDRAW) = 1;
        return;
    }
    int32_t intro = S32(st, G_CUSTOM);
    if (intro == 0 && S32(st, G_U78D828) != 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x0D, 0, 0);
        return;
    }
    U8(st, G_WINDOW) = 0x14;
    if (intro == 0 && U8(st, G_U7027C6) == 0 && (int32_t) S8(st, G_MISSION) == S32(st, G_U579638)) {
        U8(st, G_WINDOW) = 1;
        GAME_CALL(st, FN_TUTORIAL_STEP, 0x0E);
        S32(st, G_MW_PREV_WINDOW) = 0x14;
    }
    U8(st, G_REDRAW) = 1;
}

void trade_cb_empire(game_state *st)
{
    /* @bin 0x00559890 the empire button */
    empire_window_open(st);
    U8(st, G_REDRAW) = 1;
}

void trade_cb_close(game_state *st)
{
    /* @bin 0x00559E00 the dialog's close button: back to the advisors (window 7) */
    U8(st, G_WINDOW) = 7;
    U8(st, G_REDRAW) = 1;
}
