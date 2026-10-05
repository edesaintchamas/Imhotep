/* Imhotep - monuments' footprints drawn again (package rest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_REST_MONUMENT_H
#define IMHOTEP_BUILDING_REST_MONUMENT_H

#include "state/state.h"

/* The object's image function (vtable +0x1C): returns the image, writes the flags to *out. */
int32_t monument_image(game_state *st, uint32_t self, int32_t b, int32_t *out);
int32_t monument_image_401800(game_state *st, uint32_t self, int32_t b, int32_t *out); /* @bin 0x00401800 */
int32_t monument_image_417310(game_state *st, uint32_t self, int32_t b, int32_t *out); /* @bin 0x00417310 */
int32_t monument_image_4e6a70(game_state *st, uint32_t self, int32_t b, int32_t *out); /* @bin 0x004E6A70 */
int32_t monument_image_40de80(game_state *st, uint32_t self, int32_t b, int32_t *out); /* @bin 0x0040DE80 */

void monument_redraw_4017a0(game_state *st, uint32_t self, int32_t b);   /* @bin 0x004017A0 */
void monument_redraw_4e69c0(game_state *st, uint32_t self, int32_t b);   /* @bin 0x004E69C0 */
void monument_redraw_40dca0(game_state *st, uint32_t self, int32_t b);   /* @bin 0x0040DCA0 */
void monument_redraw_5468d0(game_state *st, uint32_t self, int32_t b);   /* @bin 0x005468D0 */
int32_t monument_parts_free_547ba0(game_state *st, uint32_t self, int32_t b); /* @bin 0x00547BA0 */

#endif
