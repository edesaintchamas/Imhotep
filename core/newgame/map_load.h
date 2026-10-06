/* Imhotep - reading a scenario map (maps\NAME.map) into the game state (docs/newgame.md 3.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Format: docs/formats/map.md. Check: tests/differential/oracle_function_check.py (mapload
 * entries), tests/differential/oracle_newgame_check.py. */
#ifndef IMHOTEP_NEWGAME_MAP_LOAD_H
#define IMHOTEP_NEWGAME_MAP_LOAD_H

#include <stdint.h>

#include "state/state.h"

/* A game file opened with the CRT (_open / _read / _close in the original): the whole file from
 * the platform (platform_file_read) and the read position. */
typedef struct {
    uint8_t *data;
    int32_t size; /* -1: the file does not exist */
    uint32_t pos;
} map_stream;

/* @bin 0x004D5820 load a map file (path as the game builds it, e.g. "maps\\Default.map"):
 * floodplain settings, the chunks (0x004D4AE0), the current empire 0x00784F10; unless `preview`,
 * the graphics slots of the scenario. Returns 1 when the file does not open, else 0. */
int32_t map_file_load(game_state *st, const char *path, int32_t preview);

/* @bin 0x004D4AE0 the chunks of the MAPS container (schema 0x005E17D8) and the version
 * upgrades */
void map_read_chunks(game_state *st, map_stream *f);

/* @bin 0x004D0E10 one chunk of a schema (20-byte records at schema_address) read from the file,
 * migrated when the file's record (0x00E92E50) has another element size or count. Returns 1, or
 * 0 when the file version (0x00E945C0) is below 115 (after a message box). */
int32_t chunk_read(game_state *st, map_stream *f, int32_t index, uint32_t schema_address);

/* @bin 0x0055BCF0 a compressed chunk: u32 stored size (0x80000000 raw, 0 zero-filled), then
 * PKWARE data, into `size` bytes at dst. Returns 1, or 0 on a short file or a bad stream. */
int32_t chunk_read_compressed(game_state *st, map_stream *f, uint32_t dst, int32_t size);

/* @bin 0x004D5980 the map's grids after the load: images cleared, vegetation := -1, floodplain,
 * events, empire objects, ... */
void map_setup_grids(game_state *st);

/* @bin 0x004D4040 current city +0x298C .. +0x29B8 := 0 */
void city_clear_298c(game_state *st);

/* @bin 0x0046D480 tiles flagged as buildings (terrain bit 8, no building): the map's special
 * buildings (types 0x58, 0x59, 0x5D) are created, other such tiles cleared; then each 0x58
 * building is linked to the nearest 0x59 */
void map_flagged_buildings(game_state *st);

/* @bin 0x004CE3A0 a file name cut at its first '.' */
void file_name_strip_extension(game_state *st, uint32_t name);
/* @bin 0x004CE3C0 '.' and the three characters of 0x00E38AEC appended */
void file_name_add_extension(game_state *st, uint32_t name);

/* version upgrades of 0x004D4AE0 */
void upgrade_empire_object_kinds(game_state *st);   /* @bin 0x004D17E0 */
void upgrade_figures_0x7f(game_state *st);          /* @bin 0x004D32D0 */
void upgrade_events_0x90(game_state *st);           /* @bin 0x004D3D80 */
void upgrade_events_0x98(game_state *st);           /* @bin 0x004D40C0 */
void upgrade_empire_object_masks(game_state *st);   /* @bin 0x004D19E0 */
void upgrade_buildings_0x86(game_state *st);        /* @bin 0x004D4080 */
void upgrade_city_44c8(game_state *st);             /* @bin 0x004D40A0 */
void upgrade_city_i16_to_i32(game_state *st);       /* @bin 0x004D1A70 */

#endif
