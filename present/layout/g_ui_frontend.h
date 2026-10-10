/* Imhotep - named globals: present/layout/g_ui_frontend.h, the front end and the empire / mission
 * screens (present/ui/frontend, present/ui/empire).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8d). @bin evidence: on each name's line (the port
 * that shows the use; strings and doubles read in the reference exe). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_FRONTEND_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_FRONTEND_H

/* ---- image groups: the first image of a group, `this` (G_GRAPHICS_GFX) + 8 group + 4
 * (docs/formats/graphics.md) ---- */
#define G_FULLSCREEN_39_IMAGE 0x00F55BD4u  /* @bin 0x00417EE0 window 0x39's picture */
#define G_FULLSCREEN_38_IMAGE 0x00F55BDCu  /* @bin 0x00417EB0 window 0x38's picture */
#define G_FULLSCREEN_37_IMAGE 0x00F55BE4u  /* @bin 0x00417E80 window 0x37's picture */
#define G_TITLE_IMAGE 0x00F55BECu          /* @bin 0x00520E00 window 0x3C's title picture */
#define G_FULLSCREEN_3A_IMAGE 0x00F55BFCu  /* @bin 0x00417F10 window 0x3A's picture */
#define G_MISSION_CHOICE_IMAGE 0x00F55C14u /* @bin 0x0041AAA0 window 0x34's background */
#define G_CAMPAIGN_MAP_IMAGES 0x00F55C1Cu  /* @bin 0x0041AAA0 the campaign steps' maps (+ step +0x1C) */
#define G_EDITOR_LIST_IMAGE 0x00F55C2Cu    /* @bin 0x0041E5C0 window 0x3D's picture in the editor */
#define G_MISSION_IMAGES 0x00F55C44u       /* @bin 0x0041A850 the victory picture (+0), the mission start picture (+1) */
#define G_MISSION_MARK_IMAGES 0x00F55C54u  /* @bin 0x00419140 the missions' marks: +0, +1 hovered, +2 chosen */
#define G_PERIOD_IMAGES 0x00F55C94u        /* @bin 0x0041BE10 Pharaoh's period pictures (+ period) */
#define G_FAMILY_LIST_IMAGE 0x00F55C9Cu    /* @bin 0x00516630 window 0x2F's picture */
#define G_FAMILY_MENU_IMAGE 0x00F55CACu    /* @bin 0x004CB4D0 window 0x30's picture */
#define G_SCENARIO_LIST_IMAGE 0x00F55CB4u  /* @bin 0x0041E5C0 window 0x3D's picture, list mode 0 */
#define G_PERIOD_SCREEN_IMAGE 0x00F55CBCu  /* @bin 0x0041C2D0 window 0x17's picture (and 0x3D's in list mode 1) */
#define G_ROUTE_IMAGES 0x00F568C4u         /* @bin 0x0052F8B0 the routes: +1 their dots, +2 their points */
#define G_TRADE_SIZE_IMAGES 0x00F56974u    /* @bin 0x0052DD41 the trade size marks: +0 above 0, +1 above 15, +2 above 25 */
#define G_EMPIRE_FRAME_IMAGES 0x00F5697Cu  /* @bin 0x0052D630 the empire frame: +0 sides, +1 borders, +2 corners, +3 bands */
#define G_ARMY_IMAGE_OURS 0x00F569ACu      /* @bin 0x0052F8B0 an army marker, the player's (0x00530265: `ours`) */
#define G_ARMY_IMAGE_OTHER 0x00F569B4u     /* @bin 0x0052F8B0 an army marker, the others' (route kind 2) */
#define G_FLAG_IMAGES 0x00F56B0Cu          /* @bin 0x0052FDF2 the cities' animated flag */
#define G_EMPIRE_MAP_IMAGE 0x00F56DB4u     /* @bin 0x0052F8B0 the empire map picture */
#define G_PERIOD_IMAGES_B 0x00F57314u      /* @bin 0x0041BE10 Cleopatra's period pictures (periods 5..8: +4, 2, 5, 7) */

