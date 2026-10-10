/* Imhotep - named globals: game time.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_TIME_H
#define IMHOTEP_STATE_LAYOUT_G_TIME_H

/* @bin 0x00451C50 -> 0x00452370 the day cascade (save.md §5 "Game time"; chunks 35-38) */
#define G_MONTH 0x0078D7C0u /* i32: month of the year, 0..11 (chunk 36) */
#define G_YEAR 0x0078D7BCu  /* i32 (chunk 37) */
#define G_TICK_DAY 0x0078D7C4u /* @bin 0x00452370 i32: day within the month, 0..15 (chunk 35) */
#define G_TOTAL_DAYS 0x0078D7C8u /* @bin 0x00452370 i32: total days elapsed (chunk 38) */

#endif
