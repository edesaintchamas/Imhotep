/* Imhotep - differential test: terrain rebuild vs. the image grids stored by the original game.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/terrain_check [MAP_NAME ...]     (game files from $IMHOTEP_GAME_DIR, default local/game/app)
 *   build/terrain_check --dump DIR [MAP_NAME ...]   also write DIR/<map>.img (rebuilt u32 grid);
 *                                                   with --dump, no map is skipped
 *   build/terrain_check --campaign [N]              campaign records of mission1.pak (all, or N)
 *
 * Campaign records are saved states: tiles whose images depend on saved buildings, desirability or
 * flood progress differ legitimately, so --campaign passes or fails on cliffs and rocks only (the
 * other categories are printed for information). The version-160 records were written with an
 * expansion.sg3 whose SPR_NEW_CLIFF images start 152 ids later than in the reference files: a
 * cliff tile matches if stored == ours + 152 (docs/formats/campaign.md). Version-147 records use an
 * older image numbering (mostly +539) and are skipped unless requested by number.
 *
 * The map editor stores the image grid it computed with the code that core/map/terrain.c
 * reimplements, so a scenario-start rebuild must reproduce the stored ids tile for tile.
 * Skipped without arguments: maps of file version < 147 (older image numbering) and Gateway to
 * Atlantis (bitfields edited after its images were computed); see docs/formats/map.md §6.
 * Exit status: 0 if every terrain category matches at >= 99.9 %, 1 otherwise, 2 on setup error. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "graphics/image_groups.h"
#include "io/campaign_pak.h"
#include "io/map_file.h"
#include "map/terrain.h"
#include "platform/exe_image.h"
#include "platform/game_files.h"

#define MIN_RATE 99.9

static const struct { uint32_t bit; const char *name; } CATEGORIES[] = {
    {0x4000, "wall"}, {0x100, "canal"}, {0x10000, "floodplain"}, {4, "water"}, {2, "rock"}, {0x2000000, "dune"},
    {1, "tree"}, {0x40, "road"}, {0x40000, "0x40000"}, {0x1000, "rubble"}, {0x800, "meadow"},
    {0x20, "0x20"}, {0x80, "grass"},
};
#define NCAT (sizeof(CATEGORIES) / sizeof(CATEGORIES[0]) + 4) /* + cliff, building, land, other */

static int category(uint32_t v)
{
    if ((v & 0x20000002u) == 0x20000002u) {
        return (int) NCAT - 4; /* cliff */
    }
    if (v & 8) {
        return (int) NCAT - 3; /* building */
    }
    for (size_t i = 0; i < NCAT - 4; i++) {
        if (v & CATEGORIES[i].bit) {
            return (int) i;
        }
    }
    return v == 0 ? (int) NCAT - 2 : (int) NCAT - 1;
}

static const char *category_name(int c)
{
    static const char *EXTRA[4] = {"cliff", "building", "land", "other"};
    return c < (int) NCAT - 4 ? CATEGORIES[c].name : EXTRA[c - ((int) NCAT - 4)];
}

#define CAMPAIGN_CLIFF_SHIFT 152

static game_state initial; /* Pharaoh.exe's data sections */
static game_state gs;

/* the image grid of the state (0x00B7482C), terrain grid (0x00AF598C) */
static uint32_t *image_grid(game_state *g)
{
    return (uint32_t *) state_ptr(g, 0x00B7482Cu, 4u * GRID_SIZE);
}

static uint32_t *terrain_grid(game_state *g)
{
    return (uint32_t *) state_ptr(g, 0x00AF598Cu, 4u * GRID_SIZE);
}

static void fresh_state(void)
{
    memcpy(gs.mem, initial.mem, STATE_SIZE);
    gs.heap_used = 0;
    gs.nblocks = 0;
}

static void tally(const terrain_state *t, const uint32_t *stored, const uint32_t *terrain0, long *same,
                  long *diff, const char *label, int verbose, uint32_t cliff_shift)
{
    const uint32_t *image = image_grid(t->g);
    for (int r = 0; r < S32(t->g, 0x00BCA2D4u); r++) {
        int x = S32(t->g, 0x00BC9F24u + 4u * (uint32_t) r), o = S32(t->g, 0x00BC9804u + 4u * (uint32_t) r);
        for (; x < S32(t->g, 0x00BC9B94u + 4u * (uint32_t) r); x++, o++) {
            int c = category(terrain0[o]);
            int cliff = c == (int) NCAT - 4;
            if (image[o] == stored[o] || (cliff && cliff_shift && stored[o] == image[o] + cliff_shift)) {
                same[c]++;
            } else {
                diff[c]++;
                if (verbose) {
                    printf("  %s (%d,%d) terrain %#x: ours %u stored %u\n", label, x, r, terrain0[o],
                           image[o], stored[o]);
                }
            }
        }
    }
}

static int report(const char *what, int n, const long *same, const long *diff, int gate_only_cliff_rock)
{
    int failed = 0;
    printf("terrain_check: %d %s\n", n, what);
    for (size_t c = 0; c < NCAT; c++) {
        if (same[c] + diff[c] == 0) {
            continue;
        }
        double rate = 100.0 * (double) same[c] / (double) (same[c] + diff[c]);
        int gated = !gate_only_cliff_rock || c == NCAT - 4 || strcmp(category_name((int) c), "rock") == 0;
        failed |= gated && rate < MIN_RATE;
        printf("%-11s %8ld match %6ld differ %7.2f %%%s\n", category_name((int) c), same[c], diff[c], rate,
               gated ? "" : "  (informational)");
    }
    return failed;
}

/* Campaign records of mission1.pak: loaded like a saved game, then the same scenario start
 * (@bin 0x004D58B0 -> 0x004D41E0, 0x004D52A0). */
