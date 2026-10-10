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

#endif
