/* Imhotep - the game's files as the original sees them (build/imhotep).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The game builds Windows paths relative to its directory ("data\\Pharaoh_General.555",
 * "Save\\Family\\city.sav"). Here: '\\' and '/' separate, each part is looked up case-insensitively
 * (NTFS is case-insensitive), a drive prefix "X:\\" is dropped. Two roots: the player directory
 * (saves, the campaign progress, Pharaoh.inf as the game rewrites it; IMHOTEP_PLAYER_DIR, default
 * local/player) is searched first, then the game directory (IMHOTEP_GAME_DIR, read-only). Writes and
 * deletions go to the player directory only. Reads are of regular files only (as _open). */
#ifndef IMHOTEP_PLATFORM_FILES_H
#define IMHOTEP_PLATFORM_FILES_H

#include <stddef.h>
#include <stdint.h>

void files_init(const char *game, const char *player);
const char *files_player_dir(void);

/* The host path of a game path (an existing regular file, player directory first); 0, or -1 */
int files_resolve(const char *gpath, char *out, size_t size);

/* sim_platform callbacks (core/sim/platform.h) */
int32_t files_read(const char *gpath, uint32_t offset, uint8_t *buf, uint32_t size);
int32_t files_write(const char *gpath, const uint8_t *data, uint32_t size);
int32_t files_delete(const char *gpath);
int32_t files_list(const char *pattern, int32_t index, char *name, uint32_t size);

/* Windows wildcard match ('*', '?', case-insensitive; "*.*" also matches names without a dot) */
int files_match(const char *pattern, const char *name);

#endif