static int campaign(const char *game, const image_groups *groups, int only)
{
    char path[1200];
    size_t size, start, len;
    snprintf(path, sizeof(path), "%s/mission1.pak", game);
    uint8_t *pak = read_file(path, &size);
    if (!pak) {
        fprintf(stderr, "cannot read %s\n", path);
        return 2;
    }
    terrain_state *t = malloc(sizeof(terrain_state));
    uint32_t *stored = malloc(sizeof(uint32_t) * GRID_SIZE), *terrain0 = malloc(sizeof(uint32_t) * GRID_SIZE);
    long same[NCAT] = {0}, diff[NCAT] = {0};
    int n = 0;
    for (int i = 0; i < CAMPAIGN_PAK_SLOTS; i++) {
        if ((only >= 0 && i != only) || campaign_pak_record(pak, size, i, &start, &len) != 0) {
            continue;
        }
        uint32_t version = 0;
        memcpy(&version, pak + start + 4, 4);
        if (only < 0 && version < 149) {
            continue;
        }
        map_file m;
        fresh_state();
        if (chunk_file_read(pak + start, len, &m) != 0 || terrain_init_campaign(t, &gs, &m, groups) != 0) {
            fprintf(stderr, "campaign record %d: cannot read\n", i);
            return 2;
        }
        map_file_free(&m);
        memcpy(stored, image_grid(&gs), sizeof(uint32_t) * GRID_SIZE);
        memcpy(terrain0, terrain_grid(&gs), sizeof(uint32_t) * GRID_SIZE);
        terrain_scenario_start(t);
        char label[32];
        snprintf(label, sizeof(label), "record %d", i);
        tally(t, stored, terrain0, same, diff, label, only >= 0, version >= 160 ? CAMPAIGN_CLIFF_SHIFT : 0);
        n++;
    }
    free(pak);
    return report("campaign records", n, same, diff, 1);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *) a, *(char *const *) b);
}

int main(int argc, char **argv)
{
    const char *game = game_dir();
    char maps_dir[1024], path[1200];
    snprintf(maps_dir, sizeof(maps_dir), "%s/Maps", game);
    static image_groups groups;
    if (load_image_groups(game, &groups) != 0) {
        fprintf(stderr, "image groups: cannot read the .sg3 headers\n");
        return 2;
    }
    if (state_create(&initial) != 0 || state_create(&gs) != 0 || exe_image_load(game, &initial) != 0) {
        fprintf(stderr, "cannot read %s/Pharaoh.exe\n", game);
        return 2;
    }

    if (argc > 1 && strcmp(argv[1], "--campaign") == 0) {
        return campaign(game, &groups, argc > 2 ? atoi(argv[2]) : -1);
    }

    /* map list */
    const char *dump = NULL;
    int first = 1;
    if (argc > 2 && strcmp(argv[1], "--dump") == 0) {
        dump = argv[2];
        first = 3;
    }
    char *names[256];
    int count = 0;
    int named = argc > first;
    if (named) {
        for (int i = first; i < argc && count < 256; i++) {
            names[count++] = argv[i];
        }
    } else {
        DIR *d = opendir(maps_dir);
        if (!d) {
            fprintf(stderr, "cannot open %s\n", maps_dir);
            return 2;
        }
        struct dirent *e;
        while ((e = readdir(d)) && count < 256) {
            size_t n = strlen(e->d_name);
            if (n > 4 && strcasecmp(e->d_name + n - 4, ".map") == 0) {
                names[count] = malloc(n - 3);
                memcpy(names[count], e->d_name, n - 4);
                names[count][n - 4] = 0;
                count++;
            }
        }
        closedir(d);
        qsort(names, (size_t) count, sizeof(names[0]), cmp_str);
    }

    terrain_state *t = malloc(sizeof(terrain_state));
    uint32_t *stored = malloc(sizeof(uint32_t) * GRID_SIZE), *terrain0 = malloc(sizeof(uint32_t) * GRID_SIZE);
    long same[NCAT] = {0}, diff[NCAT] = {0};
    int maps_checked = 0;
    for (int i = 0; i < count; i++) {
        size_t len;
        snprintf(path, sizeof(path), "%s/%s.map", maps_dir, names[i]);
        uint8_t *data = read_file(path, &len);
        map_file m;
        if (!data || map_file_read(data, len, &m) != 0) {
            fprintf(stderr, "cannot read map %s\n", path);
            return 2;
        }
        free(data);
        if (!named && !dump && (m.version < 147 || strcmp(names[i], "Gateway to Atlantis") == 0)) {
            map_file_free(&m);
            continue;
        }
        fresh_state();
        if (terrain_init(t, &gs, &m, &groups) != 0) {
            fprintf(stderr, "map %s: missing chunks\n", names[i]);
            return 2;
        }
        map_file_free(&m);
        memcpy(stored, image_grid(&gs), sizeof(uint32_t) * GRID_SIZE);
        memcpy(terrain0, terrain_grid(&gs), sizeof(uint32_t) * GRID_SIZE);
        terrain_scenario_start(t);
        if (dump) {
            char out[1400];
            snprintf(out, sizeof(out), "%s/%s.img", dump, names[i]);
            FILE *f = fopen(out, "wb");
            if (!f || fwrite(image_grid(&gs), sizeof(uint32_t), GRID_SIZE, f) != GRID_SIZE) {
                fprintf(stderr, "cannot write %s\n", out);
                return 2;
            }
            fclose(f);
        }
        tally(t, stored, terrain0, same, diff, names[i], named, 0);
        maps_checked++;
    }
    int failed = report("maps", maps_checked, same, diff, 0);
    return failed ? 1 : 0;
}
