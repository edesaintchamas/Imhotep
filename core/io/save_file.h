/* Imhotep - saved-game reader: file chunks -> the reference Pharaoh.exe's version-160 layout.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Format: docs/formats/save.md. Check: tests/differential/save_check.c. */
#ifndef IMHOTEP_IO_SAVE_FILE_H
#define IMHOTEP_IO_SAVE_FILE_H

#include <stddef.h>
#include <stdint.h>

#include "io/save_schema.h"

/* Chunks named so far (save.md §3, §5); each is in the version-160 layout. */
enum {
    SAVE_CHUNK_FILE_VERSION = 4,
    SAVE_CHUNK_IMAGE_GRID = 7,
    SAVE_CHUNK_BUILDING_GRID = 9,
    SAVE_CHUNK_TERRAIN_GRID = 10,
    SAVE_CHUNK_FIGURE_GRID = 12,
    SAVE_CHUNK_FIGURES = 21,
    SAVE_CHUNK_FORMATIONS = 24,
    SAVE_CHUNK_CITY_DATA = 28,
    SAVE_CHUNK_CURRENT_PLAYER = 31,
    SAVE_CHUNK_BUILDINGS = 32,
    SAVE_CHUNK_TICK = 34,       /* tick within the day, 0..50 */
    SAVE_CHUNK_DAY = 35,        /* day within the month, 0..15 */
    SAVE_CHUNK_MONTH = 36,      /* 0..11 */
    SAVE_CHUNK_YEAR = 37,
    SAVE_CHUNK_RNG_IV1 = 41,
    SAVE_CHUNK_RNG_IV2 = 42,
    SAVE_CHUNK_EMPIRE_CITIES = 50,
    SAVE_CHUNK_MESSAGES = 77,
    SAVE_CHUNK_SCENARIO_INFO = 75,
    SAVE_CHUNK_DIFFICULTY = 174,
};

#define SAVE_BUILDING_SIZE 264
#define SAVE_BUILDING_COUNT 4000
#define SAVE_FIGURE_SIZE 388
#define SAVE_FIGURE_COUNT 2000
#define SAVE_CITY_DATA_SIZE 18904

enum {
    SAVE_OK = 0,
    SAVE_ERR_FORMAT = -1,      /* container, schema or decompression error */
    SAVE_ERR_TOO_OLD = -2,     /* file version < 115: the game refuses it */
    SAVE_ERR_CONVERTER = -3,   /* a chunk needs a converter function (game code, not ported) */
    SAVE_ERR_FLAG = -4,        /* same-size chunk stored with another compression flag */
};

typedef struct {
    uint32_t version;     /* file version (chunk 4) */
    uint32_t file_chunks; /* chunks in the file's schema */
    uint32_t read_chunks; /* chunks taken from the file (175 below version 154, else 181) */
    /* chunk i in the version-160 layout, save_schema[i].elem_size * elem_count bytes */
    uint8_t *chunk[SAVE_SCHEMA_RECORDS];
} save_file;

/* Read a saved game (or a mission1.pak record) into the version-160 layout, as the game's
 * per-chunk reader does (@bin 0x004D0E10), plus the chunk-175 defaults of files below version 154
 * (0x004D3C40). The loader's later steps (version upgrades, per-load registrations, monument
 * clean-ups; save.md §2) are not applied. Returns SAVE_OK or a SAVE_ERR_* code. */
int save_file_read(const uint8_t *data, size_t len, save_file *s);
void save_file_free(save_file *s);

/* Little-endian field access within a chunk (offsets from save.md). */
static inline uint32_t save_u32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}
static inline uint16_t save_u16(const uint8_t *p)
{
    return (uint16_t) (p[0] | p[1] << 8);
}

#endif
