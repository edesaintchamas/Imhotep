/* Imhotep - the campaign flow around the start of a city: the campaign table (campaign.txt), the
 * family's progress file, the briefing, the main menu and the scenario / mission list (package
 * flow, docs/newgame.md section 2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The campaign table is the object at 0x006FDD08 (`this` of the thiscall functions below): 100 steps
 * of 0x40 bytes, the step count at +0x1900, 100 mission names of 0x32 bytes at +0x1904 (count at
 * +0x2C8C), 0x4A9C bytes in all (what the progress file holds). Player files are read through
 * platform_file_read and written through platform_file_write (core/sim/platform.h). */
#ifndef IMHOTEP_NEWGAME_FLOW_H
#define IMHOTEP_NEWGAME_FLOW_H

#include <stdint.h>

#include "state/state.h"

#define CAMPAIGN_TABLE_SIZE 0x4A9Cu

/* @bin 0x00516A20 "save\" + player name + ".dat" into 0x00EADDA0: the family's progress file;
 * returns 0x00EADDA0 */
uint32_t campaign_progress_path(game_state *st);
/* @bin 0x005169C0 "save\" + player name into 0x00EADEA4: the family's directory; returns it */
uint32_t family_dir_path(game_state *st);
/* @bin 0x0041B8A0 (thiscall on the table) the whole table into the file at `path` (a string in the
 * state); 1 when it was written */
int32_t campaign_progress_write(game_state *st, uint32_t self, uint32_t path);

/* @bin 0x0041B920 (thiscall) the step after `step` (-1: from the start) in `period` that follows
 * the choice `choice` of `step`; -1 if none */
int32_t campaign_next_step(game_state *st, uint32_t self, int32_t period, int32_t step, int32_t choice);
/* @bin 0x0041BA20 (thiscall) is `period` open (periods >= 5 always; else a won step of it leads
 * nowhere further) */
int32_t campaign_period_done(game_state *st, uint32_t self, int32_t period);
/* @bin 0x0041BAB0 (thiscall) campaign_period_done for any of the periods 0..8 */
int32_t campaign_any_period_done(game_state *st, uint32_t self);
/* @bin 0x0041AE40 (thiscall) read the campaign file `path` (a string in the state: "campaign.txt")
 * into the table; 1 when every line was understood, 0 otherwise (and when the file does not open) */
int32_t campaign_load(game_state *st, uint32_t self, uint32_t path);

/* @bin 0x0041AA20 the timed fade into screen `next` (0x00E38DDA := 4) */
void fade_start(game_state *st, int32_t a, int32_t next, int32_t ms);
/* @bin 0x00418140 the briefing's text id */
int32_t briefing_text_id(game_state *st);
/* @bin 0x004181E0 open the mission briefing (through the fade) */
void briefing_open(game_state *st);
/* @bin 0x004CB6A0 the main menu (screen 0x30) and its button flags 0x00D3B188..0x00D3B18B */
void main_menu_open(game_state *st);

/* the scenario / mission list object 0x00704C00 (thiscall) */
int32_t list_row_kind(game_state *st, uint32_t self, int32_t row);    /* @bin 0x0041DFE0 */
uint32_t list_row_name(game_state *st, uint32_t self, int32_t row);   /* @bin 0x0041E010 */
int32_t list_row_mission(game_state *st, uint32_t self, int32_t row); /* @bin 0x0041DFC0 */
int32_t list_fill(game_state *st, uint32_t self, uint32_t pattern, int32_t keep); /* @bin 0x0041DF70 */
void list_load(game_state *st, uint32_t pattern);                      /* @bin 0x0041F810 */
void list_open(game_state *st);                                        /* @bin 0x0041F870 */

#endif
