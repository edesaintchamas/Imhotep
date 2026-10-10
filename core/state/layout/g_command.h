/* Imhotep - named globals: core/state/layout/g_command.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P4). */
#ifndef IMHOTEP_STATE_LAYOUT_G_COMMAND_H
#define IMHOTEP_STATE_LAYOUT_G_COMMAND_H

/* --- named by package P4 (core/command): evidence the port named on each line; a G_U<address> name
 * is neutral (meaning not established; the comment says what the code does with it). --- */
/* construction and placement */
#define G_PYRAMID_ANCHORS_XY 0x0056FD10u /* @bin 0x004ECEC0 i32 (x, y) pairs: each pyramid piece's anchor, G_PYRAMID_KIND 5 */
#define G_PYRAMID_ANCHORS 0x0056FE30u    /* @bin 0x004ECEC0 i32 grid offsets: each piece's anchor, other kinds */
#define G_PYRAMID_ROW_COUNT 0x00570094u  /* @bin 0x004ED064 i32 per variant: the tiles of the row checked */
#define G_PYRAMID_ROW_COUNT5 0x005700A8u /* @bin 0x004ED064 the same, G_PYRAMID_KIND 5 */
#define G_PYRAMID_ROW_FROM 0x005700B4u   /* @bin 0x004ED064 i32 per variant: the row's first tile */
#define G_PYRAMID_ROW_FROM5 0x005700C8u  /* @bin 0x004ED064 the same, G_PYRAMID_KIND 5 */
#define G_PYRAMID_PIECE_TILES 0x005CFCDCu /* @bin 0x004ECEC0 i32: the tiles of a piece (offsets G_FOOT_TILES) */
#define G_PYRAMID_RINGS 0x005F5D1Cu      /* @bin 0x004ED330 i32 per variant: the rings around the mouse tile */
#define G_OBELISK_ITEM 0x00576144u       /* @bin 0x0040576D i32: the monuments menu item of the obelisk chosen */
#define G_ROAD_DIRECTIONS4 0x005D2914u   /* @bin 0x0043F250 i32[4]: grid steps N, E, S, W */
#define G_VENUE_ROADS_2 0x005D2924u      /* @bin 0x0043F3A0 u8[2][2]: the venue's road layout, size 2 */
#define G_VENUE_ROADS_3 0x005D2928u      /* @bin 0x0043F3A0 u8[3][3] */
#define G_VENUE_ROADS_4 0x005D2934u      /* @bin 0x0043F3A0 u8[4][4] */
#define G_VENUE_ROADS_5 0x005D2944u      /* @bin 0x0043F3A0 u8[5][5] */
#define G_VENUE_SCORES_3 0x005D2960u     /* @bin 0x00441130 i32[3][3]: the part spots' scores, size 3 */
#define G_VENUE_SCORES_4 0x005D2984u     /* @bin 0x00441130 i32[4][4]: size 4 */
#define G_DIRECTION_STEPS 0x005F811Cu    /* @bin 0x0051C930 i32 (dx, dy) per direction */
#define G_PATH_DIRECTIONS 0x005F815Cu    /* @bin 0x0051C930 i32[4] per direction: the directions tried in order */
#define G_PATH_TILE 0x00EAE000u          /* @bin 0x0051C930 i32: the path walk's current tile */
#define G_DRAG_START_HEIGHT 0x00BC96CCu  /* @bin 0x00405C20 i32: the elevation of the construction's first tile */
#define G_UBC9710 0x00BC9710u            /* @bin 0x00405200 i32: := G_FRAME_CLOCK when a building is chosen */
#define G_UBC9720 0x00BC9720u            /* @bin 0x004728A0 i32: := 0 before a construction is charged */
#define G_SCENARIO_ENEMY 0x007848FCu     /* @bin 0x004BC720 i16: the scenario's enemy (at most 11) */
#define G_ENEMY_GRAPHICS 0x005DEF84u     /* @bin 0x004BC720 i32 per enemy: its graphics file (graphics.md) */
#define G_ENEMY_MODELS 0x007083ACu       /* @bin 0x00540ED0 "ALL ENEMIES": 60 records of 14 i32, 5 per enemy */
/* the G_MODELS entries (0x14 bytes per type) */
#define MODELS_IMAGE_GROUP 0x08u         /* @bin 0x0043DD70 i32: the image group */
#define MODELS_IMAGE_OFFSET 0x0Cu        /* @bin 0x0043DD70 i32: the image offset in it */
/* the model files (0x00540C90, 0x00540ED0, 0x00541160) */
#define G_MODEL_FILES_BUILDINGS 0x00601908u /* @bin 0x00540C90 char *[5] per difficulty: Pharaoh_Model_<level>.txt */
#define G_MODEL_FILES_FIGURES 0x0060191Cu /* @bin 0x00540ED0 char *[5]: Figure_model_<level>.txt */
#define G_MODEL_FILES_TAX 0x00601930u    /* @bin 0x00541160 char *[5]: Tax_Sentiment_Model_<level>.txt */
#define G_STR_MODEL_LOADED 0x00601AF4u   /* @bin 0x00540C90 " OK: model loaded" */
#define G_STR_NO_BUILDING_MODEL 0x00601B88u /* @bin 0x00540C90 the log line when the file cannot be read */
#define G_STR_FIGURE_MODEL_LOADED 0x00601BC8u /* @bin 0x00540ED0 " OK: figure model data loaded" */
#define G_STR_NO_FIGURE_MODEL 0x00601C60u /* @bin 0x00540ED0 the log line when the file cannot be read (also 0x00541160) */
#define G_STR_TAX_MODEL_LOADED 0x00601C80u /* @bin 0x00541160 " OK: tax sentiment data loaded" */
/* menus, options and windows */
#define G_BUILD_BUTTONS_A 0x005C7388u    /* @bin 0x004051A0 12 sidebar buttons of 0x24 bytes: +0 enabled, +4 build menu */
#define G_BUILD_BUTTONS_B 0x005C75C8u    /* @bin 0x004051A0 the second table */
#define G_MENU_LAYOUT 0x005CF63Cu        /* @bin 0x00405200 i32 pairs by item count: G_MENU_Y, then G_U7056E4 */
#define G_U7056E4 0x007056E4u            /* @bin 0x00405200 i32: := the second dword of G_MENU_LAYOUT */
#define G_MENU_TEXT_LANGUAGE 0x005C8FD2u /* @bin 0x0040C7D0 u16: the text of Options 6 (7 / 8) */
#define G_MENU_TEXT_EEC 0x005C8FF2u      /* @bin 0x00403500 u16: the text of Options 8 (9 / 10) */
#define G_MENU_TEXT_EC3 0x005C9002u      /* @bin 0x00403540 u16: the text of Options 9 (4 / 5) */
#define G_MENU_TEXT_TIPS 0x005C9022u     /* @bin 0x00403410 u16: the mouse help item's text (2, 3, 4) */
#define G_MENU_TEXT_WARNINGS 0x005C9032u /* @bin 0x00403490 u16: the warnings item's text (5 / 6) */
#define G_MENU_TEXT_TIPS2 0x005C9062u    /* @bin 0x00403410 u16: the mouse help item's second text word */
#define G_UE38EC3 0x00E38EC3u            /* @bin 0x00403540 u8: Options 9 on / off */
#define G_UE38EEC 0x00E38EECu            /* @bin 0x00403500 i32: Options 8 on / off */
#define G_UF53F18 0x00F53F18u            /* @bin 0x00402BC0 i32: := 0 when the display options open (options.c: := 50) */
#define G_UF53F1C 0x00F53F1Cu            /* @bin 0x00402EE0 i32: := 0 when the sound options open */
#define G_UF53F20 0x00F53F20u            /* @bin 0x004032C0 i32: := 0 when the speed options open */
#define G_UF53F4C 0x00F53F4Cu            /* @bin 0x00402BE0 i32: := 0 when the difficulty options open */
/* the settings kept to cancel the options windows (0x00402EE0, 0x004032C0) */
#define G_SAVED_SCROLL_SPEED 0x0060579Cu /* @bin 0x004032C0 i32: G_SCROLL_SPEED */
#define G_SAVED_GAME_SPEED 0x006057A0u   /* @bin 0x004032C0 i32: G_GAME_SPEED */
#define G_SAVED_CITY_VOLUME 0x006057A4u  /* @bin 0x00402EE0 i32: G_CITY_VOLUME */
#define G_SAVED_CITY_SOUNDS 0x006057A8u  /* @bin 0x00402EE0 i32: G_CITY_SOUNDS_ON low byte */
#define G_SAVED_EFFECTS_VOLUME 0x006057ACu /* @bin 0x00402EE0 i32: G_EFFECTS_VOLUME */
#define G_SAVED_EFFECTS_ON 0x006057B0u   /* @bin 0x00402EE0 i32: G_SOUND_EFFECTS_ON */
#define G_SAVED_SPEECH_VOLUME 0x006057B4u /* @bin 0x00402EE0 i32: G_SPEECH_VOLUME */
#define G_SAVED_E38E65 0x006057B8u       /* @bin 0x00402EE0 i32: the second byte of G_MUSIC_ON's dword */
#define G_SAVED_MUSIC_VOLUME 0x006057BCu /* @bin 0x00402EE0 i32: G_MUSIC_VOLUME */
#define G_SAVED_MUSIC_ON 0x006057C0u     /* @bin 0x00402EE0 i32: G_MUSIC_ON low byte */
/* the list popup 0x00403C00 (its 15 arguments a1..a15) */
#define G_LIST_COLUMNS 0x006057ECu       /* @bin 0x00403C00 i32: (n + per - 1) / per */
#define G_LIST_A15 0x006057F0u           /* @bin 0x00403C00 u8: a15 */
#define G_LIST_ORDER 0x006057F4u         /* @bin 0x00403C00 i32[]: the order of the choices */
#define G_LIST_A10 0x0060592Cu           /* @bin 0x00403C00 i32: a10 */
#define G_LIST_A6 0x00605930u            /* @bin 0x00403C00 i32: a6 */
#define G_LIST_A7 0x00605934u            /* @bin 0x00403C00 i32: a7 */
#define G_LIST_A8 0x00605938u            /* @bin 0x00403C00 i32: a8 */
#define G_LIST_A9 0x0060593Cu            /* @bin 0x00403C00 i32: a9 */
#define G_LIST_TEXT_FIRST 0x00605940u    /* @bin 0x00403C00 i32: a5, added to G_LIST_ORDER[k] for the text (numpad.c) */
#define G_LIST_TEXT_GROUP 0x00605944u    /* @bin 0x00403C00 i32: a4, the choices' text group (numpad.c) */
#define G_LIST_COUNT 0x00605948u         /* @bin 0x00403C00 i32: the choices n */
#define G_LIST_A2 0x0060594Cu            /* @bin 0x00403C00 i32: a2 */
#define G_LIST_PER_COLUMN 0x00605950u    /* @bin 0x00403C00 i32: per column */
#define G_LIST_A1 0x00605954u            /* @bin 0x00403C00 i32: a1 */
#define G_LIST_PREV_WINDOW 0x00605958u   /* @bin 0x00403C00 i32: the window to go back to */
#define G_LIST_A12 0x00605A50u           /* @bin 0x00403C00 i32: a12 */
#define G_LIST_BUTTONS 0x00704DC8u       /* @bin 0x00403C00 the n buttons of 0x1C bytes */
#define G_UE38DDC 0x00E38DDCu            /* @bin 0x00403C00 i32: := 0x14 when a list popup opens */
/* the map editor's tool items (0x00405200) */
#define G_U605A1C 0x00605A1Cu            /* @bin 0x00405483 i32: items 9..16 of menu 0x1A, less 8 / 0xC */
#define G_U605A20 0x00605A20u            /* @bin 0x00405483 i32: items from 17 of menu 0x1A, less 0x10 */
#define G_U605A24 0x00605A24u            /* @bin 0x00405483 i32: items 1..8 of menu 0x1A */
#define G_U605A28 0x00605A28u            /* @bin 0x00405200 i32: the item of menu 0x11 */
#define G_U605A2C 0x00605A2Cu            /* @bin 0x00405200 i32: the item of menu 0xF */
#define G_U605A34 0x00605A34u            /* @bin 0x00405200 i32: the item of menu 0xE, less 1 */
/* the view and the camera */
#define G_U70E40C 0x0070E40Cu            /* @bin 0x00430530 i32: := 0xE4 by the view table build */
#define G_U70E410 0x0070E410u            /* @bin 0x00430530 i32: := 0xE4 */
#define G_U70E414 0x0070E414u            /* @bin 0x00430530 i32: := 0 */
#define G_VIEW_TABLE_ORIENTATION 0x0070E41Cu /* @bin 0x00430530 i32: the orientation the view table was built for */
#define G_VIEW_ROW_MAX 0x0070E420u       /* @bin 0x00430530 i32: the last view row holding a tile (minimap.c) */
#define G_VIEW_COL_MAX 0x0070E424u       /* @bin 0x00430530 i32: the last column */
#define G_VIEW_ROW_MIN 0x0070E428u       /* @bin 0x00430530 i32: the first row */
#define G_VIEW_COL_MIN 0x0070E42Cu       /* @bin 0x00430530 i32: the first column */
#define G_CAMERA_Y_MAX 0x00E94DC4u       /* @bin 0x004DDF90 i32: the camera bounds (0x004DE080): last row */
#define G_CAMERA_Y_MIN 0x00E94DC8u       /* @bin 0x004DDF90 i32: first row */
#define G_CAMERA_X_MAX 0x00E94DCCu       /* @bin 0x004DDF90 i32: last column */
#define G_CAMERA_X_MIN 0x00E94DD0u       /* @bin 0x004DDF90 i32: first column */
/* advisors, messages, military */
#define G_LAST_ADVISOR 0x00E38EA4u       /* @bin 0x0040A0E0 i32: the advisor last opened */
#define G_DISPATCH_ENABLED 0x005F8518u   /* @bin 0x0040A450 u8: set by the military advisor's drawing */
#define G_HISTORY_STACK1 0x00D3AAE4u     /* @bin 0x004C5BF0 u32[]: the message window's page stacks (msgwin.c) */
#define G_HISTORY_STACK2 0x00D3AE04u     /* @bin 0x004C5BF0 u32[] */
#define G_MW_PREV_WINDOW 0x00D3B17Cu     /* @bin 0x004C5BF0 u8: the window the message window goes back to (0x004C7F30) */
#define G_UE950D4 0x00E950D4u            /* @bin 0x004E2B10 i32: no road notice from 3 */
#define G_CURSOR_UNLOAD 0x00E390C4u      /* @bin 0x0040BA40 i32: the cursor that unloads a transport */
#define G_CURSOR_BOARD 0x00E390C8u       /* @bin 0x0040BA40 i32: the cursor that boards a transport */
/* cheats and music */
#define G_SIDE_SHOW 0x007027E1u          /* @bin 0x0041CCF0 u8: toggled by "Side Show" (the other sprite set, 0x0049F418) */
#define G_CHEATED 0x00E38DD7u            /* @bin 0x0041CFC0 u8: := 1 by "Pharaohs Tomb"; a cheated game is not recorded (highscore.c) */
#define G_UEA137C 0x00EA137Cu            /* @bin 0x0041CCB0 u8: := 1 by "Mummys Revenge" */
#define G_STR_SETUP_MUSIC 0x005761ACu    /* @bin 0x00402418 "audio\music\setup.mp3" */

