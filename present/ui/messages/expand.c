/* Imhotep - message texts: "[keyword]" expansion (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A message text of Pharaoh_Text.eng may hold "[keyword]" fields; each is replaced by the string a
 * handler of the keyword table returns (the city's name, an amount, a god, ...). The handlers read
 * the current message 0x00E951E4; they are game functions called through GAME_CALL. */
#include "messages.h"

#include "draw/mem.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


/* @bin 0x0056B240 CRT _strnicmp: n bytes compared with A-Z folded to a-z (no stop at a NUL); 0 when
 * equal. The locale path ([0x0126648C] != 0, 0x00566682) is not ported. */
static int32_t strnicmp_c(game_state *st, uint32_t a, uint32_t b, uint32_t n)
{
    if (n == 0) {
        return 0;
    }
    if (U32(st, G_LC_CTYPE) != 0) {
        state_unported(st, 0x00566682u);
        return 0;
    }
    do {
        uint8_t ca = mem_rd8(st, a++);
        uint8_t cb = mem_rd8(st, b++);
        if (ca != cb) {
            if (ca >= 0x41 && ca <= 0x5A) {
                ca += 0x20;
            }
            if (cb >= 0x41 && cb <= 0x5A) {
                cb += 0x20;
            }
            if (ca != cb) {
                return ca < cb ? -1 : 1;
            }
        }
    } while (--n != 0);
    return 0;
}

static int32_t expand_n(game_state *st, uint32_t self, uint32_t text, char *out, int32_t size, int32_t *written)
{
    /* @bin 0x004CB980 copies text into out; "[name]" is looked up in the table [self] (entries of 0x0C
     * bytes: +4 name, +8 handler; prefix compare of the name's length in the text) and replaced by
     * the handler's string. No terminator is written when out fills up or a field is not closed or
     * not found (returns 0); the text's NUL is copied (returns 1). */
    int32_t i = 0;
    int32_t n = 0;
    for (;;) {
        uint8_t c = mem_rd8(st, text + (uint32_t) i);
        while (c != 0x5B) {
            if (n >= size) {
                *written = n;
                return 0;
            }
            out[n++] = (char) c;
            if (mem_rd8(st, text + (uint32_t) i) == 0) {
                *written = n;
                return 1;
            }
            c = mem_rd8(st, text + (uint32_t) i + 1);
            i++;
        }
        int32_t start = i + 1;
        c = mem_rd8(st, text + (uint32_t) i);
        while (c != 0x5D) {
            if (c == 0) {
                *written = n;
                return 0;
            }
            c = mem_rd8(st, text + (uint32_t) i + 1);
            i++;
        }
        uint32_t e = U32(st, self);
        for (;;) {
            uint32_t name = mem_rd32(st, e + 4);
            if (name == 0) {
                *written = n;
                return 0;
            }
            /* the comparison through sim_call: _strnicmp is a partial port, which the replay takes
             * as one recorded unit wherever the original calls it */
            if (GAME_CALL(st, FN_CRT_STRNICMP, (int32_t) name, (int32_t) (text + (uint32_t) start), i - start) == 0) {
                break;
            }
            e += 0x0C;
        }
        uint32_t saved = U32(st, G_KEYWORD_CURRENT);
        U32(st, G_KEYWORD_CURRENT) = e;
        uint32_t s = (uint32_t) GAME_CALL0(st, mem_rd32(st, e + 8));
        U32(st, G_KEYWORD_CURRENT) = saved;
        if (s != 0) {
            uint8_t ch = mem_rd8(st, s);
            while (ch != 0) {
                if (n >= size) {
                    *written = n;
                    return 0;
                }
                out[n++] = (char) mem_rd8(st, s);
                s++;
                ch = mem_rd8(st, s);
            }
        }
        i++;
    }
}

