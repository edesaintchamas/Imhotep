/* Imhotep - monuments: which monument a worker figure goes to (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_MONUMENT_H
#define IMHOTEP_BUILDING_MONUMENT_H

#include <stdint.h>

#include "state/state.h"

int32_t monument_take_worker(game_state *st, int32_t x, int32_t y, int32_t f);       /* @bin 0x004BC140 */
int32_t pyramid_take_worker(game_state *st, int32_t x, int32_t y, int32_t f);        /* @bin 0x004F01E0 */
int32_t pyramid_part_ready(game_state *st, int32_t id);                              /* @bin 0x004EF100 */
int32_t pyramid_worker_busy(game_state *st, int32_t id);                             /* @bin 0x004EEA10 */
int32_t pyramid_send_figure(game_state *st, int32_t f, int32_t id);                  /* @bin 0x004F3000 */
int32_t monument_0x00420350(game_state *st, int32_t x, int32_t y, int32_t f);        /* @bin 0x00420350 */
int32_t monument_stage_count(game_state *st, int32_t id);                            /* @bin 0x004E5A30 */
int32_t monument_0x004E5C00(game_state *st, int32_t x, int32_t y, int32_t f);        /* @bin 0x004E5C00 */
int32_t monument_0x00401ED0(game_state *st, uint32_t obj, int32_t x, int32_t y, int32_t f,
                            int32_t extra);                                         /* @bin 0x00401ED0 */

int32_t mastaba_worker_busy(game_state *st, int32_t id);                             /* @bin 0x004E0530 */
int32_t mastaba_take(game_state *st, int32_t id, int32_t f, int32_t commit);         /* @bin 0x004E05F0 */

#endif
