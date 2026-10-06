/* Imhotep - reading a scenario map (maps\NAME.map) into the game state (docs/newgame.md 3.3):
 * 0x004D5820 (file), 0x004D4AE0 (MAPS chunks and version upgrades), the chunk reader 0x004D0E10
 * and the compressed-chunk reader 0x0055BCF0, the grid setup 0x004D5980 and the buildings of
 * flagged tiles 0x0046D480.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original opens the file with the CRT (_open 0x00568EAB, _read 0x005660ED, _close
 * 0x005647EB) and reads it chunk by chunk. Here the platform gives the whole file
 * (platform_file_read, core/sim/platform.h) and map_stream keeps the read position: the CRT calls
 * become reads at that position, with the same short-read behaviour at the end of the file. The
 * chunks land in the state at the addresses of the schema, as in the original. */
#include "newgame/map_load.h"

#include <stdlib.h>
#include <string.h>

#include "city/month_events.h"
#include "io/pkware.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define VERSION 0x00E945C0u     /* chunk 1 of a map: the file version */
#define FILE_SCHEMA 0x00E92E50u /* chunk 3 of a map: the file's own schema (300 x 20 bytes) */
#define USE_FILE_SCHEMA 0x00E945C4u /* chunk 2 of a map: != 0, compare with the file's schema */
#define MAP_SCHEMA 0x005E17D8u  /* @bin 0x004D4AE0 the MAPS schema (24 records of 20 bytes) */
#define ROUTES 0x00776FB0u      /* empire routes, 50 x 0x144 */
#define ROUTES_END 0x0077AEF8u
#define EMPIRE_OBJECTS 0x0077F748u /* 200 x 0x62 */
#define EMPIRE_OBJECTS_END 0x007843D8u
#define BUILDINGS 0x007999D8u
#define BUILDING_SIZE 0x108u
#define CITY_INDEX 0x00EAAC28u
#define CITY_BASE 0x00EA1870u
#define CITY_SIZE 0x49D8u
#define EDITOR 0x00605A3Cu
#define BUILDING(id) (BUILDINGS + BUILDING_SIZE * (uint32_t) (id))
#define CITY() (CITY_BASE + CITY_SIZE * U32(st, CITY_INDEX))

/* ---- state memory helpers ---------------------------------------------------------------- */

/* bytes written where the game writes them: a whole run when it is inside the state, else byte
 * by byte (bytes outside the state are dropped: the original would write into other memory) */
static void state_write(game_state *st, uint32_t address, const uint8_t *src, uint32_t n)
{
    uint8_t *p = n ? state_ptr(st, address, n) : NULL;
    if (p) {
        memmove(p, src, n);
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        uint8_t *b = state_ptr(st, address + i, 1);
        if (b) {
            *b = src[i];
        }
    }
}

static void state_read(game_state *st, uint32_t address, uint8_t *dst, uint32_t n)
{
    const uint8_t *p = n ? state_ptr(st, address, n) : NULL;
    if (p) {
        memcpy(dst, p, n);
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *b = state_ptr(st, address + i, 1);
        dst[i] = b ? *b : 0;
    }
}

/* rep stosd / stosb with zero (the game's inline memsets) */
static void state_zero(game_state *st, uint32_t address, uint32_t n)
{
    uint8_t *p = n ? state_ptr(st, address, n) : NULL;
    if (p) {
        memset(p, 0, n);
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        uint8_t *b = state_ptr(st, address + i, 1);
        if (b) {
            *b = 0;
        }
    }
}

/* @bin 0x00562880 memmove (CRT): overlapping copies keep the source bytes */
static void state_memmove(game_state *st, uint32_t dst, uint32_t src, uint32_t n)
{
    if (n == 0) {
        return;
    }
    uint8_t *tmp = malloc(n);
    if (!tmp) {
        return;
    }
    state_read(st, src, tmp, n);
    state_write(st, dst, tmp, n);
    free(tmp);
}

/* ---- the file (CRT _read on the platform's copy) ----------------------------------------- */

/* @bin 0x005660ED _read(fd, buf, n): up to n bytes from the position; the count read */
static uint32_t stream_take(map_stream *f, uint32_t n)
{
    if (f->size < 0 || f->pos >= (uint32_t) f->size) {
        return 0;
    }
    uint32_t left = (uint32_t) f->size - f->pos;
    return n < left ? n : left;
}

static uint32_t stream_read_buf(map_stream *f, uint8_t *dst, uint32_t n)
{
    uint32_t k = stream_take(f, n);
    memcpy(dst, f->data + f->pos, k);
    f->pos += k;
    return k;
}

static uint32_t stream_read(game_state *st, map_stream *f, uint32_t dst, uint32_t n)
{
    uint32_t k = stream_take(f, n);
    state_write(st, dst, f->data + f->pos, k);
    f->pos += k;
    return k;
}

