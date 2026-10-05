/* Imhotep - the image slot table of the graphics object 0x00F55BB0 (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_GRAPHICS_LAST_GRAPHICS_H
#define IMHOTEP_GRAPHICS_LAST_GRAPHICS_H

#include <stdint.h>

#include "state/state.h"

int32_t crt_strcmpi(game_state *st, uint32_t a, uint32_t b);                              /* @bin 0x00562370 */
int32_t graphics_load_slot(game_state *st, uint32_t self, uint32_t name, int32_t slot); /* @bin 0x0053DD90 */

#endif
