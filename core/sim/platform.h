/* Imhotep - what the simulation asks of the platform: videos, sounds, files.
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
    /* The sound driver (the original's Miles wrapper, the sound object 0x00E92BF8; docs/presentation.md
     * 6). Streams: channel 0 music, 1 speech, 2 ambient; volumes 0..100 (the options' scale, given to
     * Miles as they are).
     * stream_play: open the file `name` (the path the game builds, e.g. "audio\\music\\battle.mp3") on
     * the channel, which has no stream (stream_stop was asked first), and play it `loops` times (0:
     * forever) at `volume`; returns a non-zero handle when it plays, 0 when it cannot be opened */
    int32_t (*stream_play)(void *ctx, const char *name, int32_t channel, int32_t loops, int32_t volume);
    /* is the channel's stream playing (not ended) */
    int (*stream_playing)(void *ctx, int32_t channel);
    /* close the channel's stream (also asked when none is open, as the original does) */
    void (*stream_stop)(void *ctx, int32_t channel);
    /* the channel's stream volume; set it */
    int32_t (*stream_volume)(void *ctx, int32_t channel);
    void (*stream_set_volume)(void *ctx, int32_t channel, int32_t volume);
    /* a sample (a .wav of the sound registry: `sound` its index, `name` its file, NULL when the state
     * does not hold it) at `volume` (0..100), `pan` (0 left .. 127 right, 64 centre), played `loops`
     * times. The driver's part is the platform's: it plays nothing when the same sound is still
     * playing (0x0055F0E8), it picks one of its 5 voices, it loads and caches the file */
    void (*sample_play)(void *ctx, int32_t sound, const char *name, int32_t volume, int32_t pan, int32_t loops);
    /* a line of the game's log */
    void (*log)(void *ctx, const char *message, const char *text, int32_t value);
    /* a modal message box (title, text, number of buttons 1..3): the index of the button the
     * player pressed */
    int (*message_box)(void *ctx, const char *title, const char *text, int32_t kind, int32_t buttons);
    /* a game data file, by the path the game builds (only regular files: an empty name or a
     * directory is no file, as for the original's _open) (e.g. "data\\maus0_1a.sg3", relative to the
     * game's directory, case-insensitive): copies the bytes of [offset, offset + size) that exist
     * into buf (buf may be NULL with size 0); returns the file's size, or -1 when there is none */
    int32_t (*file_read)(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size);
    /* a graphics file's pixels (the .555 of a graphics slot) loaded into the platform's memory:
     * the bytes from `skip` on (`size` of them); returns a non-zero handle, which the game state
     * keeps (pointers into it: the slot's image entries); 0 when it cannot be loaded */
    uint32_t (*pixels_load)(void *ctx, const char *path, uint32_t skip, uint32_t size, int32_t convert,
                            int32_t rle);
    /* show the back buffer (the original's copy to the primary surface, 0x004D03F0): its game address,
     * size, pitch in bytes, format (1: RGB 565, 0: RGB 555) */
    void (*present)(void *ctx, uint32_t buffer, int32_t width, int32_t height, int32_t pitch, int32_t rgb565);
    /* change the display mode (the DirectDraw set-up 0x004D0060: at the start, the resolution buttons
     * F6..F9, the 640x480 switch around videos): the screen becomes width x height 16-bit pixels in
     * the format rgb565 (1: RGB 565, 0: RGB 555; the game keeps the format it started with), and a
     * new back buffer of that size (rows of 2 * width bytes, zeroed) is made in the platform's
     * memory, readable and writable through state_ptr (the original's back buffer is a system-memory
     * surface whose memory does not move while the mode lasts). Returns the buffer's game address,
     * 0 when the mode cannot be set. The buffer of the
     * previous call is released by the platform (the original releases its surfaces first,
     * 0x00426EE0). When the mode changes, the platform then delivers WM_DISPLAYCHANGE (0x7E: 16 bits,
     * height << 16 | width) to the window procedure, as Windows does after a mode change (the screen
     * size 0x00E38DB8 / 0x00E38DB4 that the scrolling and the window's place use); the same mode set
     * again sends none */
    uint32_t (*set_display)(void *ctx, int32_t width, int32_t height, int32_t rgb565);
    /* the player ends the playing video */
    void (*video_close)(void *ctx);
    /* open a web address in the user's browser (the original's ShellExecuteA on the main menu) */
    void (*open_url)(void *ctx, const char *url);
    /* post a window message to the game's own window (PostMessageA: Esc from Alt+X, F6..F9 from the
     * resolution buttons): the platform delivers it to frame_wndproc later, as Windows would */
    void (*post_message)(void *ctx, uint32_t msg, uint32_t wparam, uint32_t lparam);
    /* the game asks to quit (PostQuitMessage) */
    void (*quit)(void *ctx, int32_t code);
    /* delete a player file or an empty player directory (a saved game, a family's files and folder:
     * CRT remove / DeleteFileA / RemoveDirectoryA); 0 on success */
    int32_t (*file_delete)(void *ctx, const char *path);
    /* the cursor in client coordinates (the original's GetCursorPos); 1 if known */
    int (*cursor_pos)(void *ctx, int32_t *x, int32_t *y);
    /* a key's state as GetAsyncKeyState gives it: bit 15 held, bit 0 pressed since the last call */
    int16_t (*key_state)(void *ctx, int32_t vk);
    /* is a window message in [first, last] pending (PeekMessageA); remove: take it out */
    int (*message_pending)(void *ctx, uint32_t first, uint32_t last, int32_t remove);
    /* the wall clock in milliseconds (the original's timeGetTime / GetTickCount) */
    uint32_t (*clock_ms)(void *ctx);
    /* releases a handle from pixels_load */
    void (*pixels_free)(void *ctx, uint32_t handle);
    /* a player file (the campaign progress Save\\<family>.dat, ...), by the path the game builds,
     * created or replaced with these bytes; returns the number of bytes written, or -1 */
    int32_t (*file_write)(void *ctx, const char *path, const uint8_t *data, uint32_t size);
    /* a directory listing (FindFirstFileA / FindNextFileA): the index-th entry (0, 1, ...) whose name
     * matches `pattern` (the path the game builds, e.g. "maps\\*.map": a directory relative to the
     * game's directory, case-insensitive, then a last part with '*' and '?'), in the order the
     * file system lists them (Windows NTFS: by name, upper-cased; assumed, docs/oracle.md), the player's files included; its
     * name (without the directory) copied into name (size bytes, NUL-terminated). Returns the name's
     * length, or -1 when there is no such entry */
    int32_t (*file_list)(void *ctx, const char *pattern, int32_t index, char *name, uint32_t size);
    /* a Bink video (the period videos, the intro; the original's BinkOpen in 0x00413580): opened, its
     * frame size in *width, *height; 1, or 0 when it cannot be opened (the game then goes on without
     * it). The game then sets its display (in fullscreen the 640 x 480 mode, through set_display) and
     * asks video_modal */
    int (*video_open)(void *ctx, const char *name, int32_t *width, int32_t *height);
    /* the opened video played in the game's screen at x, y (centred by the game: 0x00413690), the
     * cursor hidden, until its end or a key, an Alt key or a left click (WM_KEYDOWN, WM_SYSKEYDOWN,
     * WM_LBUTTONDOWN); the window's other messages are the window procedure's, which ignores them while
     * a video plays (the player's object +4 is 0); then the video is closed (BinkClose) */
    void (*video_modal)(void *ctx, int32_t x, int32_t y);
    /* the game's window as the window procedure asks Windows about it (WM_MOVE / WM_SIZE:
     * IsIconic, GetClientRect + ClientToScreen; WM_ACTIVATEAPP: GetForegroundWindow, IsIconic;
     * core/frame/display.c): *iconic 1 when the window is minimised, *foreground 1 when it is the
     * foreground window, rect the client area in screen coordinates (left, top, right, bottom).
     * Returns 1, or 0 when the platform does not know (NULL too): the window is then taken as
     * shown, in front, its client area the screen (0, 0, [0x00E38DB8], [0x00E38DB4]), as the
     * oracle's window stubs answer (tools/oracle/game.py) */
    int (*window_state)(void *ctx, int32_t *iconic, int32_t *foreground, int32_t rect[4]);
    /* memory outside the state, for the original's allocations that hold no simulation data (they
     * go to the oracle's other heap: tools/oracle/game.py SIM_ALLOCATORS; e.g. the spare screen
     * 0x00E92E1C made again after a resolution change, 0x00414BD0): size > 0: a new zeroed block,
     * readable and writable through state_ptr, its address returned (0: no memory); size 0: the
     * block at `address` released (the platform ignores addresses it did not give, e.g. those of
     * the oracle's boot image). Without it, the ports that need it are replayed in comparisons
     * (partial ports) */
    uint32_t (*memory)(void *ctx, uint32_t address, uint32_t size);
    /* @imhotep enhanced fit: the enhanced display (docs/enhanced.md). Returns 1 and the game's size in
     * pixels (the window's) when the platform fits the game to its window: resolution 3 then takes
     * that size instead of 1024 x 768. 0 (or NULL): the original modes only */
    int (*display_fit)(void *ctx, int32_t *width, int32_t *height);
    /* @imhotep enhanced zoom: the city view's zoom in percent (50 .. 200); NULL: 100 */
    int32_t (*display_zoom)(void *ctx);
} sim_platform;

