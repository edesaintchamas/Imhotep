/* Imhotep - named globals: present/layout/g_ui_advisors.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8c). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_ADVISORS_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_ADVISORS_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_MENU_LABELS 0x0057BE14u /* present/ui/frontend/menu.c: i32[5] the labels' string indexes in group 0x1E */
#define G_PERIOD_BUTTONS 0x0057BE78u /* present/ui/frontend/campaign.c: 11 rectangles of 0x1C: 9 periods, 2 tabs */
#define G_NUMBER_SUFFIX 0x0057C048u /* present/sidebar/sidebar.c: the empty string given as the numbers' suffix */
#define G_FILE_NAME 0x005E02A8u /* present/ui/frontend/highscore.c: "save\highscore.jas" */
#define G_ROUTES_RECT 0x00603320u /* present/ui/advisors/d_trade.c: page 6 rect table entry 2: the open routes' tooltip area */
#define G_LIST_ROWS 0x006057CCu /* present/ui/frontend/files.c: u32 */
#define G_FILES_LIST_COUNT 0x006057D0u /* present/ui/frontend/files.c: i32 [generic] */
#define G_LIST_NAMES 0x006057D8u /* present/ui/frontend/files.c: u32 */
#define G_NUMPAD_TYPE 0x0060596Cu /* present/ui/frontend/numpad.c: i32 1, 2 or 4 [generic] */
#define G_DIGITS 0x00605978u /* present/ui/frontend/numpad.c: i32 */
#define G_NUMPAD_VALUE 0x0060597Cu /* present/ui/frontend/numpad.c: i32 [generic] */
#define G_NUMPAD_BACK 0x00605980u /* present/ui/frontend/numpad.c: u8 the window to go back to [generic] */
#define G_BIG_LIST 0x006059ECu /* present/ui/frontend/files.c: i32 0: 12 rows, else 15 (the thumb's bar) */
#define G_CLICKED 0x006059F0u /* present/ui/frontend/files.c: i32 */
#define G_FIRST 0x006059F4u /* present/ui/frontend/files.c: i32 */
#define G_FILES_ROWS 0x006059F8u /* present/ui/frontend/files.c: i32 [generic] */
#define G_SCENARIO_LIST_COUNT 0x00704C08u /* present/ui/frontend/scenario.c [generic] */
#define G_HIGHSCORE_RESULT 0x00D38700u /* present/ui/frontend/highscore.c: i32 what 0x004C5A40 did: 1 cheated, 2 new, 3 / 6 not better, 4 / 5 better [generic] */
#define G_HIGHSCORE_TABLE 0x00D38708u /* present/ui/frontend/highscore.c: the high score records (0x4C bytes each) [generic] */
#define G_HIGHSCORE_ROWS 0x00D3A4BCu /* present/ui/frontend/highscore.c: i32 the records in use [generic] */
#define G_BACK_WINDOW 0x00D3B1BCu /* present/ui/frontend/files.c: i32 */
#define G_TEXT_X 0x00E20778u /* present/sidebar/sidebar.h: i32 text cursor x after a draw (text: package blit) */
#define G_CARET_X 0x00E20790u /* present/draw/text.c */
#define G_VICTORY_LIT 0x00E2079Au /* present/ui/frontend/victory.c: u8 the last button's lit state [generic] */
#define G_DISTRIBUTION 0x00EADD44u /* present/ui/info/info.h: i32 bazaar / storage view toggle (0x004062D0) */
#define G_FAMILY_ROWS 0x00EADFACu /* present/ui/frontend/family.c: u32 the rows' offsets (an array of the heap) */
#define G_FAMILY_COUNT 0x00EADFB0u /* present/ui/frontend/family.c: i32 */
#define G_FAMILY_NAMES 0x00EADFB8u /* present/ui/frontend/family.c: u32 base added to a row's offset */
#define G_PANEL_Y 0x00F53DB4u /* present/ui/frontend/highscore.c */
#define G_PANEL_X 0x00F53DB8u /* present/ui/frontend/highscore.c */
#define G_SCALE_SHIFT 0x00F53DC8u /* present/ui/advisors/b_population.c: i32 its shift (-1: doubled) */
#define G_SCALE_MAX 0x00F53DCCu /* present/ui/advisors/b_population.c: i32 the graph's top value (0x00527AD0) */
#define G_CARET_TIME 0x00F53FE4u /* present/ui/frontend/family.c: i32 the frame clock at the last change */
#define G_CARET_PHASE 0x00F53FE8u /* present/ui/frontend/family.c: i32 0 hidden, 1 shown */
#define G_BACKGROUND 0x00F55C24u /* present/ui/frontend/menu.c: i32 the main menu's picture */
#define G_GOD_IMAGES 0x00F55C5Cu /* present/ui/info/service.c: i32 first image of the group the temples' god picture is in */
#define G_ORDER_IMAGES 0x00F56834u /* present/ui/advisors/a_military.c */
#define G_STATUS_IMAGE 0x00F5690Cu /* present/ui/advisors/a_military.c */
#define G_FONT_ROW 0x00F56F94u /* present/ui/windows.c: highlighted */
#define G_UF56FA4 0x00F56FA4u /* present/ui/messages/list.c: i32 the hovered row's font [unresolved] */
/* --- end harvested --- */

#endif
