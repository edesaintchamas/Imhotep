/* Imhotep - Pharaoh.exe's sections, for the initial game state (file system side, not core/).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PLATFORM_EXE_IMAGE_H
#define IMHOTEP_PLATFORM_EXE_IMAGE_H

#include "state/state.h"

/* Read <game>/Pharaoh.exe and copy every initialised section that lies in the state's range
 * (.rdata, .data) into `st`, at its address. Returns 0, or -1 when the file cannot be read or is
 * not a PE image. */
int exe_image_load(const char *game, game_state *st);

#endif
