/* Imhotep - the daily building update: every building spawns its walkers (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_BUILDING_UPDATE_H
#define IMHOTEP_BUILDING_BUILDING_UPDATE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00462180 day step 0x1F: for every building in use, by type, the walkers it sends out */
void building_update_all(game_state *st);

/* @bin 0x0045B880 the two goods building `id` (types 0x47 / 0xBF use list 0x00F53DDC, others list
 * 0x00F53E70) is lowest on, among those with a destination (0x0045BB50): best / second good,
 * their fill ratios (in: 100 / 100) and destinations */
void building_lowest_goods(game_state *st, int32_t id, int32_t *good1, int32_t *good2, int32_t *ratio1,
                           int32_t *ratio2, int32_t *dest1, int32_t *dest2);

/* @bin 0x004611D0 from two fill ratios and a destination count: three flags */
void building_ratio_flags(int32_t ratio1, int32_t ratio2, int32_t n, uint8_t *out1, uint8_t *out2, int32_t *out3);

/* @bin 0x00460F70 building `id`'s pending request (+0xCB, queue 0x009783C0): 1 and *out */
int building_pending_request(game_state *st, int32_t id, int32_t *out);

/* @bin 0x00460FB0 a figure type (0x19, 0x4D, 0x4E) still missing among the buildings of types
 * 0x4C, 0xB5, 0xB6: 1 and *out */
int building_missing_worker(game_state *st, int32_t *out);

#endif
