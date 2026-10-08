/* Imhotep - run ported game functions on a state file (the port's side of tools/oracle/harness.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_run IN OUT ADDR[:ARG,ARG...] ...    run the ports of these functions in order;
 *                                                   prints each one's return value
 *   build/state_run --list                          list the ported functions
 *
 * IN and OUT are state files (core/state/state.h: data segments, then the heap). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "present/present.h"
#include "state/ported.h"
#include "state/state.h"

int main(int argc, char **argv)
{
    present_register(); /* the presentation code's ports (present/) */
    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        int dup = 0;
        for (int i = 0; i < ported_count(); i++) {
            for (int k = 0; k < i; k++) {
                if (ported_at(k)->address == ported_at(i)->address) {
                    fprintf(stderr, "0x%08X is registered twice (%s, %s)\n", ported_at(i)->address,
                            ported_at(k)->name, ported_at(i)->name);
                    dup = 1;
                }
            }
        }
        if (dup) {
            return 1;
        }
        for (int i = 0; i < ported_count(); i++) {
            const ported_function *p = ported_at(i);
            printf("0x%08X %d %d 0x%08X %d %s\n", p->address, p->nargs, p->ret_bits, p->this_ptr, p->partial,
                   p->name);
        }
        return 0;
    }
    if (argc < 4) {
        fprintf(stderr, "usage: state_run IN OUT ADDR[:ARGS] ... | --list\n");
        return 2;
    }
    game_state st;
    FILE *f = fopen(argv[1], "rb");
    if (state_create(&st) != 0 || !f || state_read_file(&st, f) != 0) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);
    for (int i = 3; i < argc; i++) {
        uint32_t addr = (uint32_t) strtoul(argv[i], NULL, 16);
        const ported_function *p = ported_find(addr);
        if (!p || !p->run) {
            fprintf(stderr, "0x%08X is not ported\n", addr);
            return 3;
        }
        int32_t args[16] = {0};
        const char *c = strchr(argv[i], ':');
        for (int k = 0; c && k < 16; k++) {
            args[k] = (int32_t) strtol(c + 1, NULL, 0);
            c = strchr(c + 1, ',');
        }
        printf("%d\n", p->run(&st, args));
    }
    if (st.unported) {
        fprintf(stderr, "unported original function reached: 0x%08X\n", st.unported);
        return 3;
    }
    f = fopen(argv[2], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 2;
    }
    return 0;
}