/* @bin 0x0055BCF0 into a buffer that holds the destination's current bytes */
static int32_t compressed_read_buf(game_state *st, map_stream *f, uint8_t *dst, int32_t size)
{
    /* @bin 0x0055BCF0 the workspace (excluded from comparisons: tools/oracle/excluded.py):
     * buffer pointer := the spare screen 0x00E92E1C, length; the checksum 0x01265194 written by
     * 0x0055C000 is not computed (file I/O only) */
    U32(st, 0x01265188u) = U32(st, 0x00E92E1Cu);
    if (size > 1200000) { /* @bin 0x0055BCF0 signed compare with 0x124F80 */
        return 0;
    }
    if (size < 0) {
        return 0; /* no schema record has a negative size (the original would overrun) */
    }
    U32(st, 0x01265190u) = (uint32_t) size;
    if (stream_read(st, f, 0x0126518Cu, 4) == 0) {
        return 0;
    }
    uint32_t stored = U32(st, 0x0126518Cu);
    if (stored == 0x80000000u) {
        return stream_read_buf(f, dst, (uint32_t) size) != 0;
    }
    if (stored == 0) {
        memset(dst, 0, (uint32_t) size);
        return 1;
    }
    /* @bin 0x0055BCF0 the stored bytes into the spare screen buffer (outside the state), then
     * 0x0055C000: PKWARE explode (core/io/pkware.c) into dst; on failure the log line
     * "ERR_COMP Error uncompressing" (0x006036B4) */
    uint32_t avail = stream_take(f, stored);
    uint8_t *in = malloc(avail ? avail : 1);
    if (!in) {
        return 0;
    }
    if (stream_read_buf(f, in, stored) == 0) {
        free(in);
        return 0;
    }
    uint8_t *out = malloc(size ? (size_t) size : 1);
    int ok = out && pkware_explode(in, avail, out, (size_t) size) == 0;
    if (ok) {
        memcpy(dst, out, (size_t) size);
    }
    free(out);
    free(in);
    if (!ok) {
        GAME_CALL(st, 0x004CE6E0u, 0x006036B4, 0, 0);
        return 0;
    }
    return 1;
}

int32_t chunk_read_compressed(game_state *st, map_stream *f, uint32_t dst, int32_t size)
{
    uint32_t n = size > 0 && size <= 1200000 ? (uint32_t) size : 0;
    uint8_t *buf = malloc(n ? n : 1);
    if (!buf) {
        return 0;
    }
    state_read(st, dst, buf, n);
    int32_t r = compressed_read_buf(st, f, buf, size);
    state_write(st, dst, buf, n);
    free(buf);
    return r;
}

/* the read of a chunk's bytes: compressed (0x0055BCF0) or plain (_read) */
static void chunk_bytes(game_state *st, map_stream *f, uint32_t compressed, uint32_t dst, int32_t n)
{
    if (compressed) {
        chunk_read_compressed(st, f, dst, n);
    } else {
        stream_read(st, f, dst, (uint32_t) n);
    }
}

/* ---- 0x004D0E10 ---------------------------------------------------------------------------- */

