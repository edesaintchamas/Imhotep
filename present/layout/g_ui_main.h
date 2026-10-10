/* Imhotep - named globals: present/layout/g_ui_main.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8a). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_MAIN_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_MAIN_H

/* --- named by P8a (present/ui, present/ui/messages): @bin evidence on each line (the port that
 * shows the meaning; strings quoted from the binary). Rectangle tables have 0x1C-byte entries,
 * button tables 0x28-byte entries (sidebar buttons_draw). --- */
#define G_FLT_1 0x0056F484u /* @bin 0x004E61F0 float 1.0: the F5 / F6 step of the debug value G_ANIM_DEBUG_VALUE */
#define G_FLT_MINUS_9 0x0056F554u /* @bin 0x00434C81 float -9.0 */
#define G_FLT_0_0003125 0x0056F558u /* @bin 0x00434C81 float 0.0003125 */
#define G_FLT_80 0x0056F6E0u /* @bin 0x004E61F0 float 80.0: the debug value's upper limit */
#define G_FLT_10 0x0056F6E4u /* @bin 0x004E61F0 float 10.0: the debug value's lower limit */
#define G_CHEAT_OK_RECT 0x0057C560u /* @bin 0x0041D310 the cheat window's OK rectangle (just after the 34 entries of G_CHEAT_TABLE) */
#define G_OVL_GROUP_IDS 0x00576180u /* @bin 0x00409E40 i32[]: the overlay group of each row of the overlays menu */
#define G_EDITOR_MENU_RECTS 0x005763D8u /* @bin 0x004D86E1 the editor menu's rectangles (window 1 in the editor, docs/provenance/frontier.md) */
#define G_TUTORIAL_MISSIONS 0x00579654u /* @bin 0x00417B40 i32 per tutorial step 0x0F..0x19 (stride 0x1C): its mission */
#define G_MISSION_SCREEN_BUTTON 0x00579788u /* @bin 0x004D8310 button table (1 entry) of windows 0x34, 0x35, 0x3B and 0x36 */
#define G_CAMPAIGN_MAP_BUTTON 0x005797B0u /* @bin 0x00418350 window 0x35's button (campaign_map_pick) */
#define G_CAMPAIGN_BUTTONS 0x0057BE30u /* @bin 0x0041BB30 window 0x17's start / back buttons (present/ui/frontend/campaign.c) */
#define G_OVL_GROUP_RECTS 0x005C2BF0u /* @bin 0x004D8C08 the overlays menu's groups (window 8) */
#define G_OVL_ITEM_RECTS 0x005C2D08u /* @bin 0x004D8C08 the open overlay group's items (window 8) */
#define G_DISPLAY_OPTIONS_RECTS 0x005C2E20u /* @bin 0x004D8310 window 0x0A's rectangles (present/ui/frontend/options.c) */
#define G_MESSAGE_OPTIONS_RECTS 0x005C2EB0u /* @bin 0x004D8310 window 0x56's rectangles (0x00402CC0; docs/commands.md) */
#define G_SPEED_OPTIONS_RECTS 0x005C3038u /* @bin 0x004D8310 window 0x0C's rectangles (present/ui/frontend/options.c) */
#define G_SOUND_OPTIONS_RECTS 0x005C3070u /* @bin 0x004D8310 window 0x0B's rectangles (present/ui/frontend/options.c) */
#define G_FILE_ROWS_RECTS 0x005C3118u /* @bin 0x004D8310 the file dialogs' rows (windows 0x0D, 0x10; present/ui/frontend/files.c) */
#define G_EMPIRE_EDITOR_RECTS 0x005C3268u /* @bin 0x004D9CB5 the empire editor's rectangles (window 0x15; docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_RECTS_B 0x005C3348u /* @bin 0x004D9CB5 a rectangle of the empire editor, opens window 0x4E (docs/provenance/frontier.md) */
#define G_W16_RECTS 0x005C3368u /* @bin 0x004D8310 window 0x16's rectangles (editor; docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_RECTS_C 0x005C33C0u /* @bin 0x004D9CB5 empire editor rectangles (docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_RECTS_D 0x005C3418u /* @bin 0x004D9E33 empire editor rectangles, routes (docs/provenance/frontier.md) */
#define G_W24_RECTS 0x005C3488u /* @bin 0x004D8310 window 0x24's rectangles (opened from the empire editor) */
#define G_W4E_RECTS 0x005C3648u /* @bin 0x004D8310 window 0x4E's rectangles (opened from the empire editor) */
#define G_OPEN_ROUTE_RECT 0x005C3DA0u /* @bin 0x004D9A47 the empire map's open-route button (0x00409490) */
#define G_EMPIRE_EDITOR_RECTS_E 0x005C3DC0u /* @bin 0x004D9CB5 empire editor rectangles (docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_RECTS_F 0x005C3E68u /* @bin 0x004D9CB5 empire editor rectangle (docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_RECTS_G 0x005C3E88u /* @bin 0x004D9CB5 empire editor rectangles (docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_SELLS_RECTS 0x005C3EC0u /* @bin 0x004DB24A empire editor: a city's goods sold, 14 entries (docs/provenance/frontier.md) */
#define G_EMPIRE_EDITOR_BUYS_RECTS 0x005C4048u /* @bin 0x004DB271 empire editor: a city's goods, 16 entries (docs/provenance/frontier.md) */
#define G_NUMPAD_RECTS 0x005C4208u /* @bin 0x004D8310 window 0x0E's rectangles (present/ui/frontend/numpad.c) */
#define G_ADVISOR1_RECTS 0x005C4AB8u /* @bin 0x004DBF4E advisor page 1's rectangles */
#define G_PRIORITY_RECTS 0x005C4BB8u /* @bin 0x0040A730 window 0x2A's rectangles: a labour category's priority (docs/commands.md) */
#define G_ADVISOR2_RECTS 0x005C4CD0u /* @bin 0x004DBF4E advisor page 2's rectangles */
#define G_ADVISOR2_PANEL_A 0x005C4EC8u /* @bin 0x00522A60 advisor page 2's first tail panel */
#define G_ADVISOR2_PANEL_B 0x005C4EE4u /* @bin 0x00522A60 advisor page 2's second tail panel */
#define G_ADVISOR3_RECTS 0x005C4F00u /* @bin 0x004DBF4E advisor page 3's rectangles */
#define G_ADVISOR3_PANEL_A 0x005C50F8u /* @bin 0x00523BB0 advisor page 3's first tail panel */
#define G_ADVISOR3_PANEL_B 0x005C5114u /* @bin 0x00523BB0 advisor page 3's second tail panel */
#define G_ADVISOR7_RECTS 0x005C5130u /* @bin 0x004DBF4E advisor page 7's rectangles */
#define G_ADVISOR11_RECT 0x005C5168u /* @bin 0x004DBF4E advisor page 11's first rectangle */
#define G_GOD_RECTS 0x005C51A8u /* @bin 0x0040AC50 window 0x2B's rectangles: choose the god (docs/commands.md) */
#define G_W52_RECTS 0x005C5288u /* @bin 0x004D8310 window 0x52's rectangles (the festival dialog from the festival square) */
#define G_ADVISOR4_RECTS 0x005C5368u /* @bin 0x004DBF4E advisor page 4's rectangles */
#define G_SALARY_RECTS 0x005C5468u /* @bin 0x0040AF30 windows 0x2C / 0x4D: the salary ranks (docs/commands.md) */
#define G_W2D_RECTS 0x005C55B8u /* @bin 0x004D8310 window 0x2D's rectangles */
#define G_W2E_RECTS 0x005C5680u /* @bin 0x004D8310 window 0x2E's rectangles */
#define G_W4C_RECTS 0x005C5710u /* @bin 0x0040B1F0 window 0x4C's rectangles (docs/commands.md) */
#define G_W4C_ARROWS 0x005C5768u /* @bin 0x004D8310 window 0x4C's square buttons */
#define G_W36_RECT 0x005C5AF8u /* @bin 0x004D8178 window 0x36's rectangle */
#define G_W25_RECTS 0x005C5CD0u /* @bin 0x004D8310 window 0x25's rectangles (editor; docs/provenance/frontier.md) */
#define G_W19_RECTS 0x005C5E20u /* @bin 0x004D8310 window 0x19's rectangles (editor; docs/provenance/frontier.md) */
#define G_W1B_RECTS 0x005C5EB0u /* @bin 0x004D8310 window 0x1B's rectangles (editor; docs/provenance/frontier.md) */
#define G_W1C_RECTS 0x005C63D8u /* @bin 0x004D8310 window 0x1C's rectangles (editor; docs/provenance/frontier.md) */
#define G_W43_RECTS 0x005C65D0u /* @bin 0x004EA770 window 0x43's rectangles (docs/presentation.md 3.2, docs/commands.md) */
#define G_W3E_RECT 0x005C68A8u /* @bin 0x004D8310 window 0x3E's rectangle (present/ui/frontend/screens.c) */
#define G_HOTKEY_RECTS_FIRST 0x005C6CD8u /* @bin 0x004D8310 window 0x46's rectangles (present/ui/frontend/options.c) */
#define G_W4B_RECTS 0x005C6F08u /* @bin 0x004D8310 window 0x4B's rectangles (editor; docs/provenance/frontier.md) */
#define G_ADVISOR_HELP_BUTTON 0x005C7B80u /* @bin 0x004DBF4E the advisors' help button (present/ui/advisors/callbacks.c) */
#define G_FESTIVAL_BUTTONS 0x005C7BC8u /* @bin 0x0040AB00 window 0x2B's buttons (docs/commands.md) */
#define G_W52_BUTTONS 0x005C7C58u /* @bin 0x004D8310 window 0x52's buttons (present/ui/advisors/callbacks.c) */
#define G_EMPIRE_HELP_BUTTON 0x005C7CE8u /* @bin 0x00409510 the empire map's help button */
#define G_EMPIRE_CLOSE_BUTTON 0x005C7D10u /* @bin 0x004D9A47 the empire map's second button (bottom right) */
#define G_EMPIRE_BUTTON3 0x005C7D38u /* @bin 0x004D9A47 the empire map's third button */
#define G_W26_BUTTONS 0x005C7D60u /* @bin 0x004D8310 window 0x26's buttons */
#define G_MW_BACK_BUTTON 0x005C7DA8u /* @bin 0x004C7EE0 the message window's back button */
#define G_MW_CLOSE_BUTTON 0x005C7DD0u /* @bin 0x004C7340 the message window's close button */
#define G_ARROW_UP_SHOWN 0x005C7E10u /* @bin 0x004C5E60 u8 +0x18 of G_ARROWS_UP: 1 while the text scrolls */
#define G_ARROW_DOWN_SHOWN 0x005C7E38u /* @bin 0x004C5E60 u8 +0x18 of G_ARROWS_DOWN */
#define G_MW_HELP_BUTTON 0x005C7E48u /* @bin 0x004E2360 the message window's button without an advisor (msgwin_close_help) */
#define G_MW_GO_TO_BUTTON 0x005C7E70u /* @bin 0x004E2380 the message window's go-to button */
#define G_MW_ADVISOR1_BUTTON 0x005C7EC0u /* @bin 0x004C7340 the message window's button to advisor 1 */
#define G_MW_ADVISOR5_BUTTON 0x005C7EE8u /* @bin 0x004C7340 the button to advisor 5 */
#define G_MW_ADVISOR4_BUTTON 0x005C7F38u /* @bin 0x004C7340 the button to advisor 4 */
#define G_MW_ADVISOR2_BUTTON 0x005C7F88u /* @bin 0x004C7340 the button to advisor 2 */
#define G_MW_ADVISOR3_BUTTON 0x005C7FB0u /* @bin 0x004C7340 the button to advisor 3 */
#define G_MW_ADVISOR6_BUTTON 0x005C7FD8u /* @bin 0x004C7340 the button to advisor 6 */
#define G_MW_ADVISOR7_BUTTON 0x005C8050u /* @bin 0x004C7340 the button to advisor 7 */
#define G_POPUP_BUTTON_TABLE 0x005C8168u /* @bin 0x00425640 the yes / no popup's buttons (docs/commands.md) */
#define G_MISSION_SCREEN_BUTTONS 0x005C81B0u /* @bin 0x004D8310 windows 0x35 / 0x3B: 2 buttons at +0x6E, +0x1A0 */
#define G_EMPIRE_EDITOR_BUTTONS 0x005C8C60u /* @bin 0x004074E0 the empire editor's square buttons (docs/provenance/frontier.md) */
#define G_TAX_ARROWS 0x005C8D08u /* @bin 0x0040A260 advisor page 1's tax arrows (docs/commands.md) */
#define G_W2D_ARROWS 0x005C8D68u /* @bin 0x004D8310 window 0x2D's square buttons */
#define G_SPEED_ARROWS 0x005C8D98u /* @bin 0x004D8310 window 0x0C's arrows (present/ui/frontend/options.c) */
#define G_SOUND_ARROWS 0x005C8DF8u /* @bin 0x004D8310 window 0x0B's arrows (present/ui/frontend/options.c) */
#define G_EDITOR_MENU_BUTTONS 0x005C8EB8u /* @bin 0x00409430 the editor menu's square buttons (docs/provenance/frontier.md) */
#define G_DIFFICULTY_ARROWS 0x005C8EE8u /* @bin 0x004D8310 window 0x45's arrows (present/ui/frontend/options.c) */
#define G_GOD_MENU_GODS 0x005CF0DCu /* @bin 0x00520632 i32 per row of build menu 0x1C: its god (1-based index of G_SCENARIO_GODS) */
#define G_OVL_GROUP_OVERLAYS 0x005CF734u /* @bin 0x00409DA0 i32 per overlay group: its overlay, also its name in text group 0x0E */
#define G_OVL_ITEMS 0x005CF75Cu /* @bin 0x00409E00 i32 [group * 30 + row]: the overlay of a group's row, its name (group 0x0E) */
#define G_EVENT_LIST_BUTTONS 0x005D9A48u /* @bin 0x004D8310 window 0x27's buttons (editor event list; docs/provenance/frontier.md) */
#define G_EVENT_LIST_RECTS 0x005D9A90u /* @bin 0x004D8310 window 0x27's rectangles (docs/provenance/frontier.md) */
#define G_EVENT_RECTS 0x005D9E20u /* @bin 0x004D8310 window 0x18's rectangles (editor; docs/provenance/frontier.md) */
#define G_W53_BUTTONS 0x005E01D0u /* @bin 0x004C5400 window 0x53's buttons (present/ui/frontend/highscore.c) */
#define G_REPLY_RECT_1 0x005E02C0u /* @bin 0x004C5CE0 the message reply rectangle of reply bit 1 (docs/commands.md) */
#define G_REPLY_RECT_2 0x005E02DCu /* @bin 0x004C5CE0 the reply rectangle of bit 2 */
#define G_REPLY_RECT_4 0x005E02F8u /* @bin 0x004C5CE0 the reply rectangle of bit 4 */
#define G_STR_SMK_CREDITS 0x005E1DF8u /* @bin 0x004D8310 "smk\credits.smk" */
#define G_STR_SMK_INTRO 0x005E1E08u /* @bin 0x004D8310 "smk\intro.smk" */
#define G_STR_SMK_LOGO 0x005E1E18u /* @bin 0x004D8310 "smk\logo.smk" */
#define G_ADVISOR5_RECTS 0x005F7650u /* @bin 0x0040ACE0 advisor page 5's rectangles (present/ui/advisors/callbacks.c) */
#define G_FAMILY_RECTS 0x005F7F28u /* @bin 0x004D8310 window 0x2F's rows and buttons (present/ui/frontend/family.c) */
#define G_STR_DBG_BUILDING_IN_PROGRESS 0x005F8A04u /* @bin 0x00534610 " building_in_progress" */
#define G_STR_DBG_SIDE_PANEL_X1 0x005F8A1Cu /* @bin 0x00534610 " side_panel_x1" */
#define G_STR_DBG_MOUSE_MOVEMENT 0x005F8A2Cu /* @bin 0x00534610 " mouse_movement" */
#define G_STR_DBG_MOUSE_Y 0x005F8A3Cu /* @bin 0x00534610 " mouse_y" */
#define G_STR_DBG_MOUSE_X 0x005F8A48u /* @bin 0x00534610 " mouse_x" */
#define G_STR_DBG_TEST_VALUE4A 0x005F8A54u /* @bin 0x00534610 " test_value4a" */
#define G_STR_DBG_TEST_VALUE3A 0x005F8A64u /* @bin 0x00534610 " test_value3a" */
#define G_STR_DBG_TEST_VALUE2A 0x005F8A74u /* @bin 0x00534610 " test_value2a" */
#define G_STR_DBG_TEST_VALUE1A 0x005F8A84u /* @bin 0x00534610 " test_value1a" */
#define G_STR_DBG_AUTO_REMOVED 0x005F8A94u /* @bin 0x00534610 "Auto removed man mades tiles" */
#define G_STR_DBG_LAST_ALERT 0x005F8AB4u /* @bin 0x00534610 " last_alert_message_shown" */
#define G_STR_DBG_ALERT_MESSAGES 0x005F8AD0u /* @bin 0x00534610 " alert_messages" */
#define G_STR_DBG_WAS_IN_SET 0x005F8AE0u /* @bin 0x00534610 " was_in_set" */
#define G_STR_DBG_WAS_IN_INT 0x005F8AECu /* @bin 0x00534610 " was_in_int" */
#define G_STR_DBG_MISSION_IN_PLACE 0x005F8AF8u /* @bin 0x00534610 " mission_in_place" */
#define G_STR_DBG_NATIVES_RESTLESS 0x005F8B0Cu /* @bin 0x00534610 " natives_are_restless" */
#define G_STR_DBG_MAP_STYLE 0x005F8B24u /* @bin 0x00534610 " map_style" */
#define G_STR_DBG_MAP_ROTATION 0x005F8B30u /* @bin 0x00534610 " MAP ROTATION" */
#define G_STR_DBG_WINE_SOURCES 0x005F8B40u /* @bin 0x00534610 "wine sources =  " */
#define G_STR_DBG_COMMA 0x005F8B54u /* @bin 0x00534610 ",  " */
#define G_STR_DBG_DOCKS_LIST 0x005F8B58u /* @bin 0x00534610 "docks_list=  " */
#define G_STR_DBG_SPACES 0x005F8B68u /* @bin 0x00534610 "   " */
#define G_STR_DBG_NOF_DOCKS 0x005F8B6Cu /* @bin 0x00534610 "nof_docks=  " */
#define G_STR_DBG_IN_HOUSES 0x005F8B7Cu /* @bin 0x00534610 "  in houses " */
#define G_STR_DBG_DEATHS 0x005F8B8Cu /* @bin 0x00534610 "  deaths  " */
#define G_STR_DBG_BIRTHS 0x005F8B98u /* @bin 0x00534610 "  births  " */
#define G_STR_DBG_SPARE_CAP 0x005F8BA4u /* @bin 0x00534610 "  spare cap  " */
#define G_STR_DBG_TOT_CAP 0x005F8BB4u /* @bin 0x00534610 "  tot cap  " */
#define G_STR_DBG_POP_IS 0x005F8BC0u /* @bin 0x00534610 "Pop is " */
#define G_STR_DBG_BARB 0x005F8BC8u /* @bin 0x00534DF0 " = barb" */
#define G_STR_DBG_ROUTINGS 0x005F8BD0u /* @bin 0x00534DF0 " = routings" */
#define G_STR_DBG_EVOLVE_ERROR 0x005F8BDCu /* @bin 0x00534DF0 " = evolve error" */
#define G_STR_DBG_TOTAL_CONNECTIONS 0x005F8BECu /* @bin 0x00534DF0 " = total connections" */
#define G_STR_DBG_NO_OF_CONNECTS 0x005F8C04u /* @bin 0x00534DF0 " = no of connects" */
#define G_STR_DBG_EVER 0x005F8C18u /* @bin 0x00534DF0 " = ever" */
#define G_STR_DBG_LAST_BUILDING 0x005F8C20u /* @bin 0x00534DF0 " = last building" */
#define G_STR_DBG_YEAR 0x005F8C34u /* @bin 0x00534DF0 " = year" */
#define G_STR_DBG_MONTH 0x005F8C3Cu /* @bin 0x00534DF0 " = month" */
#define G_STR_DBG_WEEK 0x005F8C48u /* @bin 0x00534DF0 " = week" */
#define G_STR_DBG_TICK 0x005F8C50u /* @bin 0x00534DF0 " = tick" */
#define G_STR_DBG_EVOLVE_INFO 0x005F8C58u /* @bin 0x00534DF0 "Evolve info (F3)" */
#define G_STR_DBG_GOODS 0x005F8C6Cu /* @bin 0x00535080 " goods" */
#define G_STR_DBG_COMMA2 0x005F8C74u /* @bin 0x00535080 " ," */
#define G_STR_DBG_SPACES2 0x005F8C78u /* @bin 0x00535080 "  " */
#define G_STR_DBG_COMMA3 0x005F8C7Cu /* @bin 0x00535080 ", " */
#define G_STR_DBG_SHOWS 0x005F8C80u /* @bin 0x00535080 " shows" */
#define G_STR_DBG_RE 0x005F8C88u /* @bin 0x00535080 " re" */
#define G_STR_DBG_HE 0x005F8C8Cu /* @bin 0x00535080 " he" */
#define G_STR_DBG_ED 0x005F8C90u /* @bin 0x00535080 " ed" */
#define G_STR_DBG_EN 0x005F8C94u /* @bin 0x00535080 " en" */
#define G_STR_DBG_HI 0x005F8C98u /* @bin 0x00535080 "hi" */
#define G_STR_DBG_TH 0x005F8C9Cu /* @bin 0x00535080 "th" */
#define G_STR_DBG_MAX_POP 0x005F8CA0u /* @bin 0x00535080 " = max pop" */
#define G_STR_DBG_TAX_CVR 0x005F8CACu /* @bin 0x00535080 " = tax cvr" */
#define G_STR_DBG_EMPLOYEES 0x005F8CB8u /* @bin 0x00535080 " = employees" */
#define G_STR_DBG_NO_CONTACT 0x005F8CC8u /* @bin 0x00535080 " = no contact with rome" */
#define G_STR_DBG_WATER_DEP 0x005F8CE0u /* @bin 0x00535080 " = water dep" */
#define G_STR_DBG_NO_WATER 0x005F8CF0u /* @bin 0x00535080 " = no water" */
#define G_STR_DBG_WELL_SUP 0x005F8CFCu /* @bin 0x00535080 " = well sup" */
#define G_STR_DBG_WATER_SUP 0x005F8D08u /* @bin 0x00535080 " = water sup" */
#define G_STR_DBG_DAMAGE 0x005F8D18u /* @bin 0x00535080 " = damage" */
#define G_STR_DBG_SPARE_ROOM 0x005F8D24u /* @bin 0x00535080 " = spare room" */
#define G_STR_DBG_ROADNET 0x005F8D34u /* @bin 0x00535080 " = roadnet" */
#define G_STR_DBG_ROME 0x005F8D40u /* @bin 0x00535080 " = rome" */
#define G_STR_DBG_AGENT 0x005F8D48u /* @bin 0x00535080 " = agent" */
#define G_STR_DBG_SUB_AGENT 0x005F8D54u /* @bin 0x00535080 " = SUB agent" */
#define G_STR_DBG_IMMIGRANT 0x005F8D64u /* @bin 0x00535080 " = immigrant" */
#define G_STR_DBG_REL 0x005F8D74u /* @bin 0x00535080 " = rel" */
#define G_STR_DBG_ACC 0x005F8D7Cu /* @bin 0x00535080 " = acc" */
#define G_STR_DBG_GOODS2 0x005F8D84u /* @bin 0x00535080 " = goods" */
#define G_STR_DBG_POP 0x005F8D90u /* @bin 0x00535080 " = pop" */
#define G_STR_DBG_FOOD 0x005F8D98u /* @bin 0x00535080 "food" */
#define G_STR_DBG_RAW_MATERIALS 0x005F8DA0u /* @bin 0x00535080 "raw_materials" */
#define G_STR_DBG_AQUA_LINKS 0x005F8DB0u /* @bin 0x00535080 "aqua links" */
#define G_STR_DBG_SUB_TYPE 0x005F8DBCu /* @bin 0x00535080 " = sub_type" */
#define G_STR_DBG_TYPE 0x005F8DC8u /* @bin 0x00535080 " = type" */
#define G_STR_DBG_BUILDING_NO 0x005F8DD0u /* @bin 0x00535080 " = building no" */
#define G_STR_DBG_REDRAW 0x005F8DE0u /* @bin 0x00535080 " = redraw" */
#define G_STR_DBG_ALIVE 0x005F8DECu /* @bin 0x00535080 " = alive" */
#define G_STR_DBG_UNDO_BUILDINGS 0x005F8DF8u /* @bin 0x00535080 " = undo_buildings" */
#define G_STR_DBG_UNDO_VALID 0x005F8E0Cu /* @bin 0x00535080 " = undo_valid" */
#define G_STR_DBG_UNDO_READY 0x005F8E1Cu /* @bin 0x00535080 " = undo_ready" */
#define G_STR_DBG_OVER_PTR 0x005F8E2Cu /* @bin 0x00535080 " = over ptr" */
#define G_STR_DBG_NOT_OVER_BUILDING 0x005F8E38u /* @bin 0x00535080 "Not over a building" */
#define G_STR_DBG_BUILDING_INFO 0x005F8E4Cu /* @bin 0x00535080 "Building info (F4)" */
#define G_STR_DBG_DOCKERS 0x005F8E60u /* @bin 0x00536120 " = dockers " */
#define G_STR_DBG_GHOST 0x005F8E6Cu /* @bin 0x00536120 " = ghost " */
#define G_STR_DBG_PEOPLE 0x005F8E78u /* @bin 0x00536120 " = people " */
#define G_STR_DBG_IMMIGRANT_TO 0x005F8E84u /* @bin 0x00536120 " = immigrant_to " */
#define G_STR_DBG_AGENT_FOR 0x005F8E98u /* @bin 0x00536120 " = agent_for " */
#define G_STR_DBG_XY_BITS 0x005F8EA8u /* @bin 0x00536120 "x,y bits" */
#define G_STR_DBG_MAP_XY 0x005F8EB4u /* @bin 0x00536120 " map x,y" */
#define G_STR_DBG_XY 0x005F8EC0u /* @bin 0x00536120 "x,y" */
#define G_STR_DBG_RULER 0x005F8EC4u /* @bin 0x00536120 " = ruler" */
#define G_STR_DBG_FIGURE_NO 0x005F8ED0u /* @bin 0x00536120 " = figure no" */
#define G_STR_DBG_FREE_ROUTE_SLOTS 0x005F8EE0u /* @bin 0x00536120 " free route slots" */
#define G_STR_DBG_FIGS 0x005F8EF4u /* @bin 0x00536120 " Figs" */
#define G_STR_DBG_NOT_WATCHING 0x005F8EFCu /* @bin 0x00536120 "Not watching a figure" */
#define G_STR_DBG_FIGURE_INFO 0x005F8F14u /* @bin 0x00536120 "Figure info (F1)" */
#define G_ADVISOR6_RECTS 0x006032E8u /* @bin 0x004DBF4E advisor page 6's rectangles (present/ui/advisors/d_callbacks.c) */
#define G_TRADE_RECT_A 0x006034E0u /* @bin 0x004DC56E window 0x28's first rectangle */
#define G_TRADE_RECT_B 0x00603520u /* @bin 0x004DC56E window 0x28's second rectangle */
#define G_TRADE_RECT_C 0x00603540u /* @bin 0x004DC56E window 0x28's third rectangle */
#define G_TRADE_STOCKPILE_RECT 0x00603560u /* @bin 0x00559D70 window 0x28's stockpile toggle (docs/commands.md) */
#define G_TRADE_LIMIT_ARROWS 0x00603580u /* @bin 0x00559CD0 window 0x28's trade limit arrows (docs/commands.md) */
#define G_TRADE_LIMIT_ARROWS_B 0x006035B0u /* @bin 0x004DC56E window 0x28's arrows in mode 1 */
#define G_TRADE_BUTTONS 0x006035E0u /* @bin 0x00559DE0 window 0x28's buttons (present/ui/advisors/callbacks.c) */
#define G_ANIM_DEBUG_VALUE 0x006044ACu /* @bin 0x004E61F0 float, lowered / raised by F5 / F6 (10.0 .. 80.0) */
#define G_ANIM_DEBUG_TOGGLE 0x006044B0u /* @bin 0x004E61F0 i32 toggled by F2 */
#define G_ANIM_DEBUG 0x006044B4u /* @bin 0x004E61F0 i32 the debug switch of the animation step (docs/presentation.md 5.3) */
#define G_OVL_HOVER_TIME 0x00605790u /* @bin 0x00409E40 i32 the frame clock when the overlays menu was last hovered */
#define G_NUMPAD_Y 0x00605970u /* @bin 0x004D8310 i32 window 0x0E's y (present/ui/frontend/numpad.c) */
#define G_NUMPAD_X 0x00605974u /* @bin 0x004D8310 i32 window 0x0E's x */
#define G_LIST_EXTRA_ON 0x00605A58u /* @bin 0x004D8310 i32 the list window 0x0F / 0x48's extra button shown (cleared on a right press) */
#define G_TUTORIAL_DONE 0x007027C7u /* @bin 0x00417B40 u8 per tutorial step 0x0F..0x19: done */
#define G_CHEAT_BACK 0x007027E8u /* @bin 0x0041D3F0 u8 the window to go back to after the cheat window */
#define G_MENU_OPEN 0x00705724u /* @bin 0x00425F20 u32 the open menu's entry in the menu bar (0x10 bytes: i16 x, y, +4 y of the list, +6 text group, +8 items, +0x0C count) */
#define G_U775918 0x00775918u /* @bin 0x004D9CB5 i32 passed to 0x00407620 when the object is not kept */
#define G_BUF_TOTAL_SECOND 0x00784F38u /* @bin 0x00448270 char[100] keyword_quantity_total_secondary's buffer */
#define G_BUF_TOTAL_MAIN 0x00784F9Cu /* @bin 0x004481B0 char[100] keyword_quantity_total_main's buffer */
#define G_BUF_LOADS_REMAINDER_SECOND 0x00785000u /* @bin 0x00448180 char[100] the handler's buffer */
#define G_BUF_LOADS_REMAINDER_MAIN 0x00785064u /* @bin 0x00448070 char[100] the handler's buffer */
#define G_BUF_LOADS_CURRENT_SECOND 0x007850C8u /* @bin 0x00448040 char[100] the handler's buffer */
#define G_BUF_LOADS_CURRENT_MAIN 0x0078512Cu /* @bin 0x00448000 char[100] the handler's buffer */
#define G_BUF_NEEDED_REMAINDER_SECOND 0x00785190u /* @bin 0x00447F70 char[100] the handler's buffer */
#define G_BUF_NEEDED_REMAINDER_MAIN 0x007851F4u /* @bin 0x00447E30 char[100] the handler's buffer */
#define G_BUF_NEEDED_CURRENT_SECOND 0x00785258u /* @bin 0x00447DE0 char[100] the handler's buffer */
#define G_BUF_NEEDED_CURRENT_MAIN 0x007852BCu /* @bin 0x00447D80 char[100] the handler's buffer */
#define G_BUF_COURSES_COMPLETE 0x00785320u /* @bin 0x00447D60 the handler's buffer */
#define G_BUF_PERCENT_COMPLETE 0x0078532Cu /* @bin 0x00447C30 the handler's buffer ("%d%%") */
#define G_BUF_REASON_PHRASE 0x00785334u /* @bin 0x00447B10 char[200] the reason phrase expanded */
#define G_BUF_TIME_UNTIL_ATTACK 0x007853FCu /* @bin 0x00447AE0 the handler's buffer */
#define G_BUF_TRAVEL_TIME 0x00785404u /* @bin 0x00447AB0 the handler's buffer */
#define G_BUF_TIME_ALLOTTED 0x0078540Cu /* @bin 0x00447A80 the handler's buffer */
#define G_BUF_AMOUNT_GRANTED 0x00785414u /* @bin 0x004479B0 the handler's buffer */
#define G_BUF_AMOUNT 0x0078541Cu /* @bin 0x00447910 the handler's buffer */
#define G_MW_HISTORY_MESSAGES 0x00D3AE00u /* @bin 0x004C7EE0 i32[200] the message window's history: the messages (1-based by depth) */
#define G_SCROLL_TIME 0x00E94DB4u /* @bin 0x004DDD60 u32 the clock at the last scroll step */
#define G_UE94E14 0x00E94E14u /* @bin 0x004D865B i32 := 0 by window 1's input */
#define G_DEBUG_CITY_PANEL 0x00EA181Cu /* @bin 0x00534610 i32 1: the city debug panel is drawn */
#define G_DEBUG_WATCHED_FIGURE 0x00F535CCu /* @bin 0x00536120 i32 the figure watched by the figure debug panel */
#define G_MARKER_X 0x00F535D4u /* @bin 0x00407A10 i32 +4 of army marker 0 (G_MARKERS, stride 0x14): x (then y, w, h) */
#define G_UF53F30 0x00F53F30u /* @bin 0x0051FFD0 i32 := 0 before the build / overlays menu is redrawn */
#define G_FRAME_STEP_MS 0x012651A0u /* @bin 0x004E61F0 u32 the clock's step since the last frame */
#define G_FRAME_NOW_MS 0x012651A4u /* @bin 0x004E61F0 u32 the clock at this frame */
#define G_FRAME_LAST_MS 0x012651A8u /* @bin 0x004E61F0 u32 the clock at the last frame */

