/* Imhotep - what the simulation asks of the platform: videos, sounds, message boxes.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/platform.h"

#include <string.h>

#include "sim/call.h"
#include "state/ported.h"

#define SOUND_ON 0x00F55B88u    /* sound system initialised */
#define SOUND_OPTION 0x00E38E63u /* sound effects option */

void platform_video_close(game_state *st)
{
    /* @bin 0x00539170 only while a video plays (0x00F54054 != 0, set by the player's start) */
    if (S32(st, 0x00F54054u) == 0) {
        return;
    }
    const sim_platform *p = st->platform;
    if (p && p->video_close) {
        p->video_close(p->ctx);
    }
    /* @bin 0x00539220 the window id saved by the video start (0x00F53FFC) comes back; the video
     * request 0x00F54118 (set by the message posts 0x004E0CF0, 0x004E1320, 0x004EA2C0) is cleared,
     * with a redraw when it was 2 */
    U8(st, 0x00E38DDAu) = U8(st, 0x00F53FFCu);
    S32(st, 0x00F54054u) = 0;
    if (S32(st, 0x00F54118u) == 2) {
        U8(st, 0x00E38DCAu) = 1;
    }
    S32(st, 0x00F54118u) = 0;
}

static int32_t run_video_close(game_state *st, const int32_t *a)
{
    (void) a;
    platform_video_close(st);
    return 0;
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
    /* a string of the state (or of mapped game data), terminated within 4096 bytes */
    const uint8_t *s = address ? state_ptr(st, address, 1) : NULL;
    for (uint32_t i = 0; s && i < 4096; i++) {
        const uint8_t *c = state_ptr(st, address + i, 1);
        if (!c) {
            return NULL;
        }
        if (*c == 0) {
            return (const char *) s;
        }
    }
    return NULL;
}

int32_t platform_video_play(game_state *st, uint32_t name, int32_t a, int32_t b, int32_t c)
{
    /* @bin 0x00538E30 without a video device the original fails to open the file and returns 0;
     * what it writes meanwhile is the player's own state (excluded.py) */
    const sim_platform *p = st->platform;
    const char *s = cstring(st, name);
    if (!p || !p->video_play || !s) {
        return 0;
    }
    return p->video_play(p->ctx, s, a, b, c) ? 1 : 0;
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

int32_t platform_message_box(game_state *st, uint32_t title, uint32_t text, int32_t kind)
{
    /* @bin 0x00538940 the buttons of each kind (kind & 0xF) and the result code of each button */
    static const int32_t results[6][3] = {{1}, {1, 2}, {3, 4, 5}, {6, 7, 2}, {6, 7}, {4, 2}};
    static const int32_t count[6] = {1, 2, 3, 3, 2, 2};
    uint32_t k = (uint32_t) kind & 0xFu;
    if (k > 5) {
        k = 0;
    }
    const sim_platform *p = st->platform;
    int32_t pressed = p && p->message_box ? p->message_box(p->ctx, cstring(st, title), cstring(st, text), count[k]) : 0;
    if (pressed < 0 || pressed >= count[k]) {
        pressed = 0;
    }
    /* @bin 0x00538940 button count; @bin 0x00538920 the pressed button (flag, index: the button
     * table 0x005F8910 gives each button its index); @bin 0x00538CD0 the window id 0x00E38DDA is
     * restored, redraw flag 0x00E38DCA := 1 */
    S32(st, 0x00F535C8u) = count[k];
    U8(st, 0x00F535C0u) = 1;
    S32(st, 0x00F535C4u) = pressed;
    U8(st, 0x00E38DCAu) = 1;
    return results[k][pressed];
}

int32_t platform_file_read(game_state *st, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    /* @bin 0x004CEB60 the open / seek / read itself (and its CD-drive retry with the "insert the CD"
     * box 0x004CE9E0) is the platform's */
    const sim_platform *p = st->platform;
    if (!p || !p->file_read || !path) {
        return -1;
    }
    return p->file_read(p->ctx, path, offset, buf, size);
}

int32_t platform_file_list(game_state *st, const char *pattern, int32_t index, char *name, uint32_t size)
{
    /* @bin 0x0041C927 FindFirstFileA (0x0056F058), 0x0041C9DA FindNextFileA (0x0056F05C), 0x0041C9ED
     * FindClose (0x0056F128): the directory and its order are the platform's */
    const sim_platform *p = st->platform;
    if (!p || !p->file_list || !pattern || index < 0 || !name || size == 0) {
        return -1;
    }
    int32_t n = p->file_list(p->ctx, pattern, index, name, size);
    name[size - 1] = 0;
    return n < 0 ? -1 : (int32_t) strlen(name);
}

int32_t platform_video_bink(game_state *st, uint32_t self, uint32_t name, int32_t x, int32_t y)
{
    (void) self; /* the player's object: the Bink handle (+0x10, closed first), the old window id */
    /* @bin 0x00413594 with a sound driver ([0x012663AC]) the streams 0, 2, 1 are stopped (0x0055F561)
     * and Bink gets the Miles sound system (BinkSetSoundSystem: the platform's) */
    if (U32(st, 0x012663ACu) != 0) {
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 0);
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 2);
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 1);
    }
    /* @bin 0x004135DA the file: sprintf "%s", fopen "rb", else "%c:\\%s" with the CD drive 0x00E38E62;
     * BinkOpen (0x0056F24C): the platform's. It fails without a video device: 0 */
    const sim_platform *p = st->platform;
    const char *s = cstring(st, name);
    if (!p || !p->video_modal || !s || !p->video_modal(p->ctx, s, x, y)) {
        return 0;
    }
    /* @bin 0x00413655 it played: the window id 0x4A (the video) during the modal loop 0x00413690, then
     * @bin 0x00413420 the one before the call back (kept in the object), redraw */
    U8(st, 0x00E38DCAu) = 1;
    if (U32(st, 0x00E38E5Cu) != 0) {
        /* @bin 0x00413833 with the option 0x00E38E5C the display mode is set back after the video
         * (0x004D01E0, 0x004CFDC0, 0x00414BD0, 0x004D0220: surfaces, window, screen buffer, the
         * platform's), then the view rows 0x004E2AB0, 0x005E06B0 := 0, the resolution check
         * 0x00402DA0, redraw */
        GAME_CALL0(st, 0x004E2AB0u);
        U32(st, 0x005E06B0u) = 0;
        GAME_CALL0(st, 0x00402DA0u);
        U8(st, 0x00E38DCAu) = 1;
    }
    return 1;
}