/* @bin 0x00538E30 the video player (Smacker): returns 1 when the video plays */
int32_t platform_video_play(game_state *st, uint32_t name, int32_t a, int32_t b, int32_t c);
/* @bin 0x00413580 the Bink video player (thiscall on its object [0x006FDC54], outside the state: the
 * old window id, the Bink handle): opens the video `name` (a state string; sim_platform.video_open),
 * and when it opens plays it modally (0x00413690: the screen cleared, in fullscreen the display
 * switched to 640 x 480 and back after, sim_platform.video_modal, the window id 0x4A meanwhile);
 * returns 1 when it played, 0 when it cannot be opened (no platform: as the original does when
 * BinkOpen fails) */
int32_t platform_video_bink(game_state *st, uint32_t self, uint32_t name, int32_t x, int32_t y);
/* @bin 0x00539170 the player ends the video (a key or click in the video window): nothing when
 * none plays (0x00F54054, the video player's state: tools/oracle/excluded.py) */
void platform_video_close(game_state *st);
/* @bin 0x0053B4A0 a figure's sound `sound` (< 0x21) of variant `channel` (< 5), when sound is on
 * (0x00F55B88, 0x00E38E63): its sample (table 0x00F54120), panned by the figure's place on the screen,
 * silent off the screen except for 16 figure types (a quarter of the volume) */
