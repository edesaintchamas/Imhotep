/* Imhotep - pending names of package p1 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P1_FIGURE_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P1_FIGURE_H

/* @bin globals of other areas used by core/figure; the function or chunk on each line */
#define G_DIRECTIONS_DY 0x005CD97Cu /* i32: dy of the (dx, dy) pairs of G_DIRECTIONS (0x004EAB10 callers) */
#define G_U5E4EE8 0x005E4EE8u /* i32 wait limit compared with FIG_WAIT_TICKS; with 0x005E4EEC a monument stage time */
#define G_U7027BC 0x007027BCu /* u8 of the tutorial trigger flags (chunk 166 at 0x007027B8, +4) */
#define G_CHEAT_SIDE_SHOW 0x007027E1u /* u8 toggled by the "Side Show" cheat (0x0041CCF0): the other sprite set */
#define G_U7058B8 0x007058B8u /* i32 > 0 tested for forts and military (0x004E3200) */
#define G_RING_OFFSETS_1X1 0x0070C8ECu /* i16 grid offsets of the 1x1 rings 1..12 (0x00481490; core/map/terrain_rings.h) */
#define G_HOUSE_GOODS_A 0x0070CDF8u /* i32 per house level (0x58 bytes): monthly amount of a good (0x00454B40) */
#define G_HOUSE_GOODS_B 0x0070CDFCu /* i32: the next good's amount */
#define G_HOUSE_GOODS_C 0x0070CE00u /* i32: the next good's amount */
#define G_HOUSE_GOODS_D 0x0070CE04u /* i32: the next good's amount */
#define G_HOUSE_LEVEL_U14 0x0070CE0Cu /* i32 per house level: reloaded into BLD +0xE4 (save.md section 5: 0x0070CE0C) */
#define G_HOUSE_LEVEL_U1C 0x0070CE14u /* i32 per building +0x12 (0x58 bytes): amount (0x00489F70, 0x0045849B) */
#define G_U77567C 0x0077567Cu /* i32 set: redraw by 0x0047A120 (core/building/day2_destroy.c) */
#define G_INVASION_SERIAL 0x00775900u /* i16 invasion id counter, formation +0x88 (chunk 126) */
#define G_TRADER_NEXT 0x0077AEFCu /* i32 next trader record index (chunk 95) */
#define G_TRADERS 0x0077AF00u /* trader records, 100 x 0x58 (chunk 94; figure FIG_U7F = index) */
#define G_TRADERS_U04 0x0077AF04u /* i32 at +4 of the trader records (loads counted, below 12) */
#define G_U784C58 0x00784C58u /* scenario info: i32 tested before a goal (population goal, b_ratings.c) */
#define G_U784C60 0x00784C60u /* scenario info: i16 x of a point (-1: none; core/city/day_basic.c) */
#define G_U784C62 0x00784C62u /* scenario info: i16 y of that point */
#define G_SCEN_ENTRY_X 0x00784C64u /* scenario info: i16 entry point x (-1: the middle of the east edge; 0x004DE8A9) */
#define G_SCEN_ENTRY_Y 0x00784C66u /* scenario info: i16 entry point y */
#define G_SCEN_EXIT_X 0x00784C68u /* scenario info: i16 exit point x (-1: the entry point; 0x004DE8A9) */
#define G_SCEN_EXIT_Y 0x00784C6Au /* scenario info: i16 exit point y */
#define G_U784CB1 0x00784CB1u /* scenario info: u8 flag read by 0x004BBD40 */
#define G_ALT_SPRITES 0x00784CBDu /* u8: the other sprite set (0x00490A54, 0x00497FC0; set at the start, newgame.md) */
#define G_U784DC4 0x00784DC4u /* i32 x of three points (-1: none; 0x004AEAB0) */
#define G_U784DD0 0x00784DD0u /* i32 y of the three points */
#define G_MUMMY_LIMIT 0x0078D798u /* i32 mummy-attack parameter, default 10 (chunk 173, 0x004E7AC0) */
#define G_LOCUSTS 0x0078D79Cu /* i32 locust plague timer, -1 armed (chunk 40) */
#define G_CURSE_COUNTDOWN 0x0078D7A4u /* i32 god-curse countdown from Pyramid progress (chunk 106) */
#define G_FROGS 0x0078D7A8u /* i32 Plague of Frogs countdown (chunk 105) */
#define G_BLOOD_DURATION 0x0078D7ACu /* i32 River of Blood duration (chunk 107) */
#define G_BLOOD_LEFT 0x0078D7B0u /* i32 River of Blood: what is left, counting down from G_BLOOD_DURATION (chunk 131; 0x0047A1F0) */
#define G_RUIN_COUNT 0x0078D7FCu /* i32 burning-ruin building ids listed (G_RUINS; save.md chunk 96) */
#define G_RUIN_LIST_SIZE 0x0078D800u /* i32 burning-ruin list size (chunk 85) */
#define G_RUINS_FOUND 0x0078D804u /* i32 burning ruins found by the last scan (chunk 84) */
#define G_STORAGE_LIST 0x0078D8D4u /* storage buildings holding more than 100, G_STORAGE_LIST_COUNT of them (0x0045DE80) */
#define G_STORAGE_LIST_COUNT 0x0078D9A0u /* i32 entries of G_STORAGE_LIST, at most 99 */
#define G_RUINS 0x0078DD90u /* u16 x 500 burning-ruin building ids (chunk 96) */
#define G_STORAGE_SETTINGS_U08 0x007900C0u /* storage settings records of 0xC4 bytes by building +0xC1 (chunk 113 at 0x007900B8, +8) */
#define G_U978D70 0x00978D70u /* i32: 1: houses do not evolve, collapse not counted (0x004539A0) */
#define G_TREES_AVAILABLE 0x00978D98u /* u8: a free tree tile may exist (set by the scan for building type 0x6C, cleared by 0x00486810) */
#define G_REEDS_AVAILABLE 0x00978D9Au /* u8: a free reed tile may exist (set by the scan for building type 0xC3, cleared by 0x00486810) */
#define G_TILE_VISITS 0x00978D9Cu /* u32 per tile: ten 3-bit walker visit counters (chunk 148) */
#define G_GRID_MOISTURE 0x00A509BCu /* u8 per tile: moisture grid (chunk 170) */
#define G_GRID_DAMAGE 0x00A9CC1Cu /* i16 per tile: damage counter (chunk 18) */
#define G_FLOOD_STATE 0x00D386D4u /* i32 flood state 1..3 (0x004894A0; tick.md) */
#define G_UE95184 0x00E95184u /* i32 per-category message counter 4 (chunk 82 at 0x00E95174): message 0x72 once (0x00491E60) */

