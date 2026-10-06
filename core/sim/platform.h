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
    /* a modal message box (title, text, number of buttons 1..3): the index of the button the
     * player pressed */
    int (*message_box)(void *ctx, const char *title, const char *text, int32_t buttons);
    /* a game data file, by the path the game builds (e.g. "data\\maus0_1a.sg3", relative to the
     * game's directory, case-insensitive): copies the bytes of [offset, offset + size) that exist
     * into buf (buf may be NULL with size 0); returns the file's size, or -1 when there is none */
    int32_t (*file_read)(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size);
    /* a graphics file's pixels (the .555 of a graphics slot) loaded into the platform's memory:
     * the bytes from `skip` on (`size` of them); returns a non-zero handle, which the game state
     * keeps (pointers into it: the slot's image entries); 0 when it cannot be loaded */
    uint32_t (*pixels_load)(void *ctx, const char *path, uint32_t skip, uint32_t size);
    /* the player ends the playing video */
    void (*video_close)(void *ctx);
    /* releases a handle from pixels_load */
    void (*pixels_free)(void *ctx, uint32_t handle);
} sim_platform;

/* @bin 0x00538E30 the video player (Smacker): returns 1 when the video plays */
int32_t platform_video_play(game_state *st, uint32_t name, int32_t a, int32_t b, int32_t c);
/* @bin 0x00539170 the player ends the video (a key or click in the video window): nothing when
 * none plays (0x00F54054, the video player's state: tools/oracle/excluded.py) */
void platform_video_close(game_state *st);
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

/* @bin 0x00538CD0 a modal message box (title, text, kind): the platform shows it and returns the
 * pressed button; without a platform the first button is pressed (tools/oracle/game.py does the
 * same in the original). Returns the button's result code (0x00538940) */
int32_t platform_message_box(game_state *st, uint32_t title, uint32_t text, int32_t kind);

/* @bin 0x004CEB60 the game's file reads (file_read_at: open, seek, read the rest of the file): the
 * platform supplies game data files (the .sg3 indexes read by 0x0053DFF0). Copies up to `size`
 * bytes from `offset` into buf; returns the file's size, or -1 when the file cannot be read
 * (no platform, or no such file) */
int32_t platform_file_read(game_state *st, const char *path, uint32_t offset, uint8_t *buf, uint32_t size);

/* @bin 0x0053F660 a graphics slot's pixels ("data\\" name ".555", from `skip` on) loaded by the
 * platform: the previous buffer at `out` is released, the new handle stored at `out`; returns the
 * size in bytes (0: nothing loaded). The pixel conversion (0x004CDF20 / 0x004CDEB0) is the
 * platform's. In the oracle comparisons it is replayed from the original's run */
int32_t platform_pixels_load(game_state *st, uint32_t name, uint32_t out, int32_t convert, int32_t size,
                             int32_t rle, int32_t offset, int32_t skip);

#endif