int32_t chunk_read(game_state *st, map_stream *f, int32_t index, uint32_t schema)
{
    uint32_t rec = schema + 20u * (uint32_t) index;
    uint32_t file_rec = FILE_SCHEMA + 20u * (uint32_t) index;
    int32_t tsize = (int32_t) P32(st, rec + 8), tcount = (int32_t) P32(st, rec + 0xC);
    /* @bin 0x004D0E10 migrate when the file's schema is in use (0x00E945C4), the chunk is not
     * that schema itself, and the element size or count differ */
    int migrate = U32(st, USE_FILE_SCHEMA) != 0 && P32(st, rec + 4) != FILE_SCHEMA &&
                  (tsize != S32(st, file_rec + 8) || tcount != S32(st, file_rec + 0xC));
    /* @bin 0x004D0E10 converter list (16-byte entries: version threshold +8, function +0xC):
     * the first entry whose threshold is 0 or >= the file version; a non-zero threshold there
     * converts */
    uint32_t conv = P32(st, rec + 0x10);
    uint32_t ci = 0;
    int converter = 0;
    if (conv != 0) {
        while (P32(st, conv + 8 + 16 * ci) != 0 && (int32_t) P32(st, conv + 8 + 16 * ci) < S32(st, VERSION)) {
            ci++;
        }
        converter = P32(st, conv + 8 + 16 * ci) != 0;
    }
    uint32_t fn = converter ? P32(st, conv + 0xC + 16 * ci) : 0;
    if (!converter && !migrate) {
        chunk_bytes(st, f, P32(st, rec), P32(st, rec + 4), (int32_t) ((uint32_t) tsize * (uint32_t) tcount));
    } else {
        int32_t fcount = S32(st, file_rec + 0xC), fsize = S32(st, file_rec + 8);
        int32_t ttotal = (int32_t) ((uint32_t) tcount * (uint32_t) tsize);
        int32_t ftotal = (int32_t) ((uint32_t) fcount * (uint32_t) fsize);
        uint32_t fflag = U32(st, file_rec);
        uint32_t var = P32(st, rec + 4);
        if (ttotal < ftotal) {
            /* @bin 0x004D0E10 the file's data is larger: read into a temporary block (operator
             * new, outside the simulation heap), then each element into the variable, from the
             * last one down */
            size_t span = (size_t) (uint32_t) ftotal;
            if (tcount > 0 && fsize > 0 && (size_t) tcount * (size_t) fsize > span) {
                span = (size_t) tcount * (size_t) fsize; /* the original reads past the block here */
            }
            uint8_t *tmp = calloc(span ? span : 1, 1);
            if (!tmp) {
                return 0;
            }
            if (fflag) {
                compressed_read_buf(st, f, tmp, ftotal);
            } else {
                stream_read_buf(f, tmp, (uint32_t) ftotal);
            }
            if (converter && tsize < fsize) {
                state_unported(st, fn); /* converters on the temporary block (versions < 130) */
            }
            for (int32_t k = tcount - 1; k >= 0; k--) {
                state_write(st, var + (uint32_t) k * (uint32_t) tsize, tmp + (size_t) k * (size_t) fsize,
                            (uint32_t) fsize);
                if (fsize < tsize) {
                    state_zero(st, var + (uint32_t) k * (uint32_t) tsize + (uint32_t) fsize, (uint32_t) (tsize - fsize));
                }
            }
            if (converter && fsize <= tsize) {
                state_unported(st, fn);
            }
            if (!converter && tcount > fcount) {
                state_zero(st, var + (uint32_t) fcount * (uint32_t) tsize, (uint32_t) (tcount - fcount) * (uint32_t) tsize);
            }
            free(tmp);
        } else {
            /* @bin 0x004D0E10 in place: the file's bytes at the start of the variable, then each
             * element moved to its slot, from the last one down */
            state_zero(st, var, (uint32_t) ftotal);
            chunk_bytes(st, f, fflag, var, ftotal);
            for (int32_t k = fcount - 1; k >= 0; k--) {
                state_memmove(st, var + (uint32_t) k * (uint32_t) tsize, var + (uint32_t) k * (uint32_t) fsize,
                              (uint32_t) fsize);
                if (tsize - fsize > 0) {
                    state_zero(st, var + (uint32_t) k * (uint32_t) tsize + (uint32_t) fsize, (uint32_t) (tsize - fsize));
                }
            }
            if (!converter) {
                if (fcount < tcount) {
                    state_zero(st, var + (uint32_t) fcount * (uint32_t) tsize,
                               (uint32_t) (tcount - fcount) * (uint32_t) tsize);
                }
            } else {
                int32_t k = 0;
                for (; k < fcount; k++) {
                    GAME_CALL(st, fn, (int32_t) (var + (uint32_t) k * (uint32_t) tsize), 1);
                }
                if (fcount <= 0) {
                    k = 0;
                }
                for (; k < S32(st, rec + 0xC); k++) {
                    GAME_CALL(st, fn, (int32_t) (var + (uint32_t) k * (uint32_t) tsize), 0);
                }
            }
        }
    }
    /* @bin 0x004D0E10 a file older than version 115 (0x73): message box (text group 5, 0x18 /
     * 0x19), the load fails */
    if (S32(st, VERSION) < 0x73) {
        GAME_CALL(st, 0x004CC110u, 5, 0x18);
        GAME_CALL(st, 0x004CC160u, 5, 0x19);
        GAME_CALL(st, 0x00538CD0u, (int32_t) U32(st, 0x00E20764u), (int32_t) U32(st, 0x00E20768u), 0);
        return 0;
    }
    return 1;
}

/* ---- version upgrades (0x004D4AE0 calls them for old files) -------------------------------- */

void upgrade_empire_object_kinds(game_state *st)
{
    /* @bin 0x004D17E0 objects in use of kind 1: +0x18 through the jump table 0x004D1820
     * (0 -> 4, 1 -> 0, 2 -> 1, 3 -> 4, 4 -> 6, 5 -> 4, 6 -> 6); other values unchanged */
    static const uint8_t MAP[7] = {4, 0, 1, 4, 6, 4, 6};
    for (uint32_t o = EMPIRE_OBJECTS; o < EMPIRE_OBJECTS_END; o += 0x62u) {
        if (U8(st, o + 1) != 0 && U8(st, o) == 1 && U8(st, o + 0x18) <= 6) {
            U8(st, o + 0x18) = MAP[U8(st, o + 0x18)];
        }
    }
}

void upgrade_figures_0x7f(game_state *st)
{
    /* @bin 0x004D32D0 figures 1..1999: +0x02 := 0, +0x132 := -1 */
    for (uint32_t a = 0x00BCD8AEu; a < 0x00C8AE6Au; a += 0x184u) {
        U8(st, a - 0x130u) = 0;
        U16(st, a) = 0xFFFF;
    }
}

void upgrade_events_0x90(game_state *st)
{
    /* @bin 0x004D3D80 events 0..149: four bytes at +0x6A of each (0x00785D70 + 4 + 0x7C k) := 6 */
    for (uint32_t a = 0x00785DDFu; a < 0x0078A687u; a += 0x7Cu) {
        U8(st, a - 1) = 6;
        U8(st, a) = 6;
        U8(st, a + 1) = 6;
        U8(st, a + 2) = 6;
    }
}

