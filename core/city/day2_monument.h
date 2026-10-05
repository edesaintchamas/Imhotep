/* Imhotep - monument progress summaries (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_MONUMENT_H
#define IMHOTEP_CITY_DAY2_MONUMENT_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00542110 the worker figure of a part of a 0xD5 monument is still at work (else +0x5C
 * is cleared) */
int32_t monument_d5_worker_busy(game_state *st, int32_t b);
/* @bin 0x004204A0 *out = the entries of table 0x00596084 of kind `kind` whose object is live */
void monument_count_kind(game_state *st, int32_t kind, int32_t *out);
/* @bin 0x00542AD0 the state of the 0xD5 monument whose part b is (out: 0xE i32 or NULL);
 * returns its percent done */
int32_t monument_d5_status(game_state *st, int32_t b, int32_t *out);
/* @bin 0x004DF040 the same for a 0xDE monument (out: 0xC i32 or NULL) */
int32_t monument_de_status(game_state *st, int32_t b, int32_t *out);
/* @bin 0x004F57B0 the quantity of good `good` the three monument slots still need */
int32_t monument_goods_needed(game_state *st, int32_t good);

#endif
