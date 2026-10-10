/* Imhotep - pending names of package p9 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P9_CITY_VIEW_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P9_CITY_VIEW_H

/* globals (owner: the area header named in the comment, or the lead) */
#define G_MENU_OPEN 0x00705724u /* @bin 0x00426030 u32 the open menu's entry in the menu bar's table (present/ui/windows.c MENU_OPEN) */
#define G_U784B60 0x00784B60u /* @bin 0x0046E970 i16 != 0: the ratings and goals are skipped (0x0051F790: no countdown) */
#define G_U978D70 0x00978D70u /* @bin 0x004539A0 i32 1: houses are only redrawn, not evolved (0x00403D70: build menu 1 shows 0x0D) */
#define G_BUILD_TURN_VIEW 0x00BC96DCu /* @bin 0x0047CC70 i32 the fitted building's turn against the view (core/building/map_add.c) */
#define G_UE36C80 0x00E36C80u /* @bin 0x0042C740 u32 a memory block (freed by core/frame/display.c); + 0x708 per tile: the footprint pixels */
#define G_RIGHT_HELD 0x00E38DF4u /* @bin 0x00425990 u8 the right button is held (core/frame/frame.c keeps its previous value) */
#define G_UE38DF6 0x00E38DF6u /* @bin 0x00425990 u8 a one-frame flag (cleared by 0x004CF1F0), tested with the right press */
#define G_RAMP_FOUND 0x00EAAE84u /* @bin 0x004ED360 u8 the ramp walk found the building 0x00EAAE88 (core/city/events_actions.c) */
#define G_RAMP_WALK_TYPE 0x00EAAE8Cu /* @bin 0x004ED360 i32 := 0, then 0xDA / 0xDB / 0xDC by the walk (core/city/events_actions.c) */
#define G_RAMP_WALK_TILE 0x00EAAE90u /* @bin 0x004ED360 i32 := 0, then a tile of the walk */
#define G_UEAAE94 0x00EAAE94u /* @bin 0x004ED360 i32 := 2 at the walk's start */
#define G_RAMP_STEPS 0x00EAAE98u /* @bin 0x004ED360 i32 the walk's steps (at most 20) */
#define G_RAMP_STEP 0x00EAAEA0u /* @bin 0x004ED360 i32 := 1: the walk goes by 2 * this tiles */
#define G_RAMP_START 0x00EAAEA4u /* @bin 0x004ED360 i32 the walk's first tile */
#define G_AMBIENT_COUNTERS 0x00F55B50u /* @bin 0x0053B670 i32[12] the ambient counters the ground pass fills (core/frame/sound.c) */

/* record and table fields */
#define BLD_U5E 0x5Eu /* @bin 0x004EF993 i16: a 0xB7 part: the first layer's y (x: BLD_U5C) */
#define BLD_U60 0x60u /* @bin 0x004EF993 i16: a 0xB7 part: the second layer's x (y: BLD_U62); not BLD_MON_ANNOUNCED_60 (type 0xE6) */
#define CITY_MAX_PROSPERITY 0x46A8u /* @bin 0x0051FAD4 i32: drawn by the debug text " Max prosperity" */
#define CITY_RATING_CULTURE 0x42CCu /* @bin 0x004269B0 i32: the culture rating (save.md §5) */
#define CITY_RATING_MONUMENT 0x42D4u /* @bin 0x0046EF50 i32: the monument rating (save.md §5) */
#define CITY_RATING_PROSPERITY 0x42D0u /* @bin 0x0046E1E0 i32: the prosperity rating (save.md §5) */
#define CITY_U46A4 0x46A4u /* @bin 0x00454190 i32 < 2 with a level above 1: the house does not evolve (reasons 0x46 / 0x4A) */
#define EMPOBJ_CITY_KIND 0x18u /* @bin 0x004C95D0 u8 the trade window city's kind: 1, 3, 5 trade; 0 lists what it sells */
#define EMPOBJ_U19 0x19u /* @bin 0x004CACA3 u8 indexes the texts G_CITY_KIND_TEXTS (good 0x13 sold) */


/* used by core/ too (moved from present/layout at the merge) [core header] */
#define G_PARTS_D5 0x00596088u /* @bin 0x00420050 the parts of type 0xD5 (as G_PARTS_D2) */
#define G_PARTS_D5_END 0x00596158u /* @bin 0x00420050 */
#define G_PARTS_E1 0x0059EDF0u /* @bin 0x004206F0 the parts of type 0xE1 */
#define G_PARTS_E1_END 0x0059EFA0u /* @bin 0x004206F0 */
#define G_PARTS_E6 0x005A8148u /* @bin 0x00420A30 the parts of type 0xE6 */
#define G_PARTS_E6_END 0x005A84C8u /* @bin 0x00420A30 */
#define G_PARTS_E3 0x005AD218u /* @bin 0x00420D60 the parts of type 0xE3 */
#define G_PARTS_E3_END 0x005AD3C8u /* @bin 0x00420D60 */
#define G_PARTS_E4 0x005B0060u /* @bin 0x00421060 the parts of type 0xE4 */
#define G_PARTS_E4_END 0x005B0150u /* @bin 0x00421060 */
#define G_PARTS_E5 0x005BE510u /* @bin 0x00421920 the parts of types 0xE5, 0xEA..0xEC: 3 entries (0x30 bytes) per building +0x68 */
#define G_PARTS_D2 0x005C2750u /* @bin 0x0041FFF0 16-byte entries (+0 the part object): the parts drawn for type 0xD2 */
#define G_PARTS_D2_END 0x005C28B0u /* @bin 0x0041FFF0 the end of G_PARTS_D2 */

#endif
