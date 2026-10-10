/* Imhotep - named globals: core/state/layout/g_command.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P4). */
#ifndef IMHOTEP_STATE_LAYOUT_G_COMMAND_H
#define IMHOTEP_STATE_LAYOUT_G_COMMAND_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
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
#define G_CONSTRUCTION_UI_Y2 0x006059D4u /* core/command/construction_ui.c [generic] */
#define G_CONSTRUCTION_UI_Y1 0x006059D8u /* core/command/construction_ui.c [generic] */
#define G_CONSTRUCTION_UI_X2 0x006059DCu /* core/command/construction_ui.c [generic] */
#define G_CONSTRUCTION_UI_X1 0x006059E0u /* core/command/construction_ui.c [generic] */
#define G_CONSTRUCTION_DRAG 0x006059E4u /* present/city/ghost.h: i32 1: a construction drag is in progress */
#define G_CONSTRUCTION_UI_TYPE 0x006059E8u /* present/city/ghost.h: i32 the building type chosen [generic] */
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
#define G_U784E78 0x00784E78u /* core/command/kingdom.c: i32 per resource: grave goods sent [unresolved] */
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
#define G_CONSTRUCTION_UI_SIZE 0x00BC96D0u /* core/command/place.c: @bin 0x00472EEF the type's size [generic] */
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
#define G_CONSTRUCTION_LIFT_COUNT 0x00BC971Cu /* core/command/clear.c: count of cleared items (charged by 0x004728A0) [generic] */
#define G_CONSTRUCTION_LIFT_COST 0x00BC9724u /* core/command/construction_lift.c [generic] */
#define G_PLACE_COST 0x00BC9728u /* core/command/place.c: @bin 0x004728C6 the construction's cost [generic] */
#define G_WALK_Y 0x00BC9760u /* core/command/clear.c: the game's loop globals */
#define G_WALK_X 0x00BC9764u /* core/map/last_terrain.c */
#define G_FERRY_BUILDING 0x00BC9800u /* present/city/ghost_ferry.c: i32 the first landing */
#define G_CAMERA_X 0x00BCA2BCu /* core/command/rotate.c */
#define G_CAMERA_Y 0x00BCA2C0u /* core/command/rotate.c */
#define G_MOUSE_OFFSET 0x00BCA2C4u /* present/sidebar/tooltip.c: i32 grid offset under the mouse */
#define G_CHEATS_MOUSE_X 0x00BCA2C8u /* core/command/cheats.c [single+clash] */
#define G_CHEATS_MOUSE_Y 0x00BCA2CCu /* core/command/cheats.c [single+clash] */
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
#define G_ORDERS_MESSAGES_MESSAGE_TYPES 0x00E20970u /* core/command/orders_messages.c: message type records, 0x50 bytes (+0x4A kind) [single+clash] */
#define G_LAYOUT_DY 0x00E38DA4u /* present/sidebar/sidebar.h: i32 (height - 480) / 2 */
#define G_LAYOUT_DX 0x00E38DA8u /* present/sidebar/sidebar.h: i32 (width - 640) / 2 */
#define G_GATE 0x00E38E60u /* core/command/construction_ui.c: u8 */
#define G_MUSIC_ON 0x00E38E64u /* core/frame/sound.c: u8 option: music */
#define G_TIPS_ON 0x00E38EACu /* present/sidebar/tooltip.c: i32 0 off, 2 on (options) */
#define G_SPEECH_VOLUME 0x00E38ECCu /* present/ui/frontend/options.c: i32 */
#define G_EMPIRE_LANGUAGE 0x00E39088u /* present/ui/empire/empire.c: u8: the city names' group, 0x15 (set) or 0xC3 */
#define G_CURSOR 0x00E390DCu /* core/command/military.c */
#define G_UE94E04 0x00E94E04u /* core/map/routing.c: 0, 2, 4, 6 [unresolved] */
#define G_ORDERS 0x00EADCC0u /* present/ui/info/info.h: i32 1: the storage orders sub-panel is shown */
#define G_INFO_TRANSPORT 0x00EADCC8u /* present/ui/info/info.h: i32 the transport ship figure (kind 6) */
#define G_INFO_FORMATION 0x00EADCCCu /* present/ui/info/info.h: i32 the formation (kinds 4..6: the panel's argument) */
#define G_PHRASE 0x00EADCDCu /* present/ui/info/phrase.c: i32 the phrase chosen (figure +0x7D) */
#define G_FIGURE_ROW 0x00EADCE4u /* present/ui/info/info.h: i32 the selected figure of the list */
#define G_FIGURES 0x00EADD24u /* present/ui/info/info.h: i32[7] the figures on the tile */
#define G_INFO_KIND 0x00EADD50u /* present/ui/info/info.h: i32 0 ?, 1 terrain or figures, 2 building, 4 legion, 5 warship, 6 transport [generic] */
#define G_INFO_BUILDING 0x00EADD6Cu /* core/command/orders_storage.c: i32 the building of the info window */
#define G_D_CALLBACKS_LIST_COUNT 0x00F53E6Cu /* core/command/orders_storage.c: i32 count, then i32 resources (storage yards) [generic] */
#define G_ADVISOR_SLIDE 0x00F53F48u /* present/ui/advisors/a.h: i32 the window's slide-in step (pages draw a black frame while < 5) */
#define G_MOOD 0x00F55BA4u /* present/ui/info/phrase.c: u8[8] the city's mood per category (0x0053D3A0) */
#define G_VIEW_EMPTY_BASE 0x00F56B44u /* core/command/rotate.c: i32: + 0xFFF0000 marks a view position with no tile */
/* --- end harvested --- */

#endif
