/* Imhotep - the model files of a difficulty level (package cmdrest, docs/commands.md 3.9).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Changing the difficulty (0x00402E50) reads the game's model text files of the new level again;
 * they hold simulation constants:
 *   - Pharaoh_Model_<level>.txt (0x00540C90): "ALL BUILDINGS", 237 records of 10 i32 at 0x00705EA4
 *     (the building models: cost first, docs/commands.md 2.2), "ALL HOUSES", 20 records of 22 i32
 *     at 0x0070CDCC;
 *   - Figure_model_<level>.txt (0x00540ED0): "ALL FIGURES", 110 records of 14 i32 at 0x007090CC,
 *     "ALL ENEMIES", 60 records of 14 i32 at 0x007083AC;
 *   - Tax_Sentiment_Model_<level>.txt (0x00541160): 26 records of 11 i32 at 0x00EA1384;
 *   - 0x004BC720 then copies the enemy records of the scenario's enemy (0x007848FC) into the
 *     figure models and loads that enemy's graphics.
 * The original reads each file into its 100,000-byte work buffer 0x00E92E1C (a heap block outside
 * the simulation state) and parses it with the text helpers 0x004CBDD0 .. 0x004CC0E0; here the
 * file comes from the platform (platform_file_read: the game's directory) into a buffer of the
 * same size, and the helpers run on it. */
#include "command/models.h"

#include <stdlib.h>
#include <string.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define DIFFICULTY 0x00E39080u
#define SPARE_SCREEN 0x00E92E1Cu
#define BUFFER_SIZE 100000u /* @bin 0x00540CC1 */
#define POWERS 0x005E058Cu  /* @bin 0x004CC0B3 i32 powers of ten */

/* ---- the text helpers, on a byte buffer ---- */

typedef struct {
    const uint8_t *p;
    uint32_t size; /* readable bytes: past them the original reads other memory */
    int overrun;
} text_buf;

static uint8_t at(text_buf *b, uint32_t i)
{
    if (i >= b->size) {
        b->overrun = 1;
        return 0;
    }
    return b->p[i];
}

/* @bin 0x004CBE60 (a, b, n) 0 when the n bytes are equal, else the index of the first difference + 1 */
static int32_t text_compare_at(text_buf *b, uint32_t a, const uint8_t *s, int32_t n)
{
    for (int32_t i = 0; i < n; i++) {
        if (at(b, a + (uint32_t) i) != s[i]) {
            return i + 1;
        }
    }
    return 0;
}

/* @bin 0x004CBDD0 (buffer, string, n) the offset of the string's first occurrence that starts in the
 * first n bytes, else 0 */
static int32_t text_find_string_at(text_buf *b, uint32_t pos, const char *s, int32_t n)
{
    int32_t len = (int32_t) strlen(s);
    for (int32_t i = 0; i < n; i++) {
        if (at(b, pos + (uint32_t) i) == (uint8_t) s[0] &&
            text_compare_at(b, pos + (uint32_t) i, (const uint8_t *) s, len) == 0) {
            return i;
        }
    }
    return 0;
}

/* @bin 0x004CBE30 (buffer, char, n) the index + 1 of the char in the first n bytes, else 0 */
static int32_t text_find_char_at(text_buf *b, uint32_t pos, uint8_t c, int32_t n)
{
    for (int32_t i = 0; i < n; i++) {
        if (at(b, pos + (uint32_t) i) == c) {
            return i + 1;
        }
    }
    return 0;
}

/* @bin 0x004CC0E0 the bytes before the first digit or '-' (at most 999) */
static int32_t text_skip_to_number_at(text_buf *b, uint32_t pos)
{
    int32_t n = 0;
    for (int32_t k = 1;; k++) {
        uint8_t c = at(b, pos + (uint32_t) n);
        if ((c >= '0' && c <= '9') || c == '-') {
            return n;
        }
        n++;
        if (k + 1 >= 0x3E8) {
            return n;
        }
    }
}

/* @bin 0x004CC070 the decimal number at the position (an optional '-', digits weighted by the powers
 * of ten 0x005E058C: the original reads the table past its 8 entries for longer numbers) */
static int32_t text_atoi_at(game_state *st, text_buf *b, uint32_t pos)
{
    int neg = at(b, pos) == '-';
    uint32_t q = pos + (neg ? 1u : 0u);
    int32_t digits = 0;
    uint8_t c = at(b, q);
    if (c >= '0') {
        while (c <= '9') {
            /* @bin 0x004CC09B the next byte is tested >= '0' before the <= '9' test */
            c = at(b, q + (uint32_t) digits + 1);
            digits++;
            if (c < '0') {
                break;
            }
        }
    }
    uint32_t v = 0;
    for (int32_t k = digits; k > 0; k--, q++) {
        v += (uint32_t) ((int32_t) at(b, q) - '0') * U32(st, POWERS + 4u * (uint32_t) (k - 1));
    }
    return neg ? (int32_t) (0u - v) : (int32_t) v;
}

