/* Imhotep - what the gods' actions do to the city (package rest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_REST_GODS_H
#define IMHOTEP_CITY_REST_GODS_H

#include "state/state.h"

int32_t pyramid_progress_4f5340(game_state *st);              /* @bin 0x004F5340 */
void pyramid_speedup_4f3aa0(game_state *st, int32_t k);       /* @bin 0x004F3AA0 */
int32_t storage_fill_4c3950(game_state *st);                  /* @bin 0x004C3950 */
int32_t workshops_fill_4c3ac0(game_state *st);                /* @bin 0x004C3AC0 */
int32_t buildings_destroy_4c3ed0(game_state *st);             /* @bin 0x004C3ED0 */
void storage_strike_4c4020(game_state *st, int32_t destroy);  /* @bin 0x004C4020 */
int32_t fort_destroy_4c41b0(game_state *st);                  /* @bin 0x004C41B0 */
void houses_stock_4c42b0(game_state *st);                     /* @bin 0x004C42B0 */
void houses_destroy_4c45e0(game_state *st);                   /* @bin 0x004C45E0 */
void workers_recalled_4b4710(game_state *st);                 /* @bin 0x004B4710 */

/* @bin 0x00562069 the CRT qsort on num 4-byte elements (the comparator gets the elements) */
void crt_qsort4(game_state *st, int32_t *base, uint32_t num, int (*cmp)(game_state *, int32_t, int32_t));

/* @bin 0x00562069 the same on num elements of `width` bytes at state address base, the comparator
 * (an original function) called through GAME_CALL with the elements' addresses */
void crt_qsort_state(game_state *st, uint32_t base, uint32_t num, uint32_t width, uint32_t cmp);

#endif
