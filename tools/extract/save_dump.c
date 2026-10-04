/* Imhotep - dump a saved game's state (as stored) to JSON. Format: docs/formats/save.md.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/save_dump FILE.sav            a player save
 *   build/save_dump --pak N             record N of $IMHOTEP_GAME_DIR/mission1.pak
 *   build/save_dump ... --chunk I OUT   also write chunk I (version-160 layout) to OUT
 *   build/save_dump ... --stored        the state as stored, without the loader's later steps
 *   build/save_dump ... --rand SEED     CRT rand() state at load time (default 0)
 *   build/save_dump ... --raw OUT       also write the input bytes (e.g. the pak record) to OUT
 *   build/save_dump ... --image OUT     also write every chunk as {u32 address, u32 length, bytes}
 *                                       (the memory image tools/oracle seeds the emulator with)
 *
 * By default the loader's steps run (core/io/save_load.c: upgrades, route rebuild, figure names,
 * monument clean-ups) with CRT rand() state 0, i.e. the state the game holds after loading.
 *
 * Prints the fields documented in save.md §5 (buildings, figures, city data) and a chunk table.
 * Names of types: tools/extract/read_text.py (text groups 41 and 64). Output stays in local/. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/campaign_pak.h"
#include "io/save_file.h"
#include "io/save_load.h"
#include "platform/game_files.h"

static void dump(const save_file *s)
{
    printf("{\n \"version\": %u, \"file_chunks\": %u, \"read_chunks\": %u,\n", s->version, s->file_chunks,
           s->read_chunks);
    printf(" \"current_player\": %u, \"difficulty\": %u,\n", save_u32(s->chunk[SAVE_CHUNK_CURRENT_PLAYER]),
           save_u32(s->chunk[SAVE_CHUNK_DIFFICULTY]));
    printf(" \"date\": {\"year\": %d, \"month\": %u, \"day\": %u, \"tick\": %u}, \"rng\": [%u, %u],\n",
           (int32_t) save_u32(s->chunk[SAVE_CHUNK_YEAR]), save_u32(s->chunk[SAVE_CHUNK_MONTH]),
           save_u32(s->chunk[SAVE_CHUNK_DAY]), save_u32(s->chunk[SAVE_CHUNK_TICK]),
           save_u32(s->chunk[SAVE_CHUNK_RNG_IV1]), save_u32(s->chunk[SAVE_CHUNK_RNG_IV2]));
    printf(" \"chunks\": [");
    for (int i = 0; i < SAVE_SCHEMA_RECORDS; i++) {
        const save_schema_record *t = &save_schema[i];
        size_t total = (size_t) t->elem_size * t->elem_count, nonzero = 0;
        for (size_t b = 0; b < total; b++) {
            nonzero += s->chunk[i][b] != 0;
        }
        printf("%s\n  {\"index\": %d, \"address\": \"0x%08X\", \"elem_size\": %u, \"count\": %u, \"nonzero_bytes\": %zu}",
               i ? "," : "", i, t->address, t->elem_size, t->elem_count, nonzero);
    }
    printf("\n ],\n \"city\": [");
    for (int p = 0; p < 2; p++) {
        const uint8_t *c = s->chunk[SAVE_CHUNK_CITY_DATA] + (size_t) p * SAVE_CITY_DATA_SIZE;
        printf("%s\n  {\"player\": %d, \"treasury\": %d, \"balance_last_year\": %d, \"entry\": [%u, %u, %d], "
               "\"exit\": [%u, %u, %d]}",
               p ? "," : "", p, (int32_t) save_u32(c + 0x0C), (int32_t) save_u32(c + 0x2CBC), save_u16(c + 0x2778),
               save_u16(c + 0x277A), (int32_t) save_u32(c + 0x277C), save_u16(c + 0x2780), save_u16(c + 0x2782),
               (int32_t) save_u32(c + 0x2784));
    }
    printf("\n ],\n \"buildings\": [");
    int first = 1;
    for (int id = 1; id < SAVE_BUILDING_COUNT; id++) {
        const uint8_t *b = s->chunk[SAVE_CHUNK_BUILDINGS] + (size_t) id * SAVE_BUILDING_SIZE;
        if (b[0] == 0) {
            continue;
        }
        printf("%s\n  {\"id\": %d, \"state\": %u, \"owner\": %u, \"type\": %u, \"subtype\": %u, \"size\": %u, "
               "\"x\": %u, \"y\": %u, \"grid_offset\": %d, \"sequence\": %u, \"orientation\": %u}",
               first ? "" : ",", id, b[0], b[1], save_u16(b + 0x10), save_u16(b + 0x12), b[3], save_u16(b + 6),
               save_u16(b + 8), (int32_t) save_u32(b + 0x0C), save_u16(b + 0x16), b[0xAA]);
        first = 0;
    }
    printf("\n ],\n \"figures\": [");
    first = 1;
    for (int id = 1; id < SAVE_FIGURE_COUNT; id++) {
        const uint8_t *f = s->chunk[SAVE_CHUNK_FIGURES] + (size_t) id * SAVE_FIGURE_SIZE;
        if (f[0x0E] == 0) {
            continue;
        }
        printf("%s\n  {\"id\": %d, \"state\": %u, \"type\": %u, \"x\": %u, \"y\": %u, \"grid_offset\": %d, "
               "\"next_on_tile\": %u, \"action_state\": %u, \"sequence\": %u}",
               first ? "" : ",", id, f[0x0E], f[0x0A], save_u16(f + 0x14), save_u16(f + 0x16),
               (int32_t) save_u32(f + 0x20), save_u16(f + 0x08), f[0x38], save_u16(f + 0x86));
        first = 0;
    }
    printf("\n ]\n}\n");
}

int main(int argc, char **argv)
{
    uint8_t *data = NULL;
    size_t len = 0, start = 0;
    int chunk = -1;
    const char *chunk_out = NULL, *image_out = NULL, *raw_out = NULL;
    int stored = 0;
    uint32_t rand_seed = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stored") == 0) {
            stored = 1;
        } else if (strcmp(argv[i], "--chunk") == 0 && i + 2 < argc) {
            chunk = atoi(argv[++i]);
            chunk_out = argv[++i];
        } else if (strcmp(argv[i], "--rand") == 0 && i + 1 < argc) {
            rand_seed = (uint32_t) strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--raw") == 0 && i + 1 < argc) {
            raw_out = argv[++i];
        } else if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
            image_out = argv[++i];
        } else if (strcmp(argv[i], "--pak") == 0 && i + 1 < argc) {
            char path[1200];
            size_t size;
            snprintf(path, sizeof(path), "%s/mission1.pak", game_dir());
            data = read_file(path, &size);
            if (!data || campaign_pak_record(data, size, atoi(argv[++i]), &start, &len) != 0) {
                fprintf(stderr, "cannot read record %s of %s\n", argv[i], path);
                return 2;
            }
        } else {
            data = read_file(argv[i], &len);
            if (!data) {
                fprintf(stderr, "cannot read %s\n", argv[i]);
                return 2;
            }
        }
    }
    if (!data) {
        fprintf(stderr, "usage: save_dump FILE.sav | --pak N [--chunk I OUT]\n");
        return 2;
    }
    if (raw_out) {
        FILE *f = fopen(raw_out, "wb");
        if (!f || fwrite(data + start, 1, len, f) != len || fclose(f) != 0) {
            fprintf(stderr, "cannot write %s\n", raw_out);
            return 2;
        }
    }
    save_file s;
    int rc = save_file_read(data + start, len, &s);
    free(data);
    if (rc != SAVE_OK) {
        fprintf(stderr, "save_file_read: error %d\n", rc);
        return 1;
    }
    if (!stored) {
        static image_groups groups;
        static image_sizes sizes;
        if (load_image_groups(game_dir(), &groups) != 0 || load_image_sizes(game_dir(), &groups, &sizes) != 0) {
            fprintf(stderr, "cannot read the .sg3 files (use --stored)\n");
            return 2;
        }
        save_load_env env = {rand_seed, 0, image_sizes_lookup, &sizes, sizes.count};
        save_load_stats st;
        save_file_load(&s, &env, &st);
        fprintf(stderr, "loaded: %d upgrade steps, %d routes rebuilt, %d rand draws\n", st.upgrades,
                st.routes_rebuilt, st.rand_draws);
    }
    if (chunk >= 0 && chunk < SAVE_SCHEMA_RECORDS) {
        FILE *f = fopen(chunk_out, "wb");
        size_t total = (size_t) save_schema[chunk].elem_size * save_schema[chunk].elem_count;
        if (!f || fwrite(s.chunk[chunk], 1, total, f) != total) {
            fprintf(stderr, "cannot write %s\n", chunk_out);
            return 2;
        }
        fclose(f);
    }
    if (image_out) {
        FILE *f = fopen(image_out, "wb");
        for (int i = 0; f && i < SAVE_SCHEMA_RECORDS; i++) {
            uint32_t head[2] = {save_schema[i].address, save_schema[i].elem_size * save_schema[i].elem_count};
            uint8_t h[8];
            for (int b = 0; b < 8; b++) {
                h[b] = (uint8_t) (head[b / 4] >> (8 * (b % 4)));
            }
            if (fwrite(h, 1, 8, f) != 8 || fwrite(s.chunk[i], 1, head[1], f) != head[1]) {
                fclose(f);
                f = NULL;
            }
        }
        if (!f || fclose(f) != 0) {
            fprintf(stderr, "cannot write %s\n", image_out);
            return 2;
        }
    }
    dump(&s);
    save_file_free(&s);
    return 0;
}
