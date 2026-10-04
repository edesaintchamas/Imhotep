/* Imhotep - access to the reference game files (file system side, not core/).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PLATFORM_GAME_FILES_H
#define IMHOTEP_PLATFORM_GAME_FILES_H

#include <stddef.h>
#include <stdint.h>

#include "graphics/image_groups.h"

/* $IMHOTEP_GAME_DIR, or local/game/app */
const char *game_dir(void);

/* Whole file in a malloc'd buffer (NULL on error). */
uint8_t *read_file(const char *path, size_t *len);

/* Case-insensitive lookup of <dir>/<stem><ext>; 0 and the path in `out` on success. */
int find_file(const char *dir, const char *stem, const char *ext, char *out, size_t out_len);

/* Read the .sg3 headers of the startup slots from <game>/Data and build the image numbering. */
int load_image_groups(const char *game, image_groups *groups);

/* Width and height of every image loaded at startup, by global id (entry +20, +22 of the .sg3
 * files; numbering of `groups`). count = the image running total (ids 1 .. count - 1). */
typedef struct {
    int32_t count;
    int16_t *width, *height;
} image_sizes;

int load_image_sizes(const char *game, const image_groups *groups, image_sizes *sizes);
void free_image_sizes(image_sizes *sizes);
/* Callback form for core/io/save_load.h: 0 and the size, or -1 for an unknown id. */
int image_sizes_lookup(void *ctx, int32_t id, int16_t *width, int16_t *height);

#endif
