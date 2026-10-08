/* Imhotep - the frame's sound logic (core/frame/sound.c).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FRAME_SOUND_H
#define IMHOTEP_FRAME_SOUND_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0041D410 the ground pass marks a visible building's ambient slot (counter 0..4) */
void ambient_building(game_state *st, int32_t building, int32_t counter);
/* @bin 0x0041D560 the ambient slots, every frame (frame clock 0x00E92E0C) */
void ambient_step(game_state *st);
/* @bin 0x0053B670 the ground pass clears the ambient counters 0x00F55B50 */
void ambient_counters_clear(game_state *st);
/* @bin 0x0053B830 a building type's ambient class (0 .. 4) */
int32_t ambient_class(int32_t type);
/* @bin 0x0053B6F0 a stream's fade out (one step a frame) */
void stream_fade_out(game_state *st, int32_t channel);
/* @bin 0x0053B790 a stream's fade in (one step) */
void stream_fade_in(game_state *st, int32_t channel);
/* @bin 0x0053B960 the city's ambient stream (channel 2) */
void ambient_stream(game_state *st);
/* @bin 0x0053ACF0 a random sound of a point of the city view */
void city_random_sound(game_state *st);
/* @bin 0x00539A50, 0x00539730, 0x00539820 the music (thiscall on 0x00E39180) */
void music_set_window(game_state *st, uint32_t self, int32_t first, int32_t last);
void music_window(game_state *st, uint32_t self, int32_t force);
void music_step(game_state *st, uint32_t self);

#endif