void upgrade_events_0x98(game_state *st)
{
    /* @bin 0x004D40C0 events (count: i16 0x00785D70) of a kind with goods (0x004493C0): the four
     * good fields +4 .. +0xA of the event shifted by 5 from 0x1F, then the three last packed so
     * that the non-negative ones come first */
    int32_t n = S16(st, 0x00785D70u);
    for (int32_t k = 0; k < n; k++) {
        uint32_t e = 0x00785D74u + 0x7Cu * (uint32_t) k, p = e + 8;
        if ((GAME_CALLT0(st, e, 0x004493C0u) & 0xFF) == 0) {
            continue;
        }
        int16_t dx = S16(st, p - 4), ax = S16(st, p - 2), di = S16(st, p), cx = S16(st, p + 2);
        if (dx >= 0x1F) {
            dx = (int16_t) (dx + 5);
        }
        U16(st, p - 4) = (uint16_t) dx;
        if (ax >= 0x1F) {
            ax = (int16_t) (ax + 5);
        }
        if (di >= 0x1F) {
            di = (int16_t) (di + 5);
        }
        if (cx >= 0x1F) {
            cx = (int16_t) (cx + 5);
        }
        int16_t out_b = di; /* edx */
        if (ax < 0) {
            if (di >= 0) {
                ax = di;
                out_b = -1;
                goto test_c; /* 0x004D413E */
            }
            if (cx >= 0) {
                ax = cx;
                cx = -1;
            }
        }
        if (di >= 0) { /* 0x004D4139 */
            goto store;
        }
    test_c:
        if (cx >= 0) {
            out_b = cx;
            cx = -1;
        }
    store:
        U16(st, p - 2) = (uint16_t) ax;
        U16(st, p) = (uint16_t) out_b;
        U16(st, p + 2) = (uint16_t) cx;
    }
    /* @bin 0x004D415E 10 dwords 0x005D2E60 -> 0x005D2D40 */
    for (uint32_t k = 0; k < 0x28; k += 8) {
        uint32_t a = U32(st, 0x005D2E60u + k), b = U32(st, 0x005D2E64u + k);
        U32(st, 0x005D2D40u + k) = a;
        U32(st, 0x005D2D44u + k) = b;
    }
}

void upgrade_empire_object_masks(game_state *st)
{
    /* @bin 0x004D19E0 each object: a 0x62-byte copy of its first 0x4C bytes (rest zero), bytes
     * +0x3E + k (k < 32) := 3 / 2 / 1 / 0 from the bit masks +0x40, +0x44, +0x48, +0x5E .. +0x61
     * := 2, written back */
    for (uint32_t o = EMPIRE_OBJECTS; o < EMPIRE_OBJECTS_END; o += 0x62u) {
        uint8_t buf[0x62] = {0};
        memcpy(buf, state_ptr(st, o, 0x4C), 0x4C);
        uint32_t m0 = U32(st, o + 0x40), m1 = U32(st, o + 0x44), m2 = U32(st, o + 0x48);
        for (int k = 0; k < 32; k++) {
            uint32_t bit = 1u << k;
            buf[0x3E + k] = (uint8_t) ((bit & m0) ? 3 : (bit & m1) ? 2 : (bit & m2) ? 1 : 0);
        }
        memset(buf + 0x5E, 2, 4);
        memcpy(state_ptr(st, o, 0x62), buf, 0x62);
    }
}

void upgrade_buildings_0x86(game_state *st)
{
    /* @bin 0x004D4080 buildings 0..3999: +0x86 := 0 */
    for (uint32_t a = 0x00799A5Eu; a < 0x0089B75Eu; a += BUILDING_SIZE) {
        U8(st, a) = 0;
    }
}

void upgrade_city_44c8(game_state *st)
{
    /* @bin 0x004D40A0 both city records: +0x44C8, +0x44CC := 0 */
    for (uint32_t a = 0x00EA5D3Cu; a <= 0x00EAA714u; a += CITY_SIZE) {
        U32(st, a - 4) = 0;
        U32(st, a) = 0;
    }
}

void upgrade_city_i16_to_i32(game_state *st)
{
    /* @bin 0x004D1A70 both city records: i16[20] +0x2944 -> i32 +0x3D04, i16[20] +0x291C -> i32
     * +0x3C64 (sign-extended), then i32[20] +0x3D54 and +0x3CB4 := 0 */
    for (uint32_t s = 0x00EA418Cu; s < 0x00EAD53Cu; s += CITY_SIZE) {
        for (uint32_t k = 0; k < 20; k++) {
            S32(st, s + 0x1348u + 4 * k + 0xA0u) = S16(st, s + 0x28u + 2 * k);
            S32(st, s + 0x1348u + 4 * k) = S16(st, s + 2 * k);
        }
        for (uint32_t k = 0; k < 20; k++) {
            U32(st, s + 0x1398u + 4 * k + 0xA0u) = 0;
            U32(st, s + 0x1398u + 4 * k) = 0;
        }
    }
}

/* ---- 0x004D4AE0 ---------------------------------------------------------------------------- */

static int state_strcmp(game_state *st, uint32_t a, uint32_t b)
{
    for (uint32_t i = 0;; i++) {
        uint8_t x = P8(st, a + i), y = P8(st, b + i);
        if (x != y) {
            return x < y ? -1 : 1;
        }
        if (x == 0) {
            return 0;
        }
    }
}

