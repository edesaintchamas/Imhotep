/* Imhotep - the ratings of the month (0x0046E180 and its callees).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_RATINGS_H
#define IMHOTEP_CITY_MONTH_RATINGS_H

#include <stdint.h>

#include "state/state.h"

void rating_lowest(game_state *st);       /* @bin 0x00426A00 */
void rating_bonus(game_state *st);        /* @bin 0x00426A80 */
void rating_0x4269b0(game_state *st);     /* @bin 0x004269B0 (and 0x0046E1C0) city +0x42CC, +0x4434 */
void rating_0x46e7c0(game_state *st);     /* @bin 0x0046E7C0 city +0x46A8 */
void rating_0x46e930(game_state *st);     /* @bin 0x0046E930 */
void rating_0x46e880(game_state *st);     /* @bin 0x0046E880 city +0x4444 */
void rating_0x46ec70(game_state *st);     /* @bin 0x0046EC70 city +0x4580, +0x4440 */
void rating_0x46e970(game_state *st, int32_t month_end); /* @bin 0x0046E970 city +0x42D8 */
/* @bin 0x00451B10 does the city have a working 0xB7 or a 0xD2 building (see the port) */
int32_t city_0x451b10(game_state *st);
void rating_0x46e410(game_state *st);     /* @bin 0x0046E410 city +0x42E4, +0x4438 */
void rating_0x46e1e0(game_state *st);     /* @bin 0x0046E1E0 city +0x42D0 */
/* @bin 0x004BD590 (thiscall on 0x00D38360) */
int32_t flood_0x4bd590(game_state *st, uint32_t self);

#endif