void platform_figure_sound(game_state *st, int32_t figure, int32_t sound, int32_t channel);
/* @bin 0x0053B090 an interface sound (< 0x13; table 0x00F543B8), centred, when sound is on */
void platform_ui_sound(game_state *st, int32_t sound);
/* @bin 0x0053B5A9 the pan (0 .. 128) of the city view's x: __ftol(x * (128.0f / view width)) */
int32_t platform_view_pan(game_state *st, int32_t x);

/* The Miles wrappers, thiscall on the sound object 0x00E92BF8 (its fields: +0 the sound registry
 * (capacity, count, entries {name, owned}), +0x14C / +0x154 / +0x158 the default volume of channels
 * 0 / 1 / 2, +0x1D8 the 3 stream handles (the platform's: non-zero while a stream is open), +0x200
 * set when the system has no sound (Windows NT 4: 0x0055E6C4)). */
/* @bin 0x0055F449 open a stream and play it (channel <= 2, else -1); returns 1 */
int32_t platform_stream_play(game_state *st, uint32_t name, int32_t channel, int32_t loops);
/* @bin 0x0055F5A1 is a stream playing on the channel */
int32_t platform_stream_playing(game_state *st, int32_t channel);
/* @bin 0x0055F561 close the channel's stream */
void platform_stream_stop(game_state *st, int32_t channel);
/* @bin 0x0055F5F4 the channel's stream volume (0 when none is open) */
int32_t platform_stream_volume(game_state *st, int32_t channel);
/* @bin 0x0055F639 set the volume of the channel's stream (when one is open) */
void platform_stream_set_volume(game_state *st, int32_t channel, int32_t volume);
/* @bin 0x0055F38D (channel 0), 0x0055F32F (1), 0x0055F3EB (2) the channel's default volume (kept in
 * the sound object, given to the streams it opens), applied to its stream when one plays */
void platform_stream_default_volume(game_state *st, int32_t channel, int32_t volume);
/* @bin 0x0055F0E8 play a sample from a request block (the game's at 0x00E92DA8: owner -1, sound,
 * volume, pan, loops); returns -1 when the driver refuses it (no sound, sound out of the registry, no
 * file name), else 0 (the original returns the voice the driver chose: the platform's; its callers
 * ignore it) */
int32_t platform_sample_play(game_state *st, uint32_t block);
/* @bin 0x004CE6E0 the game's log (message, optional text, value) */
void platform_log(game_state *st, uint32_t message, uint32_t text, int32_t value);

