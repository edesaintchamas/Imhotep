/* Imhotep - named globals: core/state/layout/g_scenario.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P6). */
#ifndef IMHOTEP_STATE_LAYOUT_G_SCENARIO_H
#define IMHOTEP_STATE_LAYOUT_G_SCENARIO_H

/* -- harvested names checked against their uses in core/boot, core/io, core/newgame (phase 10.3); @bin evidence: the file named in each comment -- */
#define G_STR_SAV_PATTERN 0x005761C4u /* core/newgame/flow.c: "\\*.sav" */
#define G_TREASURY_PCT 0x005D1960u /* core/newgame/start.c: i32[5] by difficulty */
#define G_KINGDOM_BY_DIFF 0x005D1988u /* core/newgame/start.c: i32[5] by difficulty */
#define G_MAP_W 0x005D2A10u /* present/ui/empire/empire.h: i32 the map picture's size */
#define G_MAP_H 0x005D2A14u /* present/ui/empire/empire.h */
#define G_MAP_X 0x005D2A18u /* present/ui/empire/empire.h: i32 the map's place on the screen: left */
#define G_MAP_Y 0x005D2A1Cu /* present/ui/empire/empire.h: i32 top */
#define G_MAP_BOTTOM 0x005D2A20u /* present/ui/empire/empire.h: i32 room below */
#define G_STR_SAVE_DIR 0x005E0494u /* core/newgame/flow.c: "save\\" */
#define G_MAP_SCHEMA 0x005E17D8u /* core/newgame/map_load.c: @bin 0x004D4AE0 the MAPS schema (24 records of 20 bytes) */
#define G_HERD_RECORDS 0x005E4C20u /* core/newgame/figures.c: @bin 0x004BB407 records of 0x30 bytes */
#define G_STR_DAT 0x005F80E8u /* core/newgame/flow.c: ".dat" */
#define G_STR_FAMILY_PATTERN 0x005F80F0u /* core/newgame/flow2.c: "Save\*.dat" */
#define G_CRT_RAND_STATE 0x00604950u /* core/random/crt_rand.h: @bin 0x00562348 */
#define G_NO_GROUND 0x006DE515u /* present/city/city.h: u8: the overlay ground replaces the ground pass */
#define G_ADVISOR_AVAILABLE 0x006FDC88u /* core/command/advisor.c: u8 per advisor: the mission offers it */
#define G_MISSION_PICTURE_KIND 0x006FDCB0u /* present/ui/empire/mission.c: i32 window 4: 0 mission start, 1 plain, 2 victory [single+clash] */
#define G_FORMATIONS_KEEP 0x006FDCB4u /* core/newgame/start.c: u8 */
#define G_CARRY 0x006FDCB8u /* core/newgame/start.c: 10 dwords: the carry-over of the previous mission */
#define G_CARRY_COPY 0x006FDCE0u /* core/newgame/start.c: 10 dwords: its copy read by 0x00418BF0 */
#define G_CAMPAIGN_TABLE 0x006FDD08u /* core/frame/wndproc.c: the campaign progress object (thiscall) */
#define G_LAST_SAVE 0x00700998u /* core/newgame/flow.c: the last saved game's path (empty: none) */
#define G_CHOICE 0x007027A4u /* core/newgame/flow2.c: the mission choice made on screen 0x34: 1, 2 (0: none) */
#define G_CAMPAIGN_PERIOD 0x007027ACu /* present/ui/frontend/campaign.c: i32 the chosen period [generic] */
#define G_PERIOD_REACHED 0x007027B0u /* present/ui/frontend/campaign.c: i32 */
#define G_CAMPAIGN_KIND 0x007027B4u /* core/newgame/start.c: u8: 0 custom, 1 Pharaoh, 2 Cleopatra, 3 mission list */
#define G_CHEAT_TEXT 0x007027ECu /* present/ui/cityframe.c: char[] the typed cheat (edit field 7) */
#define G_SOUND_SLOTS 0x00702900u /* core/frame/sound.c [generic] */
#define G_SCENARIO_KIND 0x00704DACu /* present/ui/frontend/scenario.c: i32 [generic] */
#define G_SCENARIO_LIST_MODE 0x00704DB0u /* core/newgame/flow.c: 0: .map files, 1: the missions of the campaign table */
#define G_SCENARIO_ROWS 0x00704DB4u /* present/ui/frontend/scenario.c: i32 [generic] */
#define G_SCENARIO_FIRST 0x00704DB8u /* present/ui/frontend/scenario.c: i32 [single+clash] */
#define G_SCENARIO_LIST_ROW 0x00704DBCu /* core/newgame/start.c: i32: the chosen row */
#define G_HOVER_BUTTON 0x007056B8u /* present/sidebar/tooltip.c: i32 */
#define G_HOVER_3 0x007056C0u /* present/ui/ui.h: i32 a third one */
#define G_HOVER_2 0x007056C4u /* present/ui/ui.h: i32 a second one */
#define G_HOVER 0x007056C8u /* present/sidebar/tooltip.c: i32 */
#define G_CV_VIEW_BOTTOM 0x0070E430u /* present/city/city.h: i32 the viewport (clip) */
#define G_CV_VIEW_TOP 0x0070E434u /* present/city/city.h */
#define G_CV_VIEW_RIGHT 0x0070E438u /* present/city/city.h */
#define G_CV_VIEW_LEFT 0x0070E43Cu /* present/city/city.h */
#define G_CORNER_H4 0x0070E440u /* present/city/overlay.c */
#define G_CORNER_H3 0x0070E444u /* present/city/overlay.c */
#define G_CORNER_W4 0x0070E450u /* present/city/overlay.c */
#define G_CORNER_W3 0x0070E454u /* present/city/overlay.c */
#define G_SCROLL_Y 0x0077590Cu /* present/ui/empire/empire.h: i32 map scroll (pixels) */
#define G_SCROLL_X 0x00775910u /* present/ui/empire/empire.h: i32 */
#define G_ROUTES 0x00776FB0u /* core/io/save_load.c: chunk 143, 50 x 324 */
#define G_ROUTES_END 0x0077AEF8u /* core/newgame/map_load.c */
#define G_EMPIRE_OBJECTS 0x0077F748u /* core/city/route_points.c: empire cities, records of 0x62 bytes, to 0x007843D8 */
#define G_EMPIRE_OBJECTS_END 0x007843D8u /* core/city/route_points.c */
#define G_MONUMENT_MODE 0x00784CBCu /* core/newgame/graphics.c: @bin 0x004F4520 byte */
#define G_BUILDING_HIGHEST_EVER 0x0078D78Cu /* core/city/day_buildings.c */
#define G_TOTAL_DAYS 0x0078D7C8u /* core/sim/tick.c: chunk 38 */
#define G_NOT_FOUND 0x00D3B1B3u /* present/ui/frontend/files.c: u8 frames of the "not found" title */
#define G_FILES_MODE 0x00D3B1C0u /* present/ui/frontend/files.c: u8 0 save, 1 load, 2 delete [generic] */
#define G_CARET_Y 0x00E20794u /* present/draw/text.c */
#define G_TEXTEDIT_TEXT 0x00E2079Cu /* present/draw/textedit.c [generic] */
#define G_CARET_POS 0x00E207ECu /* present/draw/text.c: i32 per field: the caret's character index */
#define G_NO_PUNCT 0x00E2083Cu /* present/draw/textedit.c */
#define G_MAX_W 0x00E20850u /* present/draw/textedit.c */
#define G_TEXTEDIT_END 0x00E208A0u /* present/draw/textedit.c [generic] */
#define G_TEXTEDIT_MAX 0x00E208F0u /* present/draw/textedit.c [generic] */
#define G_FONT 0x00E20940u /* present/draw/textedit.c */
#define G_CARET_FIELD 0x00E20990u /* core/newgame/flow2.c: the text box being edited */
#define G_TEXTEDIT_RESULT 0x00E20994u /* present/draw/textedit.c: i32 0, 1 (caret at the end), 2 (field full) [generic] */
#define G_LOG_LINE 0x00E36C90u /* core/newgame/flow.c: the log line buffer (0x004CE6E0; presentation, excluded.py) */
#define G_MAP_NAME 0x00E37078u /* core/newgame/setup.c: char[65] */
#define G_PLAYER_NAME 0x00E38E74u /* core/newgame/flow.c: char[32] */
#define G_STEP_PREV 0x00E38EAAu /* core/newgame/start.c: u8 */
#define G_STEP 0x00E38EABu /* core/newgame/start.c: u8 */
#define G_PLAYED 0x00E38EB8u /* core/newgame/start.c: i32 (only its low byte survives the start) */
#define G_VERSION 0x00E945C0u /* core/newgame/map_load.c: chunk 1 of a map: the file version */
#define G_USE_FILE_SCHEMA 0x00E945C4u /* core/newgame/map_load.c: chunk 2 of a map: != 0, compare with the file's schema */
#define G_QUEUE_END 0x00E950D0u /* present/ui/messages/popup.c */
#define G_PROGRESS_PATH 0x00EADDA0u /* core/newgame/flow.c: char[0x104] */
#define G_FAMILY_DIR 0x00EADEA4u /* present/ui/frontend/files.c: char[0x104] built by 0x005169C0 */
#define G_FAMILY_SCROLL 0x00EADFC8u /* present/ui/frontend/family.c: i32 the first row shown */
#define G_FAMILY_CHOSEN 0x00EADFCCu /* present/ui/frontend/family.c: i32 */
#define G_FONT_SMALL 0x00F56F84u /* present/sidebar/sidebar.c: i32 first image of the small font */
#define G_FONT_9C 0x00F56F9Cu /* present/ui/messages/list.c: i32 a row's font */

