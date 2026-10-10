/* Imhotep - named globals: figures (the record array: state/records.h, its fields: layout/figure.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_FIGURE_H
#define IMHOTEP_STATE_LAYOUT_G_FIGURE_H

/* @bin 0x00461B20 the name counters (chunks 54-74, save.md §4): i32 index of the next name in the
 * text group named after it (FIG_NAME_GROUP, FIG_NAME_INDEX), cycling at its limit */
#define G_NAME_COUNTER_10F 0x00D0569Cu
#define G_NAME_COUNTER_10A 0x00D056B0u /* groups 0x10A .. 0x10E: this one and the four below it */
#define G_NAME_COUNTER_109 0x00D056B4u
#define G_NAME_COUNTER_108 0x00D056B8u
#define G_NAME_COUNTER_107 0x00D056BCu
#define G_NAME_COUNTER_101 0x00D056D4u

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_DIE_FRAMES 0x005D048Cu /* core/figure/actions_b.h: dying animation frame by wait ticks / 2 */
#define G_PRICE_BUY 0x005D2C48u /* present/ui/advisors/d_trade.c: i32 per resource (8-byte entries) */
#define G_PRICE_SELL 0x005D2C4Cu /* present/ui/advisors/d_trade.c */
#define G_ACTION_TABLE 0x005DC28Cu /* core/figure/figure.c: one action function per figure type */
#define G_DIRECTION_OFFSETS 0x005F80FCu /* core/map/route_path.c: i32 grid offset per direction 0..7 */
#define G_PROPS 0x007090CCu /* core/figure/actions_b.h: figure property table, 0x38 bytes per type */
#define G_FIGURE_TYPES 0x007090D0u /* core/building/building_misc.c: records of 0x38 bytes per figure type */
#define G_TYPE_TABLE 0x007090E4u /* core/figure/actions_d4.c: @bin per figure type, 0x38 bytes: +0 range, +4, +8 speed, +0x10 */
#define G_HOUSE_TAX_VALUE 0x0070CE18u /* core/command/orders.c: i32 house model field per level, stride 0x58 */
#define G_DEFAULT_LIMIT 0x00775810u /* core/command/trade.c: i32 per resource, hundredths */
#define G_TRADE_DONE 0x0077592Cu /* present/ui/empire/empire.c: i32 [route * 0x24 + resource] */
#define G_INVASION_SUB 0x00784B94u /* core/figure/invaders.c: i16 */
#define G_U784BBC 0x00784BBCu /* core/figure/actions_a2.c: non-zero: no goods are moved to the storage yards [unresolved] */
#define G_ENTRY_X 0x00784C8Cu /* core/figure/actions_d4.c: @bin the map's river entry / exit points (s16 each) */
#define G_ENTRY_Y 0x00784C8Eu /* core/building/daily.c: i16 */
#define G_EXIT_X 0x00784C90u /* core/figure/actions_d4.c */
#define G_EXIT_Y 0x00784C92u /* core/figure/actions_d4.c */
#define G_U784CB0 0x00784CB0u /* core/figure/invaders.c: low byte: the invasion kind (records of 0x30 bytes at 0x005E4C20) [unresolved] */
#define G_WORK_COUNT 0x0078D7F0u /* core/figure/actions_d2.c: @bin candidates found by 0x004BEEB0 */
#define G_WORK_LIST 0x0078D9A8u /* core/figure/actions_d2.c: @bin candidate building ids (s16, at most 500) */
#define G_STORAGE_GET 0x00790134u /* core/command/orders_storage.c: i16 fill limit per resource (other orders) */
#define G_TARGETS 0x0089B6D8u /* core/figure/helpers_xa.c: 27 i32: the chosen source per good index */
#define G_CAND_TILE 0x0089B744u /* core/figure/helpers_xa.c: 27 x 4000 i32: each candidate's road tile */
#define G_CAND_BUILDING 0x00904EC4u /* core/figure/helpers_xa.c: 27 x 4000 i32: each candidate's building */
#define G_CANDIDATES 0x0096E6B0u /* core/figure/actions_a2.c: count, then the candidates' map offsets (0x0096E6B4 ..) */
#define G_FULL_LIST 0x0096E6B4u /* core/building/day_water.c: i32[4000]: tiles at 255, floodplain from the start, others from the end */
#define G_CANDIDATE_IDS 0x00972530u /* core/figure/actions_a2.c: the candidates' building ids, 1-based like the offsets */
#define G_FERRY_SLOTS 0x009784D8u /* core/figure/actions_c.c */
#define G_UA43EAC 0x00A43EACu /* core/figure/actions_c.c: byte grid (0xFF: a tree being cut?) [unresolved] */
#define G_THREAT 0x00A835FCu /* core/figure/enemies.c: u8 per tile: threat of the player's soldiers */
#define G_FIGURE_GRID 0x00B350DCu /* core/city/day2_flood.c: i16 per tile: first figure */
#define G_MAP_YE 0x00BCA2F4u /* core/figure/actions_c2.c: last y */
#define G_PROF_STATS 0x00BCA850u /* core/figure/figure.c */
#define G_PROF_TYPE 0x00BCB610u /* core/figure/figure.c */
#define G_PROF_START 0x00BCB614u /* core/figure/figure.c */
#define G_PROF_END 0x00BCB618u /* core/figure/figure.c */
#define G_FORMATION_BASE 0x00BCB9C8u /* core/io/save_load.c: chunk 24, 50 x 144 */
#define G_FORMATION_FLAG 0x00BCBA24u /* core/figure/actions_b6.c: i16 in the 0x90-byte records indexed by figure +0x60 */
#define G_BEST_DISTANCE 0x00BCD5ECu /* core/figure/actions_c2.c: the target searches' best distance so far */
#define G_FIGURE_HIGHEST 0x00BCD5F0u /* core/figure/figure_create.c: highest figure id created */
#define G_FIGURE_PATHS 0x00C8AD38u /* core/map/route_path.c: 500 bytes per figure */
#define G_ROUTE_OWNERS 0x00D04E58u /* core/figure/actions_a3.c: route slot -> the figure using it (i16) */
#define G_FIGURES_UPDATED 0x00D056F4u /* core/figure/figure.c: live figures counted by the last update */
#define G_CURRENT_FIGURE 0x00D056F8u /* core/figure/actions_b.h: the figure being updated */
#define G_FIGURE_SERIAL 0x00D056FCu /* core/figure/figure_create.c: creation counter, kept at +0x86 */
#define G_PROF_PRECISE 0x00D05710u /* core/figure/figure.c: QueryPerformanceCounter available */
#define G_R_1_15 0x00D3B370u /* core/city/month_events.c */
#define G_R_IV1 0x00D3B384u /* core/city/month_events.c */
#define G_DIST_Y 0x00D3B4F8u /* core/figure/actions_d2.c: @bin |dy| of the last distance */
#define G_DIST_X 0x00D3B4FCu /* core/figure/actions_a.c: the last distances computed (kept by the original in globals) */
#define G_SPRITE_DIRECTION 0x00E94E00u /* core/figure/actions_a.c: the current figure's direction relative to the view */
#define G_PLAYER 0x00EAAC28u /* core/city/goods.c: chunk 31 */
#define G_REC88 0x00EAAF68u /* core/state/post_load.c */
#define G_EBAB20 0x00EBAB20u /* core/map/routing.h */
#define G_EC7630 0x00EC7630u /* core/map/routing.h */
#define G_GRID_ROUTING_WATER 0x00ED4140u /* core/figure/actions_c2.c: i8 per tile: routing (water) */
#define G_RESOURCE_ICONS 0x00F571FCu /* present/ui/messages/window.c: i32 first resource icon */
/* --- end harvested --- */

#endif
