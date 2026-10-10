/* Imhotep - pending names of package p8a (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P8A_UI_MAIN_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P8A_UI_MAIN_H

/* ---- city record fields (owner: city.h, P3); evidence on each line ---- */
#define CITY_HOUSE_CAPACITY 0x34u      /* @bin 0x00534610 i32: house capacity (save.md §5 +0x34; the debug panel's "tot cap") */
#define CITY_BIRTHS 0x26DCu            /* @bin 0x00534610 i32: births at the yearly ageing (save.md §5; "births") */
#define CITY_DEATHS 0x26E0u            /* @bin 0x00534610 i32: deaths at the yearly ageing (save.md §5; "deaths") */
#define CITY_DOCKS 0x4460u             /* @bin 0x00534610 i16[10]: the city's docks (core/building/day2_docks.c; "docks_list=") */
#define CITY_NATIVES_RESTLESS 0x4594u  /* @bin 0x00534610 i32: "natives_are_restless" */
#define CITY_MISSION_IN_PLACE 0x459Cu  /* @bin 0x00534610 i32: "mission_in_place" */
#define CITY_WINE_SOURCES 0x46A4u      /* @bin 0x00534610 i32: "wine sources" (core/city/day_city.c: the open routes) */
#define CITY_EXPLAIN_PAGE 0x4768u      /* @bin 0x00538050 i32: the advisor page window 0x4F offers */
#define CITY_U4784 0x4784u             /* @bin 0x00524840 i32: shown by advisor page 4 (cleared monthly, core/city/month.c) */
#define CITY_U4820 0x4820u             /* @bin 0x0052BAF0 i32: 0 (with CITY_U4828 >= 12) picks a finished monument's text */
#define CITY_U4828 0x4828u             /* @bin 0x0052BAF0 i32: compared with 12, see CITY_U4820 */
#define CITY_TRADE_SCROLL 0x4834u      /* @bin 0x0040C760 i32: the trade advisor's first row (docs/commands.md) */

/* ---- building record fields (owner: building.h, P2) ---- */
#define BLD_AQUA_LINKS 0x9Eu           /* @bin 0x00535080 i16[4]: type 0x5A: the debug panel's "aqua links" */

/* ---- message record fields (owner: message.h, P3): the post's data block (MSG_DATA), read by
 * the keyword handlers (present/ui/messages/keywords.c); "phrase" fields are the reason phrase's own
 * (read while G_MSG_NESTED is 1) ---- */
#define MSG_CITY 0x16u                 /* @bin 0x00447890 i16: the empire city */
#define MSG_AMOUNT 0x18u               /* @bin 0x00447910 i16: the amount */
#define MSG_KIND 0x1Au                 /* @bin 0x00447910 i16: the amount's kind (resource) */
#define MSG_TIME 0x1Cu                 /* @bin 0x00447A80 i16: the time allotted / travel time */
#define MSG_ATTACK_TIME 0x1Eu          /* @bin 0x00447AE0 i16: the time until the attack */
#define MSG_PHRASE 0x20u               /* @bin 0x00447B10 i16: the reason phrase */
#define MSG_PHRASE_CITY 0x22u          /* @bin 0x00447890 i16: the phrase's city */
#define MSG_AMOUNT_GRANTED 0x24u       /* @bin 0x004479B0 i16: the amount granted */
#define MSG_PHRASE_AMOUNT 0x28u        /* @bin 0x00447910 i16: the phrase's amount */
#define MSG_PHRASE_KIND 0x2Au          /* @bin 0x00447910 i16: the phrase's kind */
#define MSG_GOD 0x2Cu                  /* @bin 0x00447BB0 u8: the god */
#define MSG_PHRASE_GOD 0x2Du           /* @bin 0x00447BB0 u8: the phrase's god */

