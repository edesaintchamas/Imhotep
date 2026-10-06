/* Imhotep - the campaign flow around the start of a city (package flow, docs/newgame.md section 2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The campaign table (campaign.txt, 0x0041AE40), the family's progress file (0x00516A20,
 * 0x0041B8A0), the step order (0x0041B920), the briefing (0x004181E0), the main menu (0x004CB6A0)
 * and the scenario / mission list object 0x00704C00. Files go through the platform
 * (core/sim/platform.h): the CRT streams of the original (fopen, fgets, fwrite, fclose) become
 * whole-file reads and writes. */
#include "newgame/flow.h"
#include "newgame/flow2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define PLAYER_NAME 0x00E38E74u   /* char[32] */
#define STR_SAVE_DIR 0x005E0494u  /* "save\\" */
#define STR_DAT 0x005F80E8u       /* ".dat" */
#define STR_SAV_PATTERN 0x005761C4u /* "\\*.sav" */
#define PROGRESS_PATH 0x00EADDA0u /* char[0x104] */
#define FAMILY_DIR 0x00EADEA4u
#define LAST_SAVE 0x00700998u     /* the last saved game's path (empty: none) */
#define SCREEN 0x00E38DDAu
#define REDRAW 0x00E38DCAu
#define CUSTOM 0x00E38EBCu
#define MISSION 0x00E38EA8u
#define STEP 0x00E38EABu
#define LIST_OBJECT 0x00704C00u
#define LIST_MODE 0x00704DB0u     /* 0: .map files, 1: the missions of the campaign table */
#define STEPS 0x1900u             /* table + : i32 step count */
#define NAMES 0x1904u             /* table + : char[0x32] mission names */
#define NAME_COUNT 0x2C8Cu        /* table + : i32 */
#define LOG_LINE 0x00E36C90u      /* the log line buffer (0x004CE6E0; presentation, excluded.py) */

/* ---- strings of the state ---- */

static const char *state_str(game_state *st, uint32_t a)
{
    const char *s = (const char *) state_ptr(st, a, 1);
    for (uint32_t n = 0; s && n < 4096; n++) {
        const uint8_t *c = state_ptr(st, a + n, 1);
        if (!c) {
            return NULL;
        }
        if (*c == 0) {
            return s;
        }
    }
    return NULL;
}

/* strcpy / strcat between state strings (the original's inline repne scasb / rep movs: exactly
 * strlen + 1 bytes) */
static void str_copy(game_state *st, uint32_t dst, uint32_t src)
{
    uint8_t c;
    do {
        c = P8(st, src++);
        P8(st, dst++) = c;
    } while (c != 0);
}

static void str_cat(game_state *st, uint32_t dst, uint32_t src)
{
    while (P8(st, dst) != 0) {
        dst++;
    }
    str_copy(st, dst, src);
}

/* a C copy of state strings, concatenated */
static void cat_c(char *out, size_t cap, game_state *st, uint32_t a)
{
    const char *s = state_str(st, a);
    size_t n = strlen(out);
    if (s) {
        snprintf(out + n, cap - n, "%s", s);
    }
}

/* ---- the progress file ---- */

uint32_t campaign_progress_path(game_state *st)
{
    /* @bin 0x00516A20 */
    str_copy(st, PROGRESS_PATH, STR_SAVE_DIR);
    str_cat(st, PROGRESS_PATH, PLAYER_NAME);
    str_cat(st, PROGRESS_PATH, STR_DAT);
    return PROGRESS_PATH;
}

uint32_t family_dir_path(game_state *st)
{
    /* @bin 0x005169C0 */
    str_copy(st, FAMILY_DIR, STR_SAVE_DIR);
    str_cat(st, FAMILY_DIR, PLAYER_NAME);
    return FAMILY_DIR;
}

int32_t campaign_progress_write(game_state *st, uint32_t self, uint32_t path)
{
    /* @bin 0x0041B8A0 fopen(path, "wb"): 0 when it fails; fwrite(this, 0x4A9C, 1) == 1; fclose */
    const char *p = state_str(st, path);
    const uint8_t *data = state_ptr(st, self, CAMPAIGN_TABLE_SIZE);
    if (!p || !data) {
        return 0;
    }
    return platform_file_write(st, p, data, CAMPAIGN_TABLE_SIZE) == (int32_t) CAMPAIGN_TABLE_SIZE ? 1 : 0;
}

/* ---- the step order ---- */

