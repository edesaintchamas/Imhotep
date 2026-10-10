/* Imhotep - named globals: game time.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_TIME_H
#define IMHOTEP_STATE_LAYOUT_G_TIME_H

/* @bin 0x00451C50 -> 0x00452370 the day cascade (save.md §5 "Game time"; chunks 36, 37) */
#define G_MONTH 0x0078D7C0u /* i32: month of the year, 0..11 (chunk 36) */
#define G_YEAR 0x0078D7BCu  /* i32 (chunk 37) */

#endif
