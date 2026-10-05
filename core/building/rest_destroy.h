/* Imhotep - what the destruction of a building releases or redraws (package rest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_REST_DESTROY_H
#define IMHOTEP_BUILDING_REST_DESTROY_H

#include "state/state.h"

void building_0x0046a2e0(game_state *st, int32_t b);                     /* @bin 0x0046A2E0 */
void city_slot_clear_0x004f4a80(game_state *st, int32_t b, int32_t i);   /* @bin 0x004F4A80 */
void building_0x004f4ae0(game_state *st, int32_t b);                     /* @bin 0x004F4AE0 */
void formation_clear_0x004b82e0(game_state *st, int32_t f);              /* @bin 0x004B82E0 */
void building_rubble_parts_0x004c04a0(game_state *st, int32_t b);        /* @bin 0x004C04A0 */
void building_area_clear_0x004c0920(game_state *st, int32_t b);          /* @bin 0x004C0920 */

int32_t monument_slot_type_4f52b0(game_state *st, int32_t i);           /* @bin 0x004F52B0 */
void monument_slot_images_4e5950(game_state *st, int32_t i);             /* @bin 0x004E5950 */
void storage_take_45b700(game_state *st, int32_t b, int32_t amount);    /* @bin 0x0045B700 */

#endif