/* -- named with the rewrite of core/boot, core/newgame/graphics.c -- */
/* the monument graphics slots (0x004F4520, 0x0053D970; strings of the exe) */
#define G_STR_PYRAMID_DUMMY 0x00579144u /* @bin 0x004F4637 "PyramidDummy" */
#define G_STR_CAESAREUM_DUMMY 0x00579134u /* @bin 0x004F452D "CaesareumDummy" */
#define G_STR_LT_HOUSE_DUMMY 0x00579124u /* @bin 0x004F452D "LtHouseDummy" */
#define G_STR_LIB_MAIN_DUMMY 0x0057922Cu /* @bin 0x004F452D "LibMainDummy" */
#define G_STR_MAUS_CARRYOVER_DUMMY 0x00579110u /* @bin 0x004F452D "MausCarryoverDummy" */
#define G_STR_TOMB_THUT_DUMMY 0x00579100u /* @bin 0x004F452D "TombThutDummy" */
#define G_STR_ABU_SIMBEL_DUMMY 0x005790F0u /* @bin 0x004F452D "AbuSimbelDummy" */
#define G_STR_TOMB_TUT_DUMMY 0x005790E0u /* @bin 0x004F452D "TombTutDummy" */
#define G_STR_TOMB_SETI_DUMMY 0x005790D0u /* @bin 0x004F452D "TombSetiDummy" */
#define G_STR_TOMB_RAM_DUMMY 0x005790C0u /* @bin 0x004F452D "TombRamDummy" */
#define G_STR_BENT_PYRAMID 0x005F763Cu /* @bin 0x004F452D "BENT_PYRAMID" */
#define G_STR_MUDBRICK_PYRAMID 0x005F7628u /* @bin 0x004F452D "MUDBRICK_PYRAMID" */
#define G_STR_STEPPED_PYRAMID 0x005F7618u /* @bin 0x004F452D "STEPPED_PYRAMID" */
#define G_STR_MASTABA 0x005F7610u /* @bin 0x004F452D "MASTABA" */
#define G_STR_PYRAMID 0x005F7608u /* @bin 0x004F452D "PYRAMID" */
#define G_STR_LIB_EXTRA_DUMMY 0x005F75F8u /* @bin 0x004F452D "LibExtraDummy" */
#define G_STR_TOMB_THUT 0x005F75E0u /* @bin 0x004F4637 "TombThut" */
#define G_STR_TOMB_THUT_FIN 0x005F75ECu /* @bin 0x004F4637 "TombThutFin" */
#define G_STR_TOMB_TUT_FIN 0x005F75CCu /* @bin 0x004F4637 "TombTutFin" */
#define G_STR_TOMB_TUT 0x005F75D8u /* @bin 0x004F4637 "TombTut" */
#define G_STR_TOMB_SETI_FIN 0x005F75C0u /* @bin 0x004F4637 "TombSetiFin" */
#define G_STR_TOMB_SETI 0x005F75B4u /* @bin 0x004F4637 "TombSeti" */
#define G_STR_TOMB_RAM 0x005F75ACu /* @bin 0x004F4637 "TombRam" */
#define G_STR_ABU_SIMBEL 0x005760B0u /* @bin 0x004F4759 "AbuSimbel" (core/building/rest_monument.c) */
#define G_STR_CAESAREUM1 0x005F75A0u /* @bin 0x004F4837 "Caesareum1" */
#define G_STR_LT_HOUSE1 0x005E4934u /* @bin 0x004F4837 "LtHouse1" */
#define G_LIB_MAIN_B1 0x00576338u /* @bin 0x004F4837 "LibMainb1" (the G_LIB_MAIN_ series of g_building.h; core/building/rest_monument.c) */
#define G_STR_MAUS_CARRYOVER 0x005F7590u /* @bin 0x004F4837 "MausCarryover" */
#define G_STR_GLORY_OF_PTOLEMY 0x00601840u /* @bin 0x0053D972 "The Glory of Ptolemy" */
#define G_STR_OBELISK_EXTRA 0x006017E0u /* @bin 0x0053D970 "OBELISK_EXTRA" */
#define G_STR_SUN_TEMPLE_EXTRA 0x006017F0u /* @bin 0x0053D970 "SUN_TEMPLE_EXTRA" */
#define G_STR_MAUSOLEUM1_EXTRA 0x00601804u /* @bin 0x0053D970 "MAUSOLEUM1_EXTRA" */
#define G_STR_MAUSOLEUM2_EXTRA 0x00601818u /* @bin 0x0053D970 "MAUSOLEUM2_EXTRA" */
#define G_STR_MAUSOLEUM0_EXTRA 0x0060182Cu /* @bin 0x0053D970 "MAUSOLEUM0_EXTRA" */
#define G_EXTRA_SLOT_KIND 0x00F55BACu /* @bin 0x0053D970 i32: the extra slot 9's kind (2 obelisk, 3 sun temple, 4 mausoleum, else sphinx) */

