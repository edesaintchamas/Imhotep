/* Imhotep - load a saved game into the game state (Pharaoh.exe 0x004D5100), standalone: the bytes
 * of the file are given, and the game files come from a callback. The registered port of
 * 0x004D5100 (core/state/loader.c) reads the file through the platform; both share the loader.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Check: tests/differential/oracle_state_check.py (the state after the original's load). */
#ifndef IMHOTEP_STATE_LOAD_GAME_H
#define IMHOTEP_STATE_LOAD_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "io/save_load.h"
#include "state/state.h"

/* What the game reads besides its memory: files of the installation (model texts, ...). */
typedef struct {
    /* contents of a game file (name as the game spells it) or NULL; the buffer stays owned by the
     * caller */
    const uint8_t *(*file)(void *ctx, const char *name, size_t *len);
    void *file_ctx;
    /* global image id -> size (save_load_env), for the route endpoints; NULL: the image table of
     * the state, as the original */
    int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height);
    void *image_ctx;
} game_env;

/* @bin 0x004D5100 load a saved game (bytes of a .sav or a mission1.pak record). The game files are
 * served from env->file unless the state has a platform (st->platform); calls to original
 * functions that have no port leave the state unchanged unless the state has a test environment
 * (st->env). Returns 0, or -1 when the bytes are not a saved game. */
int game_load_save(game_state *st, const uint8_t *data, size_t len, const game_env *env, save_load_stats *stats);

#endif