/* ---- standalone entries of the helpers (buffers in the state) ---- */

static int span(game_state *st, uint32_t address, uint32_t n, text_buf *b)
{
    b->p = n ? state_ptr(st, address, n) : NULL;
    b->size = n;
    b->overrun = 0;
    return n == 0 || b->p != NULL;
}

static int cstr(game_state *st, uint32_t address, char *out, uint32_t cap)
{
    for (uint32_t i = 0; i < cap; i++) {
        const uint8_t *c = state_ptr(st, address + i, 1);
        if (!c) {
            return -1;
        }
        out[i] = (char) *c;
        if (*c == 0) {
            return 0;
        }
    }
    return -1;
}

static uint32_t readable(game_state *st, uint32_t address, uint32_t want)
{
    uint32_t n = 0;
    while (n < want && state_ptr(st, address + n, 1) != NULL) {
        n++;
    }
    return n;
}

static int32_t run_compare(game_state *st, const int32_t *a)
{
    text_buf b;
    int32_t n = a[2] > 0 ? a[2] : 0;
    const uint8_t *s = n ? state_ptr(st, (uint32_t) a[1], (uint32_t) n) : NULL;
    if (!span(st, (uint32_t) a[0], (uint32_t) n, &b) || (n && !s)) {
        state_unported(st, 0x004CBE60u);
        return 0;
    }
    return text_compare_at(&b, 0, s, n);
}

static int32_t run_find_string(game_state *st, const int32_t *a)
{
    char s[256];
    text_buf b;
    if (cstr(st, (uint32_t) a[1], s, sizeof(s)) != 0) {
        state_unported(st, 0x004CBDD0u);
        return 0;
    }
    uint32_t n = a[2] > 0 ? (uint32_t) a[2] + (uint32_t) strlen(s) : 0;
    if (!span(st, (uint32_t) a[0], n, &b)) {
        state_unported(st, 0x004CBDD0u);
        return 0;
    }
    return text_find_string_at(&b, 0, s, a[2]);
}

static int32_t run_find_char(game_state *st, const int32_t *a)
{
    text_buf b;
    if (!span(st, (uint32_t) a[0], a[2] > 0 ? (uint32_t) a[2] : 0, &b)) {
        state_unported(st, 0x004CBE30u);
        return 0;
    }
    return text_find_char_at(&b, 0, (uint8_t) a[1], a[2]);
}

static int32_t run_skip(game_state *st, const int32_t *a)
{
    text_buf b;
    span(st, (uint32_t) a[0], readable(st, (uint32_t) a[0], 0x3E8), &b);
    int32_t r = text_skip_to_number_at(&b, 0);
    if (b.overrun) {
        state_unported(st, 0x004CC0E0u);
    }
    return r;
}

static int32_t run_atoi(game_state *st, const int32_t *a)
{
    text_buf b;
    span(st, (uint32_t) a[0], readable(st, (uint32_t) a[0], 64), &b);
    int32_t r = text_atoi_at(st, &b, 0);
    if (b.overrun) {
        state_unported(st, 0x004CC070u);
    }
    return r;
}

int32_t file_length(game_state *st, uint32_t name)
{
    /* @bin 0x004CED50 the file's length, 0 when it cannot be opened (the open / filelength are the
     * platform's) */
    char path[261];
    if (cstr(st, name, path, sizeof(path)) != 0) {
        state_unported(st, 0x004CED50u);
        return 0;
    }
    int32_t n = platform_file_read(st, path, 0, NULL, 0);
    return n < 0 ? 0 : n;
}

static int32_t run_file_length(game_state *st, const int32_t *a) { return file_length(st, (uint32_t) a[0]); }

/* ---- the model files ---- */

typedef struct {
    uint8_t *data;
    text_buf b;
    int32_t size; /* 0x004CED50 */
} model_file;

/* @bin 0x00540CA1 .. 0x00540D22 the work buffer cleared (0x0040F8F0), the file read into it
 * (0x004CEB60, a log line and 0 when it cannot be read), its length; 0 also without a work buffer */
