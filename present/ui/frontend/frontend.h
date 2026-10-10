/* Imhotep - the front end: main menu, families, campaign choice, scenario list, options and file
 * dialogs (roadmap phase 7.6, package "frontend").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of the original's front-end windows (docs/presentation.md 3.2, docs/newgame.md 2): their
 * full-redraw functions (from the switch 0x004D71C0), per-frame functions (0x004D75D0) and the
 * button callbacks of their tables (input 0x004D8310). The dispatchers are package "wincore"'s, the
 * button tables' hit tests (0x004261D0, 0x00425990) package "sidebar"'s, the image draws, text and
 * panels package "blit"'s; the campaign flow (families, progress file, mission list, period choice)
 * is core/newgame/flow.c, flow2.c. Every call to another package goes through GAME_CALL. */
#ifndef IMHOTEP_PRESENT_UI_FRONTEND_H
#define IMHOTEP_PRESENT_UI_FRONTEND_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* globals (docs/presentation.md 2.1, 3.2, 5.2) */

void fe_button_draw(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t width, int32_t lit);

/* @bin 0x00562BB5 CRT remove of a player file (the platform's file_delete): 0, or -1 (errno set) */
int32_t crt_remove(game_state *st, const char *path);
/* the victory scoring (highscore.c) */
int32_t victory_score(game_state *st, int32_t months);  /* @bin 0x0041C380 */
int32_t victory_monuments_draw(game_state *st);         /* @bin 0x004192B0 */
int32_t mission_victory_text(game_state *st);           /* @bin 0x004181A0 */
/* @bin 0x00448370 a text with its keywords expanded into a host buffer (family.c) */
int32_t fe_text_expand(game_state *st, uint32_t text, char *out, int32_t size);

/* @bin 0x005672C8 CRT _dosmaperr: errno and _doserrno from a Win32 error */
void crt_dosmaperr(game_state *st, uint32_t error);

#endif
