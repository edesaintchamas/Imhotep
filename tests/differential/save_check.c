/* Imhotep - save reader check against the saved games written by the original game.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/save_check [FILE.sav ...]     (default: every record of $IMHOTEP_GAME_DIR/mission1.pak)
 *
 * For each saved game (mission1.pak records are saved games, docs/formats/campaign.md):
 *  1. core/io/save_file.c reads it into the version-160 layout;
 *  2. every chunk equals the file's chunk (same layout) or its migration (larger records: the
 *     file's element followed by zeros, @bin 0x004D0E10);
 *  3. the cross-references the game maintains hold in the result (docs/formats/save.md §5):
 *     building +0x0C = start + 228·y + x and the building grid holds the id there; every live
 *     figure's +0x20 = start + 228·y + x; every figure in a tile's chain (+0x08) is alive, has that
 *     tile's offset and is in no other chain (live figures may be off the grid); the city
 *     data entry/exit grid offsets match their x, y;
 *  4. the loader's later steps (core/io/save_load.c) on the version-160 records: the empire
 *     route lengths and endpoints (0x004D4720) and the figure name groups (0x004D32F0) are
 *     recomputed on every load, so a game-written save holds the values its own load produced;
 *     the recomputation must give them back. The campaign records were written from a fresh
 *     scenario (never loaded), so figures of the unnamed types still hold 0 instead of the -2 a
 *     load writes; they are counted apart. Older records are loaded (upgrades) and reported.
 * Exit status: 0 if every check passes, 1 otherwise, 2 on setup error. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/campaign_pak.h"
#include "io/map_file.h"
#include "io/save_file.h"
#include "io/save_load.h"
#include "platform/game_files.h"

#define GRID_SIDE 228

static long checks, failures, off_grid;
static image_sizes sizes;
static long load_compared[3], load_same[3]; /* route length, endpoints, figure name group */
static long load_fresh_default;

static void expect(int ok, const char *label, const char *what, int index)
{
    checks++;
    if (!ok) {
        failures++;
        if (failures <= 20) {
            printf("  %s: %s %d\n", label, what, index);
        }
    }
}

/* 2. chunk bytes against the file's own chunks */
static void check_chunks(const save_file *s, const uint8_t *data, size_t len, const char *label)
{
    map_file m;
    if (chunk_file_read(data, len, &m) != 0) {
        expect(0, label, "container", 0);
        return;
    }
    for (uint32_t i = 0; i < s->read_chunks; i++) {
        const save_schema_record *t = &save_schema[i];
        const uint8_t *rec = data + 12 + 20 * i;
        uint32_t fsize = save_u32(rec + 8), fcount = save_u32(rec + 12);
        if (i == 6 || (fsize == t->elem_size && fcount == t->elem_count)) {
            expect(memcmp(s->chunk[i], m.chunk[i], m.size[i]) == 0, label, "chunk differs", (int) i);
            continue;
        }
        int ok = fsize <= t->elem_size; /* the corpus only has growing records */
        for (uint32_t k = 0; ok && k < t->elem_count; k++) {
            const uint8_t *e = s->chunk[i] + (size_t) k * t->elem_size;
            uint32_t keep = k < fcount ? fsize : 0;
            ok = keep == 0 || memcmp(e, m.chunk[i] + (size_t) k * fsize, keep) == 0;
            for (uint32_t b = keep; ok && b < t->elem_size; b++) {
                ok = e[b] == 0;
            }
        }
        expect(ok, label, "migrated chunk differs", (int) i);
    }
    map_file_free(&m);
}