static int model_open(game_state *st, model_file *f, uint32_t names, uint32_t no_file)
{
    f->data = NULL;
    if (U32(st, SPARE_SCREEN) == 0) {
        GAME_CALL(st, 0x004CE6E0u, 0x00601BA8, 0, 0); /* "ERR:no spare screen for model" */
        return 0;
    }
    char path[261];
    if (cstr(st, U32(st, names + 4u * U32(st, DIFFICULTY)), path, sizeof(path)) != 0) {
        state_unported(st, 0x004CEB60u);
        return 0;
    }
    int32_t n = platform_file_read(st, path, 0, NULL, 0);
    if (n < 0) {
        GAME_CALL(st, 0x004CE6E0u, (int32_t) no_file, 0, 0);
        return 0;
    }
    if ((uint32_t) n > BUFFER_SIZE) {
        state_unported(st, 0x004CEB60u); /* the original writes past its buffer */
        return 0;
    }
    f->data = calloc(BUFFER_SIZE, 1);
    if (!f->data) {
        state_unported(st, 0x004CEB60u);
        return 0;
    }
    if (n > 0) {
        platform_file_read(st, path, 0, f->data, (uint32_t) n);
    }
    f->b.p = f->data;
    f->b.size = BUFFER_SIZE;
    f->b.overrun = 0;
    f->size = file_length(st, U32(st, names + 4u * U32(st, DIFFICULTY)));
    return 1;
}

static int model_close(game_state *st, model_file *f, uint32_t address)
{
    int bad = f->b.overrun;
    free(f->data);
    f->data = NULL;
    if (bad) {
        state_unported(st, address); /* a scan went past the work buffer */
    }
    return !bad;
}

/* @bin 0x00540D35 the records' '{' counted from `pos` (at most 0x12B); a count other than
 * `expected` asks the player with a Win32 message box whether to go on ("... files out of sync",
 * IDNO: not loaded) */
static int model_count(game_state *st, model_file *f, uint32_t pos, int32_t expected, uint32_t address)
{
    int32_t count = 0;
    for (int32_t left = 0x12B; left > 0; left--) {
        int32_t r = text_find_char_at(&f->b, pos, '{', f->size);
        if (r == 0) {
            break;
        }
        count++;
        pos += (uint32_t) r;
    }
    if (count != expected) {
        /* the MessageBoxA (import 0x0056F1DC) and its answer are the platform's: not reproduced */
        state_unported(st, address);
        return 0;
    }
    return 1;
}

/* @bin 0x00540DEB records of `fields` numbers: '{', then each number (0x004CC0E0, 0x004CC070)
 * followed by its ','; at most 0x12B records and up to the table's end */
static uint32_t model_records(game_state *st, model_file *f, uint32_t pos, uint32_t table, uint32_t end,
                              int32_t fields)
{
    for (int32_t left = 0x12B; left > 0 && table < end; left--) {
        pos += (uint32_t) text_find_char_at(&f->b, pos, '{', f->size);
        for (int32_t k = 0; k < fields; k++) {
            pos += (uint32_t) text_skip_to_number_at(&f->b, pos);
            S32(st, table) = text_atoi_at(st, &f->b, pos);
            pos += (uint32_t) text_find_char_at(&f->b, pos, ',', f->size);
            table += 4;
        }
    }
    return pos;
}

int32_t model_buildings_load(game_state *st)
{
    /* @bin 0x00540C90 Pharaoh_Model_<level>.txt (names 0x00601908) */
    model_file f;
    if (!model_open(st, &f, 0x00601908u, 0x00601B88u)) {
        return 0;
    }
    uint32_t pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL BUILDINGS", f.size);
    if (!model_count(st, &f, pos, 0x101, 0x00540C90u)) {
        model_close(st, &f, 0x00540C90u);
        return 0;
    }
    pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL BUILDINGS", f.size);
    model_records(st, &f, pos, 0x00705EA4u, 0x007083ACu, 10);
    pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL HOUSES", f.size); /* @bin 0x00540E37 */
    model_records(st, &f, pos, 0x0070CDCCu, 0x0070D4ACu, 0x16);
    if (!model_close(st, &f, 0x00540C90u)) {
        return 0;
    }
    GAME_CALL(st, 0x004CE6E0u, 0x00601AF4, 0, 0); /* " OK: model loaded" */
    return 1;
}