void map_read_chunks(game_state *st, map_stream *f)
{
    /* @bin 0x004D4AE0 record 0 of the schema (the magic, into 0x005E17C4), compared with "MAPS"
     * (0x005E17CC, strcmp 0x00562370) */
    int32_t n0 = (int32_t) (U32(st, MAP_SCHEMA + 0xC) * U32(st, MAP_SCHEMA + 8));
    chunk_bytes(st, f, U32(st, MAP_SCHEMA), U32(st, MAP_SCHEMA + 4), n0);
    if (state_strcmp(st, 0x005E17C4u, 0x005E17CCu) != 0) {
        GAME_CALL(st, 0x004CC110u, 5, 0x18);
        GAME_CALL(st, 0x004CC160u, 5, 0x19);
        GAME_CALL(st, 0x00538CD0u, (int32_t) U32(st, 0x00E20764u), (int32_t) U32(st, 0x00E20768u), 0);
        return;
    }
    U32(st, VERSION) = 0xA0;
    U32(st, USE_FILE_SCHEMA) = 0;
    /* @bin 0x004D4B7F the route point buffers released (operator delete 0x00561A2B) */
    for (uint32_t r = ROUTES; r < ROUTES_END; r += 0x144u) {
        if (U32(st, r + 4) != 0) {
            state_dealloc(st, U32(st, r + 4)); /* operator delete 0x00561A2B */
            U32(st, r + 4) = 0;
            U32(st, r + 0x138) = 0;
        }
    }
    for (int32_t i = 1; i < 0x18; i++) {
        if ((chunk_read(st, f, i, MAP_SCHEMA) & 0xFF) == 0) {
            return;
        }
    }
    /* @bin 0x004D4BCB the routes rebuilt (0x004D4720, thiscall) or reset */
    U8(st, 0x007770F1u) = 0;
    U32(st, 0x007770E4u) = 0;
    for (uint32_t r = ROUTES; r < ROUTES_END; r += 0x144u) {
        uint8_t kind = U8(st, r);
        if (U32(st, r + 4) == 0 && U32(st, r + 0x138) == 0 && U8(st, r + 0x141) <= 0x32 && U8(st, r + 0x140) <= 2 &&
            kind >= 1 && kind <= 0x32) {
            GAME_CALLT0(st, r, 0x004D4720u);
        } else {
            U32(st, r + 0x134) = 0;
            U32(st, r + 4) = 0;
            U8(st, r + 0x141) = 0;
            U32(st, r + 0x138) = 0;
            U8(st, r) = 5;
            U8(st, r + 0x142) = 0;
            U16(st, r + 0x13C) = 0xFFFF;
            U16(st, r + 0x13E) = 0xFFFF;
            U8(st, r + 0x140) = 1;
        }
    }
    /* @bin 0x004D4C53 the upgrades of old files, by version (re-read after each call) */
    int32_t v = S32(st, VERSION);
    if (v <= 0x74) { /* scenario +0x408: u16[100] := 1 for the first 50 */
        for (uint32_t a = 0x00784CE0u; a < 0x00784DA8u; a += 2) {
            U16(st, a) = a < 0x00784D44u;
        }
    }
    if (v <= 0x96) {
        U16(st, 0x00784B94u) = 0;
    }
    if (v < 0x76) {
        upgrade_empire_object_kinds(st);
        v = S32(st, VERSION);
    }
    if (v < 0x7D) {
        GAME_CALLT0(st, 0x00D38360u, 0x004BD330u); /* floodplain settings */
        v = S32(st, VERSION);
    }
    if (v < 0x7F) {
        upgrade_figures_0x7f(st);
        v = S32(st, VERSION);
    }
    if (v < 0x90) {
        upgrade_events_0x90(st);
        v = S32(st, VERSION);
    }
    if (v <= 0x97) {
        upgrade_events_0x98(st);
        v = S32(st, VERSION);
    }
    if (v <= 0x98) {
        upgrade_empire_object_masks(st);
        v = S32(st, VERSION);
    }
    if (v <= 0x9A) {
        upgrade_buildings_0x86(st);
        upgrade_city_44c8(st);
        memset(state_ptr(st, 0x00784D3Au, 0x6E), 0, 0x6E); /* 0x1B dwords + 1 word */
        v = S32(st, VERSION);
    }
    if (v <= 0x9B) {
        upgrade_city_i16_to_i32(st);
        v = S32(st, VERSION);
    }
    if (v <= 0x9C) {
        U32(st, 0x0078D7B0u) = 0;
        U32(st, 0x0078D7A8u) = 0;
        U32(st, 0x0078D7A4u) = 0;
        U8(st, 0x00784CBCu) = 1;
    }
    if (v <= 0x9D) {
        U32(st, 0x0078D7ACu) = 0;
        U8(st, 0x00784CBDu) = 0;
        U32(st, 0x0078D798u) = 0xA;
        U32(st, 0x0078D7A0u) = 0;
    }
}

/* ---- 0x004D5820 ---------------------------------------------------------------------------- */

int32_t map_file_load(game_state *st, const char *path, int32_t preview)
{
    GAME_CALL0(st, 0x004CE2D0u);               /* @bin 0x004D5820 the working directory */
    GAME_CALLT0(st, 0x00D38360u, 0x004BD330u); /* floodplain settings */
    /* @bin 0x004D5820 _open(path, _O_BINARY): -1 -> return 1 */
    map_stream f = {NULL, platform_file_read(st, path, 0, NULL, 0), 0};
    if (f.size < 0) {
        return 1;
    }
    f.data = malloc(f.size ? (size_t) f.size : 1);
    if (!f.data) {
        return 1;
    }
    if (f.size > 0 && platform_file_read(st, path, 0, f.data, (uint32_t) f.size) != f.size) {
        f.size = 0;
    }
    map_read_chunks(st, &f);
    free(f.data); /* _close */
    S32(st, 0x00784F10u) = S16(st, 0x007848DCu); /* the scenario's empire */
    if (preview == 0) {
        GAME_CALL0(st, 0x004F4520u);
        GAME_CALL0(st, 0x0053D970u);
        GAME_CALL0(st, 0x004BC720u);
    }
    if (U32(st, EDITOR) == 1) {
        GAME_CALL(st, 0x004F78A0u, 1);
        U32(st, 0x00784C2Cu) = (uint32_t) GAME_CALL0(st, 0x0056133Cu);
    }
    return 0;
}