int32_t campaign_next_step(game_state *st, uint32_t self, int32_t period, int32_t step, int32_t choice)
{
    /* @bin 0x0041B920 the path id of `step` (its choice's path when it has choices, +0x2A + 12 *
     * choice; else +1); from the start: 0 */
    uint32_t want;
    if (step < 0) {
        step = -1;
        want = 0;
    } else {
        if (step >= S32(st, self + STEPS)) {
            return -1;
        }
        uint32_t e = self + ((uint32_t) step << 6);
        if (S32(st, e + 0x38) > 0) {
            want = P8(st, e + 12u * (uint32_t) choice + 0x2A);
        } else {
            want = P8(st, e + 1);
        }
    }
    /* @bin 0x0041B962 the next step of the period whose path (+1) is 0 (any) or that path, or
     * that merges it (+7 ids at +2) */
    int32_t n = S32(st, self + STEPS);
    for (int32_t i = step + 1; i < n; i++) {
        uint32_t e = self + ((uint32_t) i << 6);
        if ((int32_t) P8(st, e) != period) {
            continue;
        }
        uint32_t c = P8(st, e + 1);
        if (c == 0 || c == want) {
            return i;
        }
        uint32_t m = P8(st, e + 7);
        for (uint32_t k = 0; k < m; k++) {
            if (P8(st, e + 2 + k) == want) {
                return i;
            }
        }
    }
    return -1;
}

int32_t campaign_period_done(game_state *st, uint32_t self, int32_t period)
{
    /* @bin 0x0041BA20 */
    if (period >= 5) {
        return 1;
    }
    for (int32_t i = 0; i < S32(st, self + STEPS); i++) {
        uint32_t e = self + ((uint32_t) i << 6);
        if ((int32_t) P8(st, e) == period && P8(st, e + 0x3C) != 0 &&
            campaign_next_step(st, self, period, i, 0) < 0) {
            return 1;
        }
    }
    return 0;
}

int32_t campaign_any_period_done(game_state *st, uint32_t self)
{
    /* @bin 0x0041BAB0 periods 0..8 (always 1: periods >= 5 count as done) */
    for (int32_t p = 0; p < 9; p++) {
        if (campaign_period_done(st, self, p)) {
            return 1;
        }
    }
    return 0;
}

/* ---- campaign.txt ---- */

/* @bin 0x0041AC00 a step's defaults (64 bytes): period 0xFF, path 0, merges 0, ids -1, two choices
 * (x, y := 50 / 100, text 0xFFFF, path 0), no choice, not won. Bytes +2..+6, +0x2B, +0x37 and
 * +0x3D..+0x3F are left as they are. */
static void entry_init(uint8_t *e)
{
    e[0] = 0xFF;
    e[1] = 0;
    for (int k = 0; k < 6; k++) {
        memset(e + 8 + 4 * k, 0xFF, 4);
    }
    e[7] = 0;
    for (int k = 0; k < 2; k++) {
        uint32_t v = 0x32u * (uint32_t) (k + 1);
        uint8_t *c = e + 0x20 + 12 * k;
        for (int b = 0; b < 4; b++) {
            c[b] = (uint8_t) (v >> (8 * b));
            c[4 + b] = (uint8_t) (v >> (8 * b));
        }
        c[8] = 0xFF;
        c[9] = 0xFF;
        c[10] = 0;
    }
    memset(e + 0x38, 0, 4);
    e[0x3C] = 0;
}

static int32_t rd32(const uint8_t *p)
{
    return (int32_t) ((uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24);
}

static void wr32(uint8_t *p, int32_t v)
{
    for (int b = 0; b < 4; b++) {
        p[b] = (uint8_t) ((uint32_t) v >> (8 * b));
    }
}

/* @bin 0x00561DB1 CRT strtok: the next token of `s` (NULL: of the previous string) between the
 * delimiter bytes; NULL when none. The CRT keeps its position in 0x012663C8 (a pointer into the
 * caller's buffer): here the parser's own `next`. */
static char *tok(char **next, char *s, const char *delim)
{
    char *p = s ? s : *next;
    while (*p && strchr(delim, *p)) {
        p++;
    }
    char *start = p;
    while (*p && !strchr(delim, *p)) {
        p++;
    }
    if (*p) {
        *p++ = 0;
    }
    *next = p;
    return start == p ? NULL : start;
}

/* @bin 0x0041AD50 the next token as a number into *out: 1; 0 when there is no token; 2 when it is
 * not '-'? followed by digits (0x00561D89 isdigit); the value is the CRT atoi (0x00561C96: wraps) */
static int parse_int(char **next, const char *delim, int32_t *out)
{
    char *t = tok(next, NULL, delim);
    if (!t) {
        return 0;
    }
    const char *p = t + (*t == '-');
    for (const char *q = p; *q; q++) {
        if (*q < '0' || *q > '9') {
            return 2;
        }
    }
    uint32_t v = 0;
    for (; *p; p++) {
        v = v * 10u + (uint32_t) (*p - '0');
    }
    *out = (int32_t) (*t == '-' ? 0u - v : v);
    return 1;
}

/* @bin 0x0562370 _stricmp (no locale: ASCII letters only) */
static int str_ieq(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = (unsigned char) *a, y = (unsigned char) *b;
        x = x >= 'A' && x <= 'Z' ? x + 0x20 : x;
        y = y >= 'A' && y <= 'Z' ? y + 0x20 : y;
        if (x != y) {
            return 0;
        }
        if (x == 0) {
            return 1;
        }
    }
}

