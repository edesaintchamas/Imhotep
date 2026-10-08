/* Imhotep - what the simulation asks of the platform: videos, sounds, message boxes.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/platform.h"

#include <string.h>

#include "sim/call.h"
#include "city/day_x87.h"
#include "frame/view.h"
#include "state/records.h"
#include "state/ported.h"

#define SOUND_ON 0x00F55B88u    /* sound system initialised (0x005394F0: the start-up's result) */
#define SOUND_OPTION 0x00E38E63u /* sound effects option */
#define SOUND 0x00E92BF8u        /* the sound object (Miles wrapper) */
#define SOUND_OFF (SOUND + 0x200) /* no sound on this system (0x0055E6C4) */
#define STREAMS (SOUND + 0x1D8)  /* the 3 stream handles */
#define REQUEST 0x00E92DA8u      /* the sample request block: owner, sound, volume, pan, loops */

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

/* ---- sound: the Miles wrappers of the sound object ---- */

static int32_t stream_handle(game_state *st, int32_t channel)
{
    return S32(st, STREAMS + 4u * (uint32_t) channel);
}

int32_t platform_stream_play(game_state *st, uint32_t name, int32_t channel, int32_t loops)
{
    /* @bin 0x0055F452 channels 0..2 (signed compare), else -1; @bin 0x0055F463 no sound system: 1 */
    if (channel > 2) {
        return -1;
    }
    if (S32(st, SOUND_OFF) != 0) {
        return 1;
    }
    const sim_platform *p = st->platform;
    /* @bin 0x0055F47C the channel's stream is closed first (AIL_close_stream) */
    if (stream_handle(st, channel) != 0) {
        if (p && p->stream_stop) {
            p->stream_stop(p->ctx, channel);
        }
        S32(st, STREAMS + 4u * (uint32_t) channel) = 0;
    }
    /* @bin 0x0055F4DE the volume: +0x154 for channel 1, +0x158 for 2, else +0x14C; @bin 0x0055F4BB
     * AIL_open_stream, then loop count, volume, AIL_start_stream: the platform's. The handle (0: it
     * could not be opened, as without a sound device) is kept */
    int32_t volume = channel == 1 ? S32(st, SOUND + 0x154) : channel == 2 ? S32(st, SOUND + 0x158) : S32(st, SOUND + 0x14C);
    const char *s = cstring(st, name);
    int32_t h = p && p->stream_play && s ? p->stream_play(p->ctx, s, channel, loops, volume) : 0;
    S32(st, STREAMS + 4u * (uint32_t) channel) = h;
    return 1;
}

int32_t platform_stream_playing(game_state *st, int32_t channel)
{
    /* @bin 0x0055F5A1 AIL_stream_status == 4 (playing), when a stream is open */
    if (S32(st, SOUND_OFF) != 0 || stream_handle(st, channel) == 0) {
        return 0;
    }
    const sim_platform *p = st->platform;
    return p && p->stream_playing ? (p->stream_playing(p->ctx, channel) ? 1 : 0) : 0;
}

void platform_stream_stop(game_state *st, int32_t channel)
{
    /* @bin 0x0055F561 AIL_close_stream (even of no stream), the handle cleared */
    if (S32(st, SOUND_OFF) != 0) {
        return;
    }
    const sim_platform *p = st->platform;
    if (p && p->stream_stop) {
        p->stream_stop(p->ctx, channel);
    }
    S32(st, STREAMS + 4u * (uint32_t) channel) = 0;
}

int32_t platform_stream_volume(game_state *st, int32_t channel)
{
    /* @bin 0x0055F5F4 AIL_stream_volume of an open stream, else 0 */
    if (S32(st, SOUND_OFF) != 0 || stream_handle(st, channel) == 0) {
        return 0;
    }
    const sim_platform *p = st->platform;
    return p && p->stream_volume ? p->stream_volume(p->ctx, channel) : 0;
}

