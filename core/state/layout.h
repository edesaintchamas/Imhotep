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
#include "state/layout/g_map.h"
#include "state/layout/g_message.h"
#include "state/layout/g_scenario.h"
#include "state/layout/g_sim.h"
#include "state/layout/g_time.h"
#include "state/layout/g_ui.h"

/* names waiting for the lead's merge into their area header, one file per package */
#include "state/layout/pending/p1_figure.h"
#include "state/layout/pending/p2_building.h"
#include "state/layout/pending/p3_city.h"
#include "state/layout/pending/p4_command.h"
#include "state/layout/pending/p5_map.h"
#include "state/layout/pending/p6_scenario.h"
#include "state/layout/pending/p7_frame.h"
#include "state/layout/pending/p8a_ui_main.h"
#include "state/layout/pending/p8b_ui_info.h"
#include "state/layout/pending/p8c_ui_advisors.h"
#include "state/layout/pending/p8d_ui_frontend.h"
#include "state/layout/pending/p9_city_view.h"

#include "state/functions.h"

#endif
