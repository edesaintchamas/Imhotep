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

/* @bin per-type meanings of some offsets above (docs/naming.md: one name per meaning) */
#define FIG_WORK_BUILDING 0x4Eu    /* i16: workers at a monument site: the building they work on (0x004F1CB0, 0x004F2410) */
#define FIG_SHOOTER 0x5Au          /* i16: missiles: the figure that shot it (0x004A2F48, 0x0049A5FD) */
#define FIG_PREY 0x5Eu             /* i16: hunters: the animal their missile hit (0x004A2F48) */

/* @bin wave 0: the other offsets the ports use (tools/naming/rename.py scan); a name with U and
 * the offset until its meaning is established from the code that uses it */
#define FIG_U00                0x00u
#define FIG_ANIM_FRAME         0x01u /* u8: animation frame 0..11 (the figure actions, core/figure) */
#define FIG_U02                0x02u
#define FIG_U03                0x03u /* u8: toggled each tick, the figure moves on every other one (actions_b1.c) */
#define FIG_U04                0x04u /* u32: the image drawn: image group base + frame + direction (the figure actions) */
#define FIG_U0C                0x0Cu
#define FIG_U0D                0x0Du
#define FIG_U28                0x28u
#define FIG_U29                0x29u
#define FIG_U30                0x30u
#define FIG_U32                0x32u
#define FIG_ROAM_STATE         0x46u /* u8[4]: roam state, +0x46..+0x49 (0x004B1EF0) */
#define FIG_U47                0x47u
#define FIG_U48                0x48u
#define FIG_U49                0x49u
#define FIG_CC_DELTA_X         0x52u /* i16: cross-country distance x (0x004B4220); other uses for some types */
#define FIG_CC_DELTA_Y         0x54u /* i16: cross-country distance y (0x004B4220); other uses for some types (the figure served, ...) */
#define FIG_CC_ERROR           0x56u /* i16: cross-country error term (0x004B4220); other uses for some types (by good) */
#define FIG_U59                0x59u
#define FIG_FORMATION          0x60u /* i16: formation, records of 0x90 bytes at 0x00BCB9C8 (0x004B6E10) */
#define FIG_U62                0x62u
#define FIG_U63                0x63u
#define FIG_LEADER             0x68u /* i16: the figure it follows (save.md: leader id, tentative) */
#define FIG_U6B                0x6Bu
#define FIG_U6C                0x6Cu
#define FIG_U6D                0x6Du
#define FIG_U6E                0x6Eu
#define FIG_U6F                0x6Fu
#define FIG_U75                0x75u
#define FIG_HEIGHT_TICKS       0x77u /* u8: height adjust ticks (0x004B23C0) */
#define FIG_HEIGHT             0x78u /* u8: current height, towards FIG_HEIGHT_TARGET */
#define FIG_HEIGHT_TARGET      0x79u /* u8 */
#define FIG_U7B                0x7Bu
#define FIG_RAND_A             0x7Cu /* u8: rand() & 3 at creation (0x004B4800) */
#define FIG_U7D                0x7Du
#define FIG_RAND_B             0x7Eu /* u8: the same value */
#define FIG_U7F                0x7Fu
#define FIG_U80                0x80u
#define FIG_U88                0x88u
#define FIG_U92                0x92u
#define FIG_U93                0x93u
#define FIG_BUYERS_1           0x9Cu /* i32[4]: bazaar buyer list (0x00462180) */
#define FIG_BUYERS_2           0xACu /* i32[4] */
#define FIG_BUYERS_3           0xBCu /* i32[4] */
#define FIG_U11C               0x11Cu
#define FIG_U124               0x124u
#define FIG_HIT_POINTS         0x128u /* hit points, of FIG_HIT_POINTS_MAX (the combat actions) */
#define FIG_HIT_POINTS_MAX     0x12Cu
#define FIG_U130               0x130u
#define FIG_U131               0x131u
#define FIG_U132               0x132u
#define FIG_U134               0x134u
#define FIG_U138               0x138u
#define FIG_U13C               0x13Cu
#define FIG_U140               0x140u
#define FIG_FRAME_PERIOD       0x144u /* ticks per animation frame */
#define FIG_FRAME_TICKS        0x148u /* counts ticks up to FIG_FRAME_PERIOD */
#define FIG_U14C               0x14Cu
#define FIG_U14E               0x14Eu
#define FIG_U150               0x150u
#define FIG_U154               0x154u
#define FIG_U158               0x158u
#define FIG_U15C               0x15Cu
#define FIG_U160               0x160u
#define FIG_U164               0x164u
#define FIG_U16C               0x16Cu
#define FIG_U170               0x170u
#define FIG_U174               0x174u
#define FIG_U178               0x178u
#define FIG_U17C               0x17Cu
#define FIG_U180               0x180u /* u32: a second image drawn over it (the carried good, ...), 0: none */

/* @bin 0x004B6E10, 0x004B77D0 the formation record's fields (chunk 24: G_FORMATION_BASE, 50 records of
 * 0x90 bytes, FORMATION_ADDR(id) in figure/formation.h; save.md section 5 "Other records"). Offsets
 * within one record: S16(st, FORMATION_ADDR(id) + FM_X). FM_U<offset>: meaning not established */
