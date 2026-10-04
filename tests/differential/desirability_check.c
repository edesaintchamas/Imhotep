/* Imhotep - differential test: desirability grid vs. the grids stored by the original game.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/desirability_check [-v] [-w DIR] [FILE.sav ...]   (default: every record of mission1.pak)
 *   -w DIR writes each recomputed grid to DIR/<n>.des (n = record or argument index)
 *
 * The game recomputes the whole desirability grid (chunk 16) at tick 37 of every day
 * (0x0046A0C0 + 0x0046A140) from the buildings, terrain, bitfields and the building model of the
 * save's difficulty. A save therefore holds the grid of the last tick 37 together with the state it
 * was computed from, unless buildings or tiles changed in the ticks since. This test recomputes the
 * grid from the stored state and compares every tile.
 * Exit status: 0 if at least 99.9 % of the tiles match, 1 otherwise, 2 on setup error. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "city/desirability.h"
#include "io/campaign_pak.h"
#include "io/save_file.h"
#include "model/building_model.h"
#include "platform/exe_image.h"
#include "platform/game_files.h"
#include "state/scenario_start.h"
#include "state/state_save.h"

#define GRID (DESIRABILITY_GRID_SIDE * DESIRABILITY_GRID_SIDE)
#define MIN_RATE 99.9

static building_model models[5];
static int model_loaded[5];
static long tiles, same, saves, saves_exact, stale;
static int verbose;
static const char *write_dir;
static int written; /* name of the grid file: record number, or argument index */

static const building_model *model_for(int difficulty)
{
    if (difficulty < 0 || difficulty > 4) {
        return NULL;
    }
    if (!model_loaded[difficulty]) {
        char path[1300];
        size_t len;
        snprintf(path, sizeof(path), "%s/%s", game_dir(), building_model_files[difficulty]);
        uint8_t *text = read_file(path, &len);
        if (!text || building_model_parse(text, len, &models[difficulty]) != 0) {
            fprintf(stderr, "cannot read %s\n", path);
            free(text);
            return NULL;
        }
        free(text);
        model_loaded[difficulty] = 1;
    }
    return &models[difficulty];
}

static game_state initial; /* Pharaoh.exe's data sections */

/* the state the game holds after loading `s`, as far as ported (chunks, model, map globals) */
static int prepare(game_state *st, const save_file *s, const building_model *m)
{
    memcpy(st->mem, initial.mem, STATE_SIZE);
    state_load_chunks(st, s);
    building_model_store(m, st);
    scenario_start_map(st);
    return 0;
}

static int check(const uint8_t *data, size_t len, const char *label)
{
    save_file s;
    if (save_file_read(data, len, &s) != SAVE_OK) {
        fprintf(stderr, "%s: cannot read\n", label);
        return 2;
    }
    const building_model *m = model_for((int) save_u32(s.chunk[SAVE_CHUNK_DIFFICULTY]));
    if (!m) {
        save_file_free(&s);
        return 2;
    }
    static game_state st;
    if (!st.mem && state_create(&st) != 0) {
        return 2;
    }
    prepare(&st, &s, m);
    desirability_update(&st);
    const int8_t *grid = (const int8_t *) state_ptr(&st, DESIRABILITY_GRID, GRID);
    if (write_dir) {
        char path[1300];
        snprintf(path, sizeof(path), "%s/%d.des", write_dir, written);
        FILE *f = fopen(path, "wb");
        if (!f || fwrite(grid, 1, GRID, f) != GRID || fclose(f) != 0) {
            fprintf(stderr, "cannot write %s\n", path);
            return 2;
        }
    }
    const int8_t *stored = (const int8_t *) s.chunk[16];
    long n = 0, ok = 0;
    for (int o = 0; o < GRID; o++) {
        n++;
        if (grid[o] == stored[o]) {
            ok++;
        } else if (verbose) {
            printf("  %s: tile (%d,%d) stored %d ours %d\n", label, o % DESIRABILITY_GRID_SIDE, o / DESIRABILITY_GRID_SIDE,
                   stored[o], grid[o]);
        }
    }
    tiles += n;
    same += ok;
    saves++;
    saves_exact += ok == n;
    int32_t max_id = S32(&st, MAX_BUILDING_ID);
    if (ok != n) {
        /* explanation: the stored grid predates the buildings above some id (placed after the last
         * tick 37); find the largest prefix of building ids that reproduces it exactly */
        int found = -1;
        for (int k = max_id - 1; k >= 0 && found < 0; k--) {
            prepare(&st, &s, m);
            desirability_buildings_upto(&st, k);
            desirability_tiles(&st);
            if (memcmp(grid, stored, GRID) == 0) {
                found = k;
            }
        }
        if (found >= 0) {
            stale++;
            printf("  %s: stored grid = buildings 1..%d (later ids placed after the last update)\n", label, found);
        } else {
            printf("  %s: no building-id prefix explains the difference\n", label);
        }
    }
    printf("  %s: difficulty %u, tick %u, buildings up to %d: %ld / %ld tiles\n", label,
           save_u32(s.chunk[SAVE_CHUNK_DIFFICULTY]), save_u32(s.chunk[SAVE_CHUNK_TICK]), max_id, ok, n);
    save_file_free(&s);
    return 0;
}

int main(int argc, char **argv)
{
    int first = 1;
    for (; first < argc && argv[first][0] == '-'; first++) {
        if (strcmp(argv[first], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[first], "-w") == 0 && first + 1 < argc) {
            write_dir = argv[++first];
        } else {
            fprintf(stderr, "usage: desirability_check [-v] [-w DIR] [FILE.sav ...]\n");
            return 2;
        }
    }
    if (state_create(&initial) != 0 || exe_image_load(game_dir(), &initial) != 0) {
        fprintf(stderr, "cannot read %s/Pharaoh.exe\n", game_dir());
        return 2;
    }
    if (argc > first) {
        for (int i = first; i < argc; i++) {
            size_t len;
            uint8_t *data = read_file(argv[i], &len);
            written = i - first;
            if (!data || check(data, len, argv[i]) != 0) {
                return 2;
            }
            free(data);
        }
    } else {
        char path[1200];
        size_t size, start, len;
        snprintf(path, sizeof(path), "%s/mission1.pak", game_dir());
        uint8_t *pak = read_file(path, &size);
        if (!pak) {
            fprintf(stderr, "cannot read %s\n", path);
            return 2;
        }
        for (int i = 0; i < CAMPAIGN_PAK_SLOTS; i++) {
            if (campaign_pak_record(pak, size, i, &start, &len) != 0) {
                continue;
            }
            char label[32];
            snprintf(label, sizeof(label), "record %d", i);
            written = i;
            if (check(pak + start, len, label) != 0) {
                return 2;
            }
        }
        free(pak);
    }
    double rate = tiles ? 100.0 * (double) same / (double) tiles : 0;
    printf("desirability_check: %ld saves (%ld exact, %ld explained by buildings placed after the last update), "
           "%ld / %ld tiles equal (%.3f %%)\n", saves, saves_exact, stale, same, tiles, rate);
    return saves_exact + stale == saves ? 0 : 1;
}