/* -- named with the rewrite of core/io/save_load.c, core/newgame/reset.c -- */
#define G_UEA1820 0x00EA1820u /* @bin 0x004E7E20 i32: := (u8 CITY_ADDR(1) + CITY_U03 != 0); := 0 by 0x004E7E10 */
#define G_UEA186C 0x00EA186Cu /* @bin 0x004E7D10 u16[2] (chunk 29): [0] := 0 by the init; its byte +1 := 1 by 0x00540A10 */
#define G_UBCD5F4 0x00BCD5F4u /* @bin 0x004B4760 i32: := 0 with the figure records (next to G_FIGURE_HIGHEST) */
#define G_U78D3EC 0x0078D3ECu /* @bin 0x00451BE0 i32[0x33]: cleared with the date */
#define G_U78D4B8 0x0078D4B8u /* @bin 0x00451BE0 i32[0x33]: cleared with the date */
#define G_U7028F0 0x007028F0u /* @bin 0x0041DF60 i32: := 0 */
#define G_U7028F4 0x007028F4u /* @bin 0x0041DF60 i32: := 0 (also present/ui/dispatch.c) */
#define G_U70E468 0x0070E468u /* @bin 0x004303A0 i32: view geometry, argument 0 - argument 4 */
#define G_U70E46C 0x0070E46Cu /* @bin 0x004303A0 i32: view geometry, argument 1 (as G_VIEW_FIRST_ROW) */
#define G_U70E470 0x0070E470u /* @bin 0x004303A0 i32: view geometry, argument 0 */
#define G_U5E44A8 0x005E44A8u /* @bin 0x004E2AB0 five 8-byte entries: +0 i32 := (screen width - 180) / 2, +4 i16 copied to the notice's +4 */
#define G_UE38E50 0x00E38E50u /* @bin 0x00540B50 i32: := 0 */
#define G_VIEW_BOOKMARKS 0x00E94DDCu /* @bin 0x00416B10 chunk 130: map view bookmarks, 4 x {position, i32} (save.md §3) */
#define G_SHIPWRIGHT_QUEUES_SAVED 0x009763B8u /* @bin 0x004D0A40 chunk 140: the shipwright queues as saved, 10 x 0x334 (save.md §3) */
/* @bin 0x004D41E0 the monument work-step tables kept over a load (save.md §2, §3), records of G_WORKSTEP_STRIDE bytes */
#define G_WORKSTEPS_OBELISK_SMALL 0x005E45B0u /* @bin 0x004E5B90 chunk 171: small obelisk (0xCF), 10 records */
#define G_WORKSTEPS_OBELISK_LARGE 0x005E46A0u /* @bin 0x004E5B90 chunk 172: large obelisk (0xD0), 18 records */
#define G_WORKSTEPS_SPHINX 0x005C1140u /* @bin 0x00420350 chunk 146: Sphinx (0xD2), 22 records */
#define G_WORKSTEPS_SUN_TEMPLE 0x005C1350u /* @bin 0x00420240 chunk 149: Sun Temple, 13 records */
#define G_WORKSTEP_STRIDE 24u
/* game-time countdowns (save.md §3) */
#define G_FROGS_COUNTDOWN 0x0078D7A8u /* @bin 0x004E70B0 i32 chunk 105: Plague of Frogs countdown */
#define G_PYRAMID_CURSE_COUNTDOWN 0x0078D7A4u /* @bin 0x004F3AA0 i32 chunk 106: god-curse countdown from Pyramid progress (flag G_GOD_CURSE_ON) */
#define G_BLOOD_RIVER_DURATION 0x0078D7ACu /* @bin 0x004E7200 i32 chunk 107: River of Blood duration */
#define G_MUMMY_ATTACK_PARAM 0x0078D798u /* @bin 0x004E7AC0 i32 chunk 173: mummy-attack parameter (default 10) */

