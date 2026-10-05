/* Imhotep - what the simulation asks of the platform: videos and sounds.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/platform.h"

#include <string.h>

#include "state/ported.h"

#define SOUND_ON 0x00F55B88u    /* sound system initialised */
#define SOUND_OPTION 0x00E38E63u /* sound effects option */

int32_t platform_video_play(game_state *st, uint32_t name, int32_t a, int32_t b, int32_t c)
{
    /* @bin 0x00538E30 without a video device the original fails to open the file and returns 0;
     * what it writes meanwhile is the player's own state (excluded.py) */
    const sim_platform *p = st->platform;
    const uint8_t *s = state_ptr(st, name, 1);
    if (!p || !p->video_play || !s || !memchr(s, 0, 64)) {
        return 0;
    }
    return p->video_play(p->ctx, (const char *) s, a, b, c) ? 1 : 0;
}

void platform_figure_sound(game_state *st, int32_t figure, int32_t sound, int32_t channel)
{
    /* @bin 0x0053B4A0 the gate; the rest (screen position, volume) is the platform's */
    const sim_platform *p = st->platform;
    if (S32(st, SOUND_ON) != 0 && sound < 0x21 && channel < 5 && U8(st, SOUND_OPTION) != 0 && p &&
        p->figure_sound) {
        p->figure_sound(p->ctx, figure, sound, channel);
    }
}

void platform_ui_sound(game_state *st, int32_t sound)
{
    /* @bin 0x0053B090 */
    const sim_platform *p = st->platform;
    if (S32(st, SOUND_ON) != 0 && sound < 0x13 && U8(st, SOUND_OPTION) != 0 && p && p->ui_sound) {
        p->ui_sound(p->ctx, sound);
    }
}

static const char *cstring(game_state *st, uint32_t address)
{
    const uint8_t *s = address ? state_ptr(st, address, 1) : NULL;
    return s && memchr(s, 0, 1) ? (const char *) s : NULL;
}

int32_t platform_stream_play(game_state *st, uint32_t name, int32_t channel, int32_t loops)
{
    /* @bin 0x0055F449 channels 0..2; the stream handles (sound object +0x1D8) are the platform's */
    const sim_platform *p = st->platform;
    if (channel < 3 && p && p->stream_play && cstring(st, name)) {
        p->stream_play(p->ctx, cstring(st, name), channel, loops);
    }
    return 1;
}

int32_t platform_stream_playing(game_state *st, int32_t channel)
{
    /* @bin 0x0055F5A1 no stream without a sound device: 0 */
    const sim_platform *p = st->platform;
    return p && p->stream_playing ? (p->stream_playing(p->ctx, channel) ? 1 : 0) : 0;
}

void platform_log(game_state *st, uint32_t message, uint32_t text, int32_t value)
{
    /* @bin 0x004CE6E0 the log buffer 0x00E36C90 and the log file are the platform's */
    const sim_platform *p = st->platform;
    if (p && p->log) {
        p->log(p->ctx, cstring(st, message), cstring(st, text), value);
    }
}

static int32_t run_stream_play(game_state *st, const int32_t *a)
{
    return platform_stream_play(st, (uint32_t) a[0], a[1], a[2]);
}

static int32_t run_stream_playing(game_state *st, const int32_t *a)
{
    return platform_stream_playing(st, a[0]);
}

static int32_t run_log(game_state *st, const int32_t *a)
{
    platform_log(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
    return 0;
}

static int32_t run_video(game_state *st, const int32_t *a)
{
    return platform_video_play(st, (uint32_t) a[0], a[1], a[2], a[3]);
}

static int32_t run_figure_sound(game_state *st, const int32_t *a)
{
    platform_figure_sound(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_ui_sound(game_state *st, const int32_t *a)
{
    platform_ui_sound(st, a[0]);
    return 0;
}

const ported_function platform_ports[] = {
    {0x00538E30u, "platform_video_play", 4, 32, 0, run_video, PORT_PLATFORM},
    {0x0053B4A0u, "platform_figure_sound", 3, 0, 0, run_figure_sound, PORT_PLATFORM},
    {0x0053B090u, "platform_ui_sound", 1, 0, 0, run_ui_sound, PORT_PLATFORM},
    {0x0055F449u, "platform_stream_play", 3, 32, 0x00E92BF8u, run_stream_play, PORT_PLATFORM},
    {0x0055F5A1u, "platform_stream_playing", 1, 32, 0x00E92BF8u, run_stream_playing, PORT_PLATFORM},
    {0x004CE6E0u, "platform_log", 3, 0, 0, run_log, PORT_PLATFORM},
};
const int platform_ports_count = sizeof(platform_ports) / sizeof(platform_ports[0]);