/* a C string into the state's log line buffer (presentation state, excluded.py), at most 0x7F
 * bytes: the strings the original passes from its own frame to 0x0041ADF0 */
static void stage(game_state *st, uint32_t at, const char *s)
{
    size_t n = strlen(s);
    if (n > 0x7F) {
        n = 0x7F;
    }
    memcpy(st->mem + (at - STATE_BASE), s, n);
    U8(st, at + n) = 0;
}

/* @bin 0x0041ADF0 a line of the campaign file that was not understood, to the game's log: text is the
 * line (the original's frame: staged in the log buffer) or a string of the state (text_addr) */
static void parse_error(game_state *st, uint32_t file, int32_t line, const char *text, uint32_t text_addr,
                        uint32_t msg)
{
    if (text) {
        stage(st, LOG_LINE, text);
        text_addr = LOG_LINE;
    }
    GAME_CALL(st, 0x0041ADF0u, (int32_t) file, line, (int32_t) text_addr, (int32_t) msg);
}

/* @bin 0x0041ADF0 sprintf(buffer, "ERR: %s line %d: %s %s" (0x0057C04C), ...) and the game's log
 * 0x004CE6E0 (buffer, 0, 0): a log line only, the platform's (as platform_log) */
static int32_t run_error_line(game_state *st, const int32_t *a)
{
    const sim_platform *p = st->platform;
    const char *file = state_str(st, (uint32_t) a[0]);
    const char *text = state_str(st, (uint32_t) a[2]);
    const char *msg = state_str(st, (uint32_t) a[3]);
    if (p && p->log) {
        char buf[300];
        snprintf(buf, sizeof(buf), "ERR: %s line %d: %s %s", file ? file : "", a[1], text ? text : "",
                 msg ? msg : "");
        p->log(p->ctx, buf, NULL, 0);
    }
    return 0;
}

/* the section names and keywords of the file, in the order of the original's table */
static const uint32_t KEYWORDS[13] = {0x0057C270u, 0x0057C258u, 0x0057C244u, 0x0057C234u, 0x0057C220u,
                                      0x0057C210u, 0x0057C1F8u, 0x0057C1ECu, 0x0057C1D4u, 0x0057C1BCu,
                                      0x0057C1B4u, 0x0057C1A4u, 0x0057C19Cu};

/* @bin 0x0041AC90 (thiscall) append a step: < 100 steps and period < 9 */
static int add_step(game_state *st, uint32_t self, const uint8_t *e)
{
    int32_t n = S32(st, self + STEPS);
    if (n >= 100 || e[0] >= 9) {
        return 0;
    }
    memcpy(state_ptr(st, self + ((uint32_t) n << 6), 0x40), e, 0x40);
    S32(st, self + STEPS) = S32(st, self + STEPS) + 1;
    return 1;
}

static void reset_after_add(uint8_t *e)
{
    /* @bin 0x0041B058 */
    wr32(e + 0x38, 0);
    wr32(e + 0xC, -1);
    wr32(e + 0x8, -1);
    wr32(e + 0x10, -1);
    e[1] = 0;
    e[7] = 0;
}