/* -- named with the rewrite of core/newgame/flow.c, flow2.c, figures.c, setup.c -- */
/* strings of the exe (the text in the comment) */
#define G_STR_MAPS_PATTERN 0x00576260u /* @bin 0x0041F870 "maps\*.map" */
#define G_STR_MAPS_DIR 0x0057626Cu /* @bin 0x004DE631 "maps\" */
#define G_STR_MAP 0x00576274u /* @bin 0x004DE600 "map" (the extension) */
#define G_STR_DELIM_PTR 0x0057BE28u /* @bin 0x0041AE40 char *: -> ",=" (0x0057BFAC), the campaign file's delimiters */
#define G_STR_COMMA_EQUALS 0x0057BFACu /* @bin 0x0041AE40 ",=" */
#define G_STR_CHOICE_NOT_FOUND 0x0057C064u /* @bin 0x0041B5C9 "%s mission %d: choice %d not found" */
#define G_STR_UNRECOGNIZED_LINE 0x0057C088u /* @bin 0x0041AE40 "unrecognized line" */
#define G_STR_MAX_MISSIONS_EXCEEDED 0x0057C09Cu /* @bin 0x0041B419 "max missions exceeded" */
#define G_STR_MISSION_NAME_TOO_LONG 0x0057C0B4u /* @bin 0x0041B419 "mission name too long" */
#define G_STR_BAD_TEXT_ID 0x0057C0CCu /* @bin 0x0041B27C "bad text id" */
#define G_STR_BAD_Y_VALUE 0x0057C0D8u /* @bin 0x0041B2F7 "bad y value" */
#define G_STR_BAD_X_VALUE 0x0057C0E4u /* @bin 0x0041B2F7 "bad x value" */
#define G_STR_TOO_MANY_CHOICES 0x0057C0F0u /* @bin 0x0041B2F7 "too many choices" */
#define G_STR_BAD_BACKGROUND_ID 0x0057C104u /* @bin 0x0041B27C "bad background id" */
#define G_STR_BAD_MERGE_ID 0x0057C118u /* @bin 0x0041B088 "bad merge id" */
#define G_STR_BAD_PATH_ID 0x0057C128u /* @bin 0x0041B088 "bad path_id" */
#define G_STR_BAD_TRANSITION_TEXT_ID 0x0057C134u /* @bin 0x0041B088 "bad transition text id" */
#define G_STR_BAD_INTRO_ID 0x0057C14Cu /* @bin 0x0041B088 "bad intro id" */
#define G_STR_BAD_MISSION_ID 0x0057C15Cu /* @bin 0x0041B088 "bad mission id" */
#define G_STR_FAILED_TO_ADD_MISSION_LINK 0x0057C16Cu /* @bin 0x0041B088 "failed to add mission link" */
#define G_STR_BAD_MISSION_NAME 0x0057C188u /* @bin 0x0041B088 "bad mission name" */
/* the campaign file's section names and keywords (0x0041AE40, in the order of its table) */
#define G_STR_CHOICE 0x0057C19Cu /* "choice" */
#define G_STR_CHOICESCREEN 0x0057C1A4u /* "choicescreen" */
#define G_STR_MISSION 0x0057C1B4u /* "mission" */
#define G_STR_CLEOPATRAS_ALEXANDRIA 0x0057C1BCu /* "[CLEOPATRAS_ALEXANDRIA]" */
#define G_STR_ANCIENT_CONQUERORS 0x0057C1D4u /* "[ANCIENT_CONQUERORS]" */
#define G_STR_RAMSES_II 0x0057C1ECu /* "[RAMSES_II]" */
#define G_STR_VALLEY_OF_THE_KINGS 0x0057C1F8u /* "[VALLEY_OF_THE_KINGS]" */
#define G_STR_NEW_KINGDOM 0x0057C210u /* "[NEW_KINGDOM]" */
#define G_STR_MIDDLE_KINGDOM 0x0057C220u /* "[MIDDLE_KINGDOM]" */
#define G_STR_OLD_KINGDOM 0x0057C234u /* "[OLD_KINGDOM]" */
#define G_STR_ARCHAIC_PERIOD 0x0057C244u /* "[ARCHAIC_PERIOD]" */
#define G_STR_PREDYNASTIC_PERIOD 0x0057C258u /* "[PREDYNASTIC_PERIOD]" */
#define G_STR_MISSION_NAMES 0x0057C270u /* "[MISSION_NAMES]" */
/* the period videos (0x0041BBC8, table 0x0041BDF4) */
#define G_STR_BINK_NEW_KING_BIG 0x0057C284u /* "Binks\high\new_king_big.bik" */
#define G_STR_BINK_NEW_KING_SMALL 0x0057C2A0u /* "Binks\low\new_king_small.bik" */
#define G_STR_BINK_MIDDLE_KINGDOM_BIG 0x0057C2C0u /* "Binks\high\middle_kingdom_big.bik" */
#define G_STR_BINK_MIDDLE_KINGDOM_SMALL 0x0057C2E4u /* "Binks\low\middle_kingdom_small.bik" */
#define G_STR_BINK_OLD_KINGDOM_BIG 0x0057C308u /* "Binks\high\old_kingdom_big.bik" */
#define G_STR_BINK_OLD_KINGDOM_SMALL 0x0057C328u /* "Binks\low\old_kingdom_small.bik" */
#define G_STR_BINK_ARCHAIC_BIG 0x0057C348u /* "Binks\high\archaic_big.bik" */
#define G_STR_BINK_ARCHAIC_SMALL 0x0057C364u /* "Binks\low\archaic_small.bik" */
#define G_STR_BINK_PRE_DYNASTIC_BIG 0x0057C380u /* "Binks\high\pre_dynastic_big.bik" */
#define G_STR_BINK_PRE_DYNASTIC_SMALL 0x0057C3A0u /* "Binks\low\pre_dynastic_small.bik" */
/* the file list object (core/newgame/flow2.h) */
#define G_FILE_LIST_VTABLE 0x0056F548u /* @bin 0x0041C6D0 the file list's vtable */
#define G_SORT_COMPARE 0x007027D8u /* @bin 0x0041CA10 the comparator of the file list sort (a game function) */
#define G_SORT_NAMES 0x007027DCu /* @bin 0x0041CA10 the names base of the file list sort */
/* the campaign table (G_CAMPAIGN_TABLE, core/newgame/flow.h): 100 steps of CAMPAIGN_STEP_SIZE bytes */
#define CAMPAIGN_STEP_SIZE 0x40u /* @bin 0x0041AC90 */
#define CAMPAIGN_STEP_COUNT 0x1900u /* @bin 0x0041AC90 i32 */
#define CAMPAIGN_NAMES 0x1904u /* @bin 0x0041B419 char[0x32] per mission name */
#define CAMPAIGN_NAME_SIZE 0x32u /* @bin 0x0041B419 */
#define CAMPAIGN_NAME_COUNT 0x2C8Cu /* @bin 0x0041B419 i32 */
#define G_MISSION_COUNT 0x00700994u /* @bin 0x0041DF70 i32: the missions of the mission list (mode 1) */
/* the fade (window 4) */
#define G_FADE_NEXT 0x006FDCA4u /* @bin 0x0041AA20 i32: the window after the fade */
#define G_FADE_END 0x006FDCA8u /* @bin 0x0041AA20 u32: its end time (ms) */
#define G_FADE_START 0x006FDCACu /* @bin 0x0041AA20 u32: its start time (timeGetTime) */
/* the main menu's buttons (0x004CB6A0) */
#define G_MENU_BUTTON1_SHOWN 0x00D3B188u /* @bin 0x004CB6A0 u8: := 1; the second button is shown with it (0x004CB860) */
#define G_MENU_PERIOD_DONE 0x00D3B189u /* @bin 0x004CB6A0 u8: a campaign period is done (0x0041BAB0) */
#define G_MENU_SAVES_FOUND 0x00D3B18Au /* @bin 0x004CB711 u8: a saved game in the family's directory */
#define G_MENU_LAST_SAVE_FOUND 0x00D3B18Bu /* @bin 0x004CB6A0 u8: the last saved game (G_LAST_SAVE) opens */
#define G_U5D2A2C 0x005D2A2Cu /* @bin 0x0041BB70 i32: := 1 when a period starts and before the mission choice; cleared by the empire editor */
#define G_UE20998 0x00E20998u /* @bin 0x00516C10 i32: := 0 when the name is typed in; tested and cleared after a text box focus (present/ui/dispatch.c) */
#define G_UE94DC0 0x00E94DC0u /* @bin 0x00516C10 i32: := 0x2F (the family list screen) by the new family screen */
#define G_STREAM_NAMES 0x00E92DD0u /* @bin 0x0053B690 i32 per sound channel: its stream's name slot (0: none opened) */
/* the scenario info (chunk 75, 0x007848D8; G_START_YEAR is its +0) */
#define G_SCENARIO_EMPIRE 0x007848DCu /* @bin 0x004DE820 i16: the scenario's empire, copied to G_EMPIRE_CURRENT */
#define G_SCENARIO_FUNDS 0x007848F8u /* @bin 0x004DE98C i32: the initial funds (scenario info +0x20, save.md §5 city +0x0C) */
#define G_SCENARIO_MAP_START 0x0078490Cu /* @bin 0x004DE6A2 i32: -> G_MAP_START */
#define G_SCENARIO_MAP_STRIDE_EXTRA 0x00784910u /* @bin 0x004DE6A2 i32: -> G_MAP_STRIDE_EXTRA */
#define G_SCENARIO_TITLE 0x00784914u /* @bin 0x0053D972 char[]: compared with "The Glory of Ptolemy"; text box 5 (0x004CC560) */
#define G_U784954 0x00784954u /* @bin 0x004DE600 char[]: the text of text box 6 (0x004CC560, up to 0x1F4) */
#define G_HERD_POINTS_X 0x00784B64u /* @bin 0x004BB3F0 i16[4]: herd points (x > 0 in use) */
#define G_HERD_POINTS_Y 0x00784B6Cu /* @bin 0x004BB3F0 i16[4] */
#define G_TIME_LIMIT_YEARS 0x00784C4Cu /* @bin 0x004DE9C8 i32: years after the start year */
#define G_SURVIVE_YEARS 0x00784C54u /* @bin 0x004DE9C8 i32: years after the start year */
#define G_ENTRY_POINT_X 0x00784C64u /* @bin 0x004DE8A9 i16: the entry point (scenario info +0x38C, save.md §5 city +0x2778); -1: none */
#define G_ENTRY_POINT_Y 0x00784C66u /* @bin 0x004DE8A9 i16 */
#define G_EXIT_POINT_X 0x00784C68u /* @bin 0x004DE8A9 i16: the exit point (scenario info +0x390); -1: the entry point */
#define G_EXIT_POINT_Y 0x00784C6Au /* @bin 0x004DE8A9 i16 */
#define G_U784CB1 0x00784CB1u /* @bin 0x004BBD40 u8: in the editor, 0 skips the 20 figures of type 0x25 */
#define G_HERD2_POINTS_X 0x00784CC0u /* @bin 0x004BB3F0 i32[4]: points of the second herd kind (x > 0 in use) */
#define G_HERD2_POINTS_Y 0x00784CD0u /* @bin 0x004BB3F0 i32[4] */
#define G_EMPIRE_CURRENT 0x00784F10u /* @bin 0x004DEA1C i32: the current empire (records of 0x20 bytes at G_EMPIRE_OBJECTS_END: the scroll) */
#define G_DEADLINE_YEAR 0x0078D7B8u /* @bin 0x004DE9C8 i32: start year + the time limit, or + 1000000 (chunk 76) */
/* the route records (G_ROUTES, chunk 143: 50 x 324) */
#define ROUTE_SIZE 0x144u /* @bin 0x00444950 */
#define ROUTE_KIND 0x140u /* @bin 0x00444950 u8: the route's kind (2 tested) */
/* the herd records (G_HERD_RECORDS, 0x30 bytes each) */
#define HERD_SIZE 0x30u /* @bin 0x004BB407 */
#define HERD_COUNT 0x08u /* @bin 0x004BB40D i32: the animals */
#define HERD_FORMATION_KIND 0x2Cu /* @bin 0x004BB46F i32: the formation kind */

