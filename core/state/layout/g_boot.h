/* Imhotep - named globals: core/state/layout/g_boot.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P6). */
#ifndef IMHOTEP_STATE_LAYOUT_G_BOOT_H
#define IMHOTEP_STATE_LAYOUT_G_BOOT_H

/* -- harvested names checked against their uses in core/boot, core/io, core/newgame (phase 10.3); @bin evidence: the file named in each comment -- */
#define G_STR_CAMPAIGN 0x00578D98u /* core/newgame/flow2.c: "campaign.txt" */
#define G_FILES_LIST 0x006057C8u /* present/ui/frontend/files.c: the dialog's file list object [generic] */
#define G_MONUMENT_FLAG 0x006DE514u /* present/city/ghost.c: u8 1: the type is a monument of 0xE5, 0xEA, 0xEB, 0xEC */
#define G_STRINGS_THIS 0x00784F20u /* present/ui/messages/expand.c: @bin 0x00448344 `this` of 0x004CBDB0: +4 entries, +0x0C count */
#define G_KEYWORDS_THIS 0x00784F34u /* present/ui/messages/expand.c: @bin 0x00448358 `this` of 0x004CB980: [this] = the keyword table */
#define G_TEXT_OUTLINE 0x00D3B510u /* present/draw/text.c: i32 != 0: plain (the other styles off) */
#define G_TEXT_SHADOW_V 0x00D3B518u /* present/draw/text.c: i32 != 0: black copies one pixel above and below */
#define G_CLIP_BOTTOM 0x00D3B550u /* present/ui/advisors/b_population.c: i32 the clip rectangle's bottom */
#define G_CLIP_TOP 0x00D3B554u /* present/ui/messages/window.c */
#define G_CLIP_RIGHT 0x00D3B558u /* present/ui/messages/window.c */
#define G_CLIP_LEFT 0x00D3B55Cu /* present/ui/messages/window.c */
#define G_LAST_NAME 0x00E370BCu /* present/ui/frontend/files.c: char[] the last saved game's name */
#define G_FILES_NAME 0x00E37100u /* present/ui/frontend/files.c: char[0x40] the typed saved game's name (field 2) [generic] */
#define G_SCREEN_H 0x00E38DACu /* present/sidebar/sidebar.h: i32 */
#define G_DESKTOP_H 0x00E38DB4u /* core/frame/display.c */
#define G_DESKTOP_W 0x00E38DB8u /* core/frame/display.c: the screen's size (GetSystemMetrics, WM_DISPLAYCHANGE) */
#define G_DRIVE 0x00E38E62u /* core/boot/sound.c: the CD drive's letter */
#define G_SPEECH_ON 0x00E38E65u /* present/ui/frontend/options.c: u8 */
#define G_RESOLUTION 0x00E38E68u /* core/frame/display.c: i32 1: 640 x 480, 2: 800 x 600, 3: 1024 x 768 */
#define G_SCROLL_SPEED 0x00E38E70u /* present/ui/frontend/options.c: i32 */
#define G_DD_READY 0x00E390E0u /* core/frame/display.c: the set-up succeeded */
#define G_HWND 0x00E39140u /* core/boot/display.c */
#define G_UE3914C 0x00E3914Cu /* core/frame/display.c: 0x555, 0x565, 0x556, 0x655 [unresolved] */
#define G_DD_CLIPPER 0x00E39170u /* core/boot/display.c */
#define G_MUSIC 0x00E39180u /* core/boot/sound.c: the music tables */
#define G_SPARE_SCREEN 0x00E92E1Cu /* present/ui/info/figure.c: i32 the spare screen's address */
#define G_FAMILY_LIST 0x00EADFA8u /* core/newgame/flow2.h */
#define G_BUILDING_SOUNDS 0x00F5467Cu /* present/ui/info/info.c: i32 per type (stride 0x14): the sound of the type's panel, -1: none */

