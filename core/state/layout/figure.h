/* Imhotep - the figure record's fields (chunk 21: FIGURE_BASE, 2000 records of 0x184 bytes).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Offsets within one record: U8(st, FIGURE_ADDR(id) + FIG_TYPE). Evidence: docs/formats/save.md §5
 * "Figures" (the code cited there). A field whose meaning is not established is FIG_U<offset>.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_FIGURE_H
#define IMHOTEP_STATE_LAYOUT_FIGURE_H

#include "state/records.h"

/* @bin 0x004B4800 the constructor; the readers cited in save.md §5 */
#define FIG_NEXT_ON_TILE 0x08u     /* u16: next figure on the same tile (0x00439740) */
#define FIG_TYPE 0x0Au             /* u8: figure type (names: text group 64) */
#define FIG_RESOURCE 0x0Bu         /* u8: resource carried (0x00462180) */
#define FIG_STATE 0x0Eu            /* u8: 1 alive, 2 deleted (0x00489B70) */
#define FIG_OWNER 0x0Fu            /* u8: compared with the current player (0x004B17D0) */
#define FIG_ACTION_BEFORE_ATTACK 0x10u /* u8 (0x004B1210) */
#define FIG_DIRECTION 0x11u        /* i8: 0-7, 8 arrived, 9 reroute, 10 lost (0x004B27E0) */
#define FIG_PREV_DIRECTION 0x12u   /* i8 */
#define FIG_ATTACK_DIRECTION 0x13u /* i8 (0x004B1210) */
#define FIG_X 0x14u                /* u16: tile, map-relative */
#define FIG_Y 0x16u                /* u16 */
#define FIG_PREV_X 0x18u           /* i16: previous tile (0x004B23C0) */
#define FIG_PREV_Y 0x1Au           /* i16 */
#define FIG_MISSILE_DAMAGE 0x1Cu   /* u8 (0x004B3FF0) */
#define FIG_DAMAGE 0x1Du           /* u8: above the type's maximum: dying (0x004B17D0) */
#define FIG_GRID_OFFSET 0x20u      /* u32 */
#define FIG_DEST_X 0x24u           /* i16 (0x004B28F0) */
#define FIG_DEST_Y 0x26u           /* i16 */
#define FIG_SOURCE_X 0x2Cu         /* i16 (0x004B23C0) */
#define FIG_SOURCE_Y 0x2Eu         /* i16 */
#define FIG_WAIT_TICKS 0x36u       /* i16 (0x00458D00) */
#define FIG_ACTION 0x38u           /* u8: action state (0x004B4800, 0x004EB000) */
#define FIG_PROGRESS_ON_TILE 0x39u /* i8: 0-15 (0x004B27E0) */
#define FIG_PATH_ID 0x3Au          /* i16: route path, chunk 22 (0x004B28F0) */
#define FIG_PATH_STEP 0x3Cu        /* i16 (0x004B27E0) */
#define FIG_PATH_LENGTH 0x3Eu      /* i16 (0x004B28F0) */
#define FIG_IN_BUILDING_WAIT 0x40u /* u8 (0x004B3FF0) */
#define FIG_ON_ROAD 0x41u          /* u8 (0x004B23C0) */
#define FIG_MAX_ROAM_LENGTH 0x42u  /* i16 (0x004B1EF0) */
#define FIG_ROAM_LENGTH 0x44u      /* i16 */
#define FIG_CC_X 0x4Au             /* i16: cross-country position, 1/15 tile (0x004B3FF0) */
#define FIG_CC_Y 0x4Cu             /* i16 */
#define FIG_CC_DEST_X 0x4Eu        /* i16 (0x004B4220) */
#define FIG_CC_DEST_Y 0x50u        /* i16 */
#define FIG_CC_DIRECTION 0x58u     /* u8 (0x004B3FF0) */
#define FIG_HOME 0x5Au             /* i16: home building (0x00462180) */
#define FIG_TARGET_HOUSE 0x5Cu     /* i16: immigrant's house (0x00458D00) */
#define FIG_DEST_BUILDING 0x5Eu    /* i16 (0x004BC7A0) */
#define FIG_MIGRANT_PEOPLE 0x64u   /* u8 (0x00458D00) */
#define FIG_INVISIBLE 0x65u        /* u8 (0x004B23C0) */
#define FIG_SERVICE_MAX 0x66u      /* u8 (0x004BAA10) */
#define FIG_ATTACK_IMAGE 0x6Au     /* u8 (0x004B1210) */
#define FIG_NAME_INDEX 0x70u       /* u16: index in the name group FIG_NAME_GROUP (0x004B5AF0) */
#define FIG_TERRAIN_USAGE 0x72u    /* u8: routing algorithm (0x004B28F0) */
#define FIG_AMOUNT 0x74u           /* i16: amount carried (0x004BC7A0) */
#define FIG_BOAT_MODE 0x76u        /* u8 (0x004B28F0) */
#define FIG_COLLECTING 0x7Au       /* u8: resource being collected (0x00462180) */
#define FIG_TARGET 0x82u           /* i16: target figure (0x00489B70) */
#define FIG_TARGETED_BY 0x84u      /* i16 */
#define FIG_SEQUENCE 0x86u         /* u16: creation sequence number */
#define FIG_TILE_INDEX 0x8Au       /* u8: index in the tile's figure chain (0x004B52F0) */
#define FIG_ATTACKERS 0x8Bu        /* u8: attacker count (0x004B1210) */
#define FIG_ATTACKER1 0x8Cu        /* i16 */
#define FIG_ATTACKER2 0x8Eu        /* i16 */
#define FIG_OPPONENT 0x90u         /* i16 */
#define FIG_NAME_GROUP 0x98u       /* i32: name text group (0x004D32F0, 0x004B5AF0) */
#define FIG_FOLLOW 0x120u          /* i16 (0x004B23C0) */

#endif