int32_t campaign_load(game_state *st, uint32_t self, uint32_t path)
{
    /* @bin 0x0041AE40 the step being read: a local of the original's frame, initialised once by
     * 0x0041AC00; the bytes 0x0041AC00 leaves (+2..+6, +0x2B, +0x37, +0x3D..+0x3F) are whatever
     * the stack held, copied into the table with the step: 0 here (the check clears the stack) */
    uint8_t e[0x40] = {0};
    entry_init(e);
    const char *file = state_str(st, path);
    int32_t size = file ? platform_file_read(st, file, 0, NULL, 0) : -1;
    if (size < 0) {
        return 0; /* fopen(path, "rb") (0x005763BC) failed */
    }
    char *text = malloc((size_t) size + 1);
    if (!text) {
        return 0;
    }
    platform_file_read(st, file, 0, (uint8_t *) text, (uint32_t) size);
    /* the stream is binary ("rb"): the lines keep their CR, removed with the other blanks */
    size_t len = (size_t) size;
    const char *delim = state_str(st, U32(st, 0x0057BE28u));   /* ",=" */
    const char *delim2 = state_str(st, 0x0057BFACu);          /* ",=" */
    uint8_t names = 0;
    /* @bin 0x0041AF04 the 100 steps of the table at 0x006FDD08 (absolute), the counts, 0x41 bytes */
    for (uint32_t a = CAMPAIGN_TABLE_ADDR; a < 0x006FF608u; a += 0x40) {
        entry_init(state_ptr(st, a, 0x40));
    }
    S32(st, self + STEPS) = 0;
    S32(st, self + NAME_COUNT) = 0;
    memset(state_ptr(st, self + 0x2C90u, 0x41), 0, 0x41);
    uint8_t ok = 1;
    int32_t line = 0;
    size_t pos = 0;
    char buf[200], copy[200] = {0};
    char *next = NULL;
    for (;;) {
        /* @bin 0x00561E4D fgets(buf, 200): up to 199 bytes, through the '\n' */
        if (pos >= len) {
            break;
        }
        size_t n = 0;
        while (n < 199 && pos < len) {
            char c = text[pos++];
            buf[n++] = c;
            if (c == '\n') {
                break;
            }
        }
        buf[n] = 0;
        line++;
        memcpy(copy, buf, n + 1);
        /* @bin 0x0041AD20 every byte < 0x21 is removed */
        size_t w = 0;
        for (size_t r = 0; buf[r]; r++) {
            if ((unsigned char) buf[r] >= 0x21) {
                buf[w++] = buf[r];
            }
        }
        buf[w] = 0;
        if (buf[0] == ';' || buf[0] == 0) {
            continue;
        }
        char *t = tok(&next, buf, delim);
        if (!t) {
            continue;
        }
        int idx = 0;
        while (idx < 13 && !str_ieq(t, state_str(st, KEYWORDS[idx]))) {
            idx++;
        }
        uint32_t err = 0;
        int32_t v = 0, r;
        if (idx == 0) {
            names = 1; /* [MISSION_NAMES] */
        } else if (idx <= 9) {
            /* @bin 0x0041B01E a period: the step read so far is added */
            names = 0;
            if (rd32(e + 0xC) >= 0) {
                if (!add_step(st, self, e)) {
                    err = 0x007027D4u;
                    goto error;
                }
                reset_after_add(e);
            }
            e[0] = (uint8_t) (idx - 1);
        } else if (idx == 10) {
            /* @bin 0x0041B088 mission=id,intro,transition[,path[,merge ...]] */
            if (names) {
                err = 0x0057C188u;
                goto error;
            }
            if (rd32(e + 0xC) >= 0) {
                if (!add_step(st, self, e)) {
                    err = 0x0057C16Cu;
                    goto error;
                }
                reset_after_add(e);
            }
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C15Cu;
                goto error;
            }
            wr32(e + 0xC, v);
            if (v < 0 || v >= S32(st, self + NAME_COUNT)) {
                err = 0x0057C15Cu;
                goto error;
            }
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C14Cu;
                goto error;
            }
            wr32(e + 0x8, v);
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C134u;
                goto error;
            }
            wr32(e + 0x10, v);
            r = parse_int(&next, delim, &v);
            if (r == 2) {
                err = 0x0057C128u;
                goto error;
            }
            if (r == 0) {
                continue;
            }
            if (v < 0 || v > 0xFF) {
                err = 0x0057C128u;
                goto error;
            }
            e[1] = (uint8_t) v;
            e[7] = 0;
            for (int k = 0; k < 5; k++) {
                if (!tok(&next, NULL, delim2)) {
                    break;
                }
                r = parse_int(&next, delim, &v);
                if (r == 2 || (r == 1 && (v < 0 || v > 0xFF))) {
                    err = 0x0057C118u;
                    goto error;
                }
                if (r == 0) {
                    break;
                }
                e[2 + e[7]] = (uint8_t) v;
                e[7]++;
            }
        } else if (idx == 11) {
            /* @bin 0x0041B27C choicescreen=background,text */
            if (rd32(e + 0xC) < 0) {
                wr32(e + 0xC, 0x7F);
            }
            if (names) {
                err = 0x0057C188u;
                goto error;
            }
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C104u;
                goto error;
            }
            wr32(e + 0x1C, v);
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C0CCu;
                goto error;
            }
            wr32(e + 0x18, v);
        } else if (idx == 12) {
            /* @bin 0x0041B2F7 choice=path,x,y,text (two at most) */
            if (names) {
                err = 0x0057C188u;
                goto error;
            }
            int32_t c = rd32(e + 0x38);
            if (c >= 2) {
                err = 0x0057C0F0u;
                goto error;
            }
            if (parse_int(&next, delim, &v) != 1 || v < 0 || v > 0xFF) {
                err = 0x0057C128u;
                goto error;
            }
            uint8_t *ch = e + 0x20 + 12 * c;
            ch[0xA] = (uint8_t) v;
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C0E4u;
                goto error;
            }
            wr32(ch, v);
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C0D8u;
                goto error;
            }
            wr32(ch + 4, v);
            /* @bin 0x0041ADC0 the text id: 16 bits */
            if (parse_int(&next, delim, &v) != 1) {
                err = 0x0057C0CCu;
                goto error;
            }
            ch[8] = (uint8_t) v;
            ch[9] = (uint8_t) ((uint32_t) v >> 8);
            wr32(e + 0x38, rd32(e + 0x38) + 1);
        } else if (names) {
            /* @bin 0x0041B419 a mission name: the raw line without its trailing bytes < 0x21 */
            int32_t k = (int32_t) strlen(copy) - 1;
            while (k >= 0 && (unsigned char) copy[k] <= 0x20) {
                copy[k--] = 0;
            }
            if (k + 2 > 0x32) {
                err = 0x0057C0B4u;
                goto error;
            }
            if (k <= 0) {
                continue;
            }
            int32_t count = S32(st, self + NAME_COUNT);
            if (count >= 100) {
                err = 0x0057C09Cu;
                goto error;
            }
            memcpy(state_ptr(st, self + NAMES + 0x32u * (uint32_t) count, (uint32_t) k + 2), copy, (size_t) k + 2);
            S32(st, self + NAME_COUNT) = S32(st, self + NAME_COUNT) + 1;
        } else {
            err = 0x0057C088u; /* unrecognized line */
            goto error;
        }
        continue;
    error:
        parse_error(st, path, line, copy, 0, err);
        ok = 0;
    }
    /* @bin 0x0041B50D the last step */
    if (rd32(e + 0xC) >= 0 && !add_step(st, self, e)) {
        parse_error(st, path, line, copy, 0, 0x0057C16Cu);
        ok = 0;
    }
    /* @bin 0x0041B54B every choice of the steps with two or more must lead somewhere (the table at
     * 0x006FDD08, absolute) */
    for (int32_t i = 0; i < S32(st, self + STEPS); i++) {
        uint32_t s = CAMPAIGN_TABLE_ADDR + ((uint32_t) i << 6);
        if (S32(st, s + 0x38) <= 1) {
            continue;
        }
        for (int32_t k = 0; k < S32(st, s + 0x38); k++) {
            if (campaign_next_step(st, CAMPAIGN_TABLE_ADDR, U8(st, s), i, k) >= 0) {
                continue;
            }
            /* @bin 0x0041B5C9 sprintf(line buffer, "%s mission %d: choice %d not found" (0x0057C064), the
             * period's section name, the mission, the choice's path): the buffer is the original's
             * frame (0 here, as for the other ports of crt_sprintf); the line is staged in the log
             * buffer for 0x0041ADF0 */
            uint32_t name = KEYWORDS[U8(st, s) + 1];
            int32_t mission = S32(st, s + 0xC), path_id = U8(st, s + 0x2A + 12u * (uint32_t) k);
            GAME_CALL(st, 0x00561A36u, 0, 0x0057C064, (int32_t) name, mission, path_id);
            char msg[200];
            snprintf(msg, sizeof(msg), "%s mission %d: choice %d not found", state_str(st, name), mission, path_id);
            stage(st, LOG_LINE + 0x80, msg);
            parse_error(st, path, line, NULL, 0x007027D4u, LOG_LINE + 0x80);
            ok = 0;
        }
    }
    /* @bin 0x0041B623 0x76C dwords from +0x2CD4 := 0 */
    memset(state_ptr(st, self + 0x2CD4u, 0x76Cu * 4), 0, 0x76Cu * 4);
    free(text);
    return ok;
}

