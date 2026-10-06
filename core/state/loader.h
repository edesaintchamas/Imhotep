/* Imhotep - the saved-game loader as the original's entry points: campaign records of mission1.pak
 * (0x004D58B0), saved games (0x004D5100, 0x004D51B0), the loader 0x004D41E0 between them.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Files are read through the platform (platform_file_read, core/sim/platform.h) by the path the
 * game builds; the original's CRT sequences (_open, _lseek, _read, _close) become offset reads.
 * Checks: tests/differential/oracle_function_check.py (loader entries),
 * tests/differential/oracle_newgame_check.py (campaign starts), oracle_state_check.py (saves). */
#ifndef IMHOTEP_STATE_LOADER_H
#define IMHOTEP_STATE_LOADER_H

#include <stddef.h>
#include <stdint.h>

#include "io/save_load.h"
#include "state/state.h"

/* @bin 0x004D41E0 the loader on the bytes of a saved game or campaign record (the chunks, the
 * route rebuild, the version upgrades, the clean-ups and restores; save.md §2). image_size NULL:
 * the image table of the state (0x010D6A0C, 64-byte entries), as the original reads it. Returns 0,
 * or -1 when the bytes are not a saved game (the original returns from the chunk loop then). */
int loader_read(game_state *st, const uint8_t *data, size_t len,
                int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height), void *image_ctx,
                save_load_stats *stats);

/* @bin 0x004CE480 read `size` bytes of a game file from `offset` into buf (a buffer of the caller):
 * the number of bytes read, or 0 when the file cannot be read (the original then tries the CD
 * paths and asks for the CD: the platform's) */
int32_t loader_file_read_at(game_state *st, const char *path, uint8_t *buf, uint32_t size, uint32_t offset);

/* @bin 0x004D58B0 load campaign record [0x00E38EA8] of the archive 0x005E1B98 + 20 * archive
 * ("mission1.pak"); unless no_start, the floodplain settings copy 0x004BD360 and the scenario start
 * 0x004D52A0. Returns 1, or 0 when there is no such record. */
int32_t loader_campaign_record(game_state *st, int32_t archive, int32_t no_start);

/* @bin 0x004D5100 load the saved game at `path` (a string in the state). Returns 0, or 1 when the
 * file does not open. */
int32_t loader_load_save(game_state *st, uint32_t path);
/* @bin 0x004D51B0 the same, keeping the difficulty and rescaling the treasury to it (the replay of a
 * Pharaoh campaign mission) */
int32_t loader_load_save_keep_difficulty(game_state *st, uint32_t path);

/* 0x004D5100 (keep_difficulty 0) / 0x004D51B0 (1) on a path that is not in the state (callers that
 * build it in their own stack frame: 0x00418510); NULL: the file does not open (returns 1) */
int32_t loader_load_save_path(game_state *st, const char *path, int keep_difficulty);

/* The body of 0x004D5100 / 0x004D51B0 after the file opened, on its bytes: the loader, then the
 * scenario start and the post-load rebuilds. keep_difficulty: 0x004D51B0. Returns 0, or -1 when the
 * bytes are not a saved game. */
int loader_load_save_bytes(game_state *st, const uint8_t *data, size_t len, int keep_difficulty,
                           int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height),
                           void *image_ctx, save_load_stats *stats);

/* @bin 0x004BD360 (this = 0x00D38360) floodplain settings: +0x0C..+0x14 := +0x00..+0x08,
 * +0x1C := +0x08, +0x18 := 0 */
void floodplain_settings_copy(game_state *st);

#endif
