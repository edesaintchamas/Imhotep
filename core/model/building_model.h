/* Imhotep - building and house model (Pharaoh_Model_<difficulty>.txt, read by 0x00540C90).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Format: docs/formats/model.md. */
#ifndef IMHOTEP_MODEL_BUILDING_MODEL_H
#define IMHOTEP_MODEL_BUILDING_MODEL_H

#include <stddef.h>
#include <stdint.h>

#include "state/state.h"

#define MODEL_BUILDINGS 237 /* @bin 0x00540C90 records while the target is <= 0x007083AB */
#define MODEL_HOUSES 20     /* @bin 0x00540C90 records while the target is <= 0x0070D4AB */

enum { /* building record fields (0x00705EA4 + type * 0x28) */
    MODEL_COST, MODEL_DES_VALUE, MODEL_DES_STEP, MODEL_DES_STEP_SIZE, MODEL_DES_RANGE,
    MODEL_LABORERS, MODEL_FIRE_RISK, MODEL_DAMAGE_RISK, MODEL_FIELD_8, MODEL_FIELD_9,
};

typedef struct {
    int32_t building[MODEL_BUILDINGS][10]; /* 0x00705EA4 */
    int32_t house[MODEL_HOUSES][22];       /* 0x0070CDCC */
    int32_t building_records_found;        /* '{' after "ALL BUILDINGS"; the game expects 0x101 */
} building_model;

/* Parse the model text exactly as 0x00540C90 does. Returns 0, or -1 if "ALL BUILDINGS" is missing. */
int building_model_parse(const uint8_t *text, size_t len, building_model *m);

/* @bin 0x00540C90 the records' place in the game: 0x00705EA4 (buildings), 0x0070CDCC (houses). */
void building_model_store(const building_model *m, game_state *st);

/* @bin 0x00601908 model file per difficulty (chunk 174): VeryEasy, Easy, Normal, Hard, Impossible */
extern const char *const building_model_files[5];

#endif