/* --- harvested names (tools/naming/harvest.py), checked by P4 against their uses in core/command.
 * @bin evidence: the defining port (file) and its comment. --- */
#define G_WAV_COHORT4 0x005761FCu /* core/command/military.c */
#define G_WAV_COHORT3 0x00576210u /* core/command/military.c */
#define G_WAV_COHORT2 0x00576224u /* core/command/military.c */
#define G_WAV_COHORT1 0x00576238u /* core/command/military.c */
#define G_WAV_COHORT5 0x0057624Cu /* core/command/military.c */
#define G_MISSION_LISTS_END 0x0057978Cu /* core/command/menu.c: @bin 0x0040511C (+4: the loop runs on +4) */
#define G_CHEAT_TABLE 0x0057C3C8u /* core/command/cheats.c: 12-byte entries; +4 the function */
#define G_DIRECTIONS 0x005CD978u /* core/command/cheats.c: 8 (dx, dy) pairs of i32 */
#define G_MENU_TYPES 0x005CE3B8u /* core/command/construction_ui.c: @bin 0x00405792 i32[submenu * 30 + item] */
#define G_MENU_TYPES_1 0x005CE3BCu /* core/command/menu.c: @bin 0x00404392 i32[menu * 30 + i]: the type of entry i + 1 */
#define G_FOOT_TILES 0x005CFE10u /* present/city/ghost.h: i32[36] per orientation / 2: the footprint's grid offsets */
#define G_SALARY_BY_RANK 0x005D0320u /* core/command/orders.c: i32 per rank */
#define G_MODELS 0x005D06DCu /* present/city/ghost.h: 0x14 bytes per type: +0 size, +8 image group, +0xC image offset */
#define G_DIFFICULTY_TAX 0x005D1974u /* core/command/orders.c: i32 per difficulty */
#define G_ENDS 0x005DC188u /* core/command/construction_lift.c: @bin 4 (dx, dy) pairs */
#define G_POWERS 0x005E058Cu /* core/command/models.c: @bin 0x004CC0B3 i32 powers of ten */
#define G_VARIANT 0x005E6970u /* core/command/place_nearest.c */
#define G_LABOUR_ORDER 0x00605768u /* core/command/orders.c: i32[9] scratch: categories by priority */
#define G_DRAGGED 0x00605798u /* core/command/construction_ui.c: u8 */
#define G_LABOUR_SELECTED 0x00605988u /* core/command/orders.c: i32 the category whose priority is being set */
#define G_TRADE_RESOURCE 0x0060598Cu /* core/command/trade.c */
#define G_NO_ROOM 0x00605994u /* core/command/construction_checks.c */
#define G_FLAG_B4 0x006059B4u /* core/command/construction_ui.c */
#define G_FLAG_B8 0x006059B8u /* core/command/construction_ui.c */
#define G_FLAG_BC 0x006059BCu /* core/command/construction_ui.c */
#define G_FLAG_C0 0x006059C0u /* core/command/construction_ui.c */
#define G_FLAG_C4 0x006059C4u /* core/command/construction_ui.c */
#define G_FLAG_C8 0x006059C8u /* core/command/construction_ui.c */
#define G_FLAG_CC 0x006059CCu /* core/command/construction_ui.c */
#define G_HOVER_MARK 0x006059D0u /* present/city/ground.c: i32 a tile whose screen position is kept */
#define G_CONSTRUCTION_UI_Y2 0x006059D4u /* core/command/construction_ui.c */
#define G_CONSTRUCTION_UI_Y1 0x006059D8u /* core/command/construction_ui.c */
#define G_CONSTRUCTION_UI_X2 0x006059DCu /* core/command/construction_ui.c */
#define G_CONSTRUCTION_UI_X1 0x006059E0u /* core/command/construction_ui.c */
#define G_CONSTRUCTION_DRAG 0x006059E4u /* present/city/ghost.h: i32 1: a construction drag is in progress */
#define G_CONSTRUCTION_UI_TYPE 0x006059E8u /* present/city/ghost.h: i32 the building type chosen */
#define G_FREE_BUILD 0x00605A0Cu /* core/command/menu.c: debug "everything allowed" (start-up only) */
#define G_EDITOR_TOOL 0x00605A30u /* core/command/construction_ui.c */
#define G_PREDICATE 0x00605A48u /* core/command/construction_ui.c */
#define G_FROZEN 0x00605A5Cu /* core/command/construction_ui.c: u8: the hovered tile is not updated */
#define G_MESSAGE_OPTIONS_EDIT 0x006DE510u /* core/command/orders.c */
#define G_MENU_Y 0x007056E8u /* present/ui/windows.c: i32 its top */
#define G_SUBMENU 0x007056ECu /* present/ui/windows.c: i32 the build menu */
#define G_MENU_COUNT 0x007056F0u /* present/ui/windows.c: i32 its items */
#define G_POPUP_ACTION 0x00705704u /* present/ui/windows.c: u32 the function called on "yes" */
#define G_POPUP_BUTTONS 0x00705708u /* present/ui/windows.c: i32 0: a message only (right click closes), else the buttons */
#define G_POPUP_BACK 0x0070570Cu /* present/ui/windows.c: u8 the window to go back to */
#define G_POPUP_TEXT 0x00705710u /* present/ui/windows.c: i32 the title's index in text group 5 (the message: + 1) */
#define G_POPUP_Y 0x00705714u /* present/ui/windows.c */
#define G_POPUP_X 0x00705718u /* present/ui/windows.c */
#define G_MENU_REDRAW 0x0070571Cu /* core/command/menu_items.c: the menu bar's redraw (i32) */
#define G_MODEL_BUILDING 0x00705EA4u /* core/city/desirability.h: building model: 0x28-byte records (model.md) */
#define G_MENU_AVAIL 0x0070A8DCu /* core/command/menu.c: @bin 0x00404398 i32[menu * 30 + i] */
#define G_VIEW_TABLE 0x0070E494u /* core/frame/view.c: i32 [rows][0xE7] */
#define G_GRID5 0x00775614u /* core/command/place_ent.c: @bin 0x004413E9 5 x 5 dwords */
#define G_ARMIES 0x007770F0u /* core/command/kingdom.c: u8 kind per army record of 0x144 bytes */
#define G_EMPIRE_CITY_END 0x0077F742u /* core/command/trade.c: @bin 0x00443A67 the scan stops before this record (61 cities) */
#define G_GOODS_NEEDED 0x00784DE8u /* core/command/kingdom.c: i32 per resource: grave goods asked by the scenario */
#define G_GOAL_COUNTERS 0x00784E78u /* core/command/kingdom.c: i32 per resource: grave goods sent, the goal counters (GOAL_COUNTERS in core/city/text_lookup.c; 0x004EA930) */
#define G_EMPIRE_SELECTED 0x00784F18u /* core/command/trade.c: i32 empire city of the empire window */
#define G_STORAGE_FILL 0x007900ECu /* core/command/orders_storage.c: i16 fill limit per resource (order 0) */
#define G_EDGE_BACKUP 0x009AB9ECu /* core/command/undo.c */
#define G_BITS_BACKUP 0x009B84FCu /* core/command/undo.c */
#define G_SPRITE_BACKUP 0x009C500Cu /* core/command/undo.c */
#define G_CANAL_BACKUP 0x009D1B1Cu /* core/command/undo.c */
#define G_TERRAIN_BACKUP 0x009DE62Cu /* core/command/undo.c */
#define G_IMAGE_BACKUP 0x00A1126Cu /* core/command/undo.c */
#define G_SPRITE 0x00ADC36Cu /* core/command/clear.c: u8 */
#define G_CANAL 0x00AE8E7Cu /* core/command/clear.c: u8 */
#define G_UNDO_IDS 0x00BA746Cu /* core/command/undo.c */
#define G_UNDO_RECORDS 0x00BA7888u /* core/command/undo.c */
#define G_UNDO_COUNT 0x00BC95F0u /* core/command/undo.c: buildings kept for undo: ids (i16) and their records */
#define G_UNDO_GOODS 0x00BC95F4u /* core/command/undo.c: 0x24 dwords: goods consumed, by resource */
#define G_UNDO_COST 0x00BC9684u /* core/command/undo.c */
#define G_UNDO_READY 0x00BC968Cu /* core/command/undo.c */
#define G_RECT_Y2 0x00BC9698u /* core/command/clear.c */
#define G_RECT_X2 0x00BC969Cu /* core/command/clear.c */
#define G_RECT_Y1 0x00BC96A0u /* core/command/clear.c */
#define G_RECT_X1 0x00BC96A4u /* core/command/clear.c: rectangle kept for the confirmation callbacks */
#define G_OK_TEMPLE 0x00BC96A8u /* core/command/clear.c */
#define G_OK_MONUMENT 0x00BC96ACu /* core/command/clear.c: confirmation answers: 1 yes, -1 no, 0 not asked */
#define G_OK_BRIDGE 0x00BC96B0u /* core/command/clear.c */
#define G_OK_FORT 0x00BC96B4u /* core/command/clear.c */
#define G_ROTATE_LAST 0x00BC96C8u /* core/command/orders.c */
#define G_CONSTRUCTION_UI_SIZE 0x00BC96D0u /* core/command/place.c: @bin 0x00472EEF the type's size */
#define G_REACH_DIR 0x00BC96E0u /* core/command/hover.c */
#define G_REACH_LEN 0x00BC96E4u /* core/command/hover.c */
#define G_REACH_END 0x00BC96E8u /* core/command/hover.c */
#define G_REACH_STEP 0x00BC96ECu /* core/command/hover.c */
#define G_REACH_START 0x00BC96F0u /* core/command/hover.c */
#define G_BRIDGE_DIR 0x00BC96F4u /* core/command/hover.c */
#define G_BRIDGE_LENGTH 0x00BC96F8u /* core/command/place.c: @bin 0x00472E5D */
#define G_BRIDGE_END 0x00BC96FCu /* core/command/hover.c */
#define G_BRIDGE_STEP 0x00BC9700u /* core/command/hover.c */
#define G_BRIDGE_START 0x00BC9704u /* core/command/hover.c */
#define G_END2 0x00BC9708u /* core/command/construction_lift.c */
#define G_END1 0x00BC970Cu /* core/command/construction_lift.c: 0 none, 1 new lift, 2 existing lift, -1 blocked */
#define G_BUILD_ORIENTATION 0x00BC9714u /* present/city/ghost.h: i32 the building orientation (R key) */
#define G_REFUND 0x00BC9718u /* core/command/place.c: @bin 0x004728CC subtracted from the cost */
#define G_CONSTRUCTION_COUNT 0x00BC971Cu /* core/command/place.c: the construction's count, the cost multiplier (charged by 0x004728A0) */
#define G_PREVIEW_COST 0x00BC9724u /* core/command/construction_ui.c: @bin 0x0046FF46 the cost the ghost shows (model cost; lifts 0x0047EF4C) */
#define G_PLACE_COST 0x00BC9728u /* core/command/place.c: @bin 0x004728C6 the construction's cost */
#define G_WALK_Y 0x00BC9760u /* core/command/clear.c: the game's loop globals */
#define G_WALK_X 0x00BC9764u /* core/map/last_terrain.c */
#define G_FERRY_BUILDING 0x00BC9800u /* present/city/ghost_ferry.c: i32 the first landing */
#define G_CAMERA_X 0x00BCA2BCu /* core/command/rotate.c */
#define G_CAMERA_Y 0x00BCA2C0u /* core/command/rotate.c */
#define G_MOUSE_OFFSET 0x00BCA2C4u /* present/sidebar/tooltip.c: i32 grid offset under the mouse */
#define G_MOUSE_TILE_X 0x00BCA2C8u /* core/command/cheats.c: i32 the tile under the mouse, x (core/frame/frame.c) */
#define G_MOUSE_TILE_Y 0x00BCA2CCu /* core/command/cheats.c: i32 y */
#define G_DRAG_OFFSET 0x00BCA2E4u /* core/command/construction_ui.c */
#define G_VIEW_DX 0x00BCA2E8u /* core/figure/actions_c2.c: the map's area in the 228 x 228 grid: first x */
#define G_VIEW_DY 0x00BCA2ECu /* core/figure/actions_c2.c: first y */
#define G_HOVER_TILE 0x00BCA300u /* present/city/ghost.h: i32 the hovered tile's grid offset (0: none) */
#define G_HOVER_X 0x00BCA304u /* core/command/construction_ui.c */
#define G_HOVER_Y 0x00BCA308u /* core/command/construction_ui.c */
#define G_FERRY_STATE 0x00BCA310u /* present/city/ghost_ferry.c: i32 1: the first landing is placed */
#define G_FERRY_VALID 0x00BCA314u /* core/command/hover.c: u8 */
#define G_FERRY_FLAG 0x00BCA315u /* core/command/construction_ui.c: u8 */
#define G_FORMATION 0x00BCB9C0u /* core/frame/frame.c: i32 the current formation; records of 0x90 bytes from here */
#define G_SEL_TRANSPORT 0x00D05700u /* core/command/military.c */
#define G_SEL_WARSHIP 0x00D05704u /* core/command/military.c */
#define G_TRANSPORT_MODE 0x00D05708u /* core/command/military.c */
#define G_WARSHIP_MODE 0x00D0570Cu /* core/command/military.c */
#define G_LINKS_COUNT 0x00D3A6ECu /* present/ui/messages/richtext.c */
#define G_LINKS_NEXT 0x00D3A6F0u /* present/ui/messages/richtext.c */
#define G_LINKS 0x00D3A6F8u /* present/ui/messages/richtext.c: 50 entries of 5 i32: x1, x2, y1, y2, target */
#define G_HISTORY 0x00D3AAE0u /* present/ui/msgwin.c */
#define G_MW_TYPE 0x00D3B124u /* present/ui/messages/window.c: i32 the message type */
#define G_MW_DRAGGING 0x00D3B12Cu /* present/ui/messages/window.c: i32 */
#define G_SCROLL 0x00D3B130u /* present/ui/messages/richtext.c: i32 first line drawn */
#define G_TEXT_LINES 0x00D3B138u /* present/sidebar/tooltip_draw.c: i32 lines written by 0x004C8070 */
#define G_MW_NO_CITY 0x00D3B140u /* present/ui/messages/window.c: i32 != 0: the city is not drawn behind */
#define G_MESSAGE_WINDOW_VIDEO 0x00D3B180u /* present/ui/messages/window.c: i32 != 0: the video layout (0x004C6C70) */
#define G_MW_FROM_LIST 0x00D3B184u /* present/ui/messages/window.c: u8 the message comes from the list (texts may be expanded) */
#define G_MESSAGE_WINDOW_REPLIES 0x00D3B186u /* core/command/orders_messages.c: u8 bits 1, 2, 4: replies offered by the message window */
#define G_PARAM2 0x00D3B194u /* core/command/orders.h: i32 callback parameter 2 */
#define G_PARAM1 0x00D3B198u /* core/frame/wndproc.c: the button / command parameter */
#define G_CONFIRM_ANSWER 0x00D3B439u /* core/command/orders.h: u8 confirmation popup answer (1 = yes) */
#define G_TEXT_PTR2 0x00E20764u /* present/ui/frontend/family.c: u32 the string 0x004CC160 found */
#define G_LAYOUT_DY 0x00E38DA4u /* present/sidebar/sidebar.h: i32 (height - 480) / 2 */
#define G_LAYOUT_DX 0x00E38DA8u /* present/sidebar/sidebar.h: i32 (width - 640) / 2 */
#define G_GATE 0x00E38E60u /* core/command/construction_ui.c: u8 */
#define G_MUSIC_ON 0x00E38E64u /* core/frame/sound.c: u8 option: music */
#define G_TIPS_ON 0x00E38EACu /* present/sidebar/tooltip.c: i32 0 off, 2 on (options) */
#define G_SPEECH_VOLUME 0x00E38ECCu /* present/ui/frontend/options.c: i32 */
#define G_EMPIRE_LANGUAGE 0x00E39088u /* present/ui/empire/empire.c: u8: the city names' group, 0x15 (set) or 0xC3 */
#define G_CURSOR 0x00E390DCu /* core/command/military.c */
#define G_UE94E04 0x00E94E04u /* core/map/routing.c: 0, 2, 4, 6 */
#define G_ORDERS 0x00EADCC0u /* present/ui/info/info.h: i32 1: the storage orders sub-panel is shown */
#define G_INFO_TRANSPORT 0x00EADCC8u /* present/ui/info/info.h: i32 the transport ship figure (kind 6) */
#define G_INFO_FORMATION 0x00EADCCCu /* present/ui/info/info.h: i32 the formation (kinds 4..6: the panel's argument) */
#define G_PHRASE 0x00EADCDCu /* present/ui/info/phrase.c: i32 the phrase chosen (figure +0x7D) */
#define G_FIGURE_ROW 0x00EADCE4u /* present/ui/info/info.h: i32 the selected figure of the list */
#define G_FIGURES 0x00EADD24u /* present/ui/info/info.h: i32[7] the figures on the tile */
#define G_INFO_KIND 0x00EADD50u /* present/ui/info/info.h: i32 0 ?, 1 terrain or figures, 2 building, 4 legion, 5 warship, 6 transport */
#define G_INFO_BUILDING 0x00EADD6Cu /* core/command/orders_storage.c: i32 the building of the info window */
#define G_STORAGE_LIST 0x00F53E6Cu /* core/command/orders_storage.c: i32 count, then i32 resources (storage yards) */
#define G_ADVISOR_SLIDE 0x00F53F48u /* present/ui/advisors/a.h: i32 the window's slide-in step (pages draw a black frame while < 5) */
#define G_MOOD 0x00F55BA4u /* present/ui/info/phrase.c: u8[8] the city's mood per category (0x0053D3A0) */
#define G_VIEW_EMPTY_BASE 0x00F56B44u /* core/command/rotate.c: i32: + 0xFFF0000 marks a view position with no tile */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
