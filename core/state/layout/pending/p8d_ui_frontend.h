/* Imhotep - pending names of package p8d (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line; the area in brackets. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P8D_UI_FRONTEND_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P8D_UI_FRONTEND_H

/* the scenario's goals (@bin 0x004EA2C0, core/city/day_goals.c; save.md §5 "Ratings") [g_scenario.h] */
#define G_GOAL_CULTURE 0x00784C24u        /* @bin 0x004EA2C0 i32: the culture rating goal (CITY_RATING_CULTURE) */
#define G_GOAL_PROSPERITY 0x00784C28u     /* @bin 0x004EA2C0 i32: the prosperity rating goal */
#define G_GOAL_KINGDOM 0x00784C30u        /* @bin 0x004EA2C0 i32: the kingdom rating goal */
#define G_GOAL_HOUSES 0x00784C34u         /* @bin 0x004EA2C0 i32: houses of at least the kind G_GOAL_HOUSE_LEVEL asked (0: none) */
#define G_GOAL_CULTURE_ON 0x00784C3Cu     /* @bin 0x004EA2C0 u8: the culture goal is set */
#define G_GOAL_PROSPERITY_ON 0x00784C3Du  /* @bin 0x004EA2C0 u8: the prosperity goal is set */
#define G_GOAL_MONUMENT_ON 0x00784C3Eu    /* @bin 0x004EA2C0 u8: the monument goal is set */
#define G_GOAL_KINGDOM_ON 0x00784C3Fu     /* @bin 0x004EA2C0 u8: the kingdom goal is set */
#define G_GOAL_POPULATION_ON 0x00784C58u  /* @bin 0x004EA2C0 i32: != 0: the population goal is set */
#define G_GOAL_POPULATION 0x00784C5Cu     /* @bin 0x004EA2C0 i32: the population goal (CITY_POPULATION) */
#define G_U784B60 0x00784B60u             /* @bin 0x004EA2C0 i16: != 0: no goals are tested (window 0x3D shows text 0x91, 0) */
#define G_UEA17FC 0x00EA17FCu             /* @bin 0x004EA2C0 u8: := 0 by the goals' test, := 1 in some of its cases; 0x004EA880 tests it [g_city.h] */
#define G_UEA1380 0x00EA1380u             /* @bin 0x004EA920 u8: := 0x004183D0's result; 0x004EA880: clear, mission end kind 1 [g_city.h] */
#define G_U5D2A3C 0x005D2A3Cu             /* @bin 0x00559FC0 i32, 8 bytes per empire city kind (an object's +0x19): an id from 0x26 [g_city.h] */

/* the message window's body scroll buttons (present/ui/messages, present/ui/msgwin.c) [g_ui_main.h] */

/* the layout mode's element under the cursor (present/draw/layout.c 0x004D6480, present/ui/dispatch.c) [g_city_view.h] */
#define G_UE945F8 0x00E945F8u /* @bin 0x004D6480 i32: the element's x less the layout origin (dispatch.c: the hit entry) */
#define G_UE945FC 0x00E945FCu /* @bin 0x004D6480 i32: its y less the layout origin */
#define G_UE94600 0x00E94600u /* @bin 0x004D6480 i32: its registered x */
#define G_UE94604 0x00E94604u /* @bin 0x004D6480 i32: its registered y */
#define G_UE94608 0x00E94608u /* @bin 0x004D6480 i32: its width */
#define G_UE9460C 0x00E9460Cu /* @bin 0x004D6480 i32: its height */

/* sound (core/frame/sound.c) [g_frame.h] */

/* the C runtime (_dosmaperr 0x005672C8) [g_boot.h] */
#define G_CRT_ERRNO_TABLE 0x00604F30u /* @bin 0x005672C8 0x2D pairs (Win32 error, errno) */
#define G_CRT_ERRNO 0x01266430u       /* @bin 0x005672C8 u32: errno */
#define G_CRT_DOSERRNO 0x01266434u    /* @bin 0x005672C8 u32: _doserrno */

/* city record fields (save.md §5 "Ratings") [city.h] */
#define CITY_RATING_CULTURE 0x42CCu    /* @bin 0x004269B0 i32: the culture rating */
#define CITY_RATING_PROSPERITY 0x42D0u /* @bin 0x0046E1E0 i32: the prosperity rating */
#define CITY_RATING_MONUMENT 0x42D4u   /* @bin 0x0046EF50 i32: the monument rating */

#endif
