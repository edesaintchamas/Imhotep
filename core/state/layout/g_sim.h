/* Imhotep - named globals: core/state/layout/g_sim.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P5). */
#ifndef IMHOTEP_STATE_LAYOUT_G_SIM_H
#define IMHOTEP_STATE_LAYOUT_G_SIM_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_CHUNKS 0x005E09A0u /* core/frame/autosave.c: 181 records of 20 bytes: compressed, address, size, count, - */
#define G_RECORDS_HIGHEST 0x0078D788u /* core/figure/helpers_xa.c */
#define G_TICK 0x0078D7CCu /* core/sim/tick.c: tick within the day, 0..50 (chunk 34) */
#define G_STORAGE 0x007900B8u /* core/io/save_load.c: chunk 113, 200 x 196 */
#define G_REC4A 0x009783A4u /* core/state/post_load.c */
#define G_ROW_OFFSET 0x00BC9804u /* core/command/rotate.c: i32 per row: first grid offset */
#define G_ROW_END 0x00BC9B94u /* core/command/rotate.c: i32 per row */
#define G_ROW_FIRST 0x00BC9F24u /* core/state/scenario_start.c: per map row: first column, last column + 1, grid offset */
#define G_MAP_XE 0x00BCA2F0u /* core/figure/actions_c2.c: last x */
#define G_SOUND_EFFECTS_ON 0x00E38E63u /* core/sim/platform.c: sound effects option */
#define G_EFFECTS_VOLUME 0x00E38EC4u /* present/ui/frontend/options.c: i32 */
#define G_REQUEST 0x00E92DA8u /* core/sim/platform.c: the sample request block: owner, sound, volume, pan, loops */
#define G_CITY_PLAYER_NAME 0x00EA184Cu /* core/newgame/start.c */
#define G_SIDEBAR_X_COLLAPSED 0x00F53F60u /* present/sidebar/sidebar.h: i32 x of the collapsed sidebar */
#define G_SIDEBAR_X 0x00F53F64u /* present/sidebar/sidebar.h: i32 x of the expanded sidebar (0x0051E930) */
#define G_SIDEBAR_COLLAPSED 0x00F53F78u /* present/sidebar/sidebar.h: i32 0 expanded, 1 collapsed */
#define G_IMGS_H 0x010D6A0Eu /* present/ui/messages/richtext.c */
/* --- end harvested --- */

#endif