void platform_stream_set_volume(game_state *st, int32_t channel, int32_t volume)
{
    /* @bin 0x0055F639 AIL_set_stream_volume of an open stream */
    if (S32(st, SOUND_OFF) != 0 || stream_handle(st, channel) == 0) {
        return;
    }
    const sim_platform *p = st->platform;
    if (p && p->stream_set_volume) {
        p->stream_set_volume(p->ctx, channel, volume);
    }
}

void platform_stream_default_volume(game_state *st, int32_t channel, int32_t volume)
{
    /* @bin 0x0055F38D (+0x14C, stream 0), 0x0055F32F (+0x154, 1), 0x0055F3EB (+0x158, 2): kept; when
     * the channel's stream plays (AIL_stream_status 4) its volume is set */
    uint32_t field = channel == 1 ? 0x154u : channel == 2 ? 0x158u : 0x14Cu;
    S32(st, SOUND + field) = volume;
    if (S32(st, SOUND_OFF) != 0 || stream_handle(st, channel) == 0) {
        return;
    }
    const sim_platform *p = st->platform;
    if (p && p->stream_playing && p->stream_playing(p->ctx, channel) && p->stream_set_volume) {
        p->stream_set_volume(p->ctx, channel, volume);
    }
}

int32_t platform_sample_play(game_state *st, uint32_t block)
{
    /* @bin 0x0055F0E8 no sound system, or a sound outside the registry (+4: its count): -1 */
    int32_t sound = S32(st, block + 4);
    if (S32(st, SOUND_OFF) != 0 || sound < 0 || S32(st, SOUND + 4) - 1 < sound) {
        return -1;
    }
    /* @bin 0x0055EDD0 the registry entry's file name (+8: entries of 8 bytes, the name first): a
     * sound with no name, or an empty one, is never loaded (nor cached by 0x0055F07F) */
    uint32_t entry = U32(st, SOUND + 8) + 8u * (uint32_t) sound;
    const uint8_t *e = state_ptr(st, entry, 4);
    uint32_t name = e ? (uint32_t) e[0] | (uint32_t) e[1] << 8 | (uint32_t) e[2] << 16 | (uint32_t) e[3] << 24 : 0;
    const char *s = e ? cstring(st, name) : NULL;
    if (e && (s == NULL || s[0] == 0)) {
        return -1;
    }
    /* @bin 0x0055F0E8 whether the same sound already plays, the voice, the file load and cache, then
     * AIL_set_sample_loop_count, _pan, _volume, _reverb, AIL_start_sample: the platform's */
    const sim_platform *p = st->platform;
    if (p && p->sample_play) {
        p->sample_play(p->ctx, sound, s, S32(st, block + 8), S32(st, block + 12), S32(st, block + 16));
    }
    return 0;
}

/* @bin 0x0056133C __ftol(x * (128.0f / width)): the pan of a point x of the city view (fild width;
 * fdivr dword [0x00570150] = 128.0f; fild x; fmul st(1)), in 53-bit precision. A width of 0 (no
 * display) makes the quotient infinite: fistp stores the indefinite integer, whose low half is 0 */
static int32_t view_pan(game_state *st, int32_t x)
{
    int32_t width = S32(st, 0x00F53F78u) != 0 ? S32(st, 0x00F53F60u) : S32(st, 0x00F53F64u);
    if (width == 0) {
        return 0;
    }
    return x87_ftol(x87_mul(x87_from_int(x), x87_div(128, width)));
}