/* ---- the monuments' status records (owner: present/layout/g_ui_info.h, P8b), computed by the
 * info panel's open (present/ui/info/info.c) and read by the keyword handlers
 * (present/ui/messages/keywords.c): per building type, the percent complete (keyword_percent_complete
 * 0x00447C30), the main course's remainder (0x00447E30) and total (0x004481B0) ---- */
#define G_MON_E5_PERCENT 0x00EADB4Cu    /* @bin 0x00447C30 i32 types 0xE5, 0xEA..0xEC (record 0x00EADB38 + 0x14) */
#define G_MON_DE_REMAINDER 0x00EADB74u  /* @bin 0x00447E30 i32 type 0xDE (record 0x00EADB58 + 0x1C), / 100 */
#define G_MON_DE_PERCENT 0x00EADB78u    /* @bin 0x00447C30 i32 type 0xDE */
#define G_MON_DE_TOTAL 0x00EADB7Cu      /* @bin 0x004481B0 i32 type 0xDE, / 100 */
#define G_MON_E4_REMAINDER 0x00EADB9Cu  /* @bin 0x00447E30 i32 type 0xE4 (record 0x00EADB88 + 0x14) */
#define G_MON_E4_PERCENT 0x00EADBA0u    /* @bin 0x00447C30 i32 type 0xE4 */
#define G_MON_E3_REMAINDER 0x00EADBBCu  /* @bin 0x00447E30 i32 type 0xE3 (record 0x00EADBA8 + 0x14) */
#define G_MON_E3_PERCENT 0x00EADBC0u    /* @bin 0x00447C30 i32 type 0xE3 */
#define G_MON_E6_PERCENT 0x00EADBD4u    /* @bin 0x00447C30 i32 type 0xE6 (record 0x00EADBC8 + 0x0C) */
#define G_MON_E1_REMAINDER 0x00EADC04u  /* @bin 0x00447E30 i32 type 0xE1 (record 0x00EADBD8 + 0x2C) */
#define G_MON_E1_PERCENT 0x00EADC08u    /* @bin 0x00447C30 i32 type 0xE1 */
#define G_MON_D5_REMAINDER 0x00EADC3Cu  /* @bin 0x00447E30 i32 type 0xD5 (record 0x00EADC10 + 0x2C) */
#define G_MON_D5_PERCENT 0x00EADC40u    /* @bin 0x00447C30 i32 type 0xD5 */
#define G_MON_D5_TOTAL 0x00EADC44u      /* @bin 0x004481B0 i32 type 0xD5 */
#define G_PYRAMID_COURSES_DONE 0x00EADC4Cu    /* @bin 0x00447D60 i32 G_PYRAMID_STATUS + 4: the courses complete */
#define G_PYRAMID_NEEDED_MAIN 0x00EADC60u     /* @bin 0x00447D80 i32 G_PYRAMID_STATUS + 0x18: the main course's current loads */
#define G_PYRAMID_NEEDED_SECOND 0x00EADC64u   /* @bin 0x00447DE0 i32 + 0x1C: the secondary course's current loads */
#define G_PYRAMID_REMAINDER_MAIN 0x00EADC68u  /* @bin 0x00447E30 i32 + 0x20: the main course's remainder */
#define G_PYRAMID_REMAINDER_SECOND 0x00EADC6Cu /* @bin 0x00447F70 i32 + 0x24: the secondary course's remainder */
#define G_PYRAMID_TOTAL_MAIN 0x00EADC70u      /* @bin 0x004481B0 i32 + 0x28: the main course's total */
#define G_PYRAMID_TOTAL_SECOND 0x00EADC74u    /* @bin 0x00448270 i32 + 0x2C: the secondary course's total */
#define G_PYRAMID_PERCENT 0x00EADC7Cu         /* @bin 0x00447C30 i32 + 0x34: the percent complete (type 0xB7) */

