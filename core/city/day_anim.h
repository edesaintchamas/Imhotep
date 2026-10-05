/* Imhotep - the animated sprites stepped every tick (0x0041FF60).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_ANIM_H
#define IMHOTEP_CITY_DAY_ANIM_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0041FCA0 (thiscall) restart an animation object in `mode` from its set's entry list */
void anim_restart(game_state *st, uint32_t obj, int16_t mode);
/* @bin 0x0041FDB0 (thiscall) one frame of an animation object */
void anim_step(game_state *st, uint32_t obj);
/* @bin 0x0041FF60 every tick: the animation objects of eight lists */
void anim_step_all(game_state *st);

#endif
