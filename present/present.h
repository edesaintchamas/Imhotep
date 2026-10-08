/* Imhotep - the presentation code: drawing, UI windows, frame (roadmap phase 7, docs/presentation.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ported from the reference Pharaoh.exe like core/, with the same rules (provenance tags,
 * integer arithmetic, no I/O: files, clock, sound and video go through core/sim/platform.h), but
 * it draws: into the back buffer the state points to (@bin 0x00E92E18 the back buffer's address,
 * 0x00E38D94 its pitch in bytes: every blitter writes through them, docs/presentation.md 2.3), checked pixel
 * by pixel against the original (tools/oracle harness / framestep). It depends on core; core does
 * not depend on it. Each module lists its ports in a table (core/state/ported.h), registered by
 * present_register() below (add new tables to present/present.c). */
#ifndef IMHOTEP_PRESENT_PRESENT_H
#define IMHOTEP_PRESENT_PRESENT_H

/* Adds every table of present/ to the port registry (once; before the first sim_call). */
void present_register(void);

#endif
