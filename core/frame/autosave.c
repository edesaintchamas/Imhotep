/* Imhotep - the autosave after a tick (0x004D5C30) and the game's saver (0x004D4E20).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The saver prepares the state as the original does, encodes the 181 chunks (format:
 * docs/formats/save.md; the same encoding as core/state/save_game.c, checked byte for byte by
 * tests/differential/oracle_save_check.py) and hands the file to the platform (platform_file_write),
 * then rebuilds the route buffers and posts the "game saved" notice as the original does after
 * writing. The CRT file calls the original makes (_open 0x00568EAB, _write 0x005664D5, _close
 * 0x005647EB) and its chunk compressor (0x0055BC50 -> 0x0055BF10, PKWARE implode) are done here, inside
 * the port. */
#include "frame/frame.h"

#include <stdlib.h>
#include <string.h>

#include "io/pkware.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "sim/queue.h"
#include "state/ported.h"

#define PLAYER 0x00EAAC28u
#define CITY_SIZE 0x49D8u
#define SCHEMA 0x00E92E50u       /* chunk 6: 0x5DC dwords */
#define SCHEMA_COUNT 0x00E945C4u
#define CHUNKS 0x005E09A0u       /* 181 records of 20 bytes: compressed, address, size, count, - */
#define WORKSPACE 0x01265188u    /* the compressor's globals: output pointer, size, input size, crc */
#define ROUTES 0x00776FB0u       /* 50 route records of 0x144 bytes */

typedef struct {
    uint8_t *data;
    size_t len, cap;
    int failed;
} out_buf;

static void put(out_buf *o, const void *p, size_t n)
{
    if (o->failed || n == 0) {
        return;
    }
    if (o->len + n > o->cap) {
        size_t cap = o->cap ? o->cap : 1 << 20;
        while (cap < o->len + n) {
            cap *= 2;
        }
        uint8_t *d = realloc(o->data, cap);
        if (!d) {
            o->failed = 1;
            return;
        }
        o->data = d;
        o->cap = cap;
    }
    memcpy(o->data + o->len, p, n);
    o->len += n;
}

static void put_u32(out_buf *o, uint32_t v)
{
    uint8_t b[4] = {(uint8_t) v, (uint8_t) (v >> 8), (uint8_t) (v >> 16), (uint8_t) (v >> 24)};
    put(o, b, 4);
}

/* the original's inline strcpy (repne scasb, rep movs): strlen + 1 bytes */
static void str_copy(game_state *st, uint32_t dst, uint32_t src)
{
    uint8_t c;
    do {
        c = P8(st, src++);
        P8(st, dst++) = c;
    } while (c != 0);
}

/* appends the state string at `src` to buf (size bytes); 0 when it does not fit */
static int str_append(game_state *st, char *buf, size_t size, uint32_t src)
{
    size_t n = strlen(buf);
    for (;;) {
        if (n + 1 >= size) {
            return 0;
        }
        const uint8_t *c = state_ptr(st, src++, 1);
        if (!c) {
            return 0;
        }
        buf[n++] = (char) *c;
        if (*c == 0) {
            return 1;
        }
    }
}

static void compressed_chunk(game_state *st, out_buf *o, const uint8_t *data, int32_t n)
{
    /* @bin 0x0055BC50 the output buffer is the spare screen [0x00E92E1C] (outside the state: only its
     * pointer is kept); nothing is written above 1,200,000 bytes */
    U32(st, WORKSPACE) = U32(st, 0x00E92E1Cu);
    if (n > 0x124F80) {
        return;
    }
    S32(st, WORKSPACE + 8) = n;
    /* @bin 0x0055BF10 PKWARE implode, binary literals, the dictionary size 0x00603650, output room n;
     * u32 size then the data, or u32 0x80000000 then the raw bytes when it does not fit. The input's
     * CRC the original keeps in 0x01265194 is not computed (workspace, tools/oracle/excluded.py) */
    uint8_t *tmp = malloc(n > 0 ? (size_t) n : 1);
    size_t clen;
    if (tmp && pkware_implode(data, (size_t) n, tmp, (size_t) n, &clen, S32(st, 0x00603650u)) == 0) {
        U32(st, WORKSPACE + 4) = (uint32_t) clen;
        put_u32(o, (uint32_t) clen);
        put(o, tmp, clen);
    } else {
        U32(st, WORKSPACE + 4) = 0x80000000u;
        put_u32(o, 0x80000000u);
        put(o, data, (size_t) n);
    }
    free(tmp);
}

