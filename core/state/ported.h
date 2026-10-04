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

typedef struct {
    uint32_t address; /* @bin the original function's entry point */
    const char *name;
    int nargs;
    int ret_bits; /* significant bits of the original's return value: 0 (void), 8, 16 or 32 */
    uint32_t this_ptr; /* thiscall: the object passed in ecx (0: none) */
    int32_t (*run)(game_state *st, const int32_t *args);
} ported_function;

extern const ported_function ported_functions[];
extern const int ported_function_count;

const ported_function *ported_find(uint32_t address);

#endif
