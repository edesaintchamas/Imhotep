/* Imhotep - the model files of a difficulty level (package cmdrest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_MODELS_H
#define IMHOTEP_COMMAND_MODELS_H

#include <stdint.h>

#include "state/state.h"

int32_t model_buildings_load(game_state *st); /* @bin 0x00540C90 1 when loaded */
int32_t model_figures_load(game_state *st);   /* @bin 0x00540ED0 */
int32_t model_tax_load(game_state *st);       /* @bin 0x00541160 */
void enemy_models_select(game_state *st);     /* @bin 0x004BC720 */
int32_t file_length(game_state *st, uint32_t name); /* @bin 0x004CED50 */

#endif