/* -- named with the rewrite of core/boot, core/newgame/graphics.c -- */
/* WinMain and the CRT start-up */
#define G_CTOR_TABLE 0x00576000u /* @bin 0x005646C0 _cinit: the C++ static constructors' table, run by _initterm (docs/formats/state.md) */
#define G_CTOR_TABLE_END 0x0057607Cu /* @bin 0x005646C0 its end (exclusive) */
#define G_U6FDB38 0x006FDB38u /* @bin 0x00413F09 0x44 dwords WinMain clears first (the QuickTime movie fields 0x006FDC38.. among them, docs/provenance/frontier.md) */
#define G_U605A38 0x00605A38u /* @bin 0x00414180 i32: := 0 by WinMain; > 0: the map editor's button panel (present/ui/dispatch.c, present/sidebar/tooltip.c) */
#define G_U605A34 0x00605A34u /* @bin 0x00414180 i32: := 2 by WinMain; := the menu item - 1 by the construction UI's sub-command 0xE (core/command/construction_ui.c) */
#define G_NAME_PICK 0x006059FCu /* @bin 0x0040C830 i32: the name picked in window 0x51's list (present/ui/buttons.c, present/ui/frontend/highscore.c); -1 by WinMain */
#define G_U605760 0x00605760u /* @bin 0x00401010 i32: __ftol(G_DBL_10_56F340) * 200 by its static constructor */
/* the settings (0x004147F0, 0x00414A90) */
#define G_OPTIONS 0x00E38E58u /* @bin 0x0041491C the options: 0x234 bytes read from / written to "Pharaoh.inf" (core/frame/display.c); its first i32 the display's bits per pixel (16: 0x00414802, 0x00426B46) */
#define G_UE38E61 0x00E38E61u /* @bin 0x00414802 u8: default 0 */
#define G_UE38ED4 0x00E38ED4u /* @bin 0x00414802 i32: default 2 */
#define G_RAM 0x00E38ED8u /* @bin 0x00414AF9 i32: the "RAM" key of Pharaoh.ini (default 0xF; at most 0x17: game speed, scroll speed 100, resolution 1) */
#define G_UE38EE0 0x00E38EE0u /* @bin 0x00414802 i32: default 0 */
#define G_UE39084 0x00E39084u /* @bin 0x00414802 i32: default 1; 0: no god event in the month (core/city/day_city.c) */
#define G_CHECK_CD 0x00E39089u /* @bin 0x00414B34 u8: the "CheckCD" key of Pharaoh.ini (default 1, := 1 after Pharaoh.inf) */
#define G_UE36C6C 0x00E36C6Cu /* @bin 0x00414930 i32: := 1 after the settings files */
#define G_STR_ERR_SIERRA_INI 0x00579290u /* @bin 0x00414AB8 "Err:Could not read sierra.ini" */
#define G_STR_RAM 0x0057928Cu /* @bin 0x00414AF9 "RAM" */
#define G_STR_CHECK_CD 0x00579284u /* @bin 0x00414B34 "CheckCD" */
#define G_STR_CPU 0x00579280u /* @bin 0x00414B76 "CPU" */
#define G_CRT_CTYPE 0x00604C58u /* @bin 0x00561C0B u32: the CRT's ctype table (bit 8 blank, bit 4 digit) */
#define G_CRT_MB_CUR_MAX 0x00604E64u /* @bin 0x00561C0B i32: the CRT's multi-byte length (1 in the "C" locale; > 1: the multi-byte path) */
/* the text files (0x004CEC40) */
#define G_TEXT_HEADER 0x00E36B78u /* @bin 0x004CEC40 the 0x1C-byte header of Pharaoh_Text.eng (docs/formats/state.md) */
#define G_MM_TEXT_HEAD 0x00E209A0u /* @bin 0x004CEC40 the first 0x13898 bytes of Pharaoh_MM.eng: header and entries (docs/formats/text.md) */
/* the display set-up (0x004CF960) */
#define G_GAME_DIR 0x00E36B94u /* @bin 0x004CF97C char[100]: _getcwd, the game's directory (_chdir by 0x004CE2D0, core/newgame/map_load.c) */
#define G_UD3B37C 0x00D3B37Cu /* @bin 0x004CF960 i32: := 0x34518632 with the frame-tick shift registers G_R_IV1 / G_R_IV2 (docs/presentation.md 2.1) */
#define G_UD3B494 0x00D3B494u /* @bin 0x004CF960 four bytes := 0 */
#define G_UD3B508 0x00D3B508u /* @bin 0x004CF960 i32: := 0 with the text styles G_TEXT_SHADOW_V, G_TEXT_OUTLINE */
#define G_UE38E4C 0x00E38E4Cu /* @bin 0x004CF960 u8: := 0 */
#define G_UE38DA1 0x00E38DA1u /* @bin 0x004CF960 u8: := 1 */
#define G_HINSTANCE 0x00E39144u /* @bin 0x004CF960 the instance handle (its argument) */
/* the cursors (handles) the set-up loads, by resource id (0x004CFB25; core/frame/frame.c's table) */
#define G_CURSOR_66 0x00E390D8u /* @bin 0x004CFB25 cursor 0x66 (the default of core/frame/frame.c) */
#define G_CURSOR_88 0x00E390D4u /* @bin 0x004CFB25 cursor 0x88 */
#define G_CURSOR_82 0x00E390D0u /* @bin 0x004CFB25 cursor 0x82 */
#define G_CURSOR_93 0x00E390BCu /* @bin 0x004CFB25 cursor 0x93 */
#define G_CURSOR_8E 0x00E390CCu /* @bin 0x004CFB25 cursor 0x8E */
#define G_CURSOR_92 0x00E39094u /* @bin 0x004CFB25 cursor 0x92 */
#define G_CURSOR_96 0x00E390B8u /* @bin 0x004CFB25 cursor 0x96 */
#define G_CURSOR_95 0x00E390B0u /* @bin 0x004CFB25 cursor 0x95 */
#define G_CURSOR_94 0x00E390B4u /* @bin 0x004CFB25 cursor 0x94 */
#define G_CURSOR_99 0x00E390ACu /* @bin 0x004CFB25 cursor 0x99 */
#define G_CURSOR_98 0x00E390A8u /* @bin 0x004CFB25 cursor 0x98 */
#define G_CURSOR_9B 0x00E390A4u /* @bin 0x004CFB25 cursor 0x9B */
#define G_CURSOR_9A 0x00E390A0u /* @bin 0x004CFB25 cursor 0x9A */
#define G_CURSOR_9C 0x00E3909Cu /* @bin 0x004CFB25 cursor 0x9C */
#define G_CURSOR_9D 0x00E39098u /* @bin 0x004CFB25 cursor 0x9D */
#define G_CURSOR_9F 0x00E390C0u /* @bin 0x004CFB25 cursor 0x9F */
#define G_CURSOR_A0 0x00E39090u /* @bin 0x004CFB25 cursor 0xA0 */
/* the display's log lines (strings of the exe; core/frame/display.c logs them too) */
#define G_STR_DD_ENABLE_ERROR 0x005D1CB8u /* @bin 0x00426B40 "ERR:DD Enable error level:" */
#define G_STR_DD_SET_COOPERATIVE 0x005D1C74u /* @bin 0x00426B40 "OK :DD SetCooperative Level." */
#define G_STR_DD_KEPT_RESOLUTION 0x005D1C28u /* @bin 0x00426E09 "OK :DD Kept resolution." */
#define G_STR_DD_SET_RESOLUTION 0x005D1C40u /* @bin 0x00426CA4 "OK :DD Set resolution." */
#define G_STR_DD_VRAM_ONLY 0x005D1FBCu /* @bin 0x00427142 "OK :DD Creating one Vram only." */
#define G_STR_DD_VRAM_AND_SYSTEM 0x005D1F10u /* @bin 0x00427142 "OK :DD Creating one Vram and one system surface." */
#define G_STR_DD_CREATE_SURFACE_FAILED2 0x005D1F9Cu /* @bin 0x00426FF0 "ERR:DD CreateSurface failed2:" */
#define G_STR_DD_CAN_LOCK_PRIMARY 0x005D1F78u /* @bin 0x00426FF0 "OK :DD can lock primary surface " */
#define G_STR_DD_CANT_CREATE_BACKBUFFER 0x005D1ECCu /* @bin 0x00426FF0 "ERR:DD Cant create(W) backbuffer1." */
#define G_STR_DD_SURFACE_CREATED 0x005D1F44u /* @bin 0x00426FF0 "OK :DD Surface created." */
#define G_STR_DD_CANT_CREATE_CLIPPER 0x005D1EB0u /* @bin 0x0042726D "ERR:DD Cant create clipper." */
#define G_STR_DD_CLIPPER_ATTACHED 0x005D1E2Cu /* @bin 0x0042726D "OK :DD Surfaces created and clipper attached." */
#define G_STR_DD_RGB565 0x005D1DE4u /* @bin 0x004272F3 "OK :DD surface_format is RGB 565." */
#define G_STR_DD_RGB555 0x005D1D9Cu /* @bin 0x004272F3 "OK :DD surface_format is RGB 555." */
#define G_STR_GFX_DD_NOT_ENABLED 0x005E07BCu /* @bin 0x004D0220 "ERR:GFX DirectDraw not enabled." */
#define G_STR_GFX_BEGIN_FAILED 0x005E088Cu /* @bin 0x004D0220 "ERR:gfxBegin failed." */
#define G_STR_GFX_DD_ENABLED 0x005E0870u /* @bin 0x004D0220 "OK :GFX DirectDraw enabled." */
#define G_STR_DEBUG_REPORT 0x005E0730u /* @bin 0x004CF960 "Debug and status report on " */
/* the graphics slots (0x00414420; their names, strings of the exe) */
#define G_MAP_EDITOR_SETTINGS 0x00BCA2B8u /* @bin 0x00414430 0x54 bytes: the map editor's settings, read from / written to "Pharaohmap.inf" (core/frame/display.c) */
#define G_STR_PHARAOHMAP_INF 0x0057924Cu /* @bin 0x00414430 "Pharaohmap.inf" */
#define G_U5E06D0 0x005E06D0u /* @bin 0x00414420 i32: := 1 by the graphics boot */
#define G_STR_OK_SETUP_LOADED 0x005790A4u /* @bin 0x00414420 "OK :Setup loaded DAT files." */
#define G_STR_PHARAOH_UNLOADED 0x00579218u /* @bin 0x0041448A "Pharaoh_Unloaded" */
#define G_STR_SPRMAIN 0x00579210u /* @bin 0x0041448A "SprMain" */
#define G_STR_HYKSOS 0x00579208u /* @bin 0x0041448A "Hyksos" */
#define G_STR_PHARAOH_GENERAL 0x005791F8u /* @bin 0x0041448A "Pharaoh_General" */
#define G_STR_PHARAOH_TERRAIN 0x005791E8u /* @bin 0x0041448A "Pharaoh_Terrain" */
#define G_STR_SPHINX1A 0x005791DCu /* @bin 0x0041448A "SPHINX1A" */
#define G_STR_SPHINX1B 0x005791D0u /* @bin 0x0041448A "SPHINX1B" */
#define G_STR_SPHINX1C 0x005791C4u /* @bin 0x0041448A "SPHINX1C" */
#define G_STR_SPHINX1D 0x005791B8u /* @bin 0x0041448A "SPHINX1D" */
#define G_STR_SPHINX_EXTRA 0x005791A8u /* @bin 0x0041448A "SPHINX_EXTRA" */
#define G_STR_TEMPLE_NILE 0x0057919Cu /* @bin 0x0041448A "TEMPLE_NILE" */
#define G_STR_EMPIRE 0x00579194u /* @bin 0x0041448A "empire" */
#define G_STR_SPRAMBIENT 0x00579188u /* @bin 0x0041448A "SprAmbient" */
#define G_STR_PHARAOH_FONTS 0x00579178u /* @bin 0x0041448A "Pharaoh_Fonts" */
#define G_STR_EGYPTIAN 0x0057916Cu /* @bin 0x0041448A "Egyptian" */
#define G_STR_SPRMAIN2 0x00579160u /* @bin 0x0041448A "SprMain2" */
#define G_STR_EXPANSION 0x00579154u /* @bin 0x0041448A "expansion" */
/* the sound tables (0x005394F0) */
#define G_AMBIENT_SOUNDS 0x00F54550u /* @bin 0x0053BC80 i32[12] per terrain class (stride 0x30): up to 10 variants, +0x28 its own, +0x2C the count */
#define G_BUILDING_SOUND_TABLE 0x00F54670u /* @bin 0x00539CF0 i32[5] per building type (stride 20): 3 variants, +0xC its own (G_BUILDING_SOUNDS), +0x10 the count */
#define G_INTERFACE_SOUNDS 0x00F543B8u /* @bin 0x0053AF90 i32[100]: the interface sounds' samples (-1: none) */
#define G_FIGURE_SOUNDS 0x00F54120u /* @bin 0x0053B130 i32[0xA5]: the figure sounds' samples, 5 per sound (-1: none) */
/* the steps' tables (0x00541310, 0x0044EFA0, 0x00541D50, 0x004DEE10) */
#define G_TRADE_RECOMMENDS 0x007758A0u /* @bin 0x00541310 i32[4][6] from trade_recommends.txt (by population: core/city/day_city.c) */
#define G_SUN_TEMPLE_PARTS 0x00601DD8u /* @bin 0x00541D50 100 parts of 0x34 bytes (x, y, size: core/building/map_add_monument.c) */
#define G_PART_OFFSETS 0x005E2AE8u /* @bin 0x004DEE10 116 records of 0x34 bytes: part k's place in the four orientations, as offsets */
#define G_TIMING_CALLS 0x00E390FCu /* @bin 0x004CF8E0 i32: the time (ms) of three calls of 0x0051F780 */
#define G_TIMING_PRESENTS 0x00E390F4u /* @bin 0x004CF8E0 i32: the time (ms) of three presents */
#define G_FRAME_TIME 0x00E39104u /* @bin 0x004CF8E0 i32: the sum of both (the sidebar's step, present/sidebar/sidebar.c) */
#define G_STR_ERR_NO_SPARE_SCREEN_MODEL 0x00601BA8u /* @bin 0x00541310 "ERR:no spare screen for model" */
#define G_STR_TRADE_RECOMMENDS_TXT 0x00601DC0u /* @bin 0x00541310 "trade_recommends.txt" */
#define G_STR_ERR_NO_TRADE_RECOMMENDS 0x00601D9Cu /* @bin 0x00541310 "ERR:No trade_recommends.txt file" */
#define G_STR_OK_TRADE_RECOMMENDS 0x00601D10u /* @bin 0x00541310 " OK: trade_recommends data loaded" */
#define G_STR_ERR_NO_SPARE_SCREEN_REASON 0x005DA298u /* @bin 0x0044EFA0 "ERR:no spare screen for reason table" */
#define G_STR_AUTO_REASON_PHRASES_TXT 0x005DA280u /* @bin 0x0044EFA0 "auto reason phrases.txt" */
#define G_STR_ERR_NO_AUTO_REASON_PHRASES 0x005DA260u /* @bin 0x0044EFA0 "ERR:No auto reason phrases file" */
#define G_STR_OK_REASON_TABLE 0x005DA1CCu /* @bin 0x0044EFA0 " OK: reason table loaded" */
/* the static constructors (core/boot/ctors.c, monuments.c) */
#define G_VTBL_SCENARIO_LIST 0x0056F54Cu /* @bin 0x0041E0A0 the scenario list's vtable */
#define G_VTBL_MONUMENT 0x0056F698u /* @bin 0x004E5210 the monument objects' vtable */
#define G_VTBL_MONUMENT_BASE2 0x005701A0u /* @bin 0x00545F88 the vtable of the monuments built on 0x00545F80 */
#define G_STR_SUNTEMPLE1 0x005E4578u /* @bin 0x004E5210 "suntemple1" */
#define G_STR_SUNTEMPLE2 0x005E456Cu /* @bin 0x004E5210 "suntemple2" */
#define G_STR_SUNTEMPLE3 0x005E4560u /* @bin 0x004E5210 "suntemple3" */
#define G_KEYWORD_TABLE 0x005D2EC8u /* @bin 0x004482D0 the keyword table ({index, name, handler}, 0x0C bytes: present/ui/messages/keywords.c) */
#define G_STR_EVENTMSG_TXT 0x005D9A2Cu /* @bin 0x004482F0 "EVENTMSG.TXT" */
#define G_EVENT_PHRASE_KEYS 0x005D3010u /* @bin 0x004482F0 the event phrases' key table ({?, name}, 8 bytes, a 0 name last) */
#define G_VEGETATION 0x00978D88u /* @bin 0x00486090 chunk 145: the vegetation growth state, 20 bytes (save.md §3) */
#define G_DBL_10_56F340 0x0056F340u /* @bin 0x00401010 double 10.0 */
#define G_DBL_10_56F5A8 0x0056F5A8u /* @bin 0x00489950 double 10.0 (core/state/scenario_start.c) */
#define G_DBL_9792 0x0056F680u /* @bin 0x004BD310 double 9792.0 (core/city/day_flood.c) */
#define G_DBL_25 0x0056F678u /* @bin 0x004BD310 double 25.0 (core/city/day_flood.c) */
#define G_REGISTRY_INSTANCES 0x012663B0u /* @bin 0x0055DBC0 u32: an instance counter, += 0xC by each sound registry */
#define G_SOUND_INIT_RECORDS 0x00604628u /* @bin 0x0055E4CC 26 records of 3 dwords copied into the sound object's +0xC */

/* -- named with the rewrite of core/newgame/flow.c, flow2.c, figures.c, setup.c -- */
#define G_CPU 0x00E38EDCu /* @bin 0x00414B76 i32: the "CPU" key of the settings file (default 0x42); the period videos take Binks\high\ from 0xE9 on (0x0041BBC8) */
#define G_UE38DD6 0x00E38DD6u /* @bin 0x00413F09 u8: WinMain clears it, sets 1 after 0x004CF8E0; 2 when a city starts (0x00418301) (docs/formats/state.md) */


/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_UE38D9C 0x00E38D9Cu /* present/ui/empire/empire.c: i32 the screen width (set-up copy) [unresolved] */
/* --- end harvested --- */

#endif