/* @bin 0x004D4F6D the route records after a save: each one whose buffer was released and that has
 * 2..50 points, a type 1..50 and a kind < 3 gets its point buffer back (the sum of the legs'
 * longer axis + 1, room for 50 more), filled by 0x00445D10 leg by leg, then 0x00407F90; the others
 * are reset */
static void routes_rebuild(game_state *st)
{
    for (uint32_t k = 0; k < 50; k++) {
        uint32_t r = ROUTES + 0x144u * k;
        uint8_t n = U8(st, r + 0x141);
        uint8_t type = U8(st, r);
        if (U32(st, r + 4) != 0 || S32(st, r + 0x138) != 0 || n > 0x32 || U8(st, r + 0x140) > 2 || type < 1 ||
            type > 0x32) {
            S32(st, r + 0x134) = 0;
            U32(st, r + 4) = 0;
            U8(st, r + 0x141) = 0;
            S32(st, r + 0x138) = 0;
            U8(st, r) = 5;
            U8(st, r + 0x142) = 0;
            S16(st, r + 0x13C) = -1;
            S16(st, r + 0x13E) = -1;
            U8(st, r + 0x140) = 1;
            continue;
        }
        int32_t total = 0;
        for (int32_t i = 1; i < (int32_t) n; i++) {
            uint32_t p = r + 8 + 6u * (uint32_t) (i - 1);
            int32_t x0 = (int16_t) S16(st, p), x1 = (int16_t) S16(st, p + 6);
            int32_t dx = x0 - x1 > x1 - x0 ? x0 - x1 : x1 - x0;
            int32_t y0 = (int16_t) S16(st, p + 2), y1 = (int16_t) S16(st, p + 8);
            int32_t dy = y0 - y1 > y1 - y0 ? y0 - y1 : y1 - y0;
            total += 1 + (dx > dy ? dx : dy);
        }
        int32_t cap = S32(st, r + 0x138);
        if (total > cap && cap < total + 50) {
            if (U32(st, r + 4) != 0) {
                state_dealloc(st, U32(st, r + 4)); /* @bin 0x00561A2B operator delete */
                U32(st, r + 4) = 0;
            }
            U32(st, r + 4) = state_alloc(st, (uint32_t) ((total + 50) * 6)); /* @bin 0x00561A1D operator new */
            S32(st, r + 0x138) = total + 50;
        }
        S32(st, r + 0x134) = total;
        int32_t done = 0;
        int complete = 1;
        for (int32_t i = 1; i < (int32_t) U8(st, r + 0x141); i++) {
            if (done > S32(st, r + 0x134)) {
                complete = 0;
                break;
            }
            done += GAME_CALLT(st, r, 0x00445D10u, i - 1, i, done);
        }
        if (complete) {
            S32(st, r + 0x134) = done;
            GAME_CALLT0(st, r, 0x00407F90u);
        }
    }
}

/* @bin 0x004F6810 (this = queue, record) a shipwright queue packed into its 0x334-byte saved record:
 * capacity at most 200, the three indexes wrapped (0x004F6A10), the header, then the elements (or 200
 * zeros without a block) */
static void queue_pack(game_state *st, uint32_t q, uint32_t rec)
{
    if (S32(st, q + 0x10) > 200) {
        S32(st, q + 0x10) = 200;
    }
    ring_wrap(st, q, q + 4);
    ring_wrap(st, q, q + 8);
    ring_wrap(st, q, q + 0x0C);
    S32(st, rec) = S32(st, q + 0x0C);
    S32(st, rec + 4) = S32(st, q + 0x10);
    S32(st, rec + 8) = S32(st, q + 4);
    S32(st, rec + 0x0C) = S32(st, q + 8);
    U8(st, rec + 0x10) = U8(st, q);
    uint32_t block = U32(st, q + 0x14);
    if (block != 0) {
        uint32_t n = ((uint32_t) S32(st, q + 0x10) << 2) >> 2; /* rep movsd of cap * 4 bytes */
        for (uint32_t i = 0; i < n; i++) {
            S32(st, rec + 0x14 + 4 * i) = (int32_t) P32(st, block + 4 * i);
        }
    } else {
        for (uint32_t i = 0; i < 200; i++) {
            S32(st, rec + 0x14 + 4 * i) = 0;
        }
    }
}

