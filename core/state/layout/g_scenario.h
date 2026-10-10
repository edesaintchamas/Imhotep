/* Imhotep - named globals: core/state/layout/g_scenario.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P6). */
#ifndef IMHOTEP_STATE_LAYOUT_G_SCENARIO_H
#define IMHOTEP_STATE_LAYOUT_G_SCENARIO_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
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
#define G_GRAPH 0x00605A40u /* present/ui/advisors/b_population.c: i32 the graphs shown (0-5; saved, chunk 45; 0x0040A8A0) */
#define G_NO_GROUND 0x006DE515u /* present/city/city.h: u8: the overlay ground replaces the ground pass */
#define G_ADVISOR_AVAILABLE 0x006FDC88u /* core/command/advisor.c: u8 per advisor: the mission offers it */
#define G_CAMPAIGN_CAMPAIGN_KIND 0x006FDC98u /* present/ui/frontend/campaign.c: i32 0 Pharaoh, 1 Cleopatra [single+clash] */
#define G_MISSION_PICTURE_KIND 0x006FDCB0u /* present/ui/empire/mission.c: i32 window 4: 0 mission start, 1 plain, 2 victory [single+clash] */
#define G_FORMATIONS_KEEP 0x006FDCB4u /* core/newgame/start.c: u8 */
#define G_CARRY 0x006FDCB8u /* core/newgame/start.c: 10 dwords: the carry-over of the previous mission */
#define G_CARRY_COPY 0x006FDCE0u /* core/newgame/start.c: 10 dwords: its copy read by 0x00418BF0 */
#define G_CAMPAIGN_TABLE 0x006FDD08u /* core/frame/wndproc.c: the campaign progress object (thiscall) */
#define G_U6FF608 0x006FF608u /* core/newgame/flow2.c: campaign table + 0x1900 [unresolved] */
#define G_LAST_SAVE 0x00700998u /* core/newgame/flow.c: the last saved game's path (empty: none) */
#define G_CHOICE 0x007027A4u /* core/newgame/flow2.c: the mission choice made on screen 0x34: 1, 2 (0: none) */
#define G_CAMPAIGN_PERIOD 0x007027ACu /* present/ui/frontend/campaign.c: i32 the chosen period [generic] */
#define G_PERIOD_REACHED 0x007027B0u /* present/ui/frontend/campaign.c: i32 */
#define G_CAMPAIGN_KIND 0x007027B4u /* core/newgame/start.c: u8: 0 custom, 1 Pharaoh, 2 Cleopatra, 3 mission list */
#define G_MISSION_LIST_FLAGS 0x007027B8u /* core/command/menu.c: u8 per record */
#define G_GOAL_SEPARATOR 0x007027D4u /* present/ui/frontend/scenario.c: the string between a value and its goal */
#define G_CHEAT_TEXT 0x007027ECu /* present/ui/cityframe.c: char[] the typed cheat (edit field 7) */
#define G_SOUND_SLOTS 0x00702900u /* core/frame/sound.c [generic] */
#define G_LIST_OBJECT 0x00704C00u /* core/newgame/start.c: the scenario / mission list of screen 0x3D */
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
#define G_SELECTED 0x00784F14u /* present/ui/empire/empire.h: i32 the selected object + 1 (0: none) */
#define G_KINGDOM_EVENTS 0x00785D74u /* present/ui/messages/window.c: records of 0x7C bytes [single+clash] */
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
#define G_START_EXT 0x00E38AECu /* core/newgame/start.c: char[4]: the current file extension [generic] */
#define G_PLAYER_NAME 0x00E38E74u /* core/newgame/flow.c: char[32] */
#define G_FLOW2_PERIOD 0x00E38EA9u /* core/newgame/start.c: u8 [generic] */
#define G_STEP_PREV 0x00E38EAAu /* core/newgame/start.c: u8 */
#define G_STEP 0x00E38EABu /* core/newgame/start.c: u8 */
#define G_KINGDOM_START 0x00E38EB0u /* core/newgame/start.c: i32 */
#define G_KINGDOM_CARRY 0x00E38EB4u /* core/newgame/start.c: i32 */
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
/* --- end harvested --- */

#endif
