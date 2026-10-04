/* Imhotep - how the tick calls the game functions it is made of.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/call.h"

#include "state/ported.h"

/* @bin 0x004D7090, 0x00451C50, 0x00452370 the calls of the tick skeleton (docs/formats/tick.md),
 * each to the port of its target */
int32_t sim_call(game_state *st, const sim_env *env, uint32_t address, const int32_t *args, int nargs,
                 uint32_t this_ptr)
{
    int32_t result = 0;
    if (env && env->external && env->external(env, st, address, args, nargs, this_ptr, &result)) {
        return result;
    }
    const ported_function *p = ported_find(address);
    if (!p) {
        state_unported(st, address);
        return 0;
    }
    int32_t a[16] = {0};
    for (int i = 0; i < nargs && i < 16; i++) {
        a[i] = args[i];
    }
    return p->run(st, a);
}
