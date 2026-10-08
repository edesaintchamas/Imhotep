/* Imhotep - the advisors, group e: page 4 (kingdom), pages 11, 14, the dialogs 0x26, 0x2C / 0x4D,
 * 0x2D, 0x2E (see advisors.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_E_H
#define IMHOTEP_PRESENT_UI_ADVISORS_E_H

#include "advisors.h"

/* e_kingdom.c */
void advisor_gift_amounts(game_state *st);                  /* @bin 0x004E7DB0 */
uint32_t advisor_king_name(game_state *st);                 /* @bin 0x00447840 */
int32_t advisor_event_months(game_state *st, uint32_t ev);  /* @bin 0x0044C100 */
void advisor_kingdom_draw(game_state *st);                  /* @bin 0x00523EA0 */

/* e_pages.c */
void advisor_religion_draw(game_state *st);                 /* @bin 0x005298E0 page 11 */
void advisor_monuments_draw(game_state *st);                /* @bin 0x0052B520 page 14 */

/* e_dialogs.c */
void advisor_salary_draw(game_state *st);                   /* @bin 0x00524E20 windows 0x2C, 0x4D */
void advisor_donation_draw(game_state *st);                 /* @bin 0x005250F0 window 0x2D */
void advisor_gift_draw(game_state *st);                     /* @bin 0x005255A0 window 0x2E */
void advisor_open_route_draw(game_state *st);               /* @bin 0x00525B90 window 0x26 */

#endif
