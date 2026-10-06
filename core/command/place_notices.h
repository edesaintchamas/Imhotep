/* Imhotep - the advice notices after a construction (package place), 0x004E2B10 .. 0x004E5070.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_PLACE_NOTICES_H
#define IMHOTEP_COMMAND_PLACE_NOTICES_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004E2B10 */
void notice_access_4e2b10(game_state *st, int32_t type, int32_t x, int32_t y, int32_t size);
/* @bin 0x004E2D30 */
void notice_workers_4e2d30(game_state *st, int32_t type);
/* @bin 0x004E2E20 */
void notice_4e2e20(game_state *st, int32_t type);
/* @bin 0x004E3100 */
void notice_4e3100(game_state *st, int32_t type);
/* @bin 0x004E3140 */
void notice_4e3140(game_state *st, int32_t type);
/* @bin 0x004E3180 */
void notice_4e3180(game_state *st, int32_t type);
/* @bin 0x004E31C0 */
void notice_4e31c0(game_state *st, int32_t type);
/* @bin 0x004E3200 */
void notice_4e3200(game_state *st, int32_t type);
/* @bin 0x004E3240 */
void notice_4e3240(game_state *st, int32_t type);
/* @bin 0x004E3280 */
void notice_4e3280(game_state *st, int32_t type, int32_t x, int32_t y, int32_t size);
/* @bin 0x004E32E0 */
void notice_4e32e0(game_state *st, int32_t type, int32_t x, int32_t y);
/* @bin 0x004E3330 */
void notice_4e3330(game_state *st, int32_t type);
/* @bin 0x004E33A0 */
void notice_4e33a0(game_state *st, int32_t type);
/* @bin 0x004E3460 */
void notice_4e3460(game_state *st, int32_t type);
/* @bin 0x004E3520 */
void notice_4e3520(game_state *st, int32_t type);
/* @bin 0x004E35E0 */
void notice_4e35e0(game_state *st, int32_t type);
/* @bin 0x004E36A0 */
void notice_4e36a0(game_state *st, int32_t type);
/* @bin 0x004E39D0 */
void notice_4e39d0(game_state *st, int32_t type);
/* @bin 0x004E3CF0 */
void notice_4e3cf0(game_state *st, int32_t type);
/* @bin 0x004E4490 */
void notice_4e4490(game_state *st, int32_t type);
/* @bin 0x004E4E10 */
void notice_4e4e10(game_state *st, int32_t type);
/* @bin 0x004E3760 */
void notice_4e3760(game_state *st, int32_t type);
/* @bin 0x004E3AA0 */
void notice_4e3aa0(game_state *st, int32_t type);
/* @bin 0x004E47D0 */
void notice_4e47d0(game_state *st, int32_t type);
/* @bin 0x004E3DA0 */
void notice_4e3da0(game_state *st, int32_t type);
/* @bin 0x004E4120 */
void notice_4e4120(game_state *st, int32_t type);
/* @bin 0x004E42D0 */
void notice_4e42d0(game_state *st, int32_t type);
/* @bin 0x004E3F60 */
void notice_4e3f60(game_state *st, int32_t type);
/* @bin 0x004E4550 */
void notice_4e4550(game_state *st, int32_t type);
/* @bin 0x004E4ED0 */
void notice_4e4ed0(game_state *st, int32_t type);
/* @bin 0x004E5070 */
void notice_4e5070(game_state *st, int32_t type);

#endif