/* ---- 0x004D5980, 0x004D4040 ---------------------------------------------------------------- */

void city_clear_298c(game_state *st)
{
    /* @bin 0x004D4040 +0x298C, +0x2990 (dwords), +0x2994 (byte), then 9 dwords from +0x2995 */
    uint32_t c = CITY();
    U32(st, c + 0x298Cu) = 0;
    U32(st, c + 0x2990u) = 0;
    U8(st, c + 0x2994u) = 0;
    memset(state_ptr(st, c + 0x2995u, 36), 0, 36);
}

void map_setup_grids(game_state *st)
{
    GAME_CALL(st, 0x0040F8F0u, 0x00B7482C, 0x32C40, 0); /* the image grid := 0 */
    GAME_CALL0(st, 0x004771C0u);
    GAME_CALLT0(st, 0x00D38360u, 0x004BD360u);
    if (S32(st, VERSION) < 0x91) {
        U8(st, 0x00BCA30Cu) = 1;
        GAME_CALL0(st, 0x00486E70u);
    }
    /* @bin 0x004D59BD the vegetation grid 0x00A43EAC := -1 (0x32C4 dwords) */
    for (uint32_t k = 0; k < 0x32C4u; k++) {
        U32(st, 0x00A43EACu + 4 * k) = 0xFFFFFFFFu;
    }
    GAME_CALL(st, 0x0051F780u, 0, 0, (int32_t) U32(st, 0x00BCA2D0u), (int32_t) U32(st, 0x00BCA2C0u));
    GAME_CALL0(st, 0x004860D0u);
    GAME_CALLT(st, 0x00D05720u, 0x004BE160u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1);
    /* @bin 0x004D5A07 every event (count: i32 0x00785D70, re-read) */
    for (int32_t k = 0; k < S32(st, 0x00785D70u); k++) {
        event_reset(st, 0x00785D74u + 0x7Cu * (uint32_t) k); /* 0x00448A90, thiscall */
    }
    /* @bin 0x004D5A39 the 150 events' +0 := their index */
    for (uint32_t k = 0; k < 0x96; k++) {
        U16(st, 0x00785D74u + 0x7Cu * k) = (uint16_t) k;
    }
    GAME_CALL0(st, 0x004E59A0u);
    uint32_t c = CITY();
    U16(st, c + 0x4814u) = 0;
    U32(st, c + 0x47A0u) = 0xFFFFFFFFu;
    U32(st, c + 0x47A4u) = 0xFFFFFFFFu;
    GAME_CALL0(st, 0x00442660u);
    GAME_CALL0(st, 0x004426C0u);
    /* @bin 0x004D5A7A empire objects in use of kind 1: +0x1B := 0 */
    for (uint32_t o = EMPIRE_OBJECTS; o < EMPIRE_OBJECTS_END; o += 0x62u) {
        if (U8(st, o + 1) != 0 && U8(st, o) == 1) {
            U8(st, o + 0x1B) = 0;
        }
    }
    /* @bin 0x004D5A99 empire cities 1..60: +5 := 0 */
    for (uint32_t a = 0x0077DE6Fu; a < 0x0077F747u; a += 0x6Au) {
        U8(st, a) = 0;
    }
    U32(st, VERSION) = 0xA0;
    GAME_CALL0(st, 0x004DDF90u);
    U32(st, 0x00BCB9B0u) = (uint32_t) GAME_CALL0(st, 0x0046F190u);
    city_clear_298c(st); /* tail jump */
}

/* ---- 0x0046D480 ---------------------------------------------------------------------------- */