static int32_t run_video_bink(game_state *st, const int32_t *a)
{
    return platform_video_bink(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0], a[1], a[2]);
}

int32_t platform_file_write(game_state *st, const char *path, const uint8_t *data, uint32_t size)
{
    /* @bin 0x0041B8A0 fopen(path, "wb") (0x00561A0A), fwrite (0x00561F5F), fclose (0x00561994):
     * the stream and the file are the platform's */
    const sim_platform *p = st->platform;
    if (!p || !p->file_write || !path) {
        return -1;
    }
    return p->file_write(p->ctx, path, data, size);
}

int32_t platform_pixels_load(game_state *st, uint32_t name, uint32_t out, int32_t convert, int32_t size,
                             int32_t rle, int32_t offset, int32_t skip)
{
    const sim_platform *p = st->platform;
    const char *base = cstring(st, name);
    (void) convert;
    (void) rle;
    (void) offset;
    /* @bin 0x0053F790 the previous buffer is released unless it is the spare screen buffer
     * 0x00E92E1C (kept, and the pointer then kept too) */
    uint32_t old = P32(st, out);
    if (old != 0 && old != P32(st, 0x00E92E1Cu)) {
        if (p && p->pixels_free) {
            p->pixels_free(p->ctx, old);
        }
        P32(st, out) = 0;
    }
    if (!p || !p->pixels_load || !base || strlen(base) > 240) {
        return 0;
    }
    /* @bin 0x0053F660 path "data\\" (0x006018F8) + name + ".555" (0x00601900); size 0: the file's
     * size minus skip (0x004CEA30) */
    char path[256] = "data\\";
    strcat(path, base);
    strcat(path, ".555");
    if (size == 0) {
        int32_t n = platform_file_read(st, path, 0, NULL, 0);
        /* @bin 0x004CEA30 a missing file goes to the CD-drive retry ("insert the CD"), which is
         * the platform's: here nothing is loaded */
        size = n < 0 ? 0 : n - skip;
    }
    if (size > 0) {
        P32(st, out) = p->pixels_load(p->ctx, path, (uint32_t) skip, (uint32_t) size);
    }
    return size;
}

static int32_t run_pixels_load(game_state *st, const int32_t *a)
{
    return platform_pixels_load(st, (uint32_t) a[0], (uint32_t) a[1], a[2], a[3], a[4], a[5], a[6]);
}

static int32_t run_message_box(game_state *st, const int32_t *a)
{
    return platform_message_box(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
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
    {0x00413580u, "platform_video_bink", 3, 8, 0, run_video_bink, PORT_PLATFORM},
    {0x00539170u, "platform_video_close", 0, 0, 0, run_video_close, PORT_PLATFORM},
    {0x0053B4A0u, "platform_figure_sound", 3, 0, 0, run_figure_sound, PORT_PLATFORM},
    {0x0053B090u, "platform_ui_sound", 1, 0, 0, run_ui_sound, PORT_PLATFORM},
    {0x0055F449u, "platform_stream_play", 3, 32, 0x00E92BF8u, run_stream_play, PORT_PLATFORM},
    {0x0055F5A1u, "platform_stream_playing", 1, 32, 0x00E92BF8u, run_stream_playing, PORT_PLATFORM},
    {0x004CE6E0u, "platform_log", 3, 0, 0, run_log, PORT_PLATFORM},
    {0x00538CD0u, "platform_message_box", 3, 32, 0, run_message_box, PORT_PLATFORM},
    {0x0053F660u, "platform_pixels_load", 7, 32, 0, run_pixels_load, PORT_PLATFORM},
};
const int platform_ports_count = sizeof(platform_ports) / sizeof(platform_ports[0]);
