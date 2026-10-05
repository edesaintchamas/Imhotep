/* Imhotep - monument parts taking a worker figure (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_MONUMENT_TAKE_H
#define IMHOTEP_BUILDING_MONUMENT_TAKE_H

#include <stdint.h>

#include "state/state.h"

int32_t monument_worker_busy_5477e0(game_state *st, int32_t id); /* @bin 0x005477E0 */
int32_t monument_worker_busy_401b40(game_state *st, int32_t id); /* @bin 0x00401B40 */
int32_t monument_needs_worker(game_state *st, int32_t id, int32_t f); /* @bin 0x00420C40 */
int32_t monument_take_e6(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t take, int32_t extra); /* @bin 0x00401D40 */
int32_t monument_take_547c10(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t take); /* @bin 0x00547C10 */
int32_t sun_temple_needs_worker(game_state *st, int32_t id, int32_t f); /* @bin 0x00420430 */
int32_t sun_temple_take(game_state *st, int32_t id, int32_t f, int32_t take); /* @bin 0x005421D0 */

#endif
