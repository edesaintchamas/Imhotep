/* Imhotep - pending names of package p4 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P4_COMMAND_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P4_COMMAND_H

/* ---- city record fields (owner: city.h, P3). Evidence: docs/formats/save.md §5 "City data" and the
 * core/command ports named on each line; a field whose meaning is not established is CITY_U<offset>.
 * Notes for the merge: the ports also call CITY_U4478 the temple complex (i16, command/menu.c),
 * CITY_U472C its building id and CITY_U28B6 the industry switch (command/trade.c). ---- */
#define CITY_HOUSE_ROOM_LEFT 0x38u     /* i32: house room left (save.md; place_notice_4e2fb0 0x004E2FB0) */
#define CITY_U2728 0x2728u             /* i32: tested with CITY_HOUSE_ROOM_LEFT by 0x004E2FB0 */
#define CITY_MIGRATION_PERCENT 0x272Cu /* i32: migration % from sentiment (save.md; < 0x50 by 0x004E2FB0) */
#define CITY_IMMIGRANTS_MONTH 0x273Cu  /* i32: immigrants this month (save.md; < 5 by 0x004E2FB0) */
#define CITY_U2740 0x2740u             /* i32: 0, 1 or other, read by 0x004E2FB0 */
#define CITY_STORAGE_CURSOR 0x2752u    /* i16: the storage yard the goods are taken after (0x0045B4D0, 1..3999) */
#define CITY_PALACE_X 0x2788u          /* i16: palace x (save.md "unique buildings"; types 0xBB..0xBD, 0x00485AB7) */
#define CITY_PALACE_Y 0x278Au          /* i16 */
#define CITY_PALACE_OFFSET 0x278Cu     /* i32: its grid offset */
#define CITY_TRADE_STATUS 0x2826u      /* i16[36] per resource: trade status, 3 import, 4 export (save.md; 0x00443330) */
#define CITY_TRADE_LIMIT 0x286Eu       /* i16[36] per resource: trade threshold (save.md; 0x00559AF0) */
#define CITY_FOOD_TOTAL 0x296Cu        /* i32: total food (save.md; 0x004E2DD0) */
#define CITY_GRAVE_GOODS_RESOURCE 0x297Cu /* i32: the resource of the grave goods window (0x0040CD70) */
#define CITY_GRAVE_GOODS_SENT 0x2980u  /* i32: the goal counter when the window opened (restored on cancel, 0x0040B380) */
#define CITY_GRAVE_GOODS_AMOUNT 0x2984u /* i32: the amount chosen in the window (0x0040B3B0 .. 0x0040B4B0) */
#define CITY_STOCKPILE 0x2A88u         /* i32[36] per resource: stockpiled (save.md, assumed; toggled by 0x00559D70) */
#define CITY_LABOUR_PRIORITY 0x2B3Cu   /* i32 per labour category (CITY_LABOUR_STRIDE): its priority (0x0040A730) */
#define CITY_EMPLOYED 0x2BF4u          /* i32: employed (save.md; 0x0045A8C0) */
#define CITY_LABOUR_SHORTAGE 0x2C04u   /* i32: labour shortage (save.md; >= 10 by 0x004E2D30) */
#define CITY_WAGES_SUM 0x2C14u         /* i32: the wages accumulator (save.md; 0x0045A8C0) */
#define CITY_WAGES_THIS_YEAR 0x2C18u   /* i32: wages this year (save.md; := CITY_WAGES_SUM by 0x0045A8C0) */
#define CITY_U2C3C 0x2C3Cu             /* i32: summed with CITY_U2C40 into CITY_TAXES_THIS_YEAR (0x00459E1A) */
#define CITY_U2C40 0x2C40u             /* i32 */
#define CITY_TAXES_THIS_YEAR 0x2C4Cu   /* i32: taxes this year (save.md; 0x00459E1A) */
#define CITY_TAX_BASE_LOW 0x2C54u      /* i32: tax base of the houses below level 0xE (0x00459D20) */
#define CITY_TAX_BASE_HIGH 0x2C5Cu     /* i32: tax base of the houses from level 0xE (0x00459D20) */
#define CITY_U2C74 0x2C74u             /* i32: copied into CITY_INTEREST_THIS_YEAR by 0x0045A930 */
#define CITY_INTEREST_THIS_YEAR 0x2C7Cu /* i32: interest this year (save.md; 0x0045A930) */
#define CITY_CONSTRUCTION_THIS_YEAR 0x2C8Cu /* i32: construction this year (save.md; 0x004E9680, 0x00485265) */
#define CITY_SALARY_THIS_YEAR 0x2C94u  /* i32: salary this year (save.md; 0x0045A970) */
#define CITY_SALARY 0x2C98u            /* i32: salary (save.md; 0x0040AF30) */
#define CITY_SALARY_RANK 0x2C9Cu       /* i32: salary rank (save.md; 0x0040AF30) */
#define CITY_U2CA0 0x2CA0u             /* i32: copied into CITY_SALARY_THIS_YEAR by 0x0045A970 */
/* house evolution blocker counters (save.md: +0x4304..+0x435C, +0x4388, +0x474C; read by 0x004E9500) */
#define CITY_U430C 0x430Cu
#define CITY_U4310 0x4310u
#define CITY_U4314 0x4314u
#define CITY_U4324 0x4324u
#define CITY_U4328 0x4328u
#define CITY_U433C 0x433Cu
#define CITY_U4350 0x4350u
#define CITY_U4354 0x4354u
#define CITY_U4358 0x4358u
#define CITY_U4388 0x4388u
#define CITY_U474C 0x474Cu
/* the summary codes of 0x004E9500 (meanings assumed in the port: what the advisors show first) */
#define CITY_U4410 0x4410u             /* i32: from CITY_SENTIMENT_LOW; also read by 0x004E2FB0 */
#define CITY_U441C 0x441Cu             /* i32: 0..4, the largest of four counters */
#define CITY_U4420 0x4420u             /* i32: 0..3 */
#define CITY_U4424 0x4424u             /* i32: 0..3 */
#define CITY_U4428 0x4428u             /* i32: 0..2 */
#define CITY_RATING_EXPLANATION 0x4434u /* i32: the first rating explanation id (save.md +0x4434..; 0x00426B00) */
#define CITY_RANK 0x444Cu              /* i32: player rank (save.md; command/menu.c) */
#define CITY_SAVINGS 0x4450u           /* i32: personal savings (save.md; 0x0040AF90) */
#define CITY_GIFTS_THIS_YEAR 0x4458u   /* i32: gifts this year (save.md; the donation 0x0040AF90) */
#define CITY_DONATION 0x445Cu          /* i32: the amount to donate (0x0040AFF0, 0x0040B0B0) */
#define CITY_U4486 0x4486u             /* i16: := 1 with a palace (0x00485AB7); tested for 0x54..0x57, 0xBB..0xBD */
#define CITY_U448A 0x448Au             /* i16: := 2 with a temple complex (0x00485AB7) */
#define CITY_U44A0 0x44A0u             /* i32: += 1 per cheat (0x0041D010), += 1000 by "Treasure Chest" */
#define CITY_RECRUITER_X 0x44A4u       /* i16: recruiter (save.md; type 0x5F, 0x00485AB7) */
#define CITY_RECRUITER_Y 0x44A6u       /* i16 */
#define CITY_RECRUITER_OFFSET 0x44A8u  /* i32: its grid offset */
#define CITY_U44B0 0x44B0u             /* i32: := 1 with the recruiter (0x00485AB7) */
#define CITY_KEEP_GOVERNING 0x44D0u    /* i32: := 1 by "keep governing" (0x004EA770) */
#define CITY_EXTRA_MONTHS 0x44D4u      /* i32: += 24 / 60 by 0x004EA770; the salary is fixed while > 0 */
#define CITY_EXTRA_MONTHS_GIVEN 0x44D8u /* i32: := the months given (0x004EA770) */
#define CITY_FESTIVAL_GOD_CHOSEN 0x44F0u /* i32: the festival window's god (0x0040AC50) */
#define CITY_FESTIVAL_SIZE_CHOSEN 0x44F4u /* i32: the festival window's size (0x0040AC80) */
#define CITY_FESTIVAL_COST 0x4504u     /* i32[3] by size: festival costs (save.md; 0x0040AB9D) */
#define CITY_FESTIVAL_BEER 0x4510u     /* i32: the beer a size-3 festival takes (0x0040AC0D) */
#define CITY_U4514 0x4514u             /* i32: != 0 refuses a size-3 festival (0x0040AC80) */
#define CITY_U452C 0x452Cu             /* i32: months, with CITY_U4528 (0x0040AB6A) */
#define CITY_GIFT_SIZE 0x4558u         /* i32: the gift's size 0..2 (0x0040B140) */
#define CITY_U455C 0x455Cu             /* i32: := 0 by a gift (0x0046EED7) */
#define CITY_GIFTS 0x4560u             /* i32: the gifts counter (0x0046EE10) */
#define CITY_GIFT_COUNTS 0x4568u       /* i32[3] per size, each reset at 4 (0x0046EED7) */
#define CITY_GIFT_COST 0x4574u         /* i32[3] by size: gift costs (0x0040B140, 0x0046EDD0) */
#define CITY_U45AC 0x45ACu             /* i32: CITY_FOOD_TOTAL compared with its half (0x004E2DD0) */
#define CITY_FOOD_EATEN 0x45DCu        /* i32: food eaten this month (save.md; 0x004E3330) */
#define CITY_TAX_ESTIMATE 0x45ECu      /* i32: estimated tax (save.md; 0x00459E1A) */
#define CITY_TYPE32_X 0x45F4u          /* i16: the type 0x32 building (save.md "unique buildings"; 0x00485AB7) */
#define CITY_TYPE32_Y 0x45F6u          /* i16 */
#define CITY_TYPE32_OFFSET 0x45F8u     /* i32: its grid offset */
#define CITY_TYPE32_BUILT 0x4600u      /* i32: := 1 with it (0x00485AB7); the ghost tests it (0x004705E3) */
#define CITY_U4634 0x4634u             /* i32: summed with CITY_U4644 (> 3) by 0x004E2FB0 */
#define CITY_U4644 0x4644u             /* i32 */
#define CITY_TRANSPORTS 0x4664u        /* u8: the player's transport ships (type 0x4D, 0x004B8AA0) */
#define CITY_U4665 0x4665u             /* u8: ships for kind-2 requests (0x0044D520, 0x0040A450) */
#define CITY_U4666 0x4666u             /* u8: legions for requests (0x0040A450) */
#define CITY_WARSHIPS 0x4668u          /* u8: the player's warships (type 0x4E, 0x004B8AA0) */
#define CITY_U4669 0x4669u             /* u8: warships with figure +0x02 set (0x004B8AA0) */
#define CITY_U466A 0x466Au             /* u8: read by 0x0044D520 */
#define CITY_U466B 0x466Bu             /* u8: read by 0x0044D520 */
#define CITY_WAGES_ESTIMATE 0x46A0u    /* i32: the year's estimated wages (0x0045A8C0) */
#define CITY_MONUMENT_CODE 0x4730u     /* i32[3]: the scenario's monuments placed, their kind (0: not placed; 0x004F61E0) */
#define CITY_MONUMENT_ID 0x473Cu       /* i32[3]: their building (0x004F61E0) */
#define CITY_REQUESTS_THIS_YEAR 0x4758u /* i32: requests and festivals this year (save.md; 0x0040AB9D, 0x0044C2CC) */
#define CITY_U476C 0x476Cu             /* i32: the divisor of a food ratio (0x00447530) */
#define CITY_U4770 0x4770u             /* i32 */
#define CITY_U4778 0x4778u             /* i32: the divisor of a second food ratio (0x00447530) */
#define CITY_U477C 0x477Cu             /* i32 */
#define CITY_FESTIVAL_SQUARE_X 0x47A0u /* i32: the festival square's x, -1 none (0x00440ED0) */
#define CITY_FESTIVAL_SQUARE_Y 0x47A4u /* i32 */
#define CITY_DIFFICULTY_COPY 0x47A8u   /* i32: difficulty copy (save.md; 0x004EA840) */
#define CITY_U4814 0x4814u             /* i16: := 1 with the festival square (0x00440ED0) */
#define CITY_STORAGE_STOCK 0x48F0u     /* i32[36] per resource: storage-yard stock (save.md; 0x0045AAE0) */
#define CITY_MANSION_BUILT 0x499Cu     /* i32: mansion built (save.md; types 0x4D..0x4F) */
#define CITY_U49A4 0x49A4u             /* u8 per event +0x04: := 1 by the reply 0x0044EE30 */
#define CITY_U49CC 0x49CCu             /* u8: := 0 when an advisor opens (0x0040A0E0) */
#define CITY_ACADEMY_EXISTS 0x49D0u    /* i32: academy exists (save.md; type 0x5E) */
#define CITY_U49D4 0x49D4u             /* i16: copied into CITY_U433A with type 0xD2 (0x00485AB7) */

