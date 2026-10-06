/* Imhotep - the campaign flow, continued (package flow2): the family's progress file read, the family
 * list and the new family, the game's file lists (FindFirstFileA through the platform), the period
 * choice and the mission choice (docs/newgame.md section 2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A file list (0x0041C6D0) is an object of 7 dwords: +0 its vtable, +4 the name offsets (u32[],
 * new), +8 their count, +0xC their capacity, +0x10 the names (char[], new), +0x14 their capacity,
 * +0x18 the bytes used. The lists of the state: the scenario list 0x00704C00, the family list
 * 0x00EADFA8; the main menu keeps one on its stack. The buffers are allocated in the state's heap
 * (state_alloc): the oracle allocates them in its simulation heap too (tools/oracle/game.py
 * SIM_ALLOCATORS). */
#ifndef IMHOTEP_NEWGAME_FLOW2_H
#define IMHOTEP_NEWGAME_FLOW2_H

#include <stdint.h>

#include "state/state.h"

#define FAMILY_LIST 0x00EADFA8u

/* @bin 0x0041B680 (thiscall on the campaign table) read the progress file `path` (a state string):
 * the table's mission records; 1 when it was read */
int32_t campaign_progress_read(game_state *st, uint32_t self, uint32_t path);

/* @bin 0x0041C740 (thiscall on a file list in the state) the files matching `pattern` (a state
 * string) added to the list (keep's low byte 0: emptied first); 1 when they all fitted */
int32_t file_list_fill(game_state *st, uint32_t self, uint32_t pattern, int32_t keep);
/* @bin 0x0041CA10 (thiscall) the names sorted with the comparator `cmp` (a game function: _stricmp) */
void file_list_sort(game_state *st, uint32_t self, uint32_t cmp);
/* the number of files matching `pattern` (a C string), as the main menu counts them with a file list on
 * its stack (0x004CB711: 0x0041C6D0, 0x0041C740, 0x0041C710) */
int32_t file_list_count(game_state *st, const char *pattern);

/* @bin 0x0041ACD0 the blanks (bytes <= 0x20) around the state string at `s` removed */
void name_trim(game_state *st, uint32_t s);
/* @bin 0x00516B50 the family list (0x00EADFA8) from `pattern` (a state string), extensions removed,
 * sorted */
void family_list_load(game_state *st, uint32_t pattern);
/* @bin 0x00516AB0 the family of the clicked row (0x00D3B198 + scroll 0x00EADFC8) chosen: its name, its
 * progress file read */
void family_select(game_state *st);
/* @bin 0x00516BB0 the family list screen (0x2F), or the new family screen when there is none */
void family_list_open(game_state *st);
/* @bin 0x00516C10 the new family screen (0x36): the name typed in */
void family_name_open(game_state *st);
/* @bin 0x00516C80 the typed name (0x00E38E74) becomes a new family: its progress file, its directory */
void family_create(game_state *st);
/* @bin 0x00516990 on to the main menu with a family, else a message */
void family_done(game_state *st);

/* @bin 0x00417DD0 the first mission of `period` in the campaign table: mission, step, period */
void campaign_period_first(game_state *st, int32_t period);
/* @bin 0x0041BB70 the period chosen (0x007027AC) starts: its video, its first mission's briefing */
void campaign_period_start(game_state *st);
/* @bin 0x00418220 the "continue" button of the new family, mission choice and briefing screens */
void flow_continue(game_state *st);

/* @bin 0x004CC500 text box `box` becomes the one edited (its length from its text) */
void text_box_focus(game_state *st, int32_t box);
void text_box_cursor_start(game_state *st); /* @bin 0x004CC3C0 */
void text_box_cursor_end(game_state *st);   /* @bin 0x004CC3E0 */
/* @bin 0x0053B690 the sound stream of `channel` faded out and closed */
void music_fade_out(game_state *st, int32_t channel);

#endif
