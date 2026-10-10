/* Imhotep - pending names of package p3 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P3_CITY_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P3_CITY_H

/* record fields other packages own (building.h) */
#define BLD_MONUMENT_STAGES 0x54u /* @bin 0x00401540 i32: the monuments of 0x00401540: the stages done, 56 at the end (BLD_MONUMENT_KIND's offset) */
#define BLD_MONUMENT_WORK 0x60u /* @bin 0x00546790 u32: a part's work, summed over the parts of a part-built monument (BLD_MON_ANNOUNCED_60's offset) */
/* record fields other packages own (figure.h) */
#define FIG_WORKSITE 0x68u /* @bin 0x004F26A0 i16: a figure of type 0x55 at work: the building worked on (FIG_LEADER's offset) */
#define FIG_BUILDINGS_5C 0x5Cu /* @bin 0x00452200 i16[2]: a figure of type 0x55 at work (actions 0xF, 0x10, 0x24): two buildings (FIG_TARGET_HOUSE's offset) */

/* fields of tables other packages own */
#define MONMODEL_U0C 0x0Cu /* @bin 0x004F78A0 i16: a monument's weight (G_MONUMENT_MODELS entries; g_building.h) */
#define G_FIGURE_TYPE_STRIDE 0x38u /* @bin 0x00446A30 the stride of G_FIGURE_TYPES (g_figure.h) */
#define FIGTYPE_U04 0x04u /* @bin 0x00446A30 i32 of a G_FIGURE_TYPES entry (core/city/events_troops.c; g_figure.h) */
#define FIGTYPE_U1C 0x1Cu /* @bin 0x00446A30 i32 */
#define FIGTYPE_U20 0x20u /* @bin 0x00446EF9 i32 */
#define FIGTYPE_U24 0x24u /* @bin 0x00446A30 i32 */
#define FIGTYPE_U2C 0x2Cu /* @bin 0x00446A30 i32 */
#define FIGTYPE_U30 0x30u /* @bin 0x00446A30 i32 */

/* globals used most by other packages (the area of each: [header]) */
#define G_HOUSE_LEVEL_U44 0x0070CE10u /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_HOUSE_LEVEL_U50 0x0070CE1Cu /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_HOUSE_LEVEL_U54 0x0070CE20u /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_ENTRY_POINT_X 0x00784C64u /* @bin 0x004DE8A9 i16: entry point (core/newgame/setup.c) [g_scenario.h] */
#define G_ENTRY_POINT_Y 0x00784C66u /* @bin 0x004DE8A9 i16 [g_scenario.h] */
#define G_EXIT_POINT_X 0x00784C68u /* @bin 0x004DE8A9 i16: exit point [g_scenario.h] */
#define G_EXIT_POINT_Y 0x00784C6Au /* @bin 0x004DE8A9 i16 [g_scenario.h] */
#define G_U784DC4 0x00784DC4u /* @bin 0x004AEAB0 three points (core/figure/actions_d4.c) [g_figure.h] */
#define G_U784DD0 0x00784DD0u /* @bin 0x004AEAB0 the end of G_U784DC4 [g_figure.h] */
#define G_HAIL_TIMER 0x0078D7A0u /* @bin 0x004E6EB0 i32: := 0x198 with the hail; the screen shakes while positive (core/frame/frame.c) [g_scenario.h] */
#define G_DEADLINE_YEAR 0x0078D7B8u /* @bin 0x004DE820 i32: deadline year (save.md §3 chunk 76) [g_scenario.h] */
#define G_TILE_VISITS 0x00978D9Cu /* @bin 0x0047E250 u32 per tile: walker tile-visit counters (save.md §3 chunk 148) [g_figure.h] */
#define G_FLOOD_STATE 0x00D386D4u /* @bin 0x0048BCAE i32: a mode 1..3 (flood state 1, core/building/building_extra.c) [g_figure.h] */
#define G_UF5736C 0x00F5736Cu /* @bin 0x004ED360 u32: a construction image (core/building/monument_obj.c) [g_building.h] */

#endif
