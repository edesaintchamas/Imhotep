/* Imhotep - the text helpers of the game's model-file loaders (Pharaoh_Model, Figure_model,
 * Tax_Sentiment_Model: 0x00540C90, 0x00540ED0, 0x00541160).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MODEL_MODEL_TEXT_H
#define IMHOTEP_MODEL_MODEL_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "state/state.h"

/* The loaders read the file into a zeroed buffer and every search runs over the file length from
 * the current position, so reads may run past the text into zeros: the buffer holds the text
 * followed by at least that many zero bytes. NULL when out of memory; free() it. */
uint8_t *model_text_buffer(const uint8_t *text, size_t len);

size_t model_find_str(const uint8_t *p, const char *s, size_t n);  /* @bin 0x004CBDD0 */
size_t model_find_char(const uint8_t *p, uint8_t c, size_t n);     /* @bin 0x004CBE30 */
size_t model_skip_to_number(const uint8_t *p);                     /* @bin 0x004CC0E0 */
int32_t model_parse_number(const uint8_t *p);                      /* @bin 0x004CC070 */

/* The file name for the current difficulty (0x00E39080) from a table of string pointers in the
 * exe's data (0x00601908 buildings, 0x0060191C figures, 0x00601930 tax sentiment); NULL if out of
 * range. */
const char *model_file_name(const game_state *st, uint32_t table);

/* @bin 0x00540ED0 Figure_model_<difficulty>.txt: "ALL FIGURES" 110 records x 14 i32 at
 * 0x007090CC, "ALL ENEMIES" 60 records at 0x007083AC. Returns 0, -1 without "ALL FIGURES". */
int model_load_figures(game_state *st, const uint8_t *text, size_t len);
/* @bin 0x00541160 Tax_Sentiment_Model_<difficulty>.txt: 26 rows x 11 i32 at 0x00EA1384. */
int model_load_tax_sentiment(game_state *st, const uint8_t *text, size_t len);

#endif
