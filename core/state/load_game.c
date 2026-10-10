/* Imhotep - load a saved game into the game state (Pharaoh.exe 0x004D5100), standalone.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/load_game.h"

#include <string.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/loader.h"
#include "state/ported.h"
#include "state/pre_load.h"

/* game_env's file callback as the platform's file reads (core/sim/platform.h) */
static int32_t env_file_read(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    const game_env *env = ctx;
    /* the game's paths use '\\' (e.g. "data\\Pyramid.sg3"); the callback takes '/' */
    char name[260];
    size_t k = 0;
    for (; path[k] && k + 1 < sizeof(name); k++) {
        name[k] = path[k] == '\\' ? '/' : path[k];
    }
    name[k] = 0;
    size_t len;
    const uint8_t *data = env->file ? env->file(env->file_ctx, name, &len) : NULL;
    if (!data) {
        return -1;
    }
    if (buf && size && offset < len) {
        size_t n = len - offset < size ? len - offset : size;
        memcpy(buf, data + offset, n);
    }
    return (int32_t) len;
}

/* Without a test environment, a call to an original function that has no port leaves the state as
 * it is (the steps not ported yet; oracle_state_check.py lists what they write in the original). */
static int skip_unported(const sim_env *env, game_state *st, uint32_t address, const int32_t *args, int nargs,
                         uint32_t this_ptr, int32_t *result)
{
    (void) env;
    (void) st;
    (void) args;
    (void) nargs;
    (void) this_ptr;
    const ported_function *p = ported_find(address);
    if (p && p->run) {
        return 0;
    }
    *result = 0;
    return 1;
}

int game_load_save(game_state *st, const uint8_t *data, size_t len, const game_env *env, save_load_stats *stats)
{
    const sim_platform *platform = st->platform;
    const sim_env *call_env = st->env;
    sim_platform files = {0};
    sim_env skip = {skip_unported, NULL};
    if (!platform) {
        files.ctx = (void *) env;
        files.file_read = env_file_read;
        st->platform = &files;
    }
    if (!call_env) {
        st->env = &skip;
    }
    /* @bin 0x004D5100 before the file opens: 0x004BD330 (floodplain settings, this = 0x00D38360),
     * 0x0046FA40 (grids cleared), 0x004CE2D0 (working directory), 0x00D3B1B8 := 0 (byte) */
    floodplain_settings_reset(st);
    pre_load_clear(st);
    GAME_CALL0(st, FN_WORKING_DIRECTORY);
    U8(st, G_SAVE_LOADED) = 0;
    int rc = loader_load_save_bytes(st, data, len, 0, env->image_size, env->image_ctx, stats);
    st->platform = platform;
    st->env = call_env;
    return rc;
}