/* window-system requests: a web address, a message posted to the game's own window, quitting,
 * deleting a player file (-1 without a platform) */
void platform_open_url(game_state *st, const char *url);
void platform_post_message(game_state *st, uint32_t msg, uint32_t wparam, uint32_t lparam);
void platform_quit(game_state *st, int32_t code);
int32_t platform_file_delete(game_state *st, const char *path);

/* the inputs the frame and the window procedure read (core/frame/input.c): 0 when the platform
 * does not give them (the callers then use their fallbacks) */
int platform_cursor_pos(game_state *st, int32_t *x, int32_t *y);
int16_t platform_key_state(game_state *st, int32_t vk);
int platform_message_pending(game_state *st, uint32_t first, uint32_t last, int32_t remove);

/* the wall clock in milliseconds: the original reads timeGetTime / GetTickCount (seeds of the CRT
 * rand, fade and message timers, profiling); without a platform 0, as the oracle's default */
uint32_t platform_clock_ms(game_state *st);

/* @bin 0x004D03F0 the back buffer to the screen (Blt / BltFast): the platform shows it; no state */
void platform_present(game_state *st);
/* a display mode change (sim_platform.set_display): the new back buffer's address, 0 when the
 * platform cannot set the mode or without a platform */
uint32_t platform_set_display(game_state *st, int32_t width, int32_t height, int32_t rgb565);
/* @imhotep enhanced fit: 1 and the fit size (clamped to 1024..4096 x 768..4096) when the platform fits
 * the game to its window (sim_platform.display_fit), else 0 */
int platform_display_fit(game_state *st, int32_t *width, int32_t *height);
/* @imhotep enhanced zoom: the city view's zoom in percent, 100 without a platform that zooms */
int32_t platform_display_zoom(game_state *st);
/* @imhotep enhanced zoom: a point (x, y) of the screen inside the zoomed city view (x below the sidebar,
 * y from 30) mapped to the view's own pixels (x * 100 / zoom, 30 + (y - 30) * 100 / zoom); 0 for a point
 * outside the view when zoomed, else 1 (at 100 the point is left as it is) */
int zoom_point(game_state *st, int32_t *x, int32_t *y);

/* @bin 0x00538CD0 a modal message box (title, text, kind): the platform shows it and returns the
 * pressed button; without a platform the first button is pressed (tools/oracle/game.py does the
 * same in the original). Returns the button's result code (0x00538940) */
int32_t platform_message_box(game_state *st, uint32_t title, uint32_t text, int32_t kind);

/* @bin 0x004CEB60 the game's file reads (file_read_at: open, seek, read the rest of the file): the
 * platform supplies game data files (the .sg3 indexes read by 0x0053DFF0). Copies up to `size`
 * bytes from `offset` into buf; returns the file's size, or -1 when the file cannot be read
 * (no platform, or no such file) */
int32_t platform_file_read(game_state *st, const char *path, uint32_t offset, uint8_t *buf, uint32_t size);

/* @bin 0x0041C8F0 the game's directory listings (FindFirstFileA, FindNextFileA, FindClose): the
 * index-th name matching `pattern` ("maps\\*.map", "Save\\*.dat", "save\\<family>\\*.sav") into
 * name (size bytes), in the order the original's FindNextFileA gives them; returns its length, or -1
 * when there is none (no platform, no such directory, or index past the last match) */
int32_t platform_file_list(game_state *st, const char *pattern, int32_t index, char *name, uint32_t size);

/* @bin 0x0041B8A0 the game's file writes (CRT fopen "wb", fwrite, fclose): the platform creates or
 * replaces the player file `path` with `size` bytes; returns the number of bytes written, or -1
 * when the file cannot be written (no platform, or an error) */
int32_t platform_file_write(game_state *st, const char *path, const uint8_t *data, uint32_t size);

/* @bin 0x0053F660 a graphics slot's pixels ("data\\" name ".555", from `skip` on) loaded by the
 * platform: the previous buffer at `out` is released, the new handle stored at `out`; returns the
 * size in bytes (0: nothing loaded). The pixel conversion (0x004CDF20 / 0x004CDEB0) is the
 * platform's. In the oracle comparisons it is replayed from the original's run */
int32_t platform_pixels_load(game_state *st, uint32_t name, uint32_t out, int32_t convert, int32_t size,
                             int32_t rle, int32_t offset, int32_t skip);

#endif
