/* Imhotep - building and house model (Pharaoh_Model_<difficulty>.txt, read by 0x00540C90).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "model/building_model.h"

#include "model/model_text.h"

#include <stdlib.h>
#include <string.h>

/* @bin 0x00601908 */
const char *const building_model_files[5] = {
    "Pharaoh_Model_VeryEasy.txt", "Pharaoh_Model_Easy.txt", "Pharaoh_Model_Normal.txt",
    "Pharaoh_Model_Hard.txt", "Pharaoh_Model_Impossible.txt",
};

static void read_records(const uint8_t *buf, size_t len, const char *section, int32_t *out, int records, int fields)
{
    const uint8_t *p = buf + model_find_str(buf, section, len);
    for (int r = 0; r < records; r++) {
        p += model_find_char(p, '{', len);
        for (int f = 0; f < fields; f++) {
            p += model_skip_to_number(p);
            out[r * fields + f] = model_parse_number(p);
            p += model_find_char(p, ',', len);
        }
    }
}

int building_model_parse(const uint8_t *text, size_t len, building_model *m)
{
    /* @bin 0x00540C90 the file is read into a zeroed 100,000-byte buffer and every search runs over
     * the file length from the current position, so reads may run past the text into zeros */
    size_t cap = len * 2 + 4096;
    uint8_t *buf = calloc(cap, 1);
    if (!buf) {
        return -1;
    }
    memcpy(buf, text, len);
    memset(m, 0, sizeof(*m));
    if (model_find_str(buf, "ALL BUILDINGS", len) == 0 && memcmp(buf, "ALL BUILDINGS", 13) != 0) {
        free(buf);
        return -1;
    }
    const uint8_t *p = buf + model_find_str(buf, "ALL BUILDINGS", len);
    for (int k = 0; k < 299; k++) { /* record count check (message box when not 0x101) */
        size_t step = model_find_char(p, '{', len);
        if (step == 0) {
            break;
        }
        m->building_records_found++;
        p += step;
    }
    read_records(buf, len, "ALL BUILDINGS", &m->building[0][0], MODEL_BUILDINGS, 10);
    read_records(buf, len, "ALL HOUSES", &m->house[0][0], MODEL_HOUSES, 22);
    free(buf);
    return 0;
}

void building_model_store(const building_model *m, game_state *st)
{
    for (int t = 0; t < MODEL_BUILDINGS; t++) {
        for (int f = 0; f < 10; f++) {
            S32(st, 0x00705EA4u + 0x28u * (uint32_t) t + 4u * (uint32_t) f) = m->building[t][f];
        }
    }
    for (int h = 0; h < MODEL_HOUSES; h++) {
        for (int f = 0; f < 22; f++) {
            S32(st, 0x0070CDCCu + 0x58u * (uint32_t) h + 4u * (uint32_t) f) = m->house[h][f];
        }
    }
}