/* ---- briefing, main menu ---- */

void fade_start(game_state *st, int32_t a, int32_t next, int32_t ms)
{
    /* @bin 0x0041AA20 the fade's start time is timeGetTime(): the wall clock, an input, 0 in core as
     * in the oracle (docs/porting.md 3) */
    uint32_t now = 0;
    S32(st, 0x006FDCB0u) = a;
    U32(st, 0x006FDCACu) = now;
    U32(st, 0x006FDCA8u) = now + (uint32_t) ms;
    S32(st, 0x006FDCA4u) = next;
    U8(st, SCREEN) = 4;
    U8(st, REDRAW) = 1;
}

int32_t briefing_text_id(game_state *st)
{
    /* @bin 0x00418140 a campaign step's intro id (+8; with the option byte 0x00E39088: + 200 below
     * 0x1B7, else + 15); otherwise 200 + mission (200 for none, 0x7F) */
    if (S32(st, CUSTOM) == 0) {
        int32_t s = S8(st, STEP);
        if (s >= 0 && s < S32(st, 0x006FF608u)) {
            int32_t v = S32(st, CAMPAIGN_TABLE_ADDR + ((uint32_t) s << 6) + 8);
            if (U8(st, 0x00E39088u) == 0) {
                return v;
            }
            return v < 0x1B7 ? v + 0xC8 : v + 0xF;
        }
    }
    if (S8(st, MISSION) == 0x7F) {
        return 0xC8;
    }
    return S8(st, MISSION) + 0xC8;
}