void map_flagged_buildings(game_state *st)
{
    uint32_t off = U32(st, 0x00BCA2D8u);
    for (int32_t y = 0; y < S32(st, 0x00BCA2D4u); y++) {
        for (int32_t x = 0; x < S32(st, 0x00BCA2D0u); x++, off++) {
            uint32_t grid = 0x00B4E6FCu + 2 * off;
            if (!(U32(st, 0x00AF598Cu + 4 * off) & 8) || U16(st, grid) != 0) {
                continue;
            }
            int32_t bl = U8(st, 0x00ACF85Cu + off) & 1;
            if (U32(st, EDITOR) == 1) {
                bl = 0;
            }
            uint32_t image = 0x00B7482Cu + 4 * off;
            int32_t img = S32(st, image), e = S32(st, 0x00784CA4u), type;
            if (img == e) {
                U32(st, image) = U32(st, 0x00F569D4u);
                type = 0x58;
            } else if (img == e + 1) {
                U32(st, image) = U32(st, 0x00F569D4u) + 1;
                type = 0x58;
            } else if (img == S32(st, 0x00784CA8u)) {
                U32(st, image) = U32(st, 0x00F569D4u) + 2;
                type = 0x59;
            } else if (img == S32(st, 0x00784CACu)) {
                U32(st, image) = U32(st, 0x00F565DCu) + (uint32_t) bl;
                type = 0x5D;
            } else {
                GAME_CALL(st, 0x0047C480u, 0, x, y);
                continue;
            }
            int32_t id = GAME_CALL(st, 0x0046A400u, 1, type, x, y, 0, 0);
            uint32_t b = BUILDING(id);
            U16(st, grid) = (uint16_t) id;
            U8(st, b) = 1;
            if (type == 0x5D) {
                U16(st, b + 0x54) = (uint16_t) bl;
            } else if (type == 0x59) {
                U16(st, grid + 2) = (uint16_t) id;
                U16(st, grid + 0x1C8) = (uint16_t) id;
                U16(st, grid + 0x1CA) = (uint16_t) id;
                U8(st, b + 0xC2) = 0x64;
                if (U32(st, EDITOR) == 0) {
                    GAME_CALL(st, 0x00482A90u, S16(st, b + 6), S16(st, b + 8), 2, 6);
                } else {
                    GAME_CALL(st, 0x00482B50u, S16(st, b + 6), S16(st, b + 8), 2, 8);
                }
                uint32_t c = CITY();
                if (U32(st, c + 0x45A0u) == 0) {
                    S32(st, c + 0x45A0u) = S16(st, b + 6);
                    S32(st, c + 0x45A4u) = S16(st, b + 8);
                }
            } else {
                U8(st, b + 0xC2) = 0x64;
                U8(st, b + 0x32) = (uint8_t) bl;
                if (U32(st, EDITOR) == 0) {
                    GAME_CALL(st, 0x00482A90u, S16(st, b + 6), S16(st, b + 8), 1, 3);
                } else {
                    GAME_CALL(st, 0x00482B50u, S16(st, b + 6), S16(st, b + 8), 1, 5);
                }
            }
        }
        off += U32(st, 0x00BCA2DCu);
    }
    /* @bin 0x0046D6DC the 0x59 buildings (ids 1..0x0078D790) listed at 0x0078D9A8 (at most 499
     * kept: the count stops at 0x1F3), then each 0x58 building gets the nearest of them (+0x12;
     * distance max(|dx|, |dy|), first of the smallest) */
    int32_t n = S32(st, 0x0078D790u), count = 0;
    U32(st, 0x0078D7F0u) = 0;
    if (n < 1) {
        return;
    }
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING(id);
        if (U8(st, b) == 1 && U16(st, b + 0x10) == 0x59) {
            U16(st, 0x0078D9A8u + 2 * (uint32_t) count) = (uint16_t) id;
            count++;
            S32(st, 0x0078D7F0u) = count;
            if (count >= 0x1F4) {
                S32(st, 0x0078D7F0u) = 0x1F3;
                count = 0x1F3;
            }
        }
    }
    if (count <= 0) {
        return;
    }
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING(id);
        if (U8(st, b) != 1 || U16(st, b + 0x10) != 0x58) {
            continue;
        }
        int32_t best = 1000, best_id = 0;
        U32(st, 0x0078D7F4u) = 0;
        int32_t by = S16(st, b + 8), bx = S16(st, b + 6);
        S32(st, 0x0078D7F4u) = count;
        for (int32_t j = 0; j < count; j++) {
            int32_t s = S16(st, 0x0078D9A8u + 2 * (uint32_t) j);
            uint32_t t = BUILDING(s);
            int32_t ty = S16(st, t + 8), tx = S16(st, t + 6);
            int32_t dx = bx > tx ? bx - tx : bx < tx ? tx - bx : 0;
            S32(st, 0x00D3B4FCu) = dx;
            int32_t dy = by > ty ? by - ty : by < ty ? ty - by : 0;
            S32(st, 0x00D3B4F8u) = dy;
            int32_t d = dx < dy ? dy : dx;
            if (d < best) {
                best = d;
                best_id = s;
            }
        }
        count = S32(st, 0x0078D7F0u);
        U16(st, b + 0x12) = (uint16_t) best_id;
    }
}

/* ---- file names ---------------------------------------------------------------------------- */

void file_name_strip_extension(game_state *st, uint32_t name)
{
    for (uint32_t a = name;; a++) {
        uint8_t c = P8(st, a);
        if (c == '.') {
            P8(st, a) = 0;
            return;
        }
        if (c == 0) {
            return;
        }
    }
}