/* ---- strings and constants of the exe ---- */
#define G_DBL_1_3 0x0056F518u        /* @bin 0x0041C380 double 1/3 */
#define G_DBL_1_30 0x0056F528u       /* @bin 0x0041C380 double 1/30 */
#define G_DBL_20 0x0056F530u         /* @bin 0x0041C380 double 20.0 */
#define G_DBL_0_00015625 0x0056F538u /* @bin 0x0041C380 double 0.00015625 */
#define G_DBL_0_002 0x0056F540u      /* @bin 0x0041C380 double 0.002 */
#define G_STR_SAV 0x005761D4u        /* @bin 0x00402720 "sav", the saved games' extension */
#define G_STR_FANFARE_NU5 0x0057C018u /* @bin 0x00419090 "wavs\Fanfare_Nu5.wav" */
#define G_STR_FANFARE_NU1 0x0057C030u /* @bin 0x00419090 "wavs\Fanfare_Nu1.wav" */
#define G_STR_KEY_1 0x005F8A00u      /* @bin 0x005309B0 "1": the number pad's keys "1" .. "9", "0", 4 bytes apart downwards */
#define G_STR_MISSION_VOICE_DIR 0x005F90B8u /* @bin 0x00539B20 "audio\voice\mission\" */
#define G_MISSION_SPEECH 0x00579804u        /* @bin 0x0041A850 the missions' speech files, 0x28 bytes each ("200_mission.mp3") */
#define G_MISSION_SPEECH_B 0x0057A164u      /* @bin 0x0041A850 the same with G_EMPIRE_LANGUAGE set ("200_mission_classic.mp3") */
#define G_VICTORY_SPEECH 0x0057AAC4u        /* @bin 0x0041A850 the victory speech files, 0x28 bytes each ("200_victory.mp3") */
#define G_VICTORY_SPEECH_B 0x0057B424u      /* @bin 0x0041A850 the same with G_EMPIRE_LANGUAGE set */

/* ---- button and rectangle tables (0x1C bytes per entry: x0, y0, x1, y1 i16, +8 the function) ---- */
#define G_GOAL_BOXES 0x005797D4u           /* @bin 0x0041A3E9 i32 (x, y) pairs: the briefing's goal boxes; the hint's at +0x20 */
#define G_SCENARIO_LIST_BUTTONS 0x0057CC30u /* @bin 0x0041E100 window 0x3D's buttons */
#define G_MISSION_LIST_BUTTONS 0x0057CCC0u /* @bin 0x0041E100 its buttons on the Cleopatra missions */
#define G_SCENARIO_LIST_TAB 0x0057CD50u    /* @bin 0x0041E120 the tab of the mission list */
#define G_DIFFICULTY_BUTTONS 0x0057CD70u   /* @bin 0x0041E130 the difficulty buttons */
#define G_SCENARIO_LIST_ROWS 0x0057CDB8u   /* @bin 0x0041E190 the rows' rectangles */
#define G_MISSION_LIST_ROWS 0x0057CF60u    /* @bin 0x0041E190 the rows' rectangles on the Cleopatra missions */
#define G_NAME_ROWS 0x005C4358u            /* @bin 0x0040C830 window 0x51's names (43 rectangles) */
#define G_NAME_BACK 0x005C47F0u            /* @bin 0x0040C810 window 0x51's back rectangle */
#define G_LIST_EXTRA_BUTTON 0x005C5E78u    /* @bin 0x00530D70 the pop-up list's extra button (placed by it) */
#define G_MISSION_END_BUTTONS_1 0x005C6628u /* @bin 0x004EA8D0 the mission end's buttons, case 1 */
#define G_MISSION_END_BUTTONS_2 0x005C6648u /* @bin 0x004EA8D0 case 2 */
#define G_MISSION_END_BUTTONS_3 0x005C6680u /* @bin 0x004EA8D0 case 3 */
#define G_HOTKEY_RECTS 0x005C6DF0u         /* @bin 0x00537260 window 0x46's second key boxes (5); the first ones 0x118 below */
#define G_FILE_DIALOG_BUTTONS 0x005C79F0u  /* @bin 0x00532340 the file dialog's buttons */
#define G_MAP_DIALOG_BUTTONS 0x005C7A80u   /* @bin 0x005325F0 the editor's map dialog's buttons */
#define G_DIFFICULTY_ARROWS 0x005C8EE8u    /* @bin 0x00531BE0 window 0x45's arrows */
#define G_FAMILY_MENU_BUTTONS 0x005E0408u  /* @bin 0x004CB4D0 window 0x30's buttons */
#define G_FAMILY_LIST_BUTTONS 0x005F7EE0u  /* @bin 0x00516630 window 0x2F's buttons */
#define G_FAMILY_RECTS 0x005F7F28u         /* @bin 0x00516720 window 0x2F: 12 rows, then the four buttons */

