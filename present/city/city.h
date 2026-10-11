/* Imhotep - the city view (package cityview, roadmap 7.4, docs/presentation.md 3.1).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original's city view 0x00430D30 and its passes: ground 0x00430EF0, upper 0x00436B60,
 * construction ghost 0x0043B630, overlays 0x004316F0 / 0x00439F10, 0x00439740, 0x0043E300. They
 * draw into the back buffer through the image draw 0x0053E590 and the blitters (package blit,
 * called through GAME_CALL), and make the state writes of docs/presentation.md 1.4 exactly. */
#ifndef IMHOTEP_PRESENT_CITY_H
#define IMHOTEP_PRESENT_CITY_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* the globals of the city view are named in present_layout.h */

#define TERRAIN_PLAIN_MASK 0xEEFED77Fu /* @bin 0x00436C62 terrain bits that make a tile "not plain" for the top redraws */

void city_view_draw(game_state *st, int32_t camera_x, int32_t camera_y, int32_t map_start); /* 0x00430D30 */
/* @imhotep enhanced zoom: the city view drawn zoomed (present/city/zoom.c): 1 when it was, 0 at 100 % */
int city_view_draw_zoomed(game_state *st, int32_t camera_x, int32_t camera_y, int32_t map_start);
void city_ground_draw(game_state *st);                   /* 0x00430EF0 */
void city_upper_draw(game_state *st, int32_t param);     /* 0x00436B60 */
void city_overlay_ground_draw(game_state *st);           /* 0x004316F0 */
void city_overlay_tops_draw(game_state *st);             /* 0x00439F10 */
/* the position of a footprint drawn from its corner tile (1..5: 1, past 5: -1, corner 0: 0, at x, y) */
int cv_overlay_corner_pos(game_state *st, uint32_t corner, int32_t x, int32_t y, int32_t *px, int32_t *py);
/* overlay_columns.c */
int city_overlay_shows(game_state *st, int32_t b);       /* 0x00431690 */
void city_overlay_column(game_state *st, int32_t level, int32_t x, int32_t y, int32_t mode); /* 0x004361A0 */
int32_t city_building_0x46f1c0(game_state *st, int32_t b); /* 0x0046F1C0 */
/* overlay_problems.c */
int city_building_0x459ff0(game_state *st, int32_t b);    /* 0x00459FF0 */
void city_house_evolve_reason(game_state *st, int32_t b); /* 0x00454190 */
void city_building_problems(game_state *st, int32_t b);   /* 0x00432920 */
/* overlay_tiles.c */
void city_overlay_hidden_building(game_state *st, int32_t b, int32_t x, int32_t y, int32_t extra); /* 0x00432D30 */
void city_overlay_desirability_tile(game_state *st, int32_t x, int32_t y, int top); /* 0x00433760 */

/* common.c */
void cv_image_draw(game_state *st, int32_t image, int32_t x, int32_t y);     /* 0x0053E590 on GFX */
void cv_image_draw_top(game_state *st, int32_t image, int32_t x, int32_t y); /* 0x0053F250 on GFX */
uint32_t cv_image_entry(game_state *st, int32_t image);                     /* 0x00438180 */
int32_t cv_group_base(game_state *st, int32_t group);                       /* 0x004381A0 */
void cv_image_draw_offset(game_state *st, int32_t image, int32_t x, int32_t y, int32_t dx, int32_t dy); /* 0x004381B0 */
int cv_tile_covers(game_state *st, int32_t tile);                           /* the 0xB7 / figure test */

/* parts.c: the building parts the upper pass draws */
void cv_part_b7(game_state *st, int32_t image, int32_t x, int32_t y, int32_t b); /* 0x004EF550 */
void cv_part_e6(game_state *st, int32_t b, int32_t x, int32_t y);            /* 0x00401F70 */
void cv_parts_41fff0(game_state *st, int32_t x, int32_t y);                  /* 0x0041FFF0 */
void cv_parts_420050(game_state *st, int32_t x, int32_t y);                  /* 0x00420050 */
void cv_parts_4206f0(game_state *st, int32_t x, int32_t y);                  /* 0x004206F0 */
void cv_parts_420a30(game_state *st, int32_t x, int32_t y);                  /* 0x00420A30 */
void cv_parts_420d60(game_state *st, int32_t x, int32_t y);                  /* 0x00420D60 */
void cv_parts_421060(game_state *st, int32_t x, int32_t y);                  /* 0x00421060 */
void cv_parts_421920(game_state *st, int32_t x, int32_t y, int32_t b);       /* 0x00421920 */
int32_t cv_part_set_odd(game_state *st);                                     /* 0x0041F8B0 */
void cv_part_set_select(game_state *st, int32_t odd);                        /* 0x0041F8C0 */
void cv_part_cf(game_state *st, int32_t b, int32_t x, int32_t y);            /* 0x004E5E90 */
void cv_part_d5(game_state *st, int32_t b, int32_t x, int32_t y);            /* 0x00542E10 */
void cv_part_ca(game_state *st, int32_t b);                                  /* 0x004C0160 */
/* 0x00438210 / 0x00438350: 0 when the image's direction is outside the original's stack table */
int cv_anim_4a(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y, int32_t v);
int cv_anim_4c(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y, int32_t f);
void cv_anim_a1(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x00438470 */
void cv_anim_6a(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x004384D0 */
void cv_anim_1e(game_state *st, int32_t img, int32_t frame, int32_t x, int32_t y); /* 0x00438590 */
void cv_anim_22(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x00438650 */
void cv_anim_1f(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x004386B0 */
void cv_anim_21(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x00438710 */
void cv_anim_23(game_state *st, int32_t frame, int32_t x, int32_t y);         /* 0x00438770 */

#endif