/* 3. cross-references */
static void check_relations(const save_file *s, const char *label, long *buildings, long *figures)
{
    const uint8_t *info = s->chunk[SAVE_CHUNK_SCENARIO_INFO];
    int32_t start = (int32_t) save_u32(info + 52);
    const uint8_t *bgrid = s->chunk[SAVE_CHUNK_BUILDING_GRID], *fgrid = s->chunk[SAVE_CHUNK_FIGURE_GRID];
    for (int id = 1; id < SAVE_BUILDING_COUNT; id++) {
        const uint8_t *b = s->chunk[SAVE_CHUNK_BUILDINGS] + (size_t) id * SAVE_BUILDING_SIZE;
        if (b[0] != 1) {
            continue;
        }
        (*buildings)++;
        int32_t off = (int32_t) save_u32(b + 0x0C);
        expect(off == start + GRID_SIDE * save_u16(b + 8) + save_u16(b + 6), label, "building offset", id);
        if (b[3] > 0 && off >= 0 && off < GRID_SIDE * GRID_SIDE) {
            expect(save_u16(bgrid + 2 * off) == id, label, "building grid", id);
        }
    }
    long figures_before = *figures;
    for (int id = 1; id < SAVE_FIGURE_COUNT; id++) {
        const uint8_t *f = s->chunk[SAVE_CHUNK_FIGURES] + (size_t) id * SAVE_FIGURE_SIZE;
        if (f[0x0E] != 1) {
            continue;
        }
        (*figures)++;
        int32_t off = (int32_t) save_u32(f + 0x20);
        expect(off == start + GRID_SIDE * save_u16(f + 0x16) + save_u16(f + 0x14), label, "figure offset", id);
    }
    /* every figure in a tile's chain is alive, has that tile's grid offset, and is in one chain
     * only; live figures may be off the grid (caravans and walkers in some action states) */
    static uint8_t seen[SAVE_FIGURE_COUNT];
    memset(seen, 0, sizeof(seen));
    long on_grid = 0;
    for (int tile = 0; tile < GRID_SIDE * GRID_SIDE; tile++) {
        for (int n = save_u16(fgrid + 2 * tile), steps = 0; n && steps < SAVE_FIGURE_COUNT; steps++) {
            const uint8_t *f = s->chunk[SAVE_CHUNK_FIGURES] + (size_t) n * SAVE_FIGURE_SIZE;
            expect(n < SAVE_FIGURE_COUNT && !seen[n] && f[0x0E] == 1 && (int32_t) save_u32(f + 0x20) == tile, label,
                   "figure chain entry", n);
            if (n >= SAVE_FIGURE_COUNT || seen[n]) {
                break;
            }
            seen[n] = 1;
            on_grid++;
            n = save_u16(f + 0x08);
        }
    }
    off_grid += *figures - figures_before - on_grid;
    {
    }
    const uint8_t *city = s->chunk[SAVE_CHUNK_CITY_DATA];
    for (int p = 0; p < 2; p++) {
        const uint8_t *c = city + (size_t) p * SAVE_CITY_DATA_SIZE;
        if (save_u32(c + 0x277C) == 0) {
            continue; /* player record not set up */
        }
        expect((int32_t) save_u32(c + 0x277C) == start + GRID_SIDE * save_u16(c + 0x277A) + save_u16(c + 0x2778),
               label, "entry point offset, player", p);
        expect((int32_t) save_u32(c + 0x2784) == start + GRID_SIDE * save_u16(c + 0x2782) + save_u16(c + 0x2780),
               label, "exit point offset, player", p);
    }
}

