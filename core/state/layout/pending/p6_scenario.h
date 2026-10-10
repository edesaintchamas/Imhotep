/* Imhotep - pending names of package p6 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P6_SCENARIO_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P6_SCENARIO_H

/* -- named with the rewrite of core/boot, core/newgame/graphics.c -- */
#define G_DEBUG_EVENT 0x00605A08u /* @bin 0x0051FBCC i32: debug: the event (text group 0x9D), Ctrl+G cycles it (core/frame/wndproc.c) [g_frame.h] */
#define G_DEBUG_EVENT_LEVEL 0x00605A04u /* @bin 0x0051FBCC i32: debug: its level, Ctrl+H [g_frame.h] */
#define G_DEBUG_EVENT_KIND 0x00605A00u /* @bin 0x0051FBCC i32: debug: its kind, Ctrl+J; Ctrl+K posts it [g_frame.h] */
#define G_SURFACE_LOCK 0x006DE518u /* @bin 0x004D0480 the surface lock object: +8 / +0xC the back buffer's width / height, +0x10 1 for 565, +0x1C / +0x20 the surfaces, +0x24 / +0x28 their pixels [g_frame.h] */
#define G_WINDOWED_X 0x00E390E8u /* @bin 0x00426E09 i32: the windowed window's place, centred on the screen [g_frame.h] */
#define G_WINDOWED_Y 0x00E390E4u /* @bin 0x00426E09 i32 [g_frame.h] */
#define G_UE92E00 0x00E92E00u /* @bin 0x004D01E0 an object released (never set in the runs, core/frame/display.c) [g_frame.h] */
#define G_UE92E04 0x00E92E04u /* @bin 0x00426FF0 i32: := 0 around the lock test; != 0 with a video: no present (core/frame/frame.c) [g_frame.h] */
#define G_RESOLUTION_KEPT 0x00E92E4Cu /* @bin 0x004D0220 i32: the last resolution that worked [g_frame.h] */
#define G_FULLSCREEN_KEPT 0x005E06D8u /* @bin 0x004D0220 i32: the last fullscreen option that worked [g_frame.h] */
#define G_U5E06D4 0x005E06D4u /* @bin 0x004D0220 u8: := 0 when a mode works [g_frame.h] */
#define G_APP_ACTIVE 0x00E3912Cu /* @bin 0x004D0220 i32: the window is active (core/frame/display.c) [g_frame.h] */
#define G_DRAG_STATE 0x00E38E14u /* @bin 0x004CF263 i32: a mouse drag, 1 started, 2 past 50 px from its start (core/frame/frame.c) [g_frame.h] */
#define G_UE38D90 0x00E38D90u /* @bin 0x004CF990 i32: the mode's height (with G_DRAW_H) [g_frame.h] */
#define G_SCREEN_BYTES 0x00E38D8Cu /* @bin 0x004CF990 i32: the back buffer's size in bytes, 2 * w * h (docs/presentation.md 2.1) [g_frame.h] */
#define G_RESOLUTION_SAVED 0x00E38EE4u /* @bin 0x00414902 i32: the confirmed resolution (core/frame/display.c, present/ui/frontend/screens.c) [g_frame.h] */
#define G_AMBIENT_CHOICE 0x00F55B80u /* @bin 0x0053B9A3 i32: the ambient sound chosen (-1 by the start-up; core/frame/sound.c) [g_frame.h] */
#define G_LAST_ADVISOR 0x00E38EA4u /* @bin 0x00414916 i32: the last advisor shown (core/command/advisor.c, present/sidebar/input.c) [g_command.h] */
#define G_UE38EEC 0x00E38EECu /* @bin 0x00403500 i32: the option of menu Options 8; set: no autosave by the month change (core/frame/autosave.c) [g_command.h] */
#define G_UE38EC3 0x00E38EC3u /* @bin 0x00403540 u8: the option of menu Options 9; set: the pyramid progress counts (core/city/rest_gods.c) [g_command.h] */
#define G_CURSOR_UNLOAD 0x00E390C4u /* @bin 0x0040BA40 cursor 0x90: unload (docs/commands.md) [g_command.h] */
#define G_CURSOR_BOARD 0x00E390C8u /* @bin 0x0040BA40 cursor 0x91: board (docs/commands.md) [g_command.h] */
#define G_HARVEST_SIZE 0x00BCA34Cu /* @bin 0x00489950 i32: the harvest size, __ftol(G_DBL_10_56F5A8) * 200 (core/city/day2_farm.c) [g_city.h] */
#define G_REASON_PHRASES 0x007854A8u /* @bin 0x0044EFA0 u8[0x36][0x24] from "auto reason phrases.txt": the level table of core/city/events.c [g_city.h] */
#define G_MASTABA_VARIANT 0x00E94E18u /* @bin 0x0053D970 i32: the mastaba variant (0x0053D930; core/building/rest_redraw.c) [g_building.h] */

