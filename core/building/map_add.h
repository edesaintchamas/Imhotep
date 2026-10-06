/* Imhotep - a created building put on the map (package mapadd): 0x00470990 and its callees.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_MAP_ADD_H
#define IMHOTEP_BUILDING_MAP_ADD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00470990 (player, type, id, x, y, size, new, flag2) the building's tiles, images and
 * per-type setup; `new` = 1 when the building was just placed (creates the parts, formations,
 * storage records ...), 0 when it is drawn again (loader, rotation) */
void building_map_add(game_state *st, int32_t player, int32_t type, int32_t id, int32_t x, int32_t y,
                      int32_t size, int32_t is_new, int32_t flag2);

void house_split_46cf40(game_state *st, int32_t id);                                         /* @bin 0x0046CF40 */
int32_t image_group_451bc0(int32_t v);                                                       /* @bin 0x00451BC0 */
void fort_soldiers_472790(game_state *st, int32_t n, int32_t f, int32_t x, int32_t y);       /* @bin 0x00472790 */
int32_t shore_fit_47d060(game_state *st, int32_t x, int32_t y, int32_t turn);                /* @bin 0x0047D060 */
void terrain_and_rect_482580(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, uint32_t mask); /* @bin 0x00482580 */
int32_t formation_create_4b6e10(game_state *st, int32_t id);                                 /* @bin 0x004B6E10 */
void temple_complex_add_4c3420(game_state *st, int32_t id, int32_t x, int32_t y, int32_t player, int32_t type,
                               int32_t is_new);                                              /* @bin 0x004C3420 */
int32_t rec88_reset_4f6520(game_state *st, uint32_t self);                                    /* @bin 0x004F6520 */
void sphinx_images_53d850(game_state *st);                                                    /* @bin 0x0053D850 */
void anim_lists_reset_4212f0(game_state *st);                                                 /* @bin 0x004212F0 */

/* map_add_monument.c */
int32_t monument_slot_4f4f80(game_state *st, int32_t id);                                     /* @bin 0x004F4F80 */
int32_t monument_kind_4f4b70(game_state *st, int32_t id);                                     /* @bin 0x004F4B70 */
void mastaba_add_4df9d0(game_state *st, int32_t x, int32_t y, int32_t id, int32_t flag2);    /* @bin 0x004DF9D0 */
int32_t obelisk_add_4e57f0(game_state *st, int32_t id);                                       /* @bin 0x004E57F0 */
void pyramid_add_4ee780(game_state *st, int32_t player, int32_t x, int32_t y, int32_t size, int32_t id); /* @bin 0x004EE780 */
void pyramid_ramp_4f5f10(game_state *st, int32_t id);                                         /* @bin 0x004F5F10 */
void sun_temple_add_5417b0(game_state *st, int32_t x, int32_t y, int32_t id);               /* @bin 0x005417B0 */
void monument_add_4015a0(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2);
void monument_add_40da90(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2);
void monument_add_417180(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2);
void monument_add_4e6840(game_state *st, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2);
/* @bin 0x00545C30 0x0054FBA0 0x00556480 0x00559130 (thiscall) the big tombs' parts */
void tomb_add(game_state *st, uint32_t fn, uint32_t self, int32_t x, int32_t y, int32_t id, int32_t flag2);
void anim_list_reset(game_state *st, uint32_t from, uint32_t to);

#endif