#define FM_IN_USE 0x00u        /* u8 */
#define FM_OWNER 0x01u         /* u8: owner (save.md) */
#define FM_LEGION 0x02u        /* u8: legion index 0..7, the first free one (save.md) */
#define FM_U03 0x03u           /* u8 */
#define FM_TYPE 0x04u          /* i16: the figure type of its soldiers (0x0B .. 0x0D tested; save.md: the fort's +0x12) */
#define FM_FIGURES 0x08u       /* i16[16]: figure ids */
#define FM_COUNT 0x28u         /* u8: figures */
#define FM_MAX_COUNT 0x29u     /* u8: figures wanted, compared with FM_COUNT (invaders.c; 0x004B8080 := 0x10) */
#define FM_LAYOUT 0x2Au        /* i16: layout index into G_FORMATION_LAYOUT_X (0x004BA7E0) */
#define FM_MORALE 0x2Cu        /* i16: := 50 at creation (save.md); += delta within 0..max (0x004B7B50, formations_morale_*) */
#define FM_X 0x2Eu             /* i16 */
#define FM_Y 0x30u             /* i16 */
#define FM_U32 0x32u           /* i16 */
#define FM_U34 0x34u           /* i16 */
#define FM_U36 0x36u           /* i16[2]: a point (x, y) the herd animals wander around (0x0049B750) */
#define FM_U38 0x38u           /* i16: y of the FM_U36 point */
#define FM_DEST_X 0x3Au        /* i16: the point the soldiers make for (0x004B9213, 0x00497010) */
#define FM_DEST_Y 0x3Cu        /* i16 */
#define FM_DEST_BUILDING 0x3Eu /* i16: copied to the soldiers' FIG_DEST_BUILDING (0x00497010) */
#define FM_STANDARD_BEARER 0x40u /* i16: standard-bearer figure (type 0x0E; save.md) */
#define FM_PLAYER 0x42u        /* u8: player legion (save.md) */
#define FM_U44 0x44u           /* i16 := the invasion's +0x178 (0x004AD556) */
#define FM_U48 0x48u           /* i16 */
#define FM_U4A 0x4Au           /* i16 */
#define FM_U4C 0x4Cu           /* i16 */
#define FM_U4E 0x4Eu           /* i16: above 0x2D: the enemies retreat (enemies.c) */
#define FM_U50 0x50u           /* i16: non-zero: the soldiers attack (action 0x0C; 0x00497010) */
#define FM_U52 0x52u           /* i16: one of three countdowns, with FM_U54 and FM_U56 (enemies.c) */
#define FM_U54 0x54u           /* i16: countdown (enemies.c); move delay counter (invaders.c) */
#define FM_U56 0x56u           /* i16: countdown (enemies.c) */
#define FM_U58 0x58u           /* i16 */
#define FM_U5A 0x5Au           /* i16: non-zero tested with the facing (0x0049A5FD) */
#define FM_U5C 0x5Cu           /* i16 := 6 (0x00497010) */
#define FM_U5E 0x5Eu           /* i16 := 6 when attacked (0x0049A5FD) */
#define FM_U60 0x60u           /* i16 := the attacker's formation (0x0049A5FD) */
#define FM_U62 0x62u           /* i16 */
#define FM_U64 0x64u           /* i16 */
#define FM_U66 0x66u           /* i16 */
#define FM_U67 0x67u           /* u8 */
#define FM_U68 0x68u           /* u8 */
#define FM_HALTED 0x69u        /* u8 */
#define FM_KIND 0x6Au          /* u8: kind (the invasion's kind, 0x004AD556; the name counter, 0x004B5F9A) */
#define FM_FACING 0x6Bu        /* u8: facing, from its moves (0x004B8130) */
#define FM_U6C 0x6Cu           /* i16 */
#define FM_U6E 0x6Eu           /* i16 */
#define FM_U70 0x70u           /* u8: countdown before the facing follows the moves (0x004B8130) */
#define FM_U71 0x71u           /* u8 := 0 (0x004AD556) */
#define FM_U73 0x73u           /* u8 */
#define FM_U74 0x74u           /* u8 := 0x11 or 0 (invaders.c) */
#define FM_U75 0x75u           /* u8: counter (invaders.c) */
#define FM_U76 0x76u           /* u8 (invaders.c) */
#define FM_EVENT 0x78u         /* i16: the scenario event (below the G_EVENTS count; 0x004AD556, formations_rebuild) */
#define FM_EXPERIENCE 0x7Au    /* i16: (+ 10) / 20 is the defence bonus of the player's soldiers (0x0049A5FD) */
#define FM_U7E 0x7Eu           /* i16: non-zero: FM_U80 counts up (0x0048E9E0) */
#define FM_U80 0x80u           /* i16: counter (0x0048E9E0) */
#define FM_U82 0x82u           /* i16: >= 0 tested (helpers_x.c, formation.c) */
#define FM_U84 0x84u           /* u32: cleared when its figure +0x92 is deleted (0x004B49FE; save.md: a transport) */
#define FM_INVASION 0x88u      /* i16: invasion id, from G_INVASION_SEQUENCE (save.md chunk 126) */
#define FM_TARGET 0x8Cu        /* i32: its target; only that one is taken (0x00499BD4) */

#endif
