/* Imhotep - run the core terrain rebuild on one map and write its grids, for the renderer.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/terrain_dump MAP.map OUT      (game files from $IMHOTEP_GAME_DIR)
 *
 * OUT: "IMTD", u32 format version (1), u32 image[228*228], u8 edge[228*228], little-endian.
 * stdout: one JSON line with the rebuild statistics. Exit status 0, or 2 on error. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/map_file.h"
#include "map/terrain.h"
#include "platform/exe_image.h"
#include "platform/game_files.h"

static int put_u32(FILE *f, uint32_t v)
{
    uint8_t b[4] = {(uint8_t) v, (uint8_t) (v >> 8), (uint8_t) (v >> 16), (uint8_t) (v >> 24)};
    return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s MAP.map OUT\n", argv[0]);
        return 2;
    }
    static image_groups groups;
    if (load_image_groups(game_dir(), &groups) != 0) {
        return 2;
    }
    size_t len;
    uint8_t *data = read_file(argv[1], &len);
    map_file m;
    if (!data || map_file_read(data, len, &m) != 0) {
        fprintf(stderr, "cannot read map %s\n", argv[1]);
        return 2;
    }
    free(data);
    terrain_state *t = malloc(sizeof(*t));
    static game_state gs;
    if (state_create(&gs) != 0 || exe_image_load(game_dir(), &gs) != 0) {
        fprintf(stderr, "cannot read %s/Pharaoh.exe\n", game_dir());
        return 2;
    }
    if (!t || terrain_init(t, &gs, &m, &groups) != 0) {
        fprintf(stderr, "map %s: missing chunks\n", argv[1]);
        return 2;
    }
    map_file_free(&m);
    terrain_scenario_start(t);

    FILE *f = fopen(argv[2], "wb");
    int err = !f || fwrite("IMTD", 1, 4, f) != 4 || put_u32(f, 1) != 0;
    for (int o = 0; !err && o < GRID_TILES; o++) {
        err = put_u32(f, U32(&gs, 0x00B7482Cu + 4u * (uint32_t) o)) != 0;
    }
    err = err || fwrite(state_ptr(&gs, 0x00B67D1Cu, GRID_TILES), 1, GRID_TILES, f) != GRID_TILES;
    if (f) {
        err |= fclose(f) != 0;
    }
    if (err) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 2;
    }
    printf("{\"land_0x10000000_unported\": %d, "
           "\"floodplain_objects\": %d, \"signposts_placed\": %d}\n",
           t->stats.land_0x10000000_unported,
           t->stats.floodplain_objects, t->stats.signposts_placed);
    free(t);
    return 0;
}
