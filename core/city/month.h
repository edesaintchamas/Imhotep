/* Imhotep - the month and year ends of the city (called by the day cascade 0x00452370).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_H
#define IMHOTEP_CITY_MONTH_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004C5120 the index (0..4) of the building's type in the two runs 0x3C..0x40 and
 * 0x41..0x45, else -1 */
int32_t building_type_level(game_state *st, int32_t id);
/* @bin 0x004E96C0 city +0x42D8 += delta, kept within 0..100 */
void city_add_0x42d8(game_state *st, int32_t delta);
/* @bin 0x00458F80 sums of the buildings' +0x1C by their +0x12 into the player's city record;
 * returns the total */
int32_t city_sum_building_1c(game_state *st, int32_t player);

void month_0x00455530(game_state *st);
void month_0x004E8630(game_state *st);
void month_0x004E1850(game_state *st);
void month_0x004EA740(game_state *st);
void month_0x0045A4D0(game_state *st);
void month_0x0045A500(game_state *st);
void month_0x0045A530(game_state *st);
void month_0x0045A5A0(game_state *st);
void month_0x004C5300(game_state *st);
void month_0x0045A450(game_state *st);
void month_0x0045A490(game_state *st);
void month_0x004448E0(game_state *st);
void month_0x0045A560(game_state *st);
void month_0x004474E0(game_state *st);

void month_0x004E9700(game_state *st);
void month_0x0045A3F0(game_state *st);
void month_0x00459E50(game_state *st);
void month_0x0045A2E0(game_state *st);
void month_0x0045A350(game_state *st);
void month_0x00452160(game_state *st);
void month_0x004B7960(game_state *st);
void month_0x004E8DB0(game_state *st);
void month_0x004BE640(game_state *st, uint32_t self);
void month_0x0046E180(game_state *st, int32_t arg);
void month_0x004464F0(game_state *st);
/* @bin 0x004B7B50 record id of 0x00BCB9C8 (0x90 bytes): +0x2C += delta, within 0..its maximum */
void formation_add_0x2c(game_state *st, int32_t id, int16_t delta);
/* @bin 0x004C52D0 building +0xA9 is 3 or value */
int32_t building_0xa9_is(game_state *st, int32_t id, uint32_t value);
/* @bin 0x0044BB90 the event list 0x00785D70: the next active record after `after` */
uint32_t events_next(game_state *st, uint32_t self, uint32_t after, int8_t create);
void events_0x0044BC20(game_state *st, uint32_t self);
/* @bin 0x0044BA90 the event list's month step */
void events_month(game_state *st, uint32_t self);
void month_0x00452200(game_state *st);
void month_0x004BC7A0(game_state *st);
void month_0x0045A7B0(game_state *st);
void month_0x0045A5D0(game_state *st);
void month_0x0045E420(game_state *st);
void month_0x004599D0(game_state *st);
void month_0x004EA000(game_state *st);
/* @bin 0x004E8E80 buildings' +0xC2 += delta (0..100) */
void buildings_add_0xc2(game_state *st, int8_t delta);
void month_0x004E7700(game_state *st);
/* @bin 0x004179F0 the menu flags 0x006FDC88 of the tutorial steps */
void tutorial_menu_flags(game_state *st);
/* @bin 0x00417AE0 a tutorial step is reached */
void tutorial_step(game_state *st, int32_t step);

#endif