void platform_figure_sound(game_state *st, int32_t figure, int32_t sound, int32_t channel)
{
    /* @bin 0x0053B4A0 sound on (0x00F55B88), sound < 0x21, channel < 5 (signed), effects on */
    if (S32(st, SOUND_ON) == 0 || sound >= 0x21 || channel >= 5 || U8(st, SOUND_OPTION) == 0) {
        return;
    }
    /* @bin 0x0053B4DF the city view's width: the sidebar's x (0x00F53F60 when the sidebar is
     * collapsed 0x00F53F78, else 0x00F53F64), its height 0x00E38D98 */
    int32_t width = S32(st, 0x00F53F78u) != 0 ? S32(st, 0x00F53F60u) : S32(st, 0x00F53F64u);
    int32_t height = S32(st, 0x00E38D98u);
    uint32_t f = FIGURE_ADDR(figure);
    /* @bin 0x0053B523 the figure's tile (+0x14, +0x16) on the screen; the outputs are the argument
     * slots of sound and figure, which keep their values for a rotation it does not handle */
    int32_t x = sound, y = figure;
    view_tile_to_screen(st, S16(st, f + 0x14), S16(st, f + 0x16), &x, &y);
    int quiet = 0;
    if (x < 0 || y < 0 || x > width || y > height) {
        /* @bin 0x0053B54B off the screen only the figure types of the table 0x0053B614 (0x0B..0x0D,
         * 0x2B..0x2D, 0x36..0x38, 0x4D, 0x4E, 0x5C, 0x5D, 0x63..0x65) are heard, at a quarter */
        static const uint8_t heard[] = {0x0B, 0x0C, 0x0D, 0x2B, 0x2C, 0x2D, 0x36, 0x37,
                                        0x38, 0x4D, 0x4E, 0x5C, 0x5D, 0x63, 0x64, 0x65};
        uint8_t type = U8(st, f + 0x0A);
        for (unsigned i = 0; i < sizeof(heard); i++) {
            quiet |= type == heard[i];
        }
        if (!quiet) {
            return;
        }
    }
    /* @bin 0x0053B571 the sample of (sound, channel): table 0x00F54120, 5 per sound; -1: none */
    int32_t id = S32(st, 0x00F54120u + 4u * (uint32_t) (5 * sound + channel));
    if (id <= -1) {
        return;
    }
    /* @bin 0x0053B5A9 pan; volume: effects 0x00E38EC4, a quarter (signed / 4) when off the screen */
    S32(st, REQUEST + 12) = view_pan(st, x);
    int32_t volume = S32(st, 0x00E38EC4u);
    S32(st, REQUEST + 8) = quiet ? volume / 4 : volume;
    S32(st, REQUEST + 4) = id;
    S32(st, REQUEST) = -1;
    S32(st, REQUEST + 16) = 1;
    GAME_CALLT(st, SOUND, 0x0055F0E8u, (int32_t) REQUEST);
}

void platform_ui_sound(game_state *st, int32_t sound)
{
    /* @bin 0x0053B090 sound on, sound < 0x13, effects on, a sample in the table 0x00F543B8; volume
     * 0x00E38EC4, centred (0x40), once */
    if (S32(st, SOUND_ON) == 0 || sound >= 0x13 || U8(st, SOUND_OPTION) == 0) {
        return;
    }
    int32_t id = S32(st, 0x00F543B8u + 4u * (uint32_t) sound);
    if (id <= -1) {
        return;
    }
    S32(st, REQUEST + 8) = S32(st, 0x00E38EC4u);
    S32(st, REQUEST + 12) = 0x40;
    S32(st, REQUEST + 4) = id;
    S32(st, REQUEST) = -1;
    S32(st, REQUEST + 16) = 1;
    GAME_CALLT(st, SOUND, 0x0055F0E8u, (int32_t) REQUEST);
}

