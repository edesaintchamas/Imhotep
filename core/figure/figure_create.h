/* Imhotep - creating a figure (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_FIGURE_CREATE_H
#define IMHOTEP_FIGURE_FIGURE_CREATE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004B4800 a new figure of `type` at tile (x, y) in the first free record not referenced
 * elsewhere (0x004B47A0); owner +0x0F, direction +0x11; returns its id (0 when none is free) */
int32_t figure_create(game_state *st, int32_t owner, int32_t type, int32_t x, int32_t y, int32_t dir);

/* @bin 0x004B47A0 is figure `id` still referenced by a list (50 objects at 0x00EAAF68, method
 * 0x004F6570) or a 4-figure group (50 records at 0x009784D8) */
int figure_referenced(game_state *st, int32_t id);

/* @bin 0x004B1DF0 a walker leaving its building (+0x5A): destination a road tile (0x0047FE60) 8 tiles
 * away, on a side that turns with each walker (building +0x34); +0x46 := 1 when none */
void figure_walker_start(game_state *st, int32_t id);

/* @bin 0x004B5AF0 a figure's slot (+0x70) and group value (+0x98) by type */
void figure_assign_slot(game_state *st, int32_t id);

#endif
