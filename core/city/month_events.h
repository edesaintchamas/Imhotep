/* Imhotep - the scenario events of the month (the list 0x00785D70).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_EVENTS_H
#define IMHOTEP_CITY_MONTH_EVENTS_H

#include <stdint.h>

#include "state/state.h"

uint32_t event_at(game_state *st, int32_t n);                         /* @bin 0x0044C0D0 */
int32_t event_due(game_state *st, uint32_t e, int8_t force);           /* @bin 0x0044D680 */
int32_t event_link_active(game_state *st, uint32_t e, int32_t n);      /* @bin 0x004496D0 */
int32_t event_months_left(game_state *st, uint32_t e);                 /* @bin 0x0044D750 */
int32_t empire_city_index(game_state *st, uint32_t v);                 /* @bin 0x004449D0 */
int32_t event_message_kind(game_state *st, uint32_t e);                /* @bin 0x0044F660 */
void event_fill_block(game_state *st, uint32_t e, uint8_t *block);     /* @bin 0x0044E6F0 */
void event_0x44db20(game_state *st, uint32_t e);                       /* @bin 0x0044DB20 */
void event_0x44df90(game_state *st, uint32_t e);                       /* @bin 0x0044DF90 */
void event_range_pick(game_state *st, uint32_t p);                    /* @bin 0x004487F0 */
void event_reset(game_state *st, uint32_t e);                          /* @bin 0x00448A90 */
/* @bin 0x0044C660 (thiscall) the record's month step, by its type */
void event_month_step(game_state *st, uint32_t e);

#endif
