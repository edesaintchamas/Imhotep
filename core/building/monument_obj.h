/* Imhotep - monument handler object methods and stage redraws (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_MONUMENT_OBJ_H
#define IMHOTEP_BUILDING_MONUMENT_OBJ_H

#include <stdint.h>

#include "state/state.h"

/* the vtable method +0x18 (redraw) of a handler object, natively when it is ported */
void monument_method_redraw(game_state *st, uint32_t obj, int32_t id);

void monument_e3_stage_up(game_state *st, uint32_t obj, int32_t id); /* @bin 0x00417640 */
void monument_e4_stage_up(game_state *st, uint32_t obj, int32_t id); /* @bin 0x004E6BD0 */
void monument_e6_step(game_state *st, uint32_t obj, int32_t id);     /* @bin 0x00401BE0 */

int32_t sun_temple_image(game_state *st, int32_t id, int32_t *out); /* @bin 0x005414B0 */
void sun_temple_redraw(game_state *st, int32_t id);                 /* @bin 0x00541920 */
void sun_temple_stage_up(game_state *st, int32_t id);               /* @bin 0x00542DD0 */
void sphinx_stage_up(game_state *st);                               /* @bin 0x0053D820 */
void monument_part_redraw(game_state *st, int32_t id);                            /* @bin 0x004DFBB0 */
void obelisk_redraw(game_state *st, int32_t id, int32_t x, int32_t y);            /* @bin 0x004E5840 */

#endif