/* ---- the number pad (window 0x0E) and the pop-up list ---- */
#define G_NUMPAD_FIELD32 0x0060595Cu  /* @bin 0x004035F0 u32: the i32 field the value goes to (type 4) */
#define G_NUMPAD_FIELD16 0x00605960u  /* @bin 0x004035F0 u32: the i16 field (type 2) */
#define G_NUMPAD_FIELD8 0x00605964u   /* @bin 0x004035F0 u32: the byte field (type 1) */
#define G_NUMPAD_ACTION 0x00605968u   /* @bin 0x004035F0 i32: after OK, 1 the map's period, 2 0x00444270 */
#define G_U605A10 0x00605A10u         /* @bin 0x004035F0 i32: action 2 passes it - 1 to 0x00444270 */
#define G_NUMPAD_PERIOD 0x00605A14u   /* @bin 0x004035F0 i32: action 1, the map's period + 1 */
#define G_NUMPAD_CALLBACK 0x00605A4Cu /* @bin 0x004035F0 u32: a function called after OK (cleared) */
#define G_LIST_EXTRA_LABEL 0x00605A54u /* @bin 0x00530D70 i32: the extra button's label (group 0x130); 0: callback 0x004092B0 */
#define G_LIST_EXTRA_ON 0x00605A58u   /* @bin 0x00530D70 i32: the pop-up list has the extra button */

/* ---- the campaign, the missions, the high scores ---- */
#define G_CAMPAIGN_MISSION_NAMES 0x006FF60Cu /* @bin 0x004C5660 char[0x32] per mission: the names in the campaign table */
#define G_MISSION_RECORDS 0x007009DCu  /* @bin 0x0041C600 the family's best record by mission, 0x4C bytes (as G_HIGHSCORE_TABLE's) */
#define G_SCENARIO_PICTURE 0x00784B5Eu /* @bin 0x0041E5C0 i16: the scenario's picture, Pharaoh's periods below 0x13, else Cleopatra's */
#define G_U784CB4 0x00784CB4u          /* @bin 0x0052F8B0 i32: in the editor, != 0 frames an object at its other place (+0x14, +0x16) */
#define G_U784F0C 0x00784F0Cu          /* @bin 0x00447840 i32: a string of group 0x98 put with the player's name; := 0 while a title is expanded */
#define G_HIGHSCORE_PREV_NAME 0x00D386E0u /* @bin 0x004C5A40 char[]: the name of the record's holder before it is replaced */
#define G_UE36EFC 0x00E36EFCu          /* @bin 0x004CDFF0 i32: := 0 when the file list is read */
#define G_UF53F28 0x00F53F28u          /* @bin 0x00520AB0 i32: := 100 by the front end's full redraws */
#define G_EMPIRE_ANIM 0x00F53FD8u      /* @bin 0x0052F8B0 i32: 0..100, + 1 per empire map draw (the armies' markers) */
#define G_ROUTE_ANIM 0x00F53FE0u       /* @bin 0x0052F8B0 i32: 0..100, + 1 per draw: the editor's chosen route's marker */
#define G_MISSION_SPEECH_PATH 0x00F558F4u      /* @bin 0x00539B20 char[]: the mission speech's path */

#endif
