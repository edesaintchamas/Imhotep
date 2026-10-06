/* Imhotep - opening an advisor (package cmdrest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_ADVISOR_H
#define IMHOTEP_COMMAND_ADVISOR_H

#include "state/state.h"

void advisor_open(game_state *st);          /* @bin 0x0040A0E0 advisor 0x00D3B198 */
void advisors_refresh(game_state *st);      /* @bin 0x0040A160 */
void advisor_complex_level(game_state *st); /* @bin 0x0045A930 */
void advisor_summary_flags(game_state *st); /* @bin 0x004E9500 */
void advisor_houses_count(game_state *st);  /* @bin 0x004F6AD0 */
void advisor_ratings(game_state *st);       /* @bin 0x0046E1D0 */

#endif
