/* Imhotep - terrain neighbour-pattern tables (see terrain_tables.c).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_TABLES_H
#define IMHOTEP_MAP_TERRAIN_TABLES_H

#include <stdint.h>

/* Record layout, @bin 0x00484BE0: bytes 0-7 neighbour pattern (0 clear, 1 set, 2 any), byte 8 +
 * orientation/2 image offset, byte 12 extra value, byte 13 variant modulus, byte 14 counter. */
#define TERRAIN_PATTERN_SHORE_COUNT 0x30
#define TERRAIN_PATTERN_DEEP_COUNT 0x1A
#define TERRAIN_PATTERN_FLOODPLAIN_COUNT (0x18 * 8)
#define TERRAIN_PATTERN_SPECIAL_ROCKS_COUNT 0x11
#define TERRAIN_PATTERN_ROADS_COUNT 0x11
#define TERRAIN_PATTERN_WALLS_COUNT 0x30
#define TERRAIN_PATTERN_WALLS_BUILDING_COUNT 0x0F
#define TERRAIN_PATTERN_CANALS_COUNT 0x10

extern const uint8_t terrain_pattern_shore[TERRAIN_PATTERN_SHORE_COUNT][16];
extern const uint8_t terrain_pattern_deep[TERRAIN_PATTERN_DEEP_COUNT][16];
extern const uint8_t terrain_pattern_floodplain[TERRAIN_PATTERN_FLOODPLAIN_COUNT][16];
extern const uint8_t terrain_pattern_special_rocks[TERRAIN_PATTERN_SPECIAL_ROCKS_COUNT][16];
extern const uint8_t terrain_pattern_roads[TERRAIN_PATTERN_ROADS_COUNT][16];
extern const uint8_t terrain_pattern_walls[TERRAIN_PATTERN_WALLS_COUNT][16];
extern const uint8_t terrain_pattern_walls_building[TERRAIN_PATTERN_WALLS_BUILDING_COUNT][16];
extern const uint8_t terrain_pattern_canals[TERRAIN_PATTERN_CANALS_COUNT][16];

#endif