void briefing_open(game_state *st)
{
    /* @bin 0x004181E0 */
    fade_start(st, 0, 0x35, 1000);
    U32(st, 0x007027A8u) = 0;
    U32(st, 0x00D3B130u) = 0;
    U32(st, 0x00D3B138u) = 0;
    S32(st, 0x00D3B124u) = briefing_text_id(st);
}

void main_menu_open(game_state *st)
{
    /* @bin 0x004CB6A0 continue (a campaign period done), Cleopatra, load (a saved game in the
     * family's directory), the last save (0x00700998 opens) */
    U8(st, 0x00D3B189u) = (uint8_t) campaign_any_period_done(st, CAMPAIGN_TABLE_ADDR);
    U8(st, 0x00D3B188u) = 1;
    U8(st, 0x00D3B18Bu) = 0;
    U8(st, 0x00D3B18Au) = 0;
    if (U8(st, LAST_SAVE) != 0) {
        const char *p = state_str(st, LAST_SAVE);
        if (p && platform_file_read(st, p, 0, NULL, 0) >= 0) { /* fopen "rb", fclose */
            U8(st, 0x00D3B18Bu) = 1;
        }
    }
    /* @bin 0x004CB711 a file list on the stack (0x0041C6D0 .. 0x0041C710, flow2.c file_list_count):
     * "save\" + name + "\*.sav" (0x005761C4); any file found sets 0x00D3B18A */
    char pattern[300] = "";
    cat_c(pattern, sizeof(pattern), st, STR_SAVE_DIR);
    cat_c(pattern, sizeof(pattern), st, PLAYER_NAME);
    cat_c(pattern, sizeof(pattern), st, STR_SAV_PATTERN);
    if (file_list_count(st, pattern) > 0) {
        U8(st, 0x00D3B18Au) = 1;
    }
    U8(st, SCREEN) = 0x30;
    U8(st, REDRAW) = 1;
}

/* ---- the scenario / mission list object 0x00704C00 ---- */

int32_t list_row_kind(game_state *st, uint32_t self, int32_t row)
{
    /* @bin 0x0041DFE0 0 a .map, 1 a mission, 3 no such row */
    if (row >= 0 && row < S32(st, self + 8)) {
        return S32(st, LIST_MODE) != 0;
    }
    return 3;
}

