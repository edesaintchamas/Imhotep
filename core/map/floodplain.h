/* Imhotep - the floodplain manager (Pharaoh.exe object at 0x00D05720) and its objects.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Manager: +0 a grid of 0xCB10 object pointers (one per tile); from +0x32C64, 30 depth lists of
 * 7 dwords {count, flood progress, id 1..30, manager, head node, tail node, current}; +0x32FB8
 * deepest non-empty lists, +0x32FBC (byte) built. Objects (0x1C bytes, heap): {list, node,
 * grid offset, x, y, depth, flag byte}. Nodes (0x10 bytes, heap): {next, previous, key (the
 * tile's random value), object}; each list is sorted by key. */
#ifndef IMHOTEP_MAP_FLOODPLAIN_H
#define IMHOTEP_MAP_FLOODPLAIN_H

#include <stdint.h>

#include "state/layout.h"
#include "state/state.h"

#define FLOODPLAIN_LISTS (G_FLOODPLAIN_MANAGER + 0x32C64u)
#define FLOODPLAIN_LIST_SIZE 0x1Cu

/* @bin 0x004BE0F0 (this = 0x00D05720) grid cleared, the 30 lists released and reset */
void floodplain_manager_reset(game_state *st);
/* @bin 0x004BD6A0 (this = list) release a list's objects and nodes */
void floodplain_list_release(game_state *st, uint32_t list);
/* @bin 0x004BD740 (this = list) new object for a tile, inserted by the tile's random value;
 * returns the object */
uint32_t floodplain_object_create(game_state *st, uint32_t list, int32_t offset, int32_t x, int32_t y,
                                  int32_t depth, uint8_t flag);

#endif
