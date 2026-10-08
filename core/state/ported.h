/* Imhotep - ported game functions by their address in Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The function oracle (tools/oracle/harness.py) runs an original function and its port on the
 * same state and compares every byte; this table is how it finds the port. Every function here
 * takes the state and the original's stack arguments (cdecl order; unused for argument-less
 * functions) and returns what the original returns in eax (0 for void functions). */
#ifndef IMHOTEP_STATE_PORTED_H
#define IMHOTEP_STATE_PORTED_H

#include <stdint.h>

#include "state/state.h"

/* Wrappers get the stack arguments in args[0..14] and the call's `this` (ecx) in
 * args[PORT_THIS_ARG], for thiscall functions on objects that vary (event records, ...). */
#define PORT_THIS_ARG 15
#define PORT_PARTIAL 1
#define PORT_PLATFORM 2

typedef struct {
    uint32_t address; /* @bin the original function's entry point */
    const char *name;
    int nargs;
    int ret_bits; /* significant bits of the original's return value: 0 (void), 8, 16 or 32 */
    uint32_t this_ptr; /* thiscall: the object passed in ecx (0: none) */
    int32_t (*run)(game_state *st, const int32_t *args);
    int partial; /* 1: ported for some arguments or paths only (the rest is state_unported);
                  * 2: a platform request (core/sim/platform.h): the original's run writes only
                  * presentation state, which the port does not reproduce */
} ported_function;

/* Each module of core/ lists its ports in its own table, registered in ported.c (ported_modules):
 *   const ported_function figure_ports[] = {{0x00489B70u, "figure_update_all", 0, 0, 0, run_..., 0}, ...};
 *   const int figure_ports_count = sizeof(figure_ports) / sizeof(figure_ports[0]);
 * The run wrappers adapt (state, stack arguments) to the port's own signature. */
#define PORTED_TABLE(name) \
    extern const ported_function name[]; \
    extern const int name##_count;

/* Adds a table of ports from outside core (the presentation code, present/: present_register()),
 * so that sim_call dispatches to them like core's own; 0, or -1 when too many tables. */
int ported_add_table(const ported_function *table, const int *count);

/* The port of the original function at `address`, or NULL. */
const ported_function *ported_find(uint32_t address);

/* All ports, in no particular order: ported_count() entries. */
int ported_count(void);
const ported_function *ported_at(int i);

#endif