/* -- named with the rewrite of core/io/save_load.c, core/newgame/reset.c -- */
/* @bin 0x004B5AF0 the figure name counters (save.md §3 chunks 54-74, 101-104, 150-165; seeded by
 * 0x004B58D0), named after their text group as G_NAME_COUNTER_10F [g_figure.h] */
#define G_NAME_COUNTER_FE 0x00D056E0u /* [g_figure.h] */
#define G_NAME_COUNTER_FF 0x00D056DCu /* [g_figure.h] */
#define G_NAME_COUNTER_100 0x00D056D8u /* [g_figure.h] */
#define G_NAME_COUNTER_102 0x00D056D0u /* [g_figure.h] */
#define G_NAME_COUNTER_103 0x00D056CCu /* [g_figure.h] */
#define G_NAME_COUNTER_104 0x00D056C8u /* [g_figure.h] */
#define G_NAME_COUNTER_105 0x00D056C4u /* [g_figure.h] */
#define G_NAME_COUNTER_106 0x00D056C0u /* [g_figure.h] */
#define G_NAME_COUNTER_10B 0x00D056ACu /* [g_figure.h] */
#define G_NAME_COUNTER_10C 0x00D056A8u /* [g_figure.h] */
#define G_NAME_COUNTER_10D 0x00D056A4u /* [g_figure.h] */
#define G_NAME_COUNTER_10E 0x00D056A0u /* [g_figure.h] */
#define G_NAME_COUNTER_110 0x00D05698u /* [g_figure.h] */
#define G_NAME_COUNTER_111 0x00D05694u /* [g_figure.h] */
#define G_NAME_COUNTER_112 0x00D05690u /* [g_figure.h] */
#define G_NAME_COUNTER_113 0x00D0568Cu /* Architect [g_figure.h] */
#define G_NAME_COUNTER_114 0x00D05688u /* Constable [g_figure.h] */
#define G_NAME_COUNTER_115 0x00D05684u /* Tax collector [g_figure.h] */
#define G_NAME_COUNTER_116 0x00D05680u /* Magistrate [g_figure.h] */
#define G_NAME_COUNTER_117 0x00D0567Cu /* Ferry boat [g_figure.h] */
#define G_NAME_COUNTER_118 0x00D05678u /* Sentry [g_figure.h] */
#define G_NAME_COUNTER_119 0x00D05674u /* enemy class 7 [g_figure.h] */
#define G_NAME_COUNTER_11A 0x00D05670u /* enemy class 6 [g_figure.h] */
#define G_NAME_COUNTER_11B 0x00D0566Cu /* enemy class 5 [g_figure.h] */
#define G_NAME_COUNTER_11C 0x00D05668u /* enemy class 4 [g_figure.h] */
#define G_NAME_COUNTER_11D 0x00D05664u /* enemy class 0 [g_figure.h] */
#define G_NAME_COUNTER_11E 0x00D05660u /* enemy class 2 [g_figure.h] */
#define G_NAME_COUNTER_11F 0x00D0565Cu /* enemy class 1 [g_figure.h] */
#define G_NAME_COUNTER_120 0x00D05658u /* enemy class 3 [g_figure.h] */
#define G_NAME_COUNTER_121 0x00D05654u /* ships [g_figure.h] */
#define G_NAME_COUNTER_122 0x00D05650u /* Bedouin infantry [g_figure.h] */
#define G_NAME_COUNTER_12C 0x00D0564Cu /* enemy class 8 [g_figure.h] */
#define G_NAME_COUNTER_12D 0x00D05648u /* enemy class 9 [g_figure.h] */
#define G_NAME_COUNTER_12E 0x00D05644u /* enemy class 10 [g_figure.h] */
#define G_NAME_COUNTER_12F 0x00D05640u /* enemy class 11 [g_figure.h] */
/* @bin 0x004B7C80 the military counts (save.md §3 chunks 108-112) */
#define G_ENEMY_FORMATIONS 0x00D0563Cu /* i32 chunk 108: enemy formations [g_figure.h] */
#define G_ENEMY_SOLDIERS 0x00D05638u /* i32 chunk 109: enemy soldiers [g_figure.h] */
#define G_OWN_FORMATIONS 0x00D05634u /* i32 chunk 110: own formations [g_figure.h] */
#define G_OWN_STRENGTH 0x00D05630u /* i32 chunk 111: own soldier strength [g_figure.h] */
/* formations (save.md §3 chunks 25-27) */
#define G_FORMATION_HIGHEST 0x00BCB9BCu /* @bin 0x004B6E10 i32 chunk 25: highest formation id in use [g_building.h] */
#define G_LEGION_LAST 0x00BCB9B8u /* @bin 0x004B77D0 i32 chunk 26: last player legion found by the recount [g_figure.h] */
#define G_TILE_VISITS 0x00978D9Cu /* @bin 0x0047E250 u32 per tile, chunk 148: walker tile-visit counters (ten 3-bit counters) [g_figure.h] */
#define G_SHIPWRIGHT_QUEUES 0x009783C0u /* @bin 0x004D0A40 the 10 shipwright queues (building +0xCB), G_SHIPWRIGHT_QUEUE_STRIDE bytes each: +0 u8 in use, +0x18 building id (0x0046A260) [g_building.h] */
#define G_SHIPWRIGHT_QUEUE_STRIDE 0x1Cu /* [g_building.h] */
#define G_LOCUST_TIMER 0x0078D79Cu /* @bin 0x004E6EB0 i32 chunk 40: locust plague timer (-1 armed; 2632 ticks) [g_city.h] */
#define G_HAIL_TIMER 0x0078D7A0u /* @bin 0x004E6ED0 i32 chunk 46: hailstorm timer (408 ticks) [g_frame.h] */
#define G_U78D5C8 0x0078D5C8u /* @bin 0x0046A080 i32[10]: all := 0; four of them daily by 0x0046A060 [g_city.h] */
#define G_U78D7E0 0x0078D7E0u /* @bin 0x00451BE0 i32: := 0 with the date; the debug panel's "error" (present/ui/debug_panels.c) [g_ui.h] */
#define G_DEBUG_EVOLVE 0x0078D7DCu /* @bin 0x00534DF0 i32: debug switch, 1: the evolve counters panel [g_ui.h] */
#define G_DEBUG_TILE 0x0078D7D8u /* @bin 0x00535080 i32: debug switch, 1: the tile under the mouse panel [g_ui.h] */
#define G_DEBUG_FIGURE 0x0078D7D4u /* @bin 0x00536120 i32: debug switch, 1: the figure under the mouse panel [g_ui.h] */
#define G_SOUND_LAST_TIME 0x007028FCu /* @bin 0x0041D5BE u32: the last time (timeGetTime) a city sound slot was chosen [g_frame.h] */
#define G_UE38E54 0x00E38E54u /* @bin 0x00431029 i32: 1: no border image off the map (present/city/ground.c); := 0 by 0x00540B50 [g_frame.h] */
#define G_CHEAT_WIN 0x00E38DD8u /* @bin 0x0041CFC0 u8: := 1 by the "Pharaohs Tomb" cheat; the goals check then wins (core/city/day_goals.c) [g_city.h] */
#define G_CHEATED 0x00E38DD7u /* @bin 0x0041CFC0 u8: a cheat was used: the game is not recorded in the high scores [g_command.h] */
#define G_ANIM_TIMERS 0x00605A6Cu /* @bin 0x0040F270 i32[51]: the animation timers (flags G_ANIM_FLAGS) [g_frame.h] */
/* city record fields (CITY_ADDR(p) + ...) [city.h] */
#define CITY_U03 0x03u      /* @bin 0x004E7E20 u8: G_UEA1820 := (it != 0); := 1 for record 1 by 0x00540A10 [city.h] */
#define CITY_U20 0x20u      /* @bin 0x004E7D10 i32: init 3 [city.h] */
#define CITY_U25BC 0x25BCu  /* @bin 0x004E7D10 i32: init 0 [city.h] */
#define CITY_U25C0 0x25C0u  /* @bin 0x004E7D10 i32: init 0 [city.h] */
#define CITY_U2796 0x2796u  /* @bin 0x004D18B0 i16[30] of saves before version 123, widened to CITY_U4860 [city.h] */
#define CITY_U27DE 0x27DEu  /* @bin 0x004D18B0 i16[30] of saves before version 123, widened to CITY_U48F0 [city.h] */
#define CITY_U2900 0x2900u  /* @bin 0x004D1880 i32[7] of saves before version 122, copied to CITY_U483C [city.h] */
#define CITY_U291C 0x291Cu  /* @bin 0x004D1A70 i16[20] of saves before version 156, widened to CITY_U3C64 [city.h] */
#define CITY_U2944 0x2944u  /* @bin 0x004D1A70 i16[20] of saves before version 156, widened to CITY_U3D04 [city.h] */
#define CITY_U298C 0x298Cu  /* @bin 0x004D4040 u8[0x2D]: := 0 by the version-150 upgrade [city.h] */
#define CITY_U3C64 0x3C64u  /* @bin 0x004D1A70 i32[40] [city.h] */
#define CITY_U3D04 0x3D04u  /* @bin 0x004D1A70 i32[40] [city.h] */
#define CITY_U42C4 0x42C4u  /* @bin 0x004E7D10 i32: init 1 [city.h] */
#define CITY_U42C8 0x42C8u  /* @bin 0x004E7D10 i32: init 1 [city.h] */
#define CITY_U438C 0x438Cu  /* @bin 0x004E7D10 i32: init 1 [city.h] */
#define CITY_U43C2 0x43C2u  /* @bin 0x004E9EE0 u8[5] per god: init 0 [city.h] */
#define CITY_U43CC 0x43CCu  /* @bin 0x004E9EE0 u8[5] per god: init 0 [city.h] */
#define CITY_U43D6 0x43D6u  /* @bin 0x004E9EE0 u8[5] per god: init 0 [city.h] */
#define CITY_U44C8 0x44C8u  /* @bin 0x004D4080 i32: := 0 by the version-155 upgrade [city.h] */
#define CITY_U44CC 0x44CCu  /* @bin 0x004D4080 i32: := 0 by the version-155 upgrade [city.h] */
#define CITY_U44F4 0x44F4u  /* @bin 0x004E7D10 i32: init 1 [city.h] */
#define CITY_U4574 0x4574u  /* @bin 0x004E7D10 i32: init 0 [city.h] */
#define CITY_U4578 0x4578u  /* @bin 0x004E7D10 i32: init 0 [city.h] */
#define CITY_U457C 0x457Cu  /* @bin 0x004E7D10 i32: init 0 [city.h] */
#define CITY_U45D8 0x45D8u  /* @bin 0x004E9EE0 i32: init 0 [city.h] */
#define CITY_U473C 0x473Cu  /* @bin 0x004D3C20 i32[3]: building ids, obelisks 0xCF / 0xD0 among them [city.h] */
#define CITY_U483C 0x483Cu  /* @bin 0x004D1880 i32[7] [city.h] */
#define CITY_U4860 0x4860u  /* @bin 0x004D18B0 i32[30] [city.h] */
#define CITY_U48F0 0x48F0u  /* @bin 0x004D18B0 i32[30] [city.h] */

