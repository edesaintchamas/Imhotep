/* Imhotep - our load of a saved game on a state file (the port's side of
 * tests/differential/oracle_state_check.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_load IN SAVE OUT     IN, OUT: state files (core/state/state.h)
 *
 * Runs game_load_save (core/state/load_game.c) with the game files of $IMHOTEP_GAME_DIR. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/game_files.h"
#include "state/load_game.h"

static const uint8_t *game_file(void *ctx, const char *name, size_t *len)
{
    static uint8_t *cache[16];
    static size_t sizes[16];
    static char names[16][64];
    (void) ctx;
    for (int i = 0; i < 16; i++) {
        if (cache[i] && strcmp(names[i], name) == 0) {
            *len = sizes[i];
            return cache[i];
        }
    }
    for (int i = 0; i < 16; i++) {
        if (!cache[i]) {
            char path[1300];
            snprintf(path, sizeof(path), "%s/%s", game_dir(), name);
            cache[i] = read_file(path, &sizes[i]);
            snprintf(names[i], sizeof(names[i]), "%s", name);
            *len = sizes[i];
            return cache[i];
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: state_load IN SAVE OUT\n");
        return 2;
    }
    game_state st;
    FILE *f = fopen(argv[1], "rb");
    if (state_create(&st) != 0 || !f || state_read_file(&st, f) != 0) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);
    size_t len;
    uint8_t *data = read_file(argv[2], &len);
    static image_groups groups;
    static image_sizes sizes;
    if (!data || load_image_groups(game_dir(), &groups) != 0 || load_image_sizes(game_dir(), &groups, &sizes) != 0) {
        fprintf(stderr, "cannot read %s or the .sg3 files\n", argv[2]);
        return 2;
    }
    game_env env = {game_file, NULL, image_sizes_lookup, &sizes};
    save_load_stats stats;
    if (game_load_save(&st, data, len, &env, &stats) != 0) {
        fprintf(stderr, "game_load_save failed\n");
        return 1;
    }
    if (st.unported) {
        fprintf(stderr, "unported original function reached: 0x%08X\n", st.unported);
        return 3;
    }
    f = fopen(argv[3], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[3]);
        return 2;
    }
    return 0;
}