/* ---- message record (owner: message.h, P3) ---- */
#define MSG_REPLIES 0x26u              /* i16: the replies a message offers (0x004C5BF0 copies it to G_MESSAGE_WINDOW_REPLIES) */

/* ---- globals of other areas ---- */
/* P6 (g_scenario.h): the scenario info (chunk 75 at 0x007848D8) and other scenario data */
#define G_SCENARIO_FUNDS 0x007848F8u   /* @bin 0x004DE98C i32: the scenario's initial funds */
#define G_ALLOWED_BUILDINGS 0x00784CE0u /* @bin 0x00404350 i16[100]: the scenario's allowed buildings (scenario info +0x408, save.md upgrade 117) */
#define G_U784DB0 0x00784DB0u          /* @bin 0x00404350 i16[5]: allow the temple complexes 0x41..0x45 */
#define G_STR_BACKSLASH 0x005761D0u    /* @bin 0x004183D0 "\" */
#define G_STR_MAPS_DIR 0x0057626Cu     /* @bin 0x004183D0 "maps\" */
#define G_STR_AUTOSAVE_REPLAY 0x0057BFC4u /* @bin 0x004183D0 "autosave_replay.sav" */
#define G_CARRY_GROUPS 0x006FDCBCu     /* @bin 0x004D3C40 3 records of 12 bytes (u8, i32, u8): the waiting groups of G_CARRY */
#define G_CARRY_COPY_GROUPS 0x006FDCE4u /* @bin 0x004D3C40 the same in G_CARRY_COPY (the soldiers waiting, map_add.c) */
/* P3 (g_city.h) */
#define G_RATING_LOWEST_INDEX 0x00705738u /* @bin 0x00426B00 i32: the index of the lowest rating (core/city/month_ratings.c) */
#define G_INVASION_COUNTER 0x00775900u /* @bin 0x00446A30 i16: invasion id counter (save.md chunk 126) */
#define G_FROGS_COUNTDOWN 0x0078D7A8u  /* @bin 0x004023A0 i32: Plague of Frogs countdown, save.md chunk 105 */
#define G_RAMP_LAST_TILE 0x00EAAE9Cu   /* @bin 0x004ED003 i32: the last tile of the ramp walk 0x004ED360 */
#define G_UEADB28 0x00EADB28u          /* @bin 0x004F6AD0 i32: := houses_at_least(G_U784C38) */
/* P1 (g_figure.h) */
#define G_STORAGE_SETTINGS 0x007900C0u /* @bin 0x00474B6E records of 0xC4 bytes by building +0xC1 (core/figure/helpers_xb.c) */
#define G_FORMATION_COUNT 0x00BCB9B4u  /* @bin 0x0047026B i32: the formations in use (forts; core/figure/formation.h) */
/* P2 (g_building.h) */
#define G_GATEHOUSE_PART_IDS 0x005DF78Cu /* @bin 0x004BFA50 i32[4] per rotation: the gatehouse parts */
#define G_GATEHOUSE_PARTS 0x005DF814u  /* @bin 0x004BFA50 16-byte records: +0 size, +4 / +8 offsets by rotation */
#define G_TEMPLE_COMPLEX_ANCHORS 0x005E00CCu /* @bin 0x004C3730 (x, y) pairs by rotation and view / 2 */
#define G_UBCA32C 0x00BCA32Cu          /* @bin 0x0043DE6A u8: the water-lift ghost's flag (core/building/day_water.c) */
#define G_UNDO_COUNTDOWN 0x00BC9688u   /* @bin 0x00485200 i32: := 500, the undo's countdown (core/city/day_basic.c) */
/* P5 (g_map.h) */
#define G_RAMP_OFFSETS 0x005F5D08u     /* @bin 0x004ECEA0 i32 per monument kind: the ramp tile's offset */
#define G_UBCA328 0x00BCA328u          /* @bin 0x0046FEF0 i32: := 1 after a water lift drag (core/map/terrain_roads.c) */
/* P7 (g_frame.h) */
#define G_U6FDC50 0x006FDC50u          /* @bin 0x004DE080 i32: Alt+D toggle, widens the editor's camera bounds */
#define G_U70E418 0x0070E418u          /* @bin 0x00430530 i32: := 0 by the view table build (core/frame/view.c) */
#define G_OVERLAY_PREV 0x00E94E10u     /* @bin 0x004023A0 i32: the previous overlay (core/frame/wndproc.c) */

#endif