/* -- named with the rewrite of core/newgame/flow.c, flow2.c, figures.c, setup.c -- */
#define G_U7027A8 0x007027A8u /* @bin 0x004181E0 i32: := 0 by the briefing, := 1 by 0x00417E70 and the mission window [g_sim.h] */
#define G_TRADER_NEXT 0x0077AEFCu /* @bin 0x00444B30 chunk 95: the next trader record index [g_figure.h] */
#define G_SCENARIO_MAP_WIDTH 0x00784904u /* @bin 0x004DE6A2 i32: the scenario's map width -> G_MAP_WIDTH [g_map.h] */
#define G_SCENARIO_MAP_HEIGHT 0x00784908u /* @bin 0x004DE6A2 i32 -> G_MAP_HEIGHT [g_map.h] */
#define G_OVERLAY_PREV 0x00E94E10u /* @bin 0x004168E0 i32: the previous overlay [g_frame.h] */
#define G_UF55B94 0x00F55B94u /* @bin 0x00418301 i32: := 1 when the ambient track starts (core/city/events_actions.c), := 0 here [g_city.h] */
/* city record fields */
#define CITY_BALANCE_LAST_YEAR 0x2CBCu /* @bin 0x004DE98C i32: the balance at the end of last year (save.md §5) */
#define CITY_PERSONAL_SAVINGS 0x4450u /* @bin 0x00417DD0 i32: personal savings (save.md §5, chunk 88) */
#define CITY_U471C 0x471Cu /* @bin 0x00418220 i32: := 0 by the briefing's continue */
#define CITY_DIFFICULTY 0x47A8u /* @bin 0x00418301 i32: the difficulty copy (save.md §5), at most the one chosen */

