/* Imhotep - the game tick (Pharaoh.exe 0x004D7090), the day dispatcher (0x00451C50) and the
 * day / month / year cascade (0x00452370).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_SIM_TICK_H
#define IMHOTEP_SIM_TICK_H

#include "sim/call.h"
#include "state/state.h"

/* @bin 0x004D7090 one game tick (51 per day, 16 days per month, 12 months per year) */
void game_tick(game_state *st, const sim_env *env);
/* @bin 0x00451C50 the step of the day for the current tick, then the timers and the counters */
void day_dispatch(game_state *st, const sim_env *env);
/* @bin 0x00452370 a new day: month and year ends, the autosave */
void day_cascade(game_state *st, const sim_env *env);

#endif
