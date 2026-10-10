/* Imhotep - named globals: present/layout/g_ui_advisors.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8c). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_ADVISORS_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_ADVISORS_H

/* ---- the advisors (present/ui/advisors): @bin the ports named on each line ---- */
/* text: the fonts' first images, the cursor, the strings given as suffixes (contents read from the
 * reference Pharaoh.exe's data) */
#define G_TEXT_X 0x00E20778u        /* @bin 0x0052A3D2 i32: text cursor x after a draw (text: package blit) */
#define G_FONT_ROW 0x00F56F94u      /* @bin 0x0052C570 i32: first image of a font (page 13's good verdicts, the legion rows) */
#define G_FONT_RED 0x00F56FA4u      /* @bin 0x0052A3D2 i32: first image of the font of negative values (red: f_pages.c), the hovered salary row */
#define G_NUMBER_SUFFIX 0x0057C048u /* @bin 0x0052A250 " ": the suffix of most numbers */
#define G_STR_SPACE_DB 0x005E03ECu  /* @bin 0x00524E20 " Db": the salaries' suffix */
#define G_STR_DOLLAR 0x005F76C0u    /* @bin 0x004F6AF0 "$": the ratings' suffix */
#define G_STR_DB_CAP 0x005F89CCu    /* @bin 0x0052C570 "Db": page 13's balance difference suffix */
#define G_STR_PAREN 0x005F89D0u     /* @bin 0x0052152C " )": page 1's kingdom wage suffix */
#define G_STR_PERCENT_PAREN 0x005F89D4u /* @bin 0x00521235 "%)": page 1's unemployment suffix */
#define G_STR_DB 0x005F89D8u        /* @bin 0x0052A638 "db": page 12's suffix */
#define G_FMT_S_S 0x005D9A10u       /* @bin 0x00447840 "%s %s" */
#define G_KING_NAME_BUFFER 0x00785424u /* @bin 0x00447840 char[]: advisor_king_name's result */
/* images: the first image of a group (or the image) the pages draw */
#define G_HALL_BACKDROP 0x00F55C0Cu /* @bin 0x00520F40 i32: the hall's full-screen backdrop */
#define G_TAB_STRIP_IMAGE 0x00F5691Cu /* @bin 0x00520F40 i32: the tab strip */
#define G_TAB_LIT_IMAGES 0x00F56914u /* @bin 0x00520FF8 i32: the lit tabs (page * 4 - 6 / - 2) */
#define G_ADVISOR_HEADER_IMAGES 0x00F5681Cu /* @bin 0x0052A250 i32: the pages' header image (+ 0..0xC by page) */
#define G_RATINGS_PICTURE 0x00F55BC4u /* @bin 0x004F6AF0 i32: page 5's picture */
#define G_RATING_COLUMN_IMAGES 0x00F56A04u /* @bin 0x004F77E0 i32: a rating column's base, piece (+1), top (+2) */
#define G_GRAPH_BAR_IMAGES 0x00F56904u /* @bin 0x00527577 i32: page 7's graph bars */
#define G_PRIORITY_IMAGE 0x00F5670Cu /* @bin 0x005215D0 i32: a labour category with a priority */
#define G_LEGION_AWAY_IMAGES 0x00F56434u /* @bin 0x00522565 i32: an away legion's icon (+ kind) */
#define G_HEALTH_IMAGES 0x00F5642Cu /* @bin 0x005229A5 i32: a legion's health, (+0x7A + 10) / 20 */
#define G_UF5643C 0x00F5643Cu       /* @bin 0x00521B50 i32: drawn at (0x214, 0x15B) on pages 2 (+1) and 3 (+0) */
#define G_ORDER_IMAGES 0x00F56834u  /* @bin 0x00522759 i32: the order buttons' images (+1..+5) */
#define G_STATUS_IMAGE 0x00F5690Cu  /* @bin 0x00521F89 i32: the status lines' bullet */
#define G_REQUEST_IMAGES 0x00F56B04u /* @bin 0x0052418A i32: a troops request's icon (+1: the city at war) */
#define G_GOD_IMAGES 0x00F55C5Cu    /* @bin 0x00529320 i32: the gods' portraits (+0x10 / +0x15 + god), page 11's festival (+0xF) */
#define G_GOD_WRATH_ICON 0x00F56794u /* @bin 0x00529DD9 i32: one per 10 of CITY_GOD_WRATH */
#define G_GOD_BLESSING_ICON 0x00F56464u /* @bin 0x00529DD9 i32: one per 10 of CITY_GOD_BLESSING */
/* button and rectangle tables (0x1C-byte rectangles, 0x18-byte buttons) */
#define G_ADVISOR_BUTTONS 0x005F78F0u /* @bin 0x00538050 u32 per page: the button table offering that advisor */
#define G_ADVISOR_HELP 0x00576148u  /* @bin 0x00407290 i32 per page 1-14: the help message */
#define G_PAGE13_RECTS 0x005C5878u  /* @bin 0x0052C570 page 13's 13 line rectangles (0x1C bytes each) */
#define G_RELIGION_ROWS 0x005C59E8u /* @bin 0x00529BBB page 11's 5 god rows (0x1C bytes each) */
#define G_TRADE_SCROLL_DOWN 0x005C5B78u /* @bin 0x0055A1AD page 6's scroll button down */
#define G_TRADE_SCROLL_UP 0x005C5B90u /* @bin 0x0055A1AD page 6's scroll button up */
#define G_ROUTES_RECT 0x00603320u   /* @bin 0x0055A0E0 page 6 rect table entry 2: the open routes' tooltip area */
/* the pages' state */
#define G_LEGION_ROWS 0x00F53DD4u   /* @bin 0x00521B50 i32: page 2's rows (the legions, 0x004B78F0) */
#define G_FLEET_ROWS 0x00F53DD0u    /* @bin 0x0052306F i32: page 3's rows (the warships, CITY_WARSHIPS) */
#define G_PRIORITY_CHOICES 0x00605984u /* @bin 0x0040A6C0 i32: the priorities window 0x2A offers */
#define G_GRAPH_HOVER 0x005F825Cu   /* @bin 0x0040A8A0 i32: page 7's hovered graph button (-1 none) */
#define G_SCALE_SHIFT 0x00F53DC8u   /* @bin 0x00527AD0 i32: page 7's graph scale shift (-1: doubled) */
#define G_SCALE_MAX 0x00F53DCCu     /* @bin 0x00527AD0 i32: the graph's top value */
#define G_DISTRIBUTION 0x00EADD44u  /* @bin 0x00529BBB i32: != 0: page 11 shows the raw god bytes; the bazaar / storage view toggle (0x004062D0, present/ui/info/info.h) */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_MENU_LABELS 0x0057BE14u /* present/ui/frontend/menu.c: i32[5] the labels' string indexes in group 0x1E */
#define G_PERIOD_BUTTONS 0x0057BE78u /* present/ui/frontend/campaign.c: 11 rectangles of 0x1C: 9 periods, 2 tabs */
#define G_FILE_NAME 0x005E02A8u /* present/ui/frontend/highscore.c: "save\highscore.jas" */
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
#define G_CARET_X 0x00E20790u /* present/draw/text.c */
#define G_VICTORY_LIT 0x00E2079Au /* present/ui/frontend/victory.c: u8 the last button's lit state [generic] */
#define G_FAMILY_ROWS 0x00EADFACu /* present/ui/frontend/family.c: u32 the rows' offsets (an array of the heap) */
#define G_FAMILY_COUNT 0x00EADFB0u /* present/ui/frontend/family.c: i32 */
#define G_FAMILY_NAMES 0x00EADFB8u /* present/ui/frontend/family.c: u32 base added to a row's offset */
#define G_PANEL_Y 0x00F53DB4u /* present/ui/frontend/highscore.c */
#define G_PANEL_X 0x00F53DB8u /* present/ui/frontend/highscore.c */
#define G_CARET_TIME 0x00F53FE4u /* present/ui/frontend/family.c: i32 the frame clock at the last change */
#define G_CARET_PHASE 0x00F53FE8u /* present/ui/frontend/family.c: i32 0 hidden, 1 shown */
#define G_BACKGROUND 0x00F55C24u /* present/ui/frontend/menu.c: i32 the main menu's picture */
/* --- end harvested --- */

#endif