int32_t model_figures_load(game_state *st)
{
    /* @bin 0x00540ED0 Figure_model_<level>.txt (names 0x0060191C) */
    model_file f;
    if (!model_open(st, &f, 0x0060191Cu, 0x00601C60u)) {
        return 0;
    }
    uint32_t pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL FIGURES", f.size);
    if (!model_count(st, &f, pos, 0xAA, 0x00540ED0u)) {
        model_close(st, &f, 0x00540ED0u);
        return 0;
    }
    pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL FIGURES", f.size);
    model_records(st, &f, pos, 0x007090CCu, 0x0070A8DCu, 0xE);
    /* @bin 0x0054107A the enemies: rows j + k (k 0..4, j by 5) while j < 60, 14 numbers each */
    pos = (uint32_t) text_find_string_at(&f.b, 0, "ALL ENEMIES", f.size);
    int32_t j = 0, k = 0;
    for (int32_t left = 0x12B; left > 0 && j < 0x3C; left--) {
        pos += (uint32_t) text_find_char_at(&f.b, pos, '{', f.size);
        uint32_t table = 0x007083ACu + 56u * (uint32_t) (j + k);
        for (int32_t n = 0; n < 0xE; n++) {
            pos += (uint32_t) text_skip_to_number_at(&f.b, pos);
            S32(st, table) = text_atoi_at(st, &f.b, pos);
            pos += (uint32_t) text_find_char_at(&f.b, pos, ',', f.size);
            table += 4;
        }
        if (++k >= 5) {
            k = 0;
            j += 5;
        }
    }
    if (!model_close(st, &f, 0x00540ED0u)) {
        return 0;
    }
    GAME_CALL(st, 0x004CE6E0u, 0x00601BC8, 0, 0); /* " OK: figure model data loaded" */
    return 1;
}

int32_t model_tax_load(game_state *st)
{
    /* @bin 0x00541160 Tax_Sentiment_Model_<level>.txt (names 0x00601930), from the file's start */
    model_file f;
    if (!model_open(st, &f, 0x00601930u, 0x00601C60u)) {
        return 0;
    }
    if (!model_count(st, &f, 0, 0x1A, 0x00541160u)) {
        model_close(st, &f, 0x00541160u);
        return 0;
    }
    model_records(st, &f, 0, 0x00EA1384u, 0x00EA17FCu, 0xB);
    if (!model_close(st, &f, 0x00541160u)) {
        return 0;
    }
    GAME_CALL(st, 0x004CE6E0u, 0x00601C80, 0, 0); /* " OK: tax sentiment data loaded" */
    return 1;
}

void enemy_models_select(game_state *st)
{
    /* @bin 0x004BC720 the scenario's enemy (i16 0x007848FC, at most 11, stored back): its graphics
     * (0x005DEF84[enemy] into slot 2) and its 5 records of the enemy models (0x007083AC, 5 per
     * enemy) copied to the figure models 0x00709A6C, 0x00709A34, 0x00709AA4, 0x0070A4EC,
     * 0x0070A524 */
    int32_t e = S16(st, 0x007848FCu);
    if (e >= 0xC) {
        e = 0xB;
        S16(st, 0x007848FCu) = (int16_t) e;
    }
    GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, S32(st, 0x005DEF84u + 4u * (uint32_t) e), 2);
    uint32_t src = 0x007083ACu + 280u * (uint32_t) e;
    for (uint32_t i = 0; i < 0x38; i += 4) {
        uint32_t d = 0x00709A34u + i;
        S32(st, d + 0x38) = S32(st, src + i);
        S32(st, d) = S32(st, src + 0x38 + i);
        S32(st, d + 0x70) = S32(st, src + 0x70 + i);
        S32(st, d + 0xAF0) = S32(st, src + 0xA8 + i);
        S32(st, d + 0xAB8) = S32(st, src + 0xE0 + i);
    }
}

static int32_t run_buildings(game_state *st, const int32_t *a)
{
    (void) a;
    return model_buildings_load(st);
}

static int32_t run_figures(game_state *st, const int32_t *a)
{
    (void) a;
    return model_figures_load(st);
}

static int32_t run_tax(game_state *st, const int32_t *a)
{
    (void) a;
    return model_tax_load(st);
}

static int32_t run_enemy(game_state *st, const int32_t *a)
{
    (void) a;
    enemy_models_select(st);
    return 0;
}

const ported_function models_ports[] = {
    /* a file whose record count is wrong reaches a Win32 message box: state_unported (the reference
     * files have the expected counts) */
    {0x00540C90u, "model_buildings_load", 0, 32, 0, run_buildings, 0},
    {0x00540ED0u, "model_figures_load", 0, 32, 0, run_figures, 0},
    {0x00541160u, "model_tax_load", 0, 32, 0, run_tax, 0},
    {0x004BC720u, "enemy_models_select", 0, 0, 0, run_enemy, 0},
    {0x004CBDD0u, "text_find_string", 3, 32, 0, run_find_string, 0},
    {0x004CBE30u, "text_find_char", 3, 32, 0, run_find_char, 0},
    {0x004CBE60u, "text_compare", 3, 32, 0, run_compare, 0},
    {0x004CC070u, "text_atoi", 1, 32, 0, run_atoi, 0},
    {0x004CC0E0u, "text_skip_to_number", 1, 32, 0, run_skip, 0},
    {0x004CED50u, "file_length", 1, 32, 0, run_file_length, 0},
};
const int models_ports_count = sizeof(models_ports) / sizeof(models_ports[0]);
