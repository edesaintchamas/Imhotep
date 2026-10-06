/* Imhotep - build-menu availability (package cmdrest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_MENU_H
#define IMHOTEP_COMMAND_MENU_H

#include <stdint.h>

#include "state/state.h"

void build_menu_update(game_state *st);                               /* @bin 0x00404350 */
int32_t build_menu_count(game_state *st);                             /* @bin 0x00404120 */
uint8_t menu_cheat_gate(game_state *st);                              /* @bin 0x00403EF0 */
int32_t monuments_placed(game_state *st);                             /* @bin 0x004F5020 */
uint8_t monument_kind_open(game_state *st, int32_t kind, uint8_t any); /* @bin 0x004F5050 */
uint8_t monument_menu_open(game_state *st, int32_t type);             /* @bin 0x004F5470 */

#endif