int32_t frame_save_game(game_state *st, const char *path)
{
    /* @bin 0x004D4E20 */
    /* @bin 0x004D0A40(0) the 10 shipwright queues (0x009783C0, 0x1C each) into chunk 140's records
     * (0x009763B8, 0x334 each) */
    for (uint32_t j = 0; j < 10; j++) {
        queue_pack(st, 0x009783C0u + 0x1Cu * j, 0x009763B8u + 0x334u * j);
    }
    S32(st, 0x00E945C0u) = 0xA0;                   /* the save version */
    str_copy(st, 0x005E17C4u, 0x005E17CCu);        /* "MAPS" */
    str_copy(st, 0x00EA184Cu, 0x00E38E74u);        /* the player's name into chunk 30 */
    GAME_CALL0(st, 0x004CE2D0u);                   /* the working directory */
    GAME_CALL0(st, 0x004771C0u);                   /* terrain flag 0x10 */
    U8(st, 0x007770F1u) = 0;
    S32(st, 0x007770E4u) = 0;
    for (uint32_t k = 0; k < 50; k++) {            /* @bin 0x004D4EA2 the route point buffers */
        uint32_t r = ROUTES + 4 + 0x144u * k;
        if (U32(st, r) != 0) {
            state_dealloc(st, U32(st, r)); /* @bin 0x00561A2B operator delete */
            U32(st, r) = 0;
            S32(st, r + 0x134) = 0;
        }
    }
    /* @bin 0x004D4ED6 _open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600): -1 returns 1 before
     * anything else is written. The platform writes the whole file at the end; when it fails, the
     * state written in between (schema, compressor workspace) is put back as the early return
     * leaves it */
    uint8_t *schema_before = malloc(0x5DC * 4 + 4 + 16);
    if (!schema_before) {
        return 1;
    }
    memcpy(schema_before, state_ptr(st, SCHEMA, 0x5DC * 4), 0x5DC * 4);
    memcpy(schema_before + 0x5DC * 4, state_ptr(st, SCHEMA_COUNT, 4), 4);
    memcpy(schema_before + 0x5DC * 4 + 4, state_ptr(st, WORKSPACE, 16), 16);
    memset(state_ptr(st, SCHEMA, 0x5DC * 4), 0, 0x5DC * 4);
    memcpy(state_ptr(st, SCHEMA, 0x389 * 4), state_ptr(st, CHUNKS, 0x389 * 4), 0x389 * 4);
    S32(st, SCHEMA_COUNT) = 0xB5;
    out_buf o = {NULL, 0, 0, 0};
    for (uint32_t i = 0; i < 0xB5; i++) {
        uint32_t rec = CHUNKS + 20u * i;
        int32_t n = (int32_t) (U32(st, rec + 12) * U32(st, rec + 8));
        const uint8_t *data = n > 0 ? state_ptr(st, U32(st, rec + 4), (uint32_t) n) : NULL;
        if (n > 0 && !data) {
            o.failed = 1;
            break;
        }
        if (U32(st, rec) == 0) {
            put(&o, data, n > 0 ? (size_t) n : 0); /* @bin 0x005664D5 _write */
        } else {
            compressed_chunk(st, &o, data, n);
        }
    }
    int32_t written = o.failed ? -1 : platform_file_write(st, path, o.data, (uint32_t) o.len);
    free(o.data);
    if (written < 0) {
        memcpy(state_ptr(st, SCHEMA, 0x5DC * 4), schema_before, 0x5DC * 4);
        memcpy(state_ptr(st, SCHEMA_COUNT, 4), schema_before + 0x5DC * 4, 4);
        memcpy(state_ptr(st, WORKSPACE, 16), schema_before + 0x5DC * 4 + 4, 16);
        free(schema_before);
        return 1;
    }
    free(schema_before);
    GAME_CALL(st, 0x004E2970u, 0xD4, 0, 0); /* the "game saved" notice */
    routes_rebuild(st);
    return 0;
}

