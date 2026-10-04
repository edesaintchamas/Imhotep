/* Imhotep - write a saved game from a state file (the port's side of
 * tests/differential/oracle_save_check.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_save IN OUT.sav     IN: a state file (core/state/state.h) */
#include <stdio.h>
#include <stdlib.h>

#include "state/save_game.h"
#include "state/state.h"

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: state_save IN OUT.sav\n");
        return 2;
    }
    game_state st;
    FILE *f = fopen(argv[1], "rb");
    if (state_create(&st) != 0 || !f || state_read_file(&st, f) != 0) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);
    uint8_t *data;
    size_t len;
    if (game_save(&st, &data, &len) != 0) {
        fprintf(stderr, "game_save failed\n");
        return 1;
    }
    f = fopen(argv[2], "wb");
    if (!f || fwrite(data, 1, len, f) != len || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 2;
    }
    free(data);
    return 0;
}