/* -- named with the rewrite of core/newgame/map_load.c, start.c, start2.c -- */
/* strings of the exe (the text in the comment) */
#define G_STR_ERR_COMP 0x006036B4u /* @bin 0x0055BCF0 "ERR:COMP Error uncompressing." (the log line of a failed explode) */
#define G_STR_MAPS 0x005E17CCu /* @bin 0x004D4AE0 "MAPS" (the map file's magic) */
#define G_STR_C_ROOT 0x005E06DCu /* @bin 0x004CE100 "c:\" (the CD drive's root, drive letter replaced) */
#define G_STR_555 0x005E06E0u /* @bin 0x004CE100 "555" (graphics extension) */
#define G_STR_SMK 0x005E06E4u /* @bin 0x004CE100 "SMK" */
#define G_STR_WAV 0x005E06E8u /* @bin 0x004CE100 "WAV" */
#define G_STR_BACKSLASH 0x005761D0u /* @bin 0x00418539 "\" */
#define G_STR_AUTOSAVE_REPLAY 0x0057BFC4u /* @bin 0x00418539 "autosave_replay.sav" */
#define G_STR_BINK_WIN_GRAND_BIG 0x0057BFD8u /* @bin 0x00418835 "Binks\high\win_grand_big.bik" */
#define G_STR_BINK_WIN_GRAND_SMALL 0x0057BFF8u /* @bin 0x00418835 "Binks\low\win_grand_small.bik" */
/* the map file (0x004D4AE0) */
#define G_MAP_MAGIC 0x005E17C4u /* @bin 0x004D4AE0 char[5]: record 0 of the MAPS schema, compared with G_STR_MAPS */
#define G_PRICE_PAIRS_DEFAULT 0x005D2E60u /* @bin 0x004D40C0 i32[10]: 5 price pairs, the defaults (core/io/save_tables.c) */
#define G_PRICE_PAIRS 0x005D2D40u /* @bin 0x004D40C0 i32[10]: 5 price pairs, copied from G_PRICE_PAIRS_DEFAULT (core/io/save_load.c) */
#define G_CD_DIR 0x00E36BF8u /* @bin 0x004CE100 char[40]: the CD drive's directory (G_STR_C_ROOT with the letter G_DRIVE) */
#define G_PROF_FREQUENCY 0x00BCA848u /* @bin 0x00489B40 u64: QueryPerformanceFrequency (profiler only, docs/newgame.md 3.2) */
/* the scenario info (chunk 75, 0x007848D8) */
#define G_SCENARIO_RANK 0x00784B62u /* @bin 0x005405B0 i16: the player's rank at the start (-> CITY_PLAYER_RANK, CITY_SALARY_RANK capped at 10) */
#define G_U784CA4 0x00784CA4u /* @bin 0x0046D480 i32: tile image of a flagged tile made a type 0x58 building (and + 1) */
#define G_U784CA8 0x00784CA8u /* @bin 0x0046D480 i32: tile image of a flagged tile made a type 0x59 building */
#define G_U784CAC 0x00784CACu /* @bin 0x0046D480 i32: tile image of a flagged tile made a type 0x5D building */
/* set at the scenario start (0x005405B0, docs/newgame.md 3.2 step 5) */
#define G_U78D810 0x0078D810u /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D814 0x0078D814u /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D818 0x0078D818u /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D81C 0x0078D81Cu /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D820 0x0078D820u /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D824 0x0078D824u /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D828 0x0078D828u /* @bin 0x005405B0 i32 chunk 100: := 1 at the start of campaign missions 0-4; blocks screen 0x14 (0x00409FA0) */
#define G_U78D82C 0x0078D82Cu /* @bin 0x005405B0 i32: := 1 at the start */
#define G_U78D830 0x0078D830u /* @bin 0x005405B0 i32 chunk 99: := 1 at the start of mission 0; no reader found */
/* the message list reset (0x004E1770) */
#define G_UE950E4 0x00E950E4u /* @bin 0x004E1770 i32: := 0 */
#define G_UE950E8 0x00E950E8u /* @bin 0x004E1770 i32: := 0 */
/* the carry-over of a won mission (0x004188F0, read by 0x00418BF0; G_CARRY, G_CARRY_COPY) */
#define CARRY_COMPANIES 0x04u /* @bin 0x004188F0 offset in G_CARRY / G_CARRY_COPY: 3 records of CARRY_COMPANY_SIZE: kinds 0xC, 0xB, the others */
#define CARRY_COMPANY_SIZE 0x0Cu /* @bin 0x004188F0 */
#define CARRY_COMPANY_U00 0x00u /* @bin 0x004189FF u8: the best formation's byte G_FORMATION_BASE + 2 (0xFF: none) */
#define CARRY_COMPANY_STRENGTH 0x04u /* @bin 0x004189FF i32: the best formation's strength (-1: none) */
#define CARRY_COMPANY_LEVEL 0x08u /* @bin 0x004189FF u8: its level */
#define G_CARRY_POS_E5 0x0070278Cu /* @bin 0x00418ABC u16 x, y: the carried monument of type 0xE5 (0 0: none) */
#define G_CARRY_POS_EA 0x00702790u /* @bin 0x00418ABC u16 x, y: type 0xEA */
#define G_CARRY_POS_EB 0x00702794u /* @bin 0x00418762 u16 x, y: type 0xEB */
#define G_CARRY_POS_DE 0x00702798u /* @bin 0x00418ADA u16 x | dir << 13, u16 y: type 0xDE */
#define G_CARRY_POS_E1 0x0070279Cu /* @bin 0x00418ABC u16 x, y: type 0xE1 */
#define G_CARRY_POS_E4 0x007027A0u /* @bin 0x00418B27 u16 x, y: type 0xE4 (its main building) */
#define G_IMAGE_DUNES 0x00F56BA4u /* @bin 0x00477210 i32: first image of the dunes group (core/map/terrain_rocks.c) */


