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

/* @bin the original function at `address`, called with its stack arguments and `this`:
 * call its port (or the environment). Returns its eax.
 * env NULL: the state's own environment (st->env). */
int32_t sim_call(game_state *st, const sim_env *env, uint32_t address, const int32_t *args, int nargs,
                 uint32_t this_ptr);

#define SIM_CALL0(st, env, a) sim_call(st, env, a, NULL, 0, 0)
#define SIM_CALLT(st, env, a, t) sim_call(st, env, a, NULL, 0, t)

/* A port calls an original function that is not ported yet (or is reached through a pointer, such
 * as the figure action table): GAME_CALL(st, address, args...) for cdecl / stdcall,
 * GAME_CALLT(st, this, address, args...) for thiscall. Ported functions call each other directly. */
#define GAME_CALL0(st, a) sim_call(st, NULL, a, NULL, 0, 0)
#define GAME_CALL(st, a, ...) \
    sim_call(st, NULL, a, (const int32_t[]){__VA_ARGS__}, \
             (int) (sizeof((const int32_t[]){__VA_ARGS__}) / sizeof(int32_t)), 0)
#define GAME_CALLT0(st, t, a) sim_call(st, NULL, a, NULL, 0, t)
#define GAME_CALLT(st, t, a, ...) \
    sim_call(st, NULL, a, (const int32_t[]){__VA_ARGS__}, \
             (int) (sizeof((const int32_t[]){__VA_ARGS__}) / sizeof(int32_t)), t)

#endif