/* @bin 0x0053DFF0 image groups (G_DAY2_REDRAW_GROUPS + 8 * n: the group's first image id,
 * docs/formats/graphics.md) also drawn outside core/figure */
#define G_IMAGE_GROUP_157 0x00F5609Cu
#define G_IMAGE_GROUP_323 0x00F565CCu
#define G_IMAGE_GROUP_348 0x00F56694u
#define G_IMAGE_GROUP_353 0x00F566BCu
#define G_IMAGE_GROUP_395 0x00F5680Cu
#define G_IMAGE_GROUP_396 0x00F56814u
#define G_IMAGE_GROUP_714 0x00F57204u

/* @bin fields of other records and tables, read by core/figure */
#define CITY_GOOD_SETTING 0x2826u /* i16[36] per good: the city's setting for the good, 1 / 3 and 2 tested (0x004569A5, 0x0048B778) */
#define CITY_U286E 0x286Eu        /* i16[36] per good: times 100, the amount kept when the setting is 2 (last_figure.c) */
#define CITY_U2A88 0x2A88u        /* i32[36] per good: non-zero tested (0x004571AA, 0x0048B778) */
#define CITY_U4860 0x4860u        /* i32[36] per good: counted with CITY_GOOD_STOCK by the storage code (0x0045B436) */
#define CITY_GOOD_STOCK 0x48F0u   /* i32[36] per good: the player's stock (0x0044D63D, 0x004F5770) */
#define EMPCITY_ROUTE 0x04u       /* u8: trade route id of the empire city (G_EMPIRE_CITIES, save.md section 5) */
#define EMPCITY_U54 0x54u         /* i16: +6 and +8 from here: figure ids (0x004B4E5B) */
#define MONOBJ_ROUND_ROBIN 0x08u  /* i32: the monument object's round-robin counter, its limit at +4 (0x004AB0AD) */
#define MONOBJ_U4C 0x4Cu          /* u8 := 0 when a figure delivers to or leaves the monument (actions_d2.c) */
#define CITY_U2C64 0x2C64u /* i32 += a load's value (0x00445980 caller, helpers_xb.c) */
#define CITY_U2C6C 0x2C6Cu /* i32 += a value (helpers_xb.c) */
#define CITY_U2C84 0x2C84u /* i32 += an amount (actions_b2.c) */
#define CITY_U42C4 0x42C4u /* i32 index tested in the empire city's flags (helpers_xb.c) */
#define CITY_U42C8 0x42C8u /* i32 counter (helpers_xb.c) */
#define CITY_U42E0 0x42E0u /* i32 += amount; at least 500 tested for a tutorial (0x0045FECD) */
#define CITY_U4418 0x4418u /* i32 counter (0x004B14FD) */
#define CITY_U442C 0x442Cu /* i32 cleared by the figure update (figure.c), tested with CITY_U4728 (formation.c) */
#define CITY_U4450 0x4450u /* i32 an amount taken by a percentage per difficulty (actions_b2.c) */
#define CITY_U4460 0x4460u /* base of a per-player table (last_figure.c) */
#define CITY_U447A 0x447Au /* i16 animals counted (actions_b4.c) */
#define CITY_U448C 0x448Cu /* i16 counter (actions_b2.c) */
#define CITY_U4490 0x4490u /* i32 the good trade_good_open_13 rotates through (0x00EA5D00) */
#define CITY_U4494 0x4494u /* i32 the good trade_good_open_24 rotates through (0x00EA5D04) */
#define CITY_U44AC 0x44ACu /* i32 a building id (helpers_xb.c) */
#define CITY_U4544 0x4544u /* i32 := 10 (actions_b6.c), <= 0 tested with G_ENEMY_FORMATIONS (actions_d.c) */
#define CITY_U4594 0x4594u /* i32 non-zero: the soldiers go to formation 0's point (actions_b6.c) */
#define CITY_U45A0 0x45A0u /* i32 x of a point (formation.c) */
#define CITY_U45A4 0x45A4u /* i32 y of that point */
#define CITY_U45E4 0x45E4u /* i32 += amount (helpers_xb.c) */
#define CITY_U4634 0x4634u /* i32 boats counted (0x004ADF74) */
#define CITY_U4644 0x4644u /* i32 counter, cleared by the figure update (figure.c) */
#define CITY_U4658 0x4658u /* i32 counter (helpers_xa.c) */
#define CITY_U4665 0x4665u /* u8 formation counter (formations_rebuild) */
#define CITY_U4666 0x4666u /* u8 formation counter (formations_rebuild) */
#define CITY_U466A 0x466Au /* u8 formation counter (formations_rebuild) */
#define CITY_U466B 0x466Bu /* u8 formation counter (formations_rebuild) */
#define CITY_U466C 0x466Cu /* u8 formation counter (formations_rebuild) */
#define CITY_U466D 0x466Du /* u8 formation counter (formations_rebuild) */
#define CITY_U466F 0x466Fu /* u8 += a formation's FM_COUNT (formations_rebuild) */
#define CITY_U4678 0x4678u /* i32 counter (0x0048EA3A company members, 0x004A70C3 warships) */
#define CITY_U467F 0x467Fu /* u8 counter by figure type (actions_b6.c) */
#define CITY_U4680 0x4680u /* u8 counter for figure types 0x30, 0x34 (actions_b6.c) */
#define CITY_U4694 0x4694u /* i32 += 1 when a figure of type 0x39 is deleted (0x004B49A0) */
#define CITY_U4698 0x4698u /* formations of type 0xD counted from here (formations_count) */
#define CITY_U4700 0x4700u /* i32 flag (actions_b1.c) */
#define CITY_U4704 0x4704u /* i32 cleared by the figure update (figure.c) */
#define CITY_U4728 0x4728u /* i32 counter (actions_b6.c), cleared by the figure update (figure.c) */
#define CITY_U4760 0x4760u /* i32 counter (actions_b2.c) */
#define CITY_U4764 0x4764u /* i32 += an amount (actions_b2.c) */
#define CITY_U4784 0x4784u /* i32 += an amount (actions_b2.c, helpers_xa.c) */
#define CITY_U47A0 0x47A0u /* i32 x of a point, -1: none (actions_d.c) */
#define CITY_U47A4 0x47A4u /* i32 y of that point */
#define CITY_U47B0 0x47B0u /* i32 += amount (helpers_xb.c) */
#define CITY_U4980 0x4980u /* i32 -= 1 when a figure of type 0x62 is deleted, not below 0 (0x004B4B50) */
#define CITY_U49A0 0x49A0u /* i32 -= 1 when a figure is deleted (0x004B49C0) */
#define BLD_DOCK_FIGURE 0x54u /* i16: docks: the boat figure using it (0x004950F6, the dock actions) */
#define BLD_U54 0x54u         /* i16: of the building figure type 0x6B goes to: kept at most G_LOCUSTS - 1000 (0x004B0576) */
#define BLD_MON_U56 0x56u     /* u8: monument sites: with BLD_U57 x 6, a joining worker's FIG_CC_DEST_Y (0x004F2410) */
#define BLD_U9E 0x9Eu         /* i16[4]: := 0 when a figure of type 0x0F is deleted (0x004B4980); delays counting down (0x00456DEB) */

#endif
