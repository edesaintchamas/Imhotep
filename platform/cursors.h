/* Imhotep - the mouse cursors of build/imhotep, from the user's Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original loads 19 cursors at start-up (0x004CF960: LoadCursorA(hInstance, id) for the group
 * cursor resources 0x66, 0x88, 0x82, 0x93, 0x90, 0x91, 0x92, 0x8E, 0x96, 0x95, 0x94, 0x99, 0x98,
 * 0x9B, 0x9A, 0x9C, 0x9D, 0x9F, 0xA0, kept at 0x00E390D8, 0x00E390D4, ... 0x00E39090) and the frame
 * selects one (0x004CF460: its handle into 0x00E390DC, then SetCursor). The images are the exe's
 * resources (.rsrc: RT_GROUP_CURSOR -> RT_CURSOR, 32x32 monochrome AND / XOR masks with a hot spot):
 * they are read from the user's own Pharaoh.exe, nothing is shipped.
 *
 * The handles the platform gives the game are CURSOR_HANDLE(resource id); the platform shows the
 * cursor whose handle 0x00E390DC holds after each frame. */
#ifndef IMHOTEP_PLATFORM_CURSORS_H
#define IMHOTEP_PLATFORM_CURSORS_H

#include <stdint.h>

#include "state/state.h"

#define CURSOR_HANDLE(id) (0x00C50000u | (uint32_t) (id))

/* read the cursors from <game>/Pharaoh.exe (scale: integer magnification for a scaled window);
 * 0, or -1 */
int cursors_load(const char *game, int scale);
/* write the 19 handles into the state, as 0x004CF960 does */
void cursors_install(game_state *st);
/* show the cursor of the handle (unknown handle: the system arrow) */
void cursors_show(uint32_t handle);
void cursors_free(void);

#endif