/* --- harvested names used by present/ui and present/ui/messages, checked against their uses
 * (evidence: the defining port and its comment, as harvested) --- */
#define G_MENU_TABLE 0x0057BD88u /* present/ui/frontend/menu.c: 5 rectangles of 0x1C: i16 x0, y0, x1, y1, callback +8, param +0x14 [generic] */
#define G_MSGLIST_DELETE_READ_RECT 0x005C4810u /* present/ui/messages/list.c: the message list's "delete read" rectangle */
#define G_MSGLIST_ROWS_RECTS 0x005C4830u /* present/ui/messages/list.c: the message list's rows */
#define G_GOAL_RECTS 0x005C5798u /* present/ui/frames.c: the page's rectangle table (0x1C bytes per row) */
#define G_CLOSE_BUTTONS 0x005C7B10u /* present/ui/info/info.c: the help and close buttons (0x24 bytes each): 2 */
#define G_MSGLIST_HELP_BUTTON 0x005C7E98u /* present/ui/messages/list.c: @bin 0x004E21B0 the message list's help button */
#define G_MSGLIST_CLOSE_BUTTON 0x005C80F0u /* present/ui/messages/list.c: the message list's close button */
#define G_MSGLIST_UP_BUTTON 0x005C8118u /* present/ui/messages/list.c: @bin 0x004E2160 the message list's up arrow */
#define G_MSGLIST_DOWN_BUTTON 0x005C8140u /* present/ui/messages/list.c: @bin 0x004E2160 the message list's down arrow */
#define G_TAX_BUTTONS 0x005C8D38u /* present/ui/info/service.c: the image button table of the tax panels */
#define G_ROUTE_SELECTED 0x005D2A30u /* present/ui/empire/editor.c: i32 the selected route */
#define G_ARMY_SEL 0x005D2E88u /* present/ui/empire/empire.h: i32 the hovered / selected army marker (-1: none) */
#define G_FMT_D 0x005D9A18u /* present/ui/messages/keywords.c: "%d" */
#define G_FMT_PERCENT 0x005D9A1Cu /* present/ui/messages/keywords.c: "%d%%" */
#define G_FMT_D_S 0x005D9A24u /* present/ui/messages/keywords.c: "%d %s" */
#define G_GLYPH_DRAW_MAP 0x005E049Cu /* present/ui/messages/richtext.c: u8 glyph number per character - 0x20 */
#define G_ROUTE_DRAG 0x005E1DB0u /* present/ui/empire_editor.c: i32 the waypoint being dragged, -1 none */
#define G_TABS 0x005F8320u /* present/ui/advisors/advisors.h: the hall's 14 page buttons (0x24-byte entries, sidebar.h buttons_draw) */
#define G_SCREEN_Y 0x006FDC9Cu /* present/ui/empire/mission.c */
#define G_SCREEN_X 0x006FDCA0u /* present/ui/empire/mission.c: i32 the screens' origin */
#define G_CHEATS_ON 0x007027E0u /* present/ui/cityframe.c: u8 a cheat was typed once */
#define G_CHEAT_HOVER 0x007027E4u /* present/ui/cityframe.c: i32 the OK button hovered */
#define G_OVL_ROW 0x007056D0u /* present/ui/windows.c: i32 the open group's row in the menu */
#define G_OVL_ROWS 0x007056D8u /* present/ui/windows.c: i32 its rows */
#define G_OVL_GROUP 0x007056DCu /* present/ui/windows.c: i32 the open group (its id from 0x00576180), 0 none */
#define G_MENU_ITEM 0x00705720u /* present/ui/windows.c: u32 the item being drawn / chosen */
#define G_MENU_PREV 0x00705728u /* present/ui/windows.c: u32 the window under the menu */
#define G_MENU_HOVER 0x0070572Cu /* present/ui/windows.c: i32 the hovered item, 1-based */
#define G_MENU_TITLE 0x00705730u /* present/ui/windows.c: i32 the open menu's title, 1-based */
#define G_EDITOR_OBJECT 0x0077591Cu /* present/ui/empire/editor.c: i32 the selected object [generic] */
#define G_PICTURE_KIND 0x00775920u /* present/ui/empire/editor.c: i32 0..2 */
#define G_OBJECT_IMAGE 0x00775924u /* present/ui/empire/editor.c: i32 the object picture chosen */
#define G_EDITOR_MODE 0x00775928u /* present/ui/empire/editor.c: i32 the editor's mode [generic] */
#define G_MSG_NESTED 0x007854A4u /* present/ui/messages/keywords.c: u8 1 while the reason phrase (0x00447B10) expands: its own fields */
#define G_HIGHSCORE_FIRST 0x00D3A4B8u /* present/ui/frontend/highscore.c: i32 the first row shown [single+clash] */
#define G_MW_THUMB_Y 0x00D3B128u /* present/ui/messages/window.c: i32 the scroll thumb while dragged */
#define G_MESSAGE_WINDOW_SCROLL_MAX 0x00D3B134u /* present/ui/messages/window.c: i32 lines beyond the box (0: no scroll bar) */
#define G_MESSAGE_WINDOW_VISIBLE 0x00D3B13Cu /* present/ui/messages/richtext.c: i32 lines drawn */
#define G_MW_FOOT 0x00D3B144u /* present/ui/messages/window.c: i32 room below the box: 0x30, 0x20 for kinds != 0 */
#define G_MESSAGE_WINDOW_BOX_H 0x00D3B148u /* present/ui/messages/window.c: i32 the body box's height in pieces */
#define G_MESSAGE_WINDOW_BOX_W 0x00D3B14Cu /* present/ui/messages/richtext.c: i32 the text box's width in pieces */
#define G_MESSAGE_WINDOW_BOX_Y 0x00D3B150u /* present/ui/messages/window.c: i32 the body box's y */
#define G_MESSAGE_WINDOW_BOX_X 0x00D3B154u /* present/ui/messages/window.c: i32 the body box's x */
#define G_MW_SUB_W 0x00D3B158u /* present/ui/messages/window.c: i32 the subtitle's width */
#define G_MESSAGE_WINDOW_Y 0x00D3B15Cu /* present/ui/messages/window.c: i32 the window's y */
#define G_MESSAGE_WINDOW_X 0x00D3B160u /* present/ui/messages/window.c: i32 the window's x */
#define G_MESSAGE_WINDOW_FONT_LINK 0x00D3B164u /* present/ui/messages/richtext.c: i32 link font */
#define G_MESSAGE_WINDOW_FONT_TEXT 0x00D3B168u /* present/ui/messages/richtext.c: i32 text font */
#define G_KEYWORD_CURRENT 0x00D3B18Cu /* present/ui/messages/expand.c: u32 the keyword entry whose handler runs */
#define G_CLIP_Y 0x00D3B530u /* present/draw/mem.h: u8 vertical clip: 0 none, 3 top, 4 bottom, 5 out */
#define G_CLIP_X 0x00D3B531u /* present/draw/mem.h: u8 horizontal clip: 0 none, 1 left, 2 right, 5 out */
#define G_PIC_Y 0x00D3B568u /* present/ui/empire/mission.c */
#define G_PIC_X 0x00D3B56Cu /* present/ui/empire/mission.c: i32 the source offset of a full-screen picture (1024 x 768) */
#define G_PIC_H 0x00D3B570u /* present/ui/empire/mission.c */
#define G_PIC_W 0x00D3B574u /* present/ui/empire/mission.c */
#define G_UD3B5BC 0x00D3B5BCu /* present/ui/info/figure.c: i16 columns [unresolved] */
#define G_UD3B5BE 0x00D3B5BEu /* present/ui/info/figure.c: i16 rows [unresolved] */
#define G_WORD_CHARS 0x00E20760u /* present/ui/messages/richtext.c: i32 bytes the last measured word takes */
#define G_LINE_MAX_W 0x00E2076Cu /* present/ui/messages/richtext.c: i32 widest line */
#define G_MSGWIN_TABLE 0x00E209B8u /* present/ui/msgwin.c: G_MESSAGE_TYPES + 8, the message window's view of a type: i16 +0 kind, +2 sub kind (MSGTYPE_U0A), +0x0A width, +0x0C height in pieces */
#define G_LINE_BUF 0x00E3439Cu /* present/ui/messages/richtext.c: char[200] the line being laid out */
#define G_HIT_Y 0x00E38E28u /* present/sidebar/input.c */
#define G_HIT_X 0x00E38E2Cu /* present/sidebar/input.c */
#define G_MSGLIST_IN_H 0x00E950F4u /* present/ui/messages/messages.h: i32 inner panel height in pieces */
#define G_MSGLIST_IN_W 0x00E950F8u /* present/ui/messages/messages.h: i32 inner panel width in pieces */
#define G_MSGLIST_IN_Y 0x00E950FCu /* present/ui/messages/messages.h: i32 inner panel y */
#define G_MSGLIST_IN_X 0x00E95100u /* present/ui/messages/messages.h: i32 inner panel x */
#define G_MSGLIST_H 0x00E95104u /* present/ui/messages/messages.h: i32 outer panel height in pieces */
#define G_MSGLIST_W 0x00E95108u /* present/ui/messages/messages.h: i32 outer panel width in 16-pixel pieces */
#define G_MSGLIST_Y 0x00E9510Cu /* present/ui/messages/messages.h: i32 outer panel y */
#define G_MSGLIST_X 0x00E95110u /* present/ui/messages/messages.h: i32 outer panel x */
#define G_MSGLIST_THUMB_Y 0x00E95114u /* present/ui/messages/messages.h: i32 the scroll thumb's offset while dragged */
#define G_MSGLIST_DRAGGING 0x00E95118u /* present/ui/messages/messages.h: i32 1 while the thumb is dragged */
#define G_INFO_Y 0x00EADD8Cu /* present/ui/info/info.h: i32 the panel's y */
#define G_INFO_X 0x00EADD90u /* present/ui/info/info.h: i32 the panel's x */
#define G_PANEL_H 0x00F53F00u /* present/ui/advisors/advisors.h: i32 the page panel's height in 16-px pieces */
#define G_PANEL_W 0x00F53F04u /* present/ui/advisors/advisors.h: i32 the page panel's width in 16-px pieces */
#define G_ADVISOR_Y 0x00F53F08u /* present/ui/advisors/advisors.h: i32 */
#define G_ADVISOR_X 0x00F53F0Cu /* present/ui/ui.h: i32 the advisor windows' origin */
#define G_MARKER_COUNT 0x00F53FDCu /* present/ui/empire/empire.h */
#define G_PICTURES 0x00F55C04u /* present/ui/messages/richtext.c: i32 first image of the message pictures */
#define G_PICTURES_BIG 0x00F55C34u /* present/ui/messages/window.c: i32 big pictures (the video layout) */
#define G_ICONS 0x00F566ECu /* present/ui/messages/list.c: i32 first image of the message icons */
#define G_FONT_LINK 0x00F56FACu /* present/ui/messages/window.c */
#define G_PICTURES2 0x00F5731Cu /* present/ui/messages/window.c: i32 pictures from 1001 on */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_MARKERS 0x00F535D0u /* present/ui/empire/empire.h: 100 army markers of 5 i32: event, x, y, w, h */
#define G_IMG_ENTRIES 0x010D69F8u /* present/ui/empire/empire.c: image entries of 0x40 bytes: +0x14 width, +0x16 height (i16) */
/* --- end harvested --- */

#endif