void file_name_add_extension(game_state *st, uint32_t name)
{
    uint32_t a = name;
    while (P8(st, a) != 0) {
        a++;
    }
    P8(st, a) = '.';
    P8(st, a + 1) = U8(st, 0x00E38AECu);
    P8(st, a + 2) = U8(st, 0x00E38AEDu);
    P8(st, a + 3) = U8(st, 0x00E38AEEu);
    P8(st, a + 4) = 0;
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_map_file_load(game_state *st, const int32_t *a)
{
    char path[260];
    uint32_t i = 0;
    for (; i < sizeof(path) - 1; i++) {
        const uint8_t *c = state_ptr(st, (uint32_t) a[0] + i, 1);
        if (!c || *c == 0) {
            break;
        }
        path[i] = (char) *c;
    }
    path[i] = 0;
    return map_file_load(st, path, a[1]);
}

static int32_t run_memmove(game_state *st, const int32_t *a)
{
    state_memmove(st, (uint32_t) a[0], (uint32_t) a[1], (uint32_t) a[2]);
    return a[0];
}

static int32_t run_working_directory(game_state *st, const int32_t *a)
{
    /* @bin 0x004CE2D0 _chdir(0x00E36B94), the game's directory (on failure _getcwd and a log
     * line): the platform's; the state does not change */
    (void) st;
    (void) a;
    return 0;
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_map_setup_grids, map_setup_grids)
VOID0(run_city_clear_298c, city_clear_298c)
VOID0(run_map_flagged_buildings, map_flagged_buildings)
VOID0(run_upgrade_17e0, upgrade_empire_object_kinds)
VOID0(run_upgrade_32d0, upgrade_figures_0x7f)
VOID0(run_upgrade_3d80, upgrade_events_0x90)
VOID0(run_upgrade_40c0, upgrade_events_0x98)
VOID0(run_upgrade_19e0, upgrade_empire_object_masks)
VOID0(run_upgrade_4080, upgrade_buildings_0x86)
VOID0(run_upgrade_40a0, upgrade_city_44c8)
VOID0(run_upgrade_1a70, upgrade_city_i16_to_i32)

static int32_t run_strip_extension(game_state *st, const int32_t *a)
{
    file_name_strip_extension(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_add_extension(game_state *st, const int32_t *a)
{
    file_name_add_extension(st, (uint32_t) a[0]);
    return 0;
}

/* The file functions below 0x004D5820 run inside the port with the file from the platform: they
 * are registered without an entry point (run = NULL), so that the oracle's recorder runs them
 * natively inside a ported caller (their state effects are the chunk bytes the port writes). */
const ported_function newgame_map_load_ports[] = {
    {0x004D5820u, "map_file_load", 2, 32, 0, run_map_file_load, 0},
    {0x004D4AE0u, "map_read_chunks (file read: map_stream)", 1, 0, 0, NULL, 0},
    {0x004D0E10u, "chunk_read (file read: map_stream)", 3, 8, 0, NULL, 0},
    {0x0055BCF0u, "chunk_read_compressed (file read: map_stream)", 3, 32, 0, NULL, 0},
    {0x0055C000u, "explode wrapper (core/io/pkware.c, in chunk_read_compressed)", 5, 32, 0, NULL, 0},
    {0x0055CBD0u, "explode (core/io/pkware.c)", 4, 32, 0, NULL, 0},
    {0x0055C0F0u, "explode: output checksum (not computed: excluded workspace)", 3, 32, 0, NULL, 0},
    {0x0055CD50u, "explode: decode (core/io/pkware.c)", 1, 32, 0, NULL, 0},
    {0x0055CE50u, "explode: literal / length (core/io/pkware.c)", 1, 32, 0, NULL, 0},
    {0x0055D000u, "explode: distance (core/io/pkware.c)", 2, 32, 0, NULL, 0},
    {0x0055D080u, "explode: bits (core/io/pkware.c)", 2, 32, 0, NULL, 0},
    {0x0055D110u, "explode: tables (core/io/pkware.c)", 4, 0, 0, NULL, 0},
    {0x0055D150u, "explode: tables (core/io/pkware.c)", 1, 0, 0, NULL, 0},
    {0x0055D250u, "explode: tables (core/io/pkware.c)", 3, 0, 0, NULL, 0},
    {0x005660EDu, "_read (CRT; map_stream / platform_file_read)", 3, 32, 0, NULL, 0},
    {0x00568EABu, "_open (CRT; platform_file_read)", 2, 32, 0, NULL, 0},
    {0x005647EBu, "_close (CRT; map_stream)", 1, 32, 0, NULL, 0},
    {0x0056B2DDu, "_chdir (CRT; the platform's)", 1, 32, 0, NULL, 0},
    {0x004CE2D0u, "working_directory (the platform's)", 0, 0, 0, run_working_directory, 0},
    {0x00562880u, "memmove", 3, 32, 0, run_memmove, 0},
    {0x004D5980u, "map_setup_grids", 0, 0, 0, run_map_setup_grids, 0},
    {0x004D4040u, "city_clear_298c", 0, 0, 0, run_city_clear_298c, 0},
    {0x0046D480u, "map_flagged_buildings", 0, 0, 0, run_map_flagged_buildings, 0},
    {0x004CE3A0u, "file_name_strip_extension", 1, 0, 0, run_strip_extension, 0},
    {0x004CE3C0u, "file_name_add_extension", 1, 0, 0, run_add_extension, 0},
    {0x004D17E0u, "upgrade_empire_object_kinds", 0, 0, 0, run_upgrade_17e0, 0},
    {0x004D32D0u, "upgrade_figures_0x7f", 0, 0, 0, run_upgrade_32d0, 0},
    {0x004D3D80u, "upgrade_events_0x90", 0, 0, 0, run_upgrade_3d80, 0},
    {0x004D40C0u, "upgrade_events_0x98", 0, 0, 0, run_upgrade_40c0, 0},
    {0x004D19E0u, "upgrade_empire_object_masks", 0, 0, 0, run_upgrade_19e0, 0},
    {0x004D4080u, "upgrade_buildings_0x86", 0, 0, 0, run_upgrade_4080, 0},
    {0x004D40A0u, "upgrade_city_44c8", 0, 0, 0, run_upgrade_40a0, 0},
    {0x004D1A70u, "upgrade_city_i16_to_i32", 0, 0, 0, run_upgrade_1a70, 0},
};
const int newgame_map_load_ports_count = sizeof(newgame_map_load_ports) / sizeof(newgame_map_load_ports[0]);
