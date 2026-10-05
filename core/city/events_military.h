/* Imhotep - the requests for troops and the empire's armies (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_EVENTS_MILITARY_H
#define IMHOTEP_CITY_EVENTS_MILITARY_H

#include <stdint.h>

#include "state/state.h"

int32_t army_travel_months(game_state *st, uint32_t army);                /* @bin 0x004477D0 */
int32_t army_of_city(game_state *st, int32_t city, uint8_t *ours);        /* @bin 0x00447430 */
int32_t event_army_months(game_state *st, uint32_t e);                    /* @bin 0x0044C110 */
int32_t event_troops_months_left(game_state *st, uint32_t e);             /* @bin 0x0044C9A0 */
int32_t event_troops_sent(game_state *st, int32_t ev);                    /* @bin 0x00446260 */
void event_troops_request(game_state *st, uint32_t e);                    /* @bin 0x0044C9E0 */

int32_t ship_strength(game_state *st, int32_t f);                                      /* @bin 0x004B8330 */
int32_t company_strength(game_state *st, int32_t ev, int32_t *ships, int32_t *companies); /* @bin 0x004B8360 */
int32_t formation_ship(game_state *st, int32_t k);                                     /* @bin 0x004B8460 */
void companies_losses(game_state *st, int32_t pct, int32_t ev);                        /* @bin 0x004B8890 */
void companies_return(game_state *st, int32_t ev);                                     /* @bin 0x004B8690 */

#endif
