/* Imhotep - the .sg3 image index reader of the graphics object (package sg3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_GRAPHICS_SG3_H
#define IMHOTEP_GRAPHICS_SG3_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0053DFF0 load <name>.sg3 into the slot record `rec` (thiscall), its entries at `entries`
 * (= first image id), its group records at `groups` (= first group); returns 1 when the .555 was
 * loaded too */
uint8_t sg3_load(game_state *st, uint32_t rec, uint32_t name, int32_t slot, uint32_t entries, int32_t first_image,
                 uint32_t groups, int32_t first_group);

/* @bin 0x004CDF20 / 0x004CDEB0 plain / RLE pixels 555 -> 565 in place, when the screen is 565 */
void sg3_pixels_555_to_565(game_state *st, uint32_t pixels, int32_t length);
void sg3_rle_555_to_565(game_state *st, uint32_t data, int32_t length);

#endif
