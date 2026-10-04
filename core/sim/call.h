/* Imhotep - how the tick calls the game functions it is made of.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every call the tick skeleton makes (core/sim/tick.c) goes through sim_call with the original
 * function's address, its stack arguments and its `this` pointer. A ported function
 * (core/state/ported.c) runs natively. A test harness can take any call over first: the lockstep
 * runner (tools/oracle/lockstep.py) replays what the original did in that call, so the skeleton
 * runs whole ticks while its callees are being ported. A call nobody handles is recorded as
 * unported in the state (state_unported). */
#ifndef IMHOTEP_SIM_CALL_H
#define IMHOTEP_SIM_CALL_H

#include <stdint.h>

#include "state/state.h"

typedef struct sim_env sim_env;
struct sim_env {
    /* Takes the call over when it returns 1 (*result: the original's eax); 0: run the port. */
    int (*external)(const sim_env *env, game_state *st, uint32_t address, const int32_t *args, int nargs,
                    uint32_t this_ptr, int32_t *result);
    void *ctx;
};

/* @bin the original function at `address`: call its port (or the environment's replacement).
 * Call the original function at `address` (its port, or the environment). Returns its eax. */
int32_t sim_call(game_state *st, const sim_env *env, uint32_t address, const int32_t *args, int nargs,
                 uint32_t this_ptr);

#define SIM_CALL0(st, env, a) sim_call(st, env, a, NULL, 0, 0)
#define SIM_CALLT(st, env, a, t) sim_call(st, env, a, NULL, 0, t)

#endif