uint32_t list_row_name(game_state *st, uint32_t self, int32_t row)
{
    /* @bin 0x0041E010 the file name (offset +4[row] in the name buffer +0x10), or the mission's
     * name in the campaign table (0x006FF60C + 0x32 * [+0x1C + 4 row]); 0 for no such row */
    if (row < 0 || row >= S32(st, self + 8)) {
        return 0;
    }
    if (S32(st, LIST_MODE) == 0) {
        return P32(st, P32(st, self + 4) + 4u * (uint32_t) row) + P32(st, self + 0x10);
    }
    return 0x006FF60Cu + 0x32u * U32(st, self + 0x1C + 4u * (uint32_t) row);
}

int32_t list_row_mission(game_state *st, uint32_t self, int32_t row)
{
    /* @bin 0x0041DFC0 */
    if (row >= 0 && row < S32(st, self + 8)) {
        return S32(st, self + 0x1C + 4u * (uint32_t) row);
    }
    return -1;
}

int32_t list_fill(game_state *st, uint32_t self, uint32_t pattern, int32_t keep)
{
    /* @bin 0x0041DF70 files (mode 0: 0x0041C740) or the [0x00700994] missions, in order */
    if (S32(st, LIST_MODE) == 0) {
        return GAME_CALLT(st, self, 0x0041C740u, (int32_t) pattern, keep);
    }
    S32(st, self + 8) = 0;
    int32_t i = 0;
    for (; i < S32(st, 0x00700994u); i++) {
        S32(st, self + 0x1C + 4u * (uint32_t) S32(st, self + 8)) = i;
        S32(st, self + 8) = S32(st, self + 8) + 1;
    }
    return (int32_t) (((uint32_t) i & 0xFFFFFF00u) | 1u);
}

void list_load(game_state *st, uint32_t pattern)
{
    /* @bin 0x0041F810 */
    list_fill(st, LIST_OBJECT, pattern, 0);
    S32(st, 0x00704DB4u) = S32(st, LIST_OBJECT + 8);
    if (S32(st, LIST_MODE) != 0) {
        return;
    }
    /* the file names without their extension (0x004CE3A0), sorted with _stricmp (0x0041CA10) */
    for (int32_t i = 0; i < S32(st, 0x00704DB4u); i++) {
        uint32_t name = list_row_name(st, LIST_OBJECT, i);
        GAME_CALL(st, 0x004CE3A0u, (int32_t) name);
    }
    GAME_CALLT(st, LIST_OBJECT, 0x0041CA10u, 0x00562370);
}

void list_open(game_state *st)
{
    /* @bin 0x0041F870 screen 0x3D, the list ("maps\*.map", 0x00576260), the first row clicked */
    U8(st, SCREEN) = 0x3D;
    list_load(st, 0x00576260u);
    U8(st, 0x00D3B1B3u) = 0;
    S32(st, 0x00704DB8u) = 0;
    U8(st, 0x00D3B1C0u) = 1;
    S32(st, 0x00D3B198u) = S32(st, 0x00704DBCu);
    GAME_CALL0(st, 0x0041F670u);
    U8(st, REDRAW) = 1;
}

/* ---- the table ---- */

static int32_t run_progress_path(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) campaign_progress_path(st);
}

static int32_t run_family_dir(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) family_dir_path(st);
}

static int32_t run_progress_write(game_state *st, const int32_t *a)
{
    return campaign_progress_write(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]);
}