void frame_autosave(game_state *st)
{
    /* @bin 0x004D5C30 */
    if (S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    /* @bin 0x004D5C4E the city's autosave flag (0x00EA5F8C + player * 0x49D8): 0 after a start, 2 set by
     * the month change in months 0 and 6 (core/sim/tick.c), 1 once saved */
    uint32_t flag = 0x00EA5F8Cu + U32(st, PLAYER) * CITY_SIZE;
    int32_t f = S32(st, flag);
    if ((f == 2 && S32(st, 0x00E38EECu) != 0) || f == 1) {
        return;
    }
    /* @bin 0x004D5C79 a campaign mission (0x007027B4 = 1): "autosave_replay.sav" on its first frame,
     * "autosave_history.sav" after; otherwise "autosave.sav" */
    uint32_t name = 0x005E1C14u;
    if (U8(st, 0x007027B4u) == 1) {
        name = f == 0 ? 0x0057BFC4u : 0x005E1BFCu;
    }
    S32(st, flag) = 1;
    char path[256] = "";
    uint32_t dir = (uint32_t) GAME_CALL0(st, 0x005169C0u); /* the family's directory */
    int ok = str_append(st, path, sizeof(path), dir) && str_append(st, path, sizeof(path), 0x005761D0u) &&
             str_append(st, path, sizeof(path), name);
    if (!ok) {
        state_unported(st, 0x004D5C30u); /* a path longer than the original's 0x64-byte buffer */
        return;
    }
    frame_save_game(st, path);
    if (U8(st, 0x007027B4u) == 1) {
        /* @bin 0x004D5D46 the path is kept in 0x00700998, the family's progress written */
        uint32_t i = 0;
        do {
            U8(st, 0x00700998u + i) = (uint8_t) path[i];
        } while (path[i++] != 0);
        int32_t progress = GAME_CALL0(st, 0x00516A20u);
        GAME_CALLT(st, 0x006FDD08u, 0x0041B8A0u, progress);
    }
}

/* ---- the port table ---- */

static int32_t run_autosave(game_state *st, const int32_t *a)
{
    (void) a;
    frame_autosave(st);
    return 0;
}

static int32_t run_save_game(game_state *st, const int32_t *a)
{
    char path[512];
    uint32_t i = 0;
    for (;; i++) {
        const uint8_t *c = state_ptr(st, (uint32_t) a[0] + i, 1);
        if (!c || i + 1 >= sizeof(path)) {
            state_unported(st, 0x004D4E20u);
            return 1;
        }
        path[i] = (char) *c;
        if (*c == 0) {
            break;
        }
    }
    return frame_save_game(st, path);
}

const ported_function frame_autosave_ports[] = {
    {0x004D5C30u, "frame_autosave", 0, 0, 0, run_autosave, 0},
    {0x004D4E20u, "frame_save_game", 1, 32, 0, run_save_game, 0},
    /* the saver's CRT writes and chunk compressor, done inside frame_save_game */
    {0x005664D5u, "_write (CRT; frame_save_game)", 3, 32, 0, NULL, 0},
    {0x0055BC50u, "compressed chunk write (frame_save_game)", 3, 32, 0, NULL, 0},
    {0x0055BF10u, "PKWARE implode wrapper (frame_save_game)", 5, 32, 0, NULL, 0},
    {0x0055C130u, "PKWARE implode (pkware_implode)", 6, 32, 0, NULL, 0},
    {0x0055C2D0u, "PKWARE implode body (pkware_implode)", 0, 0, 0, NULL, 0},
    {0x0055C650u, "PKWARE implode (pkware_implode)", 0, 0, 0, NULL, 0},
    {0x0055C990u, "PKWARE implode (pkware_implode)", 0, 0, 0, NULL, 0},
    {0x0055CA20u, "PKWARE implode (pkware_implode)", 0, 0, 0, NULL, 0},
    {0x0055CA90u, "PKWARE implode (pkware_implode)", 0, 0, 0, NULL, 0},
    {0x0055CB10u, "PKWARE implode (pkware_implode)", 0, 0, 0, NULL, 0},
};
const int frame_autosave_ports_count = sizeof(frame_autosave_ports) / sizeof(frame_autosave_ports[0]);
