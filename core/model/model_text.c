/* Imhotep - the text helpers of the game's model-file loaders.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "model/model_text.h"
#include "state/layout.h"

#include <stdlib.h>
#include <string.h>

uint8_t *model_text_buffer(const uint8_t *text, size_t len)
{
    uint8_t *buf = calloc(len * 2 + 4096, 1);
    if (buf) {
        memcpy(buf, text, len);
    }
    return buf;
}

size_t model_find_str(const uint8_t *p, const char *s, size_t n)
{
    /* @bin 0x004CBDD0 offset of the first match within n bytes, 0 if none */
    size_t k = strlen(s);
    for (size_t i = 0; i < n; i++) {
        if (p[i] == (uint8_t) s[0] && memcmp(p + i, s, k) == 0) {
            return i;
        }
    }
    return 0;
}

size_t model_find_char(const uint8_t *p, uint8_t c, size_t n)
{
    /* @bin 0x004CBE30 offset just past the first c within n bytes, 0 if none */
    for (size_t i = 0; i < n; i++) {
        if (p[i] == c) {
            return i + 1;
        }
    }
    return 0;
}

size_t model_skip_to_number(const uint8_t *p)
{
    /* @bin 0x004CC0E0 bytes before the next digit or '-', at most 999 */
    size_t i = 0;
    for (int k = 1; k < 1000; k++, i++) {
        if ((p[i] >= '0' && p[i] <= '9') || p[i] == '-') {
            return i;
        }
    }
    return i;
}

int32_t model_parse_number(const uint8_t *p)
{
    /* @bin 0x004CC070 optional '-', then decimal digits (place values from 0x005E058C) */
    int neg = p[0] == '-';
    const uint8_t *q = p + neg;
    int digits = 0;
    while (q[digits] >= '0' && q[digits] <= '9') {
        digits++;
    }
    uint32_t v = 0, place = 1;
    for (int i = digits - 1; i >= 0; i--) {
        v += (uint32_t) (q[i] - '0') * place;
        place *= 10;
    }
    return neg ? -(int32_t) v : (int32_t) v;
}

const char *model_file_name(const game_state *st, uint32_t table)
{
    game_state *s = (game_state *) st;
    int32_t d = S32(s, G_DIFFICULTY);
    if (d < 0 || d > 4) {
        return NULL;
    }
    return (const char *) state_ptr(st, U32(s, table + 4u * (uint32_t) d), 1);
}

/* one record: past the next '{', then `fields` numbers each followed by a ',' */
static const uint8_t *read_record(game_state *st, const uint8_t *p, size_t len, uint32_t dst, int fields)
{
    p += model_find_char(p, '{', len);
    for (int f = 0; f < fields; f++) {
        p += model_skip_to_number(p);
        S32(st, dst + 4u * (uint32_t) f) = model_parse_number(p);
        p += model_find_char(p, ',', len);
    }
    return p;
}

int model_load_figures(game_state *st, const uint8_t *text, size_t len)
{
    uint8_t *buf = model_text_buffer(text, len);
    if (!buf) {
        return -1;
    }
    size_t at = model_find_str(buf, "ALL FIGURES", len);
    if (at == 0 && memcmp(buf, "ALL FIGURES", 11) != 0) {
        free(buf);
        return -1;
    }
    /* @bin 0x00540ED0 figures: 14 fields per record into 0x007090CC.. while the target is <= 0x0070A8DB */
    const uint8_t *p = buf + at;
    for (uint32_t dst = G_PROPS; dst <= G_PROPS + 0x180Fu; dst += 0x38) {
        p = read_record(st, p, len, dst, 14);
    }
    /* @bin 0x00540ED0 enemies: records 0..59 into 0x007083AC + i * 0x38 */
    p = buf + model_find_str(buf, "ALL ENEMIES", len);
    for (uint32_t i = 0; i < 60; i++) {
        p = read_record(st, p, len, G_ENEMY_MODELS + 0x38u * i, 14);
    }
    free(buf);
    return 0;
}

int model_load_tax_sentiment(game_state *st, const uint8_t *text, size_t len)
{
    uint8_t *buf = model_text_buffer(text, len);
    if (!buf) {
        return -1;
    }
    /* @bin 0x00541160 from the start of the file: 11 fields per record into 0x00EA1384.. while the
     * target is <= 0x00EA17FB */
    const uint8_t *p = buf;
    for (uint32_t dst = G_SENTIMENT_TABLE; dst <= G_SENTIMENT_TABLE + 0x477u; dst += 44) {
        p = read_record(st, p, len, dst, 11);
    }
    free(buf);
    return 0;
}
