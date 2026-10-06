/* Imhotep - the saved-game loader's steps after reading the chunks (Pharaoh.exe 0x004D41E0):
 * empire route rebuild, version upgrades, per-load passes, monument clean-ups, restores.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Rules: docs/formats/save.md §2, §4. Check: tests/differential/save_check.c.
 * @bin 0x004D41E0 steps after the chunk loop; @diff save_check (route rebuild, figure names) */
#ifndef IMHOTEP_IO_SAVE_LOAD_H
#define IMHOTEP_IO_SAVE_LOAD_H

#include <stdint.h>

#include "io/save_file.h"
#include "state/state.h"

/* What the game holds before the load and the loader reads (none of it is in the file). */
typedef struct {
    /* MSVC CRT rand() state (0x00604950), advanced by the monument clean-ups; in and out. The
     * game reseeds it from the clock (save.md §5), so it is an input of the load. */
    uint32_t crt_rand;
    /* 0x005E4EF4 (pyramid kind chosen at the previous scenario start, 0x004F4520): read by the
     * version < 146 step 0x004D3DA0. */
    int32_t previous_pyramid_kind;
    /* Global image id -> width, height (startup numbering), for the empire route endpoints
     * (0x004D4720). NULL: the endpoints stored in the file are kept. */
    int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height);
    void *image_ctx;
    int32_t image_count; /* image running total (0x0126517C) */
} save_load_env;

typedef struct {
    int32_t routes_rebuilt, routes_reset;
    int32_t route_segments_unproven; /* segments >= 2896 long: outside the proven integer range */
    int32_t endpoints_kept;          /* routes whose endpoints were kept (no image sizes) */
    int32_t rand_draws;              /* CRT rand() calls */
    int32_t dropped_writes;          /* writes the game makes to memory outside every chunk */
    int32_t upgrades;                /* version-upgrade steps applied */
} save_load_stats;

/* Apply the loader's steps to a save read by save_file_read, in the game's order. The result is
 * the state the game holds after loading (save.md §2), in s's chunks; writes outside every chunk
 * are dropped (counted). Returns 0. */
int save_file_load(save_file *s, save_load_env *env, save_load_stats *stats);

/* The same on the game state: the chunks read from the file are copied to their variables
 * (@bin 0x004D0E10), then the steps read and write the state, so writes outside the chunks land
 * where the game puts them, and what the file does not contain (chunks 176-180 and bytes +5..+7
 * of chunk 175 below version 154) keeps what the state held, as in the game. s is not updated.
 * Returns 0. */
int save_file_load_state(game_state *st, save_file *s, save_load_env *env, save_load_stats *stats);

/* @bin 0x004D4720 (this = an empire route of chunk 143) the route's point count and endpoints on
 * the game state (the points themselves, x87 display data, are not computed: docs/formats/state.md).
 * Returns 1, or 0 when the route has more points than estimated. */
int save_route_rebuild_state(game_state *gs, uint32_t route, save_load_env *env);

#endif
