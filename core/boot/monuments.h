/* Imhotep - the boot: the Cleopatra monument objects' constructors and set-ups (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BOOT_MONUMENTS_H
#define IMHOTEP_BOOT_MONUMENTS_H

#include <stddef.h>
#include <stdint.h>

#include "state/state.h"

/* a run of stores relative to the object: `count` values of `size` bytes (1 or 4) from `off` */
typedef struct {
    uint32_t off;
    uint16_t count;
    uint8_t size;
    uint32_t value;
} monument_run;

/* what a monument constructor stores (core/boot/boot_data.c, tools/gen/gen_boot_data.py):
 * its base constructor, its parts (0x34 bytes each, from `parts`), their rectangles, the other
 * stores, the bytes of rand() % 5 (in call order) and the number of rand calls */
typedef struct {
    uint32_t ctor, base, parts;
    int32_t nparts;
    const int16_t (*rects)[4];
    const monument_run *runs;
    int32_t nruns;
    const uint16_t *rand;
    int32_t nrand;
} monument_def;

extern const monument_def monument_defs[8];

/* a call of a sound table builder (core/boot/sound.c): index, file name (a string of the state), kind */
typedef struct {
    int32_t index;
    uint32_t name;
    int32_t kind;
} sound_entry;

extern const sound_entry sound_list_539d60[], sound_list_53bcf0[], sound_list_53af90[], sound_list_53b130[];
extern const int sound_list_539d60_count, sound_list_53bcf0_count, sound_list_53af90_count, sound_list_53b130_count;

/* the table entry `fn` of the static constructors (0x00413C70 .. 0x00413E30): its object built */
void monument_ctor(game_state *st, uint32_t fn);
/* @bin 0x004D1350, 0x004D4A60 the chunk descriptor tables (generated stores) */
void boot_ctor_4d1350(game_state *st);
void boot_ctor_4d4a60(game_state *st);

/* @bin 0x004E5190 (thiscall on a part) its screen offsets from its x, y and the view's tile sizes */
void monument_part_layout(game_state *st, uint32_t part);
/* @bin 0x004E52B0 (parts, count) 0x004E5190 on each part */
void monument_parts_layout(game_state *st, uint32_t parts, int32_t count);

#endif
