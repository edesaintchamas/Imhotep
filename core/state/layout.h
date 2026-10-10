/* Imhotep - the state's layout by name: record fields and named globals (docs/naming.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every name is defined once, in the header of its area, with its evidence (@bin address or doc).
 * Original functions by name: state/functions.h (generated). */
#ifndef IMHOTEP_STATE_LAYOUT_H
#define IMHOTEP_STATE_LAYOUT_H

#include "state/records.h"

/* records */
#include "state/layout/building.h"
#include "state/layout/city.h"
#include "state/layout/figure.h"
#include "state/layout/message.h"

/* globals, one header per area (owner package in docs/naming.md) */
#include "state/layout/g_boot.h"
#include "state/layout/g_building.h"
#include "state/layout/g_city.h"
#include "state/layout/g_command.h"
#include "state/layout/g_figure.h"
#include "state/layout/g_frame.h"
#include "state/layout/g_graphics.h"
#include "state/layout/g_map.h"
#include "state/layout/g_message.h"
#include "state/layout/g_scenario.h"
#include "state/layout/g_sim.h"
#include "state/layout/g_time.h"
#include "state/layout/g_ui.h"

/* names waiting for a merge into their area header (docs/naming.md "Pending names"): one
 * #include "state/layout/pending/<package>.h" per fragment, none now */

#include "state/functions.h"

#endif
