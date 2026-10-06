/* Imhotep - starting a new city: the new-game entry 0x005405B0 and its start-only steps
 * (package start, docs/newgame.md section 3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_START_H
#define IMHOTEP_NEWGAME_START_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x005405B0 (name, flag) start a city: the campaign record [0x00E38EA8] of mission1.pak
 * (0x00E38EBC == 0 or flag != 0) or the custom scenario maps\<name>.map; name is the address of the
 * map name in the state (always 0x00E37078 in the game) */
void newgame_start(game_state *st, uint32_t name, int32_t flag);

/* @bin 0x005409D0 does maps\<name>.map exist: the path (a buffer of at least strlen + 5 bytes)
 * gets its extension replaced by ".map" for the check and removed after it (as the original
 * does to its caller's buffer); 1 if the file opens */
int32_t newgame_map_file_exists(game_state *st, char *path);

void newgame_map_points_reset(game_state *st);   /* @bin 0x004DE140 */
void newgame_messages_reset(game_state *st);     /* @bin 0x004E1770 */
void newgame_message_timer_reset(game_state *st); /* @bin 0x004E1830 */
void newgame_mission_buildings(game_state *st);  /* @bin 0x00418BF0 */
void newgame_profiler_reset(game_state *st);     /* @bin 0x00489B40 */
/* @bin 0x0041B9D0 (thiscall on the campaign table) the index of the step that plays `mission`,
 * -1 if none */
int32_t campaign_step_of_mission(game_state *st, uint32_t self, int32_t mission);

/* the menu handlers that write the start inputs (docs/newgame.md section 2) */
void newgame_carry_over(game_state *st);   /* @bin 0x004188F0 */
void newgame_next_mission(game_state *st); /* @bin 0x00418640 */
void newgame_replay(game_state *st);       /* @bin 0x00418510 */
void newgame_list_start(game_state *st);   /* @bin 0x0041F5C0 */
void newgame_list_row(game_state *st);     /* @bin 0x0041F670 */

#endif