/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_GRAPH 0x00605A40u /* present/ui/advisors/b_population.c: i32 the graphs shown (0-5; saved, chunk 45; 0x0040A8A0) */
#define G_CAMPAIGN_CAMPAIGN_KIND 0x006FDC98u /* present/ui/frontend/campaign.c: i32 0 Pharaoh, 1 Cleopatra [single+clash] */
#define G_U6FF608 0x006FF608u /* core/newgame/flow2.c: campaign table + 0x1900 [unresolved] */
#define G_MISSION_LIST_FLAGS 0x007027B8u /* core/command/menu.c: u8 per record */
#define G_GOAL_SEPARATOR 0x007027D4u /* present/ui/frontend/scenario.c: the string between a value and its goal */
#define G_LIST_OBJECT 0x00704C00u /* core/newgame/start.c: the scenario / mission list of screen 0x3D */
#define G_SELECTED 0x00784F14u /* present/ui/empire/empire.h: i32 the selected object + 1 (0: none) */
#define G_KINGDOM_EVENTS 0x00785D74u /* present/ui/messages/window.c: records of 0x7C bytes [single+clash] */
#define G_START_EXT 0x00E38AECu /* core/newgame/start.c: char[4]: the current file extension [generic] */
#define G_FLOW2_PERIOD 0x00E38EA9u /* core/newgame/start.c: u8 [generic] */
#define G_KINGDOM_START 0x00E38EB0u /* core/newgame/start.c: i32 */
#define G_KINGDOM_CARRY 0x00E38EB4u /* core/newgame/start.c: i32 */
/* --- end harvested --- */

#endif