int32_t text_expand(game_state *st, uint32_t self, uint32_t text, char *out, int32_t size)
{
    int32_t written = 0;
    return expand_n(st, self, text, out, size, &written);
}

static uint32_t strings_entry(game_state *st, uint32_t self, int32_t index)
{
    /* @bin 0x004CBDB0 entry `index` of the string list (+4 pointers, +0x0C count), 0 out of range */
    if (index >= 0 && index < S32(st, self + 0x0C)) {
        return mem_rd32(st, U32(st, self + 4) + 4u * (uint32_t) index);
    }
    return 0;
}

static void message_text_expand_n(game_state *st, int32_t index, char *out, int32_t size, int32_t *written)
{
    /* @bin 0x00448330 out[0] := 0, then the expansion of string `index` (if any) */
    *written = 0;
    if (size < 1) {
        return;
    }
    out[0] = 0;
    *written = 1;
    uint32_t t = strings_entry(st, G_STRINGS_THIS, index);
    if (t != 0) {
        int32_t n = 0;
        expand_n(st, G_KEYWORDS_THIS, t, out, size, &n);
        if (n > *written) {
            *written = n;
        }
    }
}

void message_text_expand(game_state *st, int32_t index, char *out, int32_t size)
{
    int32_t written;
    message_text_expand_n(st, index, out, size, &written);
}

/* ---- port table: the output buffer is a state address on the original's side ---- */

static void copy_out(game_state *st, uint32_t dst, const char *src, int32_t n)
{
    for (int32_t k = 0; k < n; k++) {
        mem_wr8(st, dst + (uint32_t) k, (uint8_t) src[k]);
    }
}

/* the wrappers' work buffers, one per nesting level: a keyword handler may expand a text of its own
 * (0x00447B10 "[reason_phrase]" calls 0x00448330) while its caller's expansion is in progress */
#define EXPAND_DEPTH 4
#define EXPAND_BUF 0x10000
static char expand_bufs[EXPAND_DEPTH][EXPAND_BUF];
static int expand_depth;

static int32_t run_expand(game_state *st, const int32_t *a)
{
    if (expand_depth >= EXPAND_DEPTH) {
        state_unported(st, FN_TEXT_EXPAND);
        return 0;
    }
    char *buf = expand_bufs[expand_depth++];
    int32_t size = a[2];
    if (size > EXPAND_BUF) {
        size = EXPAND_BUF;
    }
    int32_t n = 0;
    int32_t r = expand_n(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0], buf, size, &n);
    copy_out(st, (uint32_t) a[1], buf, n);
    expand_depth--;
    return r;
}

static int32_t run_message_expand(game_state *st, const int32_t *a)
{
    if (expand_depth >= EXPAND_DEPTH) {
        state_unported(st, FN_MESSAGE_TEXT_EXPAND);
        return 0;
    }
    char *buf = expand_bufs[expand_depth++];
    int32_t size = a[2];
    if (size > EXPAND_BUF) {
        size = EXPAND_BUF;
    }
    int32_t n = 0;
    message_text_expand_n(st, a[0], buf, size, &n);
    copy_out(st, (uint32_t) a[1], buf, n);
    expand_depth--;
    return 0;
}

static int32_t run_entry(game_state *st, const int32_t *a)
{
    return (int32_t) strings_entry(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_strnicmp(game_state *st, const int32_t *a)
{
    return strnicmp_c(st, (uint32_t) a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

const ported_function msg_expand_ports[] = {
    {0x00448330u, "message_text_expand", 3, 0, 0, run_message_expand, 0},
    {0x004CB980u, "text_expand", 3, 8, 0, run_expand, 0},
    {0x004CBDB0u, "strings_entry", 1, 32, 0, run_entry, 0},
    {0x0056B240u, "crt_strnicmp", 3, 32, 0, run_strnicmp, PORT_PARTIAL},
};
const int msg_expand_ports_count = sizeof(msg_expand_ports) / sizeof(msg_expand_ports[0]);