/* -- named with the rewrite of core/newgame/map_load.c, start.c, start2.c -- */
#define G_ALLOWED_BUILDINGS 0x00784CE0u /* @bin 0x004D4AE0 u16[100] (scenario info +0x408): allowed-building flags (0x00404350, core/command/menu.c) [g_command.h] */
#define G_UE950D4 0x00E950D4u /* @bin 0x004E2B10 i32: the no-road notice only while < 3; := 0 by 0x004E1770 [g_command.h] */
#define G_GOAL_MONUMENT 0x00784C2Cu /* @bin 0x004EA2C0 i32: the monument rating goal (CITY + 0x42D4, core/city/day_goals.c) [g_city.h] */
#define G_POP_MILESTONES 0x00E951C4u /* @bin 0x00458540 u8[10] chunk 81: population milestone flags (+1 .. +9) [g_city.h] */
#define G_IMAGE_58_59 0x00F569D4u /* @bin 0x00470990 i32: first image of types 0x58 (+0, +1) and 0x59 (+2) [g_building.h] */
#define G_IMAGE_5D 0x00F565DCu /* @bin 0x00470990 i32: first image of type 0x5D (+ BLD_U54, 0x0047C260) [g_building.h] */
#define G_MASTABA_PART_KIND 0x005E2AE4u /* @bin 0x004DF2C0 the monument part table: 116 parts of 0x34 bytes (13 dwords; low byte of +0 the part's kind, core/building/monument.c) [g_building.h] */
#define G_NEXT_MESSAGE_INDEX 0x00E951D4u /* @bin 0x004E23E0 i32: the next-message button's place in its cycle [g_message.h] */
#define G_NEXT_MESSAGE_TIME 0x00E951D0u /* @bin 0x00406030 u32: wall clock of the button's last use (the cycle restarts after 3 s) [g_message.h] */
#define G_MESSAGE_COUNTERS 0x00E95174u /* @bin 0x00468B70 i32[20] chunk 82: per-category message counters / timers [g_message.h] */
#define G_MESSAGE_DELAYS 0x00E95124u /* @bin 0x004E1850 i32[20] chunk 83: per-category message delays, monthly -1 [g_message.h] */
/* city record fields (CITY_ADDR(p) + ...) [city.h] */
#define CITY_SALARY 0x2C98u /* @bin 0x005405B0 i32: the salary (save.md §5), G_SALARY_BY_RANK[CITY_SALARY_RANK] [city.h] */
#define CITY_SALARY_RANK 0x2C9Cu /* @bin 0x005405B0 i32: the salary rank (save.md §5), at most 10 [city.h] */
#define CITY_RATING_KINGDOM 0x42D8u /* @bin 0x0046E970 i32: the kingdom rating (save.md §5) [city.h] */
#define CITY_PLAYER_RANK 0x444Cu /* @bin 0x005405B0 i32: the player's rank (save.md §5) [city.h] */
#define CITY_U44D0 0x44D0u /* @bin 0x00418640 i32: := 0 by the next mission [city.h] */
#define CITY_U44D4 0x44D4u /* @bin 0x00418640 i32: := 0 by the next mission [city.h] */
#define CITY_U44D8 0x44D8u /* @bin 0x00418640 i32: := 0 by the next mission [city.h] */
#define CITY_U45A0 0x45A0u /* @bin 0x0046D480 i32: x of the first type 0x59 building of the map (0: none) [city.h] */
#define CITY_U45A4 0x45A4u /* @bin 0x0046D480 i32: its y [city.h] */
#define CITY_U4720 0x4720u /* @bin 0x005405B0 i32: := 1 at the start [city.h] */
#define CITY_U4724 0x4724u /* @bin 0x005405B0 i32: := 1 at the start [city.h] */
#define CITY_U47A0 0x47A0u /* @bin 0x004D5980 i32: := -1 by the grid setup [city.h] */
#define CITY_U47A4 0x47A4u /* @bin 0x004D5980 i32: := -1 by the grid setup [city.h] */
#define CITY_U4814 0x4814u /* @bin 0x004D5980 i16: := 0 by the grid setup [city.h] */
/* building record fields [building.h] */
#define BLD_U54 0x54u /* @bin 0x0047C260 i16: type 0x5D: the image step from G_IMAGE_5D (0..4; 0x0046D480 sets 0 / 1) [building.h] */

#endif
