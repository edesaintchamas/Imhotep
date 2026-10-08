/* Imhotep - the advisors, group a: pages 1-3 (labour, military, fleet) and the labour priority
 * dialog (window 0x2A).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_A_H
#define IMHOTEP_PRESENT_UI_ADVISORS_A_H

#include <stdint.h>

#include "state/state.h"

#define ADV_SLIDE 0x00F53F48u /* i32 the window's slide-in step (pages draw a black frame while < 5) */

/* a_labour.c */
void advisor_labour_draw(game_state *st);          /* @bin 0x00521080 page 1 */
void advisor_labour_rows_draw(game_state *st);     /* @bin 0x005215D0 the 9 categories */
void advisor_labour_priority_draw(game_state *st); /* @bin 0x00521800 window 0x2A */

/* a_military.c */
int32_t legions_count(game_state *st);                 /* @bin 0x004B78F0 */
void advisor_military_draw(game_state *st);            /* @bin 0x00521B50 page 2 */
int32_t ship_at_wharf(game_state *st, int32_t id);     /* @bin 0x00522D80 */
void advisor_fleet_draw(game_state *st);               /* @bin 0x00522E10 page 3 */
int32_t ship_health_text(game_state *st, int32_t id);  /* @bin 0x0050FC10 */
int32_t ship_state_text(game_state *st, int32_t id);   /* @bin 0x0050FC70 */

#endif
