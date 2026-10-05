/* Imhotep - what the simulation asks of the platform: videos and sounds.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original's tick calls its video player and sound functions directly. They write only
 * presentation state (the video player's globals, the sound request block, path and log buffers:
 * tools/oracle/excluded.py), so core turns them into requests to an optional platform layer
 * (st->platform) and writes nothing. Without a platform (tests, the oracle comparisons) they
 * behave as the original does with no video or sound device: the video fails to open. */
#ifndef IMHOTEP_SIM_PLATFORM_H
#define IMHOTEP_SIM_PLATFORM_H

#include <stdint.h>

#include "state/state.h"

typedef struct sim_platform {
    void *ctx;
    /* play a video (file name, the original's three other arguments); 1 if it plays */
    int (*video_play)(void *ctx, const char *name, int32_t a, int32_t b, int32_t c);
    /* a figure's sound (figure id, sound slot, channel) */
    void (*figure_sound)(void *ctx, int32_t figure, int32_t sound, int32_t channel);
    /* an interface sound */
    void (*ui_sound)(void *ctx, int32_t sound);
    /* a sound stream (speech, music) on channel 0..2: open and play the file; is one playing */
    void (*stream_play)(void *ctx, const char *name, int32_t channel, int32_t loops);
    int (*stream_playing)(void *ctx, int32_t channel);
    /* a line of the game's log */
    void (*log)(void *ctx, const char *message, const char *text, int32_t value);
} sim_platform;

/* @bin 0x00538E30 the video player (Smacker): returns 1 when the video plays */
int32_t platform_video_play(game_state *st, uint32_t name, int32_t a, int32_t b, int32_t c);
/* @bin 0x0053B4A0 a figure's sound, when sound is on (0x00F55B88, 0x00E38E63) */
void platform_figure_sound(game_state *st, int32_t figure, int32_t sound, int32_t channel);
/* @bin 0x0053B090 an interface sound, when sound is on */
void platform_ui_sound(game_state *st, int32_t sound);

/* @bin 0x0055F449 open a stream (thiscall on the sound object 0x00E92BF8); returns 1 */
int32_t platform_stream_play(game_state *st, uint32_t name, int32_t channel, int32_t loops);
/* @bin 0x0055F5A1 is a stream playing on the channel (thiscall on 0x00E92BF8) */
int32_t platform_stream_playing(game_state *st, int32_t channel);
/* @bin 0x004CE6E0 the game's log (message, optional text, value) */
void platform_log(game_state *st, uint32_t message, uint32_t text, int32_t value);

#endif
