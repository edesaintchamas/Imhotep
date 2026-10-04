/* Imhotep - global image ids and image groups of the graphics families loaded at startup.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Formats: docs/formats/graphics.md, docs/formats/map.md §6. */
#ifndef IMHOTEP_GRAPHICS_IMAGE_GROUPS_H
#define IMHOTEP_GRAPHICS_IMAGE_GROUPS_H

#include <stddef.h>
#include <stdint.h>

#define IMAGE_SLOT_COUNT 30 /* @bin 0x00414420 30 startup slots */
#define IMAGE_GROUPS_MAX 2048
#define SG3_HEADER_BYTES 745 /* @bin 0x0053DFF0 header (80), group table (600), first bitmap name (65) */

/* Graphics family loaded into each slot at startup (file stem in Data/, case-insensitive). */
extern const char *const image_startup_slots[IMAGE_SLOT_COUNT];

typedef struct {
    int32_t slot_base[IMAGE_SLOT_COUNT];  /* global id of the first image of the slot */
    int32_t slot_count[IMAGE_SLOT_COUNT]; /* images in the slot */
    int32_t slot_skip[IMAGE_SLOT_COUNT];  /* system.bmp images skipped (0 or 200) */
    int32_t group_count;                  /* highest group number + 1 */
    int32_t group_first[IMAGE_GROUPS_MAX]; /* global id of the first image of each group */
} image_groups;

/* Build the numbering from the first SG3_HEADER_BYTES of each slot's .sg3 file.
 * Returns 0 on success, -1 if a header is too short or too many groups exist. */
int image_groups_build(image_groups *g, const uint8_t *const headers[IMAGE_SLOT_COUNT],
                       const size_t lengths[IMAGE_SLOT_COUNT]);

#endif