static int32_t run_next_step(game_state *st, const int32_t *a)
{
    return campaign_next_step(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
}

static int32_t run_period_done(game_state *st, const int32_t *a)
{
    return campaign_period_done(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_any_period_done(game_state *st, const int32_t *a)
{
    return campaign_any_period_done(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_campaign_load(game_state *st, const int32_t *a)
{
    return campaign_load(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0]);
}

static int32_t run_entry_init(game_state *st, const int32_t *a)
{
    uint8_t *e = state_ptr(st, (uint32_t) a[PORT_THIS_ARG], 0x40);
    if (!e) {
        state_unported(st, 0x0041AC00u); /* a step on the caller's stack */
        return 0;
    }
    entry_init(e);
    return 0;
}

static int32_t run_fade(game_state *st, const int32_t *a)
{
    fade_start(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_text_id(game_state *st, const int32_t *a)
{
    (void) a;
    return briefing_text_id(st);
}

static int32_t run_briefing(game_state *st, const int32_t *a)
{
    (void) a;
    briefing_open(st);
    return 0;
}

static int32_t run_main_menu(game_state *st, const int32_t *a)
{
    (void) a;
    main_menu_open(st);
    return 0;
}

static int32_t run_row_kind(game_state *st, const int32_t *a)
{
    return list_row_kind(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_row_name(game_state *st, const int32_t *a)
{
    return (int32_t) list_row_name(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_row_mission(game_state *st, const int32_t *a)
{
    return list_row_mission(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_list_fill(game_state *st, const int32_t *a)
{
    return list_fill(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0], a[1]);
}

static int32_t run_list_load(game_state *st, const int32_t *a)
{
    list_load(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_list_open(game_state *st, const int32_t *a)
{
    (void) a;
    list_open(st);
    return 0;
}

const ported_function newgame_flow_ports[] = {
    {0x00516A20u, "campaign_progress_path", 0, 32, 0, run_progress_path, 0},
    {0x005169C0u, "family_dir_path", 0, 32, 0, run_family_dir, 0},
    /* the whole function is the file write (fopen "wb", fwrite, fclose): a platform request */
    {0x0041B8A0u, "campaign_progress_write (platform_file_write)", 1, 8, CAMPAIGN_TABLE_ADDR, run_progress_write,
     PORT_PLATFORM},
    {0x0041B920u, "campaign_next_step", 3, 32, CAMPAIGN_TABLE_ADDR, run_next_step, 0},
    {0x0041BA20u, "campaign_period_done", 1, 8, CAMPAIGN_TABLE_ADDR, run_period_done, 0},
    {0x0041BAB0u, "campaign_any_period_done", 0, 8, CAMPAIGN_TABLE_ADDR, run_any_period_done, 0},
    {0x0041AE40u, "campaign_load (campaign.txt)", 1, 8, CAMPAIGN_TABLE_ADDR, run_campaign_load, 0},
    {0x0041AC00u, "campaign_entry_init", 0, 0, 0, run_entry_init, 0},
    {0x0041AA20u, "fade_start", 3, 0, 0, run_fade, 0},
    {0x00418140u, "briefing_text_id", 0, 32, 0, run_text_id, 0},
    {0x004181E0u, "briefing_open", 0, 0, 0, run_briefing, 0},
    {0x004CB6A0u, "main_menu_open", 0, 0, 0, run_main_menu, 0},
    {0x0041DFE0u, "list_row_kind", 1, 32, LIST_OBJECT, run_row_kind, 0},
    {0x0041E010u, "list_row_name", 1, 32, LIST_OBJECT, run_row_name, 0},
    {0x0041DFC0u, "list_row_mission", 1, 32, LIST_OBJECT, run_row_mission, 0},
    {0x0041DF70u, "list_fill", 2, 8, LIST_OBJECT, run_list_fill, 0},
    {0x0041F810u, "list_load", 1, 0, 0, run_list_load, 0},
    {0x0041F870u, "list_open", 0, 0, 0, run_list_open, 0},
    /* inside campaign_load, on its own frame (stack arguments) */
    {0x0041AC90u, "campaign add step (campaign_load)", 1, 8, 0, NULL, 0},
    {0x0041AD20u, "campaign line blanks removed (campaign_load)", 1, 0, 0, NULL, 0},
    {0x0041AD50u, "campaign number token (campaign_load)", 1, 32, 0, NULL, 0},
    {0x0041ADC0u, "campaign 16-bit token (campaign_load)", 1, 32, 0, NULL, 0},
    /* a log line only (sprintf, 0x004CE6E0): a platform request */
    {0x0041ADF0u, "campaign error line (platform log)", 4, 0, 0, run_error_line, PORT_PLATFORM},
    /* the CRT streams and string functions these use, as platform reads and writes and C code */
    {0x00561A0Au, "CRT fopen (platform_file_read / platform_file_write)", 2, 32, 0, NULL, 0},
    {0x00561E4Du, "CRT fgets (campaign_load)", 3, 32, 0, NULL, 0},
    {0x00561994u, "CRT fclose (platform)", 1, 32, 0, NULL, 0},
    {0x00561F5Fu, "CRT fwrite (platform_file_write)", 4, 32, 0, NULL, 0},
    {0x00561DB1u, "CRT strtok (campaign_load)", 2, 32, 0, NULL, 0},
    {0x00561C96u, "CRT atoi (campaign_load)", 1, 32, 0, NULL, 0},
    {0x00561C0Bu, "CRT atol (campaign_load)", 1, 32, 0, NULL, 0},
    {0x00561D89u, "CRT isdigit (campaign_load)", 1, 32, 0, NULL, 0},
};
const int newgame_flow_ports_count = sizeof(newgame_flow_ports) / sizeof(newgame_flow_ports[0]);
