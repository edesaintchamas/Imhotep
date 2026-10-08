/* Imhotep - the game's start-up: WinMain's initialisation (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A state created from the exe's initialised sections (platform/exe_image.c) becomes the booted
 * game: the C++ static constructors, then WinMain (@bin 0x00413ED0) up to its message loop: the
 * settings, the text files, the graphics slots, the model files, the campaign table, the sound and
 * display set-ups, the monument objects. Docs: docs/oracle.md "Start-up", docs/formats/state.md.
 *
 * Memory outside the state. The original keeps some of what it loads in heap blocks that are not
 * simulation state (the text files, the spare screen, the sound registry, the pixel data, the
 * DirectDraw surfaces): only their addresses are in the state. Core asks the host for such blocks
 * (boot_host.alloc) and reads or writes them through state_ptr: the host maps them
 * (state_map_external). boot_arena below is a host that gives them out of one mapped region by
 * bumping a pointer in 16-byte steps, as the oracle's "other heap" does (tools/oracle/emu.py);
 * the comparison with the oracle gives the original's addresses instead
 * (tests/differential/oracle_boot_run.c). */
#ifndef IMHOTEP_BOOT_BOOT_H
#define IMHOTEP_BOOT_BOOT_H

#include <stdint.h>

#include "state/state.h"

typedef struct boot_host {
    void *ctx;
    /* `size` bytes outside the state for the original's allocation made at `site` (the return
     * address of its call to operator new / malloc: an address of the reference binary, so that
     * tests can match it to the original's block); the block reads as zero (the original's heap is
     * not zeroed, but the boot reads no block before writing it) and is readable and writable through
     * state_ptr. Returns its address, 0 when there is no memory (the original's allocation failure) */
    uint32_t (*alloc)(void *ctx, game_state *st, uint32_t size, uint32_t site);
    /* a block of alloc released (operator delete / free): nothing is reused by boot_arena */
    void (*release)(void *ctx, game_state *st, uint32_t address);
    /* DirectDraw (display boots): a surface of w x h 16-bit pixels, rows of 2 * w bytes (the
     * original's surfaces are system-memory surfaces whose memory does not move: the game keeps
     * its address), `primary` 1 for the screen itself; returns the pixels' address (mapped, writable),
     * 0 on failure. NULL: the surfaces come from alloc */
    uint32_t (*surface)(void *ctx, game_state *st, int32_t w, int32_t h, int32_t primary);
    /* the window system and DirectDraw (display boots): an opaque non-zero handle that the game keeps
     * in its state (the window, a cursor of the exe's resources by id, the DirectDraw object, a
     * surface's object, after its pixels from `surface`); 0 when it cannot be made. NULL: handles 1 */
    uint32_t (*handle)(void *ctx, game_state *st, int32_t kind, int32_t id);
    /* the current directory (GetCurrentDirectoryA, read by the CRT _getcwd); NULL: "C:\\" */
    const char *(*cwd)(void *ctx);
    /* GDI: the size in pixels of `n` characters of `text` in the window's font (GetTextExtentPointA, the
     * loading screen 0x004CF7B0); 0 when unknown. NULL: 8 * n by 16 (the oracle's answer) */
    int (*text_extent)(void *ctx, const char *text, int32_t n, int32_t *w, int32_t *h);
} boot_host;

/* handle kinds: the window, a cursor (id: its resource), DirectDraw, a surface (id 1: the primary one),
 * the sound driver (Miles' AIL_waveOutOpen), a sample handle of the driver (id: the voice) */
enum { BOOT_WINDOW = 1, BOOT_CURSOR, BOOT_DIRECTDRAW, BOOT_SURFACE, BOOT_SOUND_DRIVER, BOOT_SOUND_SAMPLE };

/* The host's display at the boot: the screen's size (GetSystemMetrics before the game sets its mode)
 * and its 16-bit pixel format 555 or 565. width 0: no display (the oracle's default boot: DirectDraw
 * is not set up). The display mode the game sets is its own (the resolution 0x00E38E68 and the
 * fullscreen option 0x00E38E5C read from Pharaoh.inf by the settings, core/boot/settings.c): a screen
 * of another size gets WM_DISPLAYCHANGE when the mode is set (core/boot/display.c). The oracle's display
 * boots (tools/oracle/game.py Game(display=...)) have a screen of the mode's size and a Pharaoh.inf
 * with that resolution. */
typedef struct boot_display {
    int32_t width, height, format;
    /* GetSystemMetrics(SM_CXSCREEN) as the settings read it (0x004148D9, before the display mode is
     * set): 0 means the display's width, or 1024 without a display (tools/oracle/emu.py and game.py
     * answer the same) */
    int32_t screen_width;
} boot_display;

/* The boot's steps in order (WinMain's, as tools/oracle/game.py Game.boot_steps lists them for the
 * same display and sound): their count, and the name of step k. */
int boot_step_count(const boot_display *display, int sound);
const char *boot_step_name(const boot_display *display, int sound, int step);

/* Run step k of the boot (0 .. boot_step_count - 1). Returns 0, or -1 when the original would stop
 * there (a file missing: WinMain logs the error and returns) or a function it needs is not ported
 * (st->unported). */
int boot_step(game_state *st, const boot_host *host, const boot_display *display, int sound, int step);

/* The whole boot: every step in order. 0, or -1 (see boot_step). */
int game_boot(game_state *st, const boot_host *host, const boot_display *display, int sound);

/* ---- a simple host: one region of memory outside the state, mapped at `base` ---- */
typedef struct boot_arena {
    uint32_t base, used, cap;
    uint8_t *mem; /* cap bytes, zeroed, owned by the caller */
} boot_arena;

#define BOOT_ARENA_BASE 0x68000000u /* the oracle's other heap (tools/oracle/emu.py OTHER_HEAP_BASE) */

/* Map the arena into the state (state_map_external, writable through state_ptr). 0, or -1 when the
 * state's mapping table is full. */
int boot_arena_map(boot_arena *arena, game_state *st);
/* `size` bytes out of the arena (16-byte steps, never reused); 0 when it is full */
uint32_t boot_arena_alloc(boot_arena *arena, uint32_t size);
/* a boot_host over the arena (alloc and surfaces from it, release does nothing) */
boot_host boot_arena_host(boot_arena *arena);

#endif