/* random sounds of the city (0x0053ACF0) use the same pan */
int32_t platform_view_pan(game_state *st, int32_t x)
{
    return view_pan(st, x);
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

void platform_log(game_state *st, uint32_t message, uint32_t text, int32_t value)
{
    /* @bin 0x004CE6E0 the log buffer 0x00E36C90 and the log file are the platform's */
    const sim_platform *p = st->platform;
    if (p && p->log) {
        p->log(p->ctx, cstring(st, message), cstring(st, text), value);
    }
}

void platform_open_url(game_state *st, const char *url)
{
    const sim_platform *p = st->platform;
    if (p && p->open_url && url) {
        p->open_url(p->ctx, url);
    }
}

void platform_post_message(game_state *st, uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    const sim_platform *p = st->platform;
    if (p && p->post_message) {
        p->post_message(p->ctx, msg, wparam, lparam);
    }
}

void platform_quit(game_state *st, int32_t code)
{
    const sim_platform *p = st->platform;
    if (p && p->quit) {
        p->quit(p->ctx, code);
    }
}

int32_t platform_file_delete(game_state *st, const char *path)
{
    const sim_platform *p = st->platform;
    return p && p->file_delete && path ? p->file_delete(p->ctx, path) : -1;
}

int platform_cursor_pos(game_state *st, int32_t *x, int32_t *y)
{
    const sim_platform *p = st->platform;
    return p && p->cursor_pos ? p->cursor_pos(p->ctx, x, y) : 0;
}

int16_t platform_key_state(game_state *st, int32_t vk)
{
    const sim_platform *p = st->platform;
    return p && p->key_state ? p->key_state(p->ctx, vk) : 0;
}

int platform_message_pending(game_state *st, uint32_t first, uint32_t last, int32_t remove)
{
    const sim_platform *p = st->platform;
    return p && p->message_pending ? p->message_pending(p->ctx, first, last, remove) : 0;
}

void platform_present(game_state *st)
{
    /* @bin 0x004D03F0 the back buffer 0x00E92E18 (pitch 0x00E38D94) of the display mode's size
     * (width 0x00E38DB0, height 0x00E38DAC) in the format 0x00E92DFC (2: RGB 565) */
    const sim_platform *p = st->platform;
    if (p && p->present) {
        p->present(p->ctx, U32(st, 0x00E92E18u), S32(st, 0x00E38DB0u), S32(st, 0x00E38DACu), S32(st, 0x00E38D94u),
                   S32(st, 0x00E92DFCu) == 2);
    }
}

uint32_t platform_set_display(game_state *st, int32_t width, int32_t height, int32_t rgb565)
{
    const sim_platform *p = st->platform;
    return p && p->set_display ? p->set_display(p->ctx, width, height, rgb565) : 0;
}

uint32_t platform_clock_ms(game_state *st)
{
    const sim_platform *p = st->platform;
    return p && p->clock_ms ? p->clock_ms(p->ctx) : 0;
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
    /* the kinds are the Win32 MB_ types: 0 OK, 1 OK / Cancel, 2 Abort / Retry / Ignore, 3 Yes / No /
     * Cancel, 4 Yes / No, 5 Retry / Cancel (results 1..7 are the Win32 IDOK..IDNO codes) */
    int32_t pressed = p && p->message_box ? p->message_box(p->ctx, cstring(st, title), cstring(st, text), (int32_t) k,
                                                           count[k]) : 0;
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

/* @bin 0x00413690 (dis 0x004136D2 / 0x00413833) the display mode switch around a video in fullscreen:
 * 0x005E06B0 := 1 (the present waits), DirectDraw closed (0x004D01E0(1)), the resolution `mode`
 * (0x004CFDC0), the screen buffers (0x00414BD0), the view rows (0x004E2AB0), DirectDraw set up again
 * (0x004D0220: platform_set_display), 0x005E06B0 := 0, the resolution check (0x00402DA0), a redraw */
static void bink_mode(game_state *st, int32_t mode)
{
    S32(st, 0x005E06B0u) = 1;
    GAME_CALL(st, 0x004D01E0u, 1);
    GAME_CALL(st, 0x004CFDC0u, mode);
    GAME_CALL0(st, 0x00414BD0u);
    GAME_CALL0(st, 0x004E2AB0u);
    GAME_CALL0(st, 0x004D0220u);
    S32(st, 0x005E06B0u) = 0;
    GAME_CALL0(st, 0x00402DA0u);
    U8(st, 0x00E38DCAu) = 1;
}

int32_t platform_video_bink(game_state *st, uint32_t self, uint32_t name, int32_t x, int32_t y)
{
    /* the player's object (0x1C bytes outside the state, made by 0x004CF960: core/boot/display.c):
     * +0 the window id before the video, +4 (byte) 0 while a video plays (the window procedure then
     * passes every message to DefWindowProcA: core/frame/wndproc.c), +0xC a buffer, +0x10 the Bink
     * handle, +0x14 / +0x18 the video's place. The handle is the platform's: the platform's video is
     * closed when video_modal returns, so +0x10 stays 0 */
    uint8_t *obj = self ? state_ptr(st, self, 0x1C) : NULL;
    /* @bin 0x00413586 a handle still open (+0x10) closed first (BinkClose: the platform's; none is) */
    /* @bin 0x00413594 with a sound driver ([0x012663AC]) the streams 0, 2, 1 are stopped (0x0055F561)
     * and Bink gets the Miles sound system (BinkSetSoundSystem: the platform's) */
    if (U32(st, 0x012663ACu) != 0) {
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 0);
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 2);
        GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 1);
    }
    /* @bin 0x004135DA the file: sprintf "%s", fopen "rb", else "%c:\\%s" with the CD drive 0x00E38E62;
     * BinkOpen (0x0056F24C): the platform's (video_open). It fails without a video device: 0, after
     * OutputDebugStringA (the platform's) */
    const sim_platform *p = st->platform;
    const char *s = cstring(st, name);
    int32_t vw = 0, vh = 0;
    if (!p || !p->video_open || !s || !p->video_open(p->ctx, s, &vw, &vh)) {
        return 0;
    }
    /* @bin 0x00413655 it opened: the window id kept (+0) and 0x4A (the video) set, +4 := 0, no redraw
     * (0x00E38DCA := 0), the place x, y (+0x14, +0x18) */
    uint8_t old = U8(st, 0x00E38DDAu);
    if (obj) {
        obj[0] = old;
        obj[1] = obj[2] = obj[3] = 0;
        obj[4] = 0;
        memcpy(obj + 0x18, &y, 4);
        memcpy(obj + 0x14, &x, 4);
    }
    U8(st, 0x00E38DDAu) = 0x4A;
    U8(st, 0x00E38DCAu) = 0;
    /* @bin 0x00413690 the modal player (thiscall on the object; it uses the global 0x006FDC54): nothing
     * without the global object */
    if (U32(st, 0x006FDC54u) != 0) {
        /* @bin 0x004136AC the back buffer and the screen cleared (0x00426F60) */
        GAME_CALL0(st, 0x00426F60u);
        /* @bin 0x004136B1 fullscreen (0x00E38E5C): the display switched to 640 x 480 for the video, the
         * resolution 0x00E38E68 kept to come back to */
        int full = S32(st, 0x00E38E5Cu) != 0;
        int32_t mode = S32(st, 0x00E38E68u);
        if (full) {
            bink_mode(st, 1);
        }
        /* @bin 0x0041370E the video centred on the screen: (0x00E38DB0 - width) >> 1, (0x00E38DAC -
         * height) >> 1 (unsigned shifts) into +0x14, +0x18 (the caller's x, y are overwritten);
         * ShowCursor(0); then the frame loop (BinkDoFrame, BinkNextFrame, the copy 0x00413470 to the
         * primary surface, BinkWait; a key, an Alt key or a left click ends it: WM_KEYDOWN, WM_SYSKEYDOWN,
         * WM_LBUTTONDOWN; other messages go to the window procedure, which passes them on while +4 is 0)
         * and ShowCursor(1): the platform's (video_modal) */
        int32_t cx = (int32_t) ((uint32_t) (S32(st, 0x00E38DB0u) - vw) >> 1);
        int32_t cy = (int32_t) ((uint32_t) (S32(st, 0x00E38DACu) - vh) >> 1);
        if (obj) {
            memcpy(obj + 0x14, &cx, 4);
            memcpy(obj + 0x18, &cy, 4);
        }
        if (p->video_modal) {
            p->video_modal(p->ctx, cx, cy);
        }
        /* @bin 0x00413420 the end: the window id back (+0), +4 := 1, a redraw, BinkClose (+0x10 := 0),
         * the buffer +0xC released (BinkBufferClose; +0xC := 0): the platform's */
        U8(st, 0x00E38DDAu) = obj ? obj[0] : old;
        if (obj) {
            obj[4] = 1;
        }
        U8(st, 0x00E38DCAu) = 1;
        if (full) {
            bink_mode(st, mode); /* @bin 0x00413833 back to the resolution of before */
        }
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
        P32(st, out) = p->pixels_load(p->ctx, path, (uint32_t) skip, (uint32_t) size, convert, rle);
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

static int32_t run_log(game_state *st, const int32_t *a)
{
    platform_log(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
    return 0;
}

static int32_t run_video(game_state *st, const int32_t *a)
{
    return platform_video_play(st, (uint32_t) a[0], a[1], a[2], a[3]);
}

static int32_t run_stream_play(game_state *st, const int32_t *a)
{
    return platform_stream_play(st, (uint32_t) a[0], a[1], a[2]);
}

static int32_t run_stream_playing(game_state *st, const int32_t *a)
{
    return platform_stream_playing(st, a[0]);
}

static int32_t run_volume_music(game_state *st, const int32_t *a)
{
    platform_stream_default_volume(st, 0, a[0]);
    return 0;
}

static int32_t run_volume_speech(game_state *st, const int32_t *a)
{
    platform_stream_default_volume(st, 1, a[0]);
    return 0;
}

static int32_t run_volume_ambient(game_state *st, const int32_t *a)
{
    platform_stream_default_volume(st, 2, a[0]);
    return 0;
}

static int32_t run_sample_play(game_state *st, const int32_t *a)
{
    return platform_sample_play(st, (uint32_t) a[0]);
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
    {0x00413690u, "bink_modal (in 0x00413580)", 0, 0, 0, NULL, 0},
    {0x00413420u, "bink_end (in 0x00413580)", 0, 8, 0, NULL, 0},
    {0x00539170u, "platform_video_close", 0, 0, 0, run_video_close, PORT_PLATFORM},
    /* the sound requests that read the state (they write only the request block, excluded.py) */
    {0x0053B4A0u, "figure_sound", 3, 0, 0, run_figure_sound, 0},
    {0x0053B090u, "ui_sound", 1, 0, 0, run_ui_sound, 0},
    /* the Miles wrappers of the sound object */
    {0x0055F449u, "platform_stream_play", 3, 32, 0x00E92BF8u, run_stream_play, PORT_PLATFORM},
    {0x0055F5A1u, "platform_stream_playing", 1, 32, 0x00E92BF8u, run_stream_playing, PORT_PLATFORM},
    {0x0055F38Du, "platform_stream_volume_music", 1, 0, 0x00E92BF8u, run_volume_music, PORT_PLATFORM},
    {0x0055F32Fu, "platform_stream_volume_speech", 1, 0, 0x00E92BF8u, run_volume_speech, PORT_PLATFORM},
    {0x0055F3EBu, "platform_stream_volume_ambient", 1, 0, 0x00E92BF8u, run_volume_ambient, PORT_PLATFORM},
    {0x0055F0E8u, "platform_sample_play", 1, 0, 0x00E92BF8u, run_sample_play, PORT_PLATFORM},
    {0x004CE6E0u, "platform_log", 3, 0, 0, run_log, PORT_PLATFORM},
    {0x00538CD0u, "platform_message_box", 3, 32, 0, run_message_box, PORT_PLATFORM},
    {0x0053F660u, "platform_pixels_load", 7, 32, 0, run_pixels_load, PORT_PLATFORM},
};
const int platform_ports_count = sizeof(platform_ports) / sizeof(platform_ports[0]);