static int check_one(const uint8_t *data, size_t len, const char *label, long *b, long *f)
{
    save_file s;
    int rc = save_file_read(data, len, &s);
    if (rc != SAVE_OK) {
        printf("  %s: save_file_read error %d\n", label, rc);
        failures++;
        return 0;
    }
    check_chunks(&s, data, len, label);
    check_relations(&s, label, b, f);
    save_file loaded;
    save_file_read(data, len, &loaded);
    save_load_env env = {0x12345678u, 0, image_sizes_lookup, &sizes, sizes.count};
    save_load_stats st;
    save_file_load(&loaded, &env, &st);
    if (s.version == SAVE_VERSION) {
        for (int k = 0; k < 50; k++) {
            const uint8_t *a = s.chunk[143] + 324 * k, *z = loaded.chunk[143] + 324 * k;
            load_compared[0]++;
            load_same[0] += memcmp(a + 0x134, z + 0x134, 4) == 0;
            load_compared[1]++;
            load_same[1] += memcmp(a + 0x13C, z + 0x13C, 4) == 0;
            if (memcmp(a + 0x134, z + 0x134, 4) || memcmp(a + 0x13C, z + 0x13C, 4)) {
                printf("  %s: route %d stored len %d ends %d %d, loaded %d %d %d\n", label, k, (int32_t) save_u32(a + 0x134),
                       (int16_t) save_u16(a + 0x13C), (int16_t) save_u16(a + 0x13E), (int32_t) save_u32(z + 0x134),
                       (int16_t) save_u16(z + 0x13C), (int16_t) save_u16(z + 0x13E));
            }
        }
        for (int id = 1; id < SAVE_FIGURE_COUNT; id++) {
            const uint8_t *a = s.chunk[SAVE_CHUNK_FIGURES] + (size_t) id * SAVE_FIGURE_SIZE;
            const uint8_t *z = loaded.chunk[SAVE_CHUNK_FIGURES] + (size_t) id * SAVE_FIGURE_SIZE;
            if (a[0x0E] != 1) {
                continue;
            }
            /* the campaign records were saved from a fresh scenario, never loaded: figures of the
             * types 0x004D32F0 gives no name (-2) still hold the constructor's 0 */
            if (save_u32(a + 0x98) == 0 && (int32_t) save_u32(z + 0x98) == -2) {
                load_fresh_default++;
                continue;
            }
            load_compared[2]++;
            int same = memcmp(a + 0x98, z + 0x98, 4) == 0;
            load_same[2] += same;
            if (!same) {
                printf("  %s: figure %d type %#x stored group %d loaded %d\n", label, id, a[0x0A],
                       (int32_t) save_u32(a + 0x98), (int32_t) save_u32(z + 0x98));
            }
        }
    }
    printf("  %s: version %u, %d upgrade steps, %d routes rebuilt, %d reset, %d rand draws, %d dropped writes\n",
           label, s.version, st.upgrades, st.routes_rebuilt, st.routes_reset, st.rand_draws, st.dropped_writes);
    save_file_free(&loaded);
    save_file_free(&s);
    return 1;
}

int main(int argc, char **argv)
{
    long buildings = 0, figures = 0;
    static image_groups groups;
    if (load_image_groups(game_dir(), &groups) != 0 || load_image_sizes(game_dir(), &groups, &sizes) != 0) {
        fprintf(stderr, "cannot read the .sg3 files\n");
        return 2;
    }
    int read = 0;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            size_t len;
            uint8_t *data = read_file(argv[i], &len);
            if (!data) {
                fprintf(stderr, "cannot read %s\n", argv[i]);
                return 2;
            }
            read += check_one(data, len, argv[i], &buildings, &figures);
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
            read += check_one(pak + start, len, label, &buildings, &figures);
        }
        free(pak);
    }
    static const char *WHAT[3] = {"route lengths", "route endpoints", "figure name groups"};
    for (int k = 0; k < 3; k++) {
        printf("save_check: load, %s: %ld / %ld equal to the stored values\n", WHAT[k], load_same[k], load_compared[k]);
        if (load_same[k] != load_compared[k]) {
            failures++;
        }
    }
    printf("save_check: load, %ld figures of unnamed types still at the constructor's 0 (fresh records)\n",
           load_fresh_default);
    printf("save_check: %ld live figures off the figure grid (informational)\n", off_grid);
    printf("save_check: %d saved games read, %ld buildings, %ld figures, %ld checks, %ld failed\n", read,
           buildings, figures, checks, failures);
    return failures ? 1 : 0;
}
