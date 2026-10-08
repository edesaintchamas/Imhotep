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

#include "state/state.h"

/* globals of the city view (@bin addresses in the functions that use them) */
#define CV_ANIM_CLOCK 0x0070E3C0u   /* i32 frame clock of the last 60 ms animation step */
#define CV_ANIM_FLAG 0x0070E3C4u    /* i32 1: the 60 ms animation step is due this frame */
#define CV_VIEW_BOTTOM 0x0070E430u  /* i32 the viewport (clip) */
#define CV_VIEW_TOP 0x0070E434u
#define CV_VIEW_RIGHT 0x0070E438u
#define CV_VIEW_LEFT 0x0070E43Cu
#define CV_CAMERA_Y 0x0070E474u
#define CV_CAMERA_X 0x0070E478u
#define CV_MAP_START 0x0070E484u
#define CLIP_BOTTOM 0x00D3B550u
#define CLIP_TOP 0x00D3B554u
#define CLIP_RIGHT 0x00D3B558u
#define CLIP_LEFT 0x00D3B55Cu
#define FRAME_CLOCK 0x00E92E0Cu
#define OVERLAY 0x00E94E0Cu         /* i32 the overlay shown (0: none) */
#define SCREEN_W 0x00E38DB0u
#define SCREEN_H 0x00E38DACu
#define GFX 0x00F55BB0u             /* the graphics object: `this` of the image draws */

#define NO_GROUND 0x006DE515u     /* u8: the overlay ground replaces the ground pass */
#define ORIENTATION 0x00BCA2E0u   /* i32 the view orientation 0, 2, 4, 6 (as the city view reads it) */

/* the view walk (0x00430EF0, 0x00436B60, ...) */
#define VIEW_TABLE 0x0070E494u    /* i32[0x1C9][0xE7]: tile offset, or >= 0xFFF0000 (image + 0xFFF0000) */
#define VIEW_ROW 0x0070E47Cu      /* i32 the row walked (view y) */
#define VIEW_COL 0x0070E480u      /* i32 the column walked (view x) */
#define VIEW_TILE 0x0070E488u     /* i32 the tile drawn */
#define VIEW_COLS 0x0070E464u     /* i32 (+ 6: the last column walked) */
#define VIEW_ROWS 0x0070E460u     /* i32 (+ 0xE: the rows walked) */
#define VIEW_X0 0x0070E490u       /* i32 screen x of the first column */
#define VIEW_Y0 0x0070E48Cu       /* i32 screen y of the first row (+ 9 tile heights) */
#define TILE_HALF_W 0x0070E458u   /* i32 (+ 2: the column step in pixels) */
#define TILE_H 0x0070E44Cu        /* i32 the row step in pixels */
#define DRAW_IMAGE 0x00D3B5C0u    /* i32 the image drawn */
#define TINT 0x0070E120u          /* u16 the tint of the tinted blitters (0: none) */

/* map grids (u8 / i16 / i32 / u32 per tile) */
#define TILE_DRAW 0x00B67D1Cu     /* u8, 0x40: the tile draws its building / terrain */
#define BUILDING_GRID 0x00B4E6FCu /* i16 */
#define IMAGE_GRID 0x00B7482Cu    /* i32 (chunk 7) */
#define TERRAIN 0x00AF598Cu       /* u32 */
#define EDGE_GRID 0x00B285CCu     /* u8: footprint corner (low 4 bits: the size), 0x10 */
#define FIGURE_GRID 0x00B350DCu   /* i16: first figure */
#define SPRITE_GRID 0x00ADC36Cu   /* u8 (chunk 14) */
#define TERRAIN_PLAIN_MASK 0xEEFED77Fu /* terrain bits that make a tile "not plain" for the top redraws */

/* the image index of the graphics object 0x00F55BB0 */
#define IMAGE_ENTRIES 0x010D69F8u /* 64 bytes per image: GFX + 0x180E48 */
#define IMAGE_COUNT 0x0126517Cu   /* i32: GFX + 0x30F5CC */

/* image draw primitives (package blit) */
#define IMAGE_DRAW 0x0053E590u
#define IMAGE_DRAW_TOP 0x0053F250u

void city_view_draw(game_state *st, int32_t camera_x, int32_t camera_y, int32_t map_start); /* 0x00430D30 */
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
