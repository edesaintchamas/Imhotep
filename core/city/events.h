/* Imhotep - scenario events: requests and invasions of the month, starting linked events, the
 * message chosen for an event (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_EVENTS_H
#define IMHOTEP_CITY_EVENTS_H

#include <stdint.h>

#include "state/state.h"

int32_t event_before_date(game_state *st, uint32_t e);                 /* @bin 0x0044D6D0 */
int32_t event_pending(game_state *st, uint32_t e);                     /* @bin 0x0044D700 */
int32_t event_linked_flag(game_state *st, uint32_t e);                 /* @bin 0x0044EF60 */
int32_t event_type_linkable(game_state *st, uint32_t e);               /* @bin 0x00449390 */
int32_t event_good_allowed(game_state *st, uint32_t e, int32_t g);     /* @bin 0x00449550 */
int32_t event_range_count(game_state *st, uint32_t p);                 /* @bin 0x0044C960 */
int32_t events_started_later(game_state *st, uint32_t self, int32_t n); /* @bin 0x0044C430 */
void event_init(game_state *st, uint32_t e);                           /* @bin 0x004489C0 */
uint32_t events_alloc(game_state *st, uint32_t self, int8_t quiet);    /* @bin 0x00448D10 */
/* @bin 0x0044C490 (thiscall on the record `e`) start a copy of event n, linked to e */
void event_start(game_state *st, uint32_t e, int32_t n, int32_t kind);
/* @bin 0x00451540 (thiscall) the month step of a request / invasion record (type 2) */
void event_request_month(game_state *st, uint32_t e);
/* @bin 0x0044FD30 (thiscall) the message of the event's outcome `slot` (0..3), or -1 */
int32_t event_outcome_message(game_state *st, uint32_t e, int32_t slot);
/* @bin 0x0044CDC0 (thiscall) the type-1 handler: a request of goods */
void event_request(game_state *st, uint32_t e);
int32_t event_message_id(int32_t kind, int32_t level);                 /* @bin 0x0044FEC0 */

#endif