/* ---- globals of other areas (core or another present package uses them) ---- */
#define G_FLT_4 0x0056F550u /* @bin 0x00434C81 float 4.0 (house levels of the overlay level images; core/city/monument_rating.c) */
#define G_BUILD_MENU_RECTS 0x005C28A8u /* @bin 0x00403E00 the build menu's rows (window 6, 0x1C-byte entries; docs/commands.md) */
#define G_SIDEBAR_RECT 0x005C6CB8u /* @bin 0x004D8310 the sidebar's rectangle table (1 entry; present/sidebar/input.c) */
#define G_SIDEBAR_BUTTONS_TOP 0x005C7300u /* @bin 0x0051EC90 the expanded sidebar's 2 buttons (present/sidebar/sidebar.c) */
#define G_SIDEBAR_BUTTONS_COLLAPSED 0x005C7348u /* @bin 0x0051EC90 the collapsed sidebar's button (present/sidebar/sidebar.c) */
#define G_SIDEBAR_BUTTONS 0x005C7370u /* @bin 0x0051EC90 the expanded sidebar's 16 buttons (present/sidebar/sidebar.c) */
#define G_SIDEBAR_BUTTONS_COLLAPSED2 0x005C75B0u /* @bin 0x0051EC90 the collapsed sidebar's 12 buttons (present/sidebar/sidebar.c) */
#define G_EDITOR_SIDEBAR_BUTTONS 0x005C7760u /* @bin 0x004D8746 the editor's sidebar buttons (docs/provenance/frontier.md) */
#define G_SIDEBAR_BUTTONS_ADVISORS 0x005C7980u /* @bin 0x0051EC90 the expanded sidebar's advisors / empire buttons (present/sidebar/input.c) */
#define G_MENU_BAR 0x005C91F0u /* @bin 0x00426100 the game's menu bar, four menus of 0x10 bytes (core/command/menu_items.c) */
#define G_EDITOR_MENU_BAR 0x005C9230u /* @bin 0x00426100 the editor's menu bar (docs/provenance/frontier.md) */
#define G_STR_SPACE_DB 0x005E03ECu /* @bin 0x00524840 " Db" (the suffix of an amount of deben) */
#define G_STR_COMMA 0x005E0400u /* @bin 0x00535080 "," */
#define G_STR_CLOSE_PAREN 0x005F7E78u /* @bin 0x00535080 ")" */
#define G_MESSAGE_BOX_RECTS 0x005F8910u /* @bin 0x00538920 the message box's button table (core/sim/platform.c) */
#define G_CARRY_GROUP_0 0x006FDCC4u /* @bin 0x00417B40 u8 the first waiting group's flag (G_CARRY_GROUPS + 8) */
#define G_CARRY_GROUP_1 0x006FDCD0u /* @bin 0x00417B40 u8 the second waiting group's flag */
#define G_CARRY_GROUP_2 0x006FDCDCu /* @bin 0x00417B40 u8 the third waiting group's flag */
#define G_TOPBAR_HOVER 0x0070569Cu /* @bin 0x00406250 i32 the top bar's figure under the mouse (present/sidebar/input.c) */
#define G_MENU_BAR_HOVER 0x007056A8u /* @bin 0x00426100 i32 the menu bar's title under the mouse (present/sidebar/input.c) */
#define G_SQUARE_BUTTON_HOVER 0x007056B0u /* @bin 0x00425E80 i32 the square button under the mouse (base + index) */
#define G_HOUSE_NEED_ENTERTAINMENT 0x0070CDD4u /* @bin 0x00453B40 i32 +0 of house level 0's needs (stride 0x58, G_HOUSE_LEVELS + 8): entertainment (core/building/day_houses.c) */
#define G_HOUSE_NEED_EDUCATION 0x0070CDDCu /* @bin 0x00453B40 i32 +8 of the needs: education */
#define G_HOUSE_NEED_RELIGION 0x0070CDE0u /* @bin 0x00453B40 i32 +0x0C of the needs: religion */
#define G_HOUSE_NEED_SERVICES 0x0070CDF0u /* @bin 0x00453B40 i32 +0x1C of the needs: services */
#define G_EMPIRE_CITY_KIND 0x0077DE02u /* @bin 0x004D9A47 u8 +2 of empire city 0 (G_EMPIRE_CITIES, stride 0x6A): its kind */
#define G_EMPIRE_CITY_NAME 0x0077DE03u /* @bin 0x00447890 u8 +3 of empire city 0: its name (text index) */
#define G_EMPIRE_CITY_ROUTE 0x0077DE04u /* @bin 0x00409490 u8 +4 of empire city 0: its route (core/figure/helpers_xb.c) */
#define G_EMPIRE_CITY_OPEN 0x0077DE05u /* @bin 0x00409490 u8 +5 of empire city 0: below 50 not yet open */
#define G_EMPIRE_OBJECT_IMAGE 0x0077F754u /* @bin 0x004D9CB5 u16 +0x0C of empire object 0 (G_EMPIRE_OBJECTS, stride 0x62): its image (core/city/events_handlers.c) */
#define G_EMPIRE_OBJECT_ALT_X 0x0077F75Cu /* @bin 0x00407A10 i16 +0x14 of empire object 0: the alternative x (+0x16 y) when 0x00784CB4 is set */
#define G_EMPIRE_SCROLL_Y 0x007843DAu /* @bin 0x0040A090 i16 +2 of the records of 0x20 bytes at G_EMPIRE_OBJECTS_END: the scroll y */
#define G_EMPIRE_ALT_POSITIONS 0x00784CB4u /* @bin 0x00407A10 i32 set: the empire objects' alternative positions (present/play/play.c) */
#define G_SCENARIO_GODS 0x00784DDEu /* @bin 0x00520632 i16, 1-based: the scenario's list read by build menu 0x1C (G_MONUMENT_SLOTS - 2; core/command/construction_ui.c) */
#define G_MONUMENT_SLOT_2 0x00784DE2u /* @bin 0x00520632 i16 G_MONUMENT_SLOTS[1] */
#define G_PHARAOH_NAME 0x00784F08u /* @bin 0x00447B50 i32 the pharaoh's name (string of group 0x97) */
#define G_TOTAL_CONNECTIONS 0x0078D794u /* @bin 0x00534DF0 i32 the evolve panel's "total connections" */
#define G_CONNECTIONS 0x0078D7E8u /* @bin 0x00534DF0 i32 the evolve panel's "no of connects" (shown - 1) */
#define G_U978D7C 0x00978D7Cu /* @bin 0x004C5E60 i32 2: the message window draws image 0x00F5690C instead of the god's */
#define G_FORMATION_X 0x00BCB9F6u /* @bin 0x004B80B0 i16 +0x2E of formation 0 (G_FORMATION_BASE, stride 0x90): x */
#define G_FORMATION_Y 0x00BCB9F8u /* @bin 0x004B80B0 i16 +0x30 of formation 0: y (present/sidebar/minimap.c) */
#define G_FORMATION_U42 0x00BCBA0Au /* @bin 0x004B80B0 u8 +0x42 of formation 0: 0 when not dying */
#define G_FORMATION_IN_FIELD 0x00BCBA2Fu /* @bin 0x00522A60 u8 +0x67 of formation 0: set makes the legions page's "any" */
#define G_FORMATION_U82 0x00BCBA4Au /* @bin 0x00522A60 i16 +0x82 of formation 0: < 0 still to place */
#define G_QUIT_REQUEST 0x00D3B43Du /* @bin 0x004D8310 u8 the quit request (WM_CLOSE: 3; docs/presentation.md 5.1) */
#define G_TEST_VALUE4A 0x00D3B460u /* @bin 0x00534610 i32 "test_value4a" */
#define G_TEST_VALUE3A 0x00D3B470u /* @bin 0x00534610 i32 "test_value3a" */
#define G_TEST_VALUE2A 0x00D3B480u /* @bin 0x00534610 i32 "test_value2a" */
#define G_TEST_VALUE1A 0x00D3B490u /* @bin 0x00534610 i32 "test_value1a" */
#define G_UE38DE0 0x00E38DE0u /* @bin 0x00425F20 u8 := 1 when a menu opens / closes / chooses */
#define G_UE38DE1 0x00E38DE1u /* @bin 0x00425F20 u8 := 1 (2 when the menu closes) */
#define G_MOUSE_MOVED 0x00E38E40u /* @bin 0x00534610 i32 "mouse_movement" (docs/presentation.md 5.2: "moved") */
#define G_GROUP_WIDTH_2 0x00E945D4u /* @bin 0x004D5EF0 i32 group 2's */
#define G_GROUP_WIDTH_3 0x00E945D8u /* @bin 0x004D5EF0 i32 group 3's */
#define G_GROUP_WIDTH_4 0x00E945DCu /* @bin 0x004D5EF0 i32 group 4's */
#define G_GROUP_WIDTH_7 0x00E945E0u /* @bin 0x004D5EF0 i32 group 7's */
#define G_GROUP_WIDTH_10 0x00E945E4u /* @bin 0x004D5EF0 i32 group 10's */
#define G_LAYOUT_HIT 0x00E945F8u /* @bin 0x004D6740 i32 the layout element's hit (present/draw/layout.c) */
#define G_LAYOUT_HIT_U 0x00E945FCu /* @bin 0x004D6740 i32 := 0 with the element kept */
#define G_LAYOUT_HIT_X 0x00E94600u /* @bin 0x004D6740 i32 the element's x */
#define G_LAYOUT_HIT_Y 0x00E94604u /* @bin 0x004D6740 i32 the element's y */
#define G_LAYOUT_HIT_W 0x00E94608u /* @bin 0x004D6740 i32 the element's width */
#define G_LAYOUT_HIT_H 0x00E9460Cu /* @bin 0x004D6740 i32 the element's height */
#define G_LAYOUT_RECT_LISTS 0x00E9461Cu /* @bin 0x004D6740 u32 per window key (stride 16): its list of rectangle overrides (present/draw/layout.c) */
#define G_LAYOUT_RECTS_DONE 0x00E94624u /* @bin 0x004D6740 i32 per window key (stride 16): the overrides applied */
#define G_LAYOUT_HIT_ENTRY 0x00E94DA0u /* @bin 0x004D6740 u32 the hit rectangle's entry */
#define G_LAYOUT_HIT_INDEX 0x00E94DA4u /* @bin 0x004D6740 i32 the hit rectangle's index */
#define G_SIDEBAR_REDRAW_BG 0x00F53F74u /* @bin 0x00425F20 i32 asks the sidebar for a background redraw (present/sidebar/sidebar.c) */
#define G_WAS_IN_SET 0x00F55B9Cu /* @bin 0x00534610 i32 "was_in_set" */
#define G_WAS_IN_INT 0x00F55BA0u /* @bin 0x00534610 i32 "was_in_int" */
#define G_PORTRAIT_IMAGES 0x00F55C7Cu /* @bin 0x004FA8C0 i32 the first figure portrait image (present/ui/info/figure.c) */
#define G_PANEL_IMAGES 0x00F5683Cu /* @bin 0x00536820 i32 the first image of the bordered panels (present/sidebar/sidebar.c) */
#define G_IMG_4100 0x00F56B8Cu /* @bin 0x00434AA0 i32 the image of terrain kinds 0x4100, + variant 0..7 (present/city/overlay_tiles.c) */
#define G_IMG_OVERLAY_LEVELS 0x00F56CA4u /* @bin 0x00434AA0 i32 the first overlay level image (present/city/overlay_tiles.c) */
#define G_LC_CTYPE 0x0126648Cu /* @bin 0x0056B240 u32 the CRT's LC_CTYPE handle, 0: the "C" locale (core/graphics/last_graphics.c) */

#endif
