/* Imhotep - the saved-game loader as the original's entry points (Pharaoh.exe 0x004D58B0,
 * 0x004D5100, 0x004D51B0, 0x004D41E0). Copyright (C) 2026 Imhotep contributors. Licensed under the
 * GNU AGPL-3.0. */
#include "state/loader.h"

#include <stdlib.h>
#include <string.h>

#include "io/campaign_pak.h"
#include "io/save_file.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/post_load.h"
#include "state/pre_load.h"
#include "state/scenario_start.h"
#include "state/vec.h"

#define DIFFICULTY 0x00E39080u
#define CURRENT_PLAYER 0x00EAAC28u
#define CITY(st) (0x00EA1870u + 0x49D8u * U32(st, CURRENT_PLAYER))

/* a NUL-terminated string of the state (path names), or NULL */
static const char *state_cstr(game_state *st, uint32_t address)
{
    const char *p = (const char *) state_ptr(st, address, 1);
    for (uint32_t n = 0; p && n < 260; n++) {
        const char *q = (const char *) state_ptr(st, address + n, 1);
        if (!q) {
            return NULL;
        }
        if (*q == 0) {
            return p;
        }
    }
    return NULL;
}

/* @bin 0x004D4720 the image size of an empire object: i16 width / height at 0x010D6A0C / 0x010D6A0E
 * + 64 * id (the image table the graphics slots fill) */
static int state_image_size(void *ctx, int32_t id, int16_t *width, int16_t *height)
{
    game_state *st = ctx;
    *width = S16(st, 0x010D6A0Cu + 64u * (uint32_t) id);
    *height = S16(st, 0x010D6A0Eu + 64u * (uint32_t) id);
    return 0;
}

int loader_read(game_state *st, const uint8_t *data, size_t len,
                int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height), void *image_ctx,
                save_load_stats *stats)
{
    save_file s;
    if (save_file_read(data, len, &s) != SAVE_OK) {
        return -1;
    }
    /* @bin 0x004D41E0 inputs of the steps that the file does not hold: the pyramid kind of the
     * previous scenario start (0x005E4EF4), the image count (0x0126517C), the CRT rand state (read
     * from the state by save_file_load_state) */
    save_load_env le = {0, S32(st, 0x005E4EF4u), image_size ? image_size : state_image_size,
                        image_size ? image_ctx : st, S32(st, 0x0126517Cu)};
    save_file_load_state(st, &s, &le, stats);
    save_file_free(&s);
    return 0;
}

int32_t loader_file_read_at(game_state *st, const char *path, uint8_t *buf, uint32_t size, uint32_t offset)
{
    /* @bin 0x004CE480 _open, _lseek(offset), _read(size), _close: the bytes read, when > 0. Otherwise
     * the original retries with the CD drive's path (0x005E05AC, the "insert the CD" box 0x004CE9E0)
     * and the alternative directories (0x004CE330, 0x004CE100): the platform's (it serves the game's
     * files wherever they are), so here the file cannot be read: 0. */
    int32_t n = platform_file_read(st, path, offset, buf, size);
    if (n < 0 || (uint32_t) n <= offset) {
        return 0;
    }
    uint32_t got = (uint32_t) n - offset;
    return (int32_t) (got < size ? got : size);
}

/* The whole file at `path` from `offset` on, or NULL (the caller frees it). */
static uint8_t *read_from(game_state *st, const char *path, uint32_t offset, uint32_t end, size_t *len)
{
    int32_t n = platform_file_read(st, path, 0, NULL, 0);
    if (n < 0) {
        return NULL;
    }
    if (end == 0 || end > (uint32_t) n) {
        end = (uint32_t) n;
    }
    *len = end > offset ? end - offset : 0;
    uint8_t *data = malloc(*len ? *len : 1);
    if (data && *len) {
        platform_file_read(st, path, offset, data, (uint32_t) *len);
    }
    return data;
}

int32_t loader_campaign_record(game_state *st, int32_t archive, int32_t no_start)
{
    /* @bin 0x004D58B0 */
    int8_t record = S8(st, 0x00E38EA8u);
    if (record == 0x7F) {
        return 0;
    }
    int32_t difficulty = S32(st, DIFFICULTY);
    GAME_CALL0(st, 0x004CE2D0u); /* working directory */
    const char *path = state_cstr(st, 0x005E1B98u + 0x14u * (uint32_t) archive);
    if (!path) {
        return 0;
    }
    /* @bin 0x004D58B0 the record's offset: 4 bytes at 4 * record (movsx: signed) */
    uint8_t b[4];
    if (loader_file_read_at(st, path, b, 4, (uint32_t) ((int32_t) record << 2)) == 0) {
        return 0;
    }
    int32_t offset = (int32_t) ((uint32_t) b[0] | (uint32_t) b[1] << 8 | (uint32_t) b[2] << 16 | (uint32_t) b[3] << 24);
    if (offset < 1) {
        return 0;
    }
    /* @bin 0x004D58B0 _open (-1: return 0), _lseek(offset), 0x004D41E0, _close. The loader reads
     * the record sequentially and stops after its last chunk: the record ends at the next record's
     * offset or the end of the file (campaign_pak_record; @diff terrain_check) */
    uint8_t table[4 * CAMPAIGN_PAK_SLOTS];
    int32_t size = platform_file_read(st, path, 0, table, sizeof(table));
    if (size < 0) {
        return 0;
    }
    uint32_t end = (uint32_t) size;
    for (int k = 0; size >= (int32_t) sizeof(table) && k < CAMPAIGN_PAK_SLOTS; k++) {
        uint32_t o = (uint32_t) table[4 * k] | (uint32_t) table[4 * k + 1] << 8 | (uint32_t) table[4 * k + 2] << 16 |
                     (uint32_t) table[4 * k + 3] << 24;
        if (o > (uint32_t) offset && o < end) {
            end = o;
        }
    }
    size_t len;
    uint8_t *data = read_from(st, path, (uint32_t) offset, end, &len);
    if (data) {
        save_load_stats stats;
        loader_read(st, data, len, NULL, NULL, &stats);
        free(data);
    }
    S32(st, DIFFICULTY) = difficulty;
    S32(st, CITY(st) + 0x44A0u) = 0; /* @bin 0x004D58B0 0x00EA5D10 + player * 0x49D8 := 0 */
    if (no_start == 0) {
        floodplain_settings_copy(st);
        scenario_start(st);
    }
    return 1;
}

int loader_load_save_bytes(game_state *st, const uint8_t *data, size_t len, int keep_difficulty,
                           int (*image_size)(void *ctx, int32_t id, int16_t *width, int16_t *height),
                           void *image_ctx, save_load_stats *stats)
{
    /* @bin 0x004D5100 / 0x004D51B0 after the file opened */
    int32_t difficulty = S32(st, DIFFICULTY);
    GAME_CALLT(st, 0x00E92BF8u, 0x0055F561u, 2); /* the sound stream of channel 2 stops */
    U32(st, 0x00F55B94u) = 0;
    int rc = loader_read(st, data, len, image_size, image_ctx, stats);
    if (keep_difficulty) {
        /* @bin 0x004D51B0 the difficulty kept; treasury := 0x004CDE40(treasury, 0x005D1960[d]) */
        S32(st, DIFFICULTY) = difficulty;
        int32_t percent = S32(st, 0x005D1960u + 4u * (uint32_t) difficulty);
        int32_t t = GAME_CALL(st, 0x004CDE40u, S32(st, CITY(st) + 0x0Cu), percent);
        U8(st, 0x00D3B1B8u) = 1;
        S32(st, CITY(st) + 0x0Cu) = t;
    } else {
        U8(st, 0x00D3B1B8u) = 1; /* @bin 0x004D5100 byte store */
    }
    scenario_start(st);
    load_rebuild_storage_links(st); /* 0x0046B320 */
    load_rebuild_0x88_links(st);    /* 0x0046F030 */
    load_rebuild_0x4a_links(st);    /* 0x0046A320 */
    /* @bin 0x004D5100 strcpy(0x00EA184C, 0x00E38E74): the player name */
    const char *src = state_cstr(st, 0x00E38E74u);
    if (src) {
        size_t n = strlen(src) + 1;
        uint8_t *dst = state_ptr(st, 0x00EA184Cu, (uint32_t) n);
        if (dst) {
            memmove(dst, src, n);
        }
    }
    return rc;
}

int32_t loader_load_save_path(game_state *st, const char *path, int keep_difficulty)
{
    /* @bin 0x004D5100 / 0x004D51B0: floodplain settings 0x004BD330 (this = 0x00D38360), grids
     * cleared 0x0046FA40, working directory 0x004CE2D0, 0x00D3B1B8 := 0 (byte), _open (-1: return 1) */
    floodplain_settings_reset(st);
    pre_load_clear(st);
    GAME_CALL0(st, 0x004CE2D0u);
    U8(st, 0x00D3B1B8u) = 0;
    size_t len;
    uint8_t *data = path ? read_from(st, path, 0, 0, &len) : NULL;
    if (!data) {
        return 1;
    }
    save_load_stats stats;
    loader_load_save_bytes(st, data, len, keep_difficulty, NULL, NULL, &stats);
    free(data);
    return 0;
}

int32_t loader_load_save(game_state *st, uint32_t path)
{
    return loader_load_save_path(st, state_cstr(st, path), 0);
}

int32_t loader_load_save_keep_difficulty(game_state *st, uint32_t path)
{
    return loader_load_save_path(st, state_cstr(st, path), 1);
}

void floodplain_settings_copy(game_state *st)
{
    /* @bin 0x004BD360 (this = 0x00D38360) */
    const uint32_t f = 0x00D38360u;
    S32(st, f + 0x0C) = S32(st, f + 0x00);
    S32(st, f + 0x10) = S32(st, f + 0x04);
    S32(st, f + 0x14) = S32(st, f + 0x08);
    S32(st, f + 0x1C) = S32(st, f + 0x08);
    S32(st, f + 0x18) = 0;
}

/* ---- registered ports ---------------------------------------------------------------------- */

static int32_t run_campaign_record(game_state *st, const int32_t *a)
{
    return loader_campaign_record(st, a[0], a[1]);
}

static int32_t run_load_save(game_state *st, const int32_t *a)
{
    return loader_load_save(st, (uint32_t) a[0]);
}

static int32_t run_load_save_keep(game_state *st, const int32_t *a)
{
    return loader_load_save_keep_difficulty(st, (uint32_t) a[0]);
}

static int32_t run_file_read_at(game_state *st, const int32_t *a)
{
    const char *path = state_cstr(st, (uint32_t) a[0]);
    uint8_t *buf = state_ptr(st, (uint32_t) a[1], (uint32_t) a[2]);
    if (!path || !buf) {
        state_unported(st, 0x004CE480u); /* a buffer outside the state (the caller's stack) */
        return 0;
    }
    return loader_file_read_at(st, path, buf, (uint32_t) a[2], (uint32_t) a[3]);
}

static int32_t run_floodplain_reset(game_state *st, const int32_t *a)
{
    (void) a;
    floodplain_settings_reset(st);
    return 0;
}

static int32_t run_floodplain_copy(game_state *st, const int32_t *a)
{
    (void) a;
    floodplain_settings_copy(st);
    return 0;
}

static int32_t run_route_rebuild(game_state *st, const int32_t *a)
{
    save_load_env le = {U32(st, 0x00604950u), S32(st, 0x005E4EF4u), state_image_size, st, S32(st, 0x0126517Cu)};
    return save_route_rebuild_state(st, (uint32_t) a[PORT_THIS_ARG], &le);
}

/* the growable arrays (core/state/vec.c) work on objects of the data segments only */
static int vec_in_segments(game_state *st, uint32_t vec)
{
    if (vec < STATE_BASE || vec + 0x18u > STATE_END) {
        state_unported(st, 0x004F6700u);
        return 0;
    }
    return 1;
}

static int32_t run_vec_reserve(game_state *st, const int32_t *a)
{
    uint32_t vec = (uint32_t) a[PORT_THIS_ARG];
    return vec_in_segments(st, vec) ? vec_reserve(st, vec, a[0]) : 0;
}

static int32_t run_vec_reset(game_state *st, const int32_t *a)
{
    uint32_t vec = (uint32_t) a[PORT_THIS_ARG];
    if (vec_in_segments(st, vec)) {
        vec_reset(st, vec);
    }
    return 1;
}

const ported_function loader_ports[] = {
    {0x004D58B0u, "loader_campaign_record", 2, 32, 0, run_campaign_record, 0},
    {0x004D5100u, "loader_load_save", 1, 32, 0, run_load_save, 0},
    {0x004D51B0u, "loader_load_save_keep_difficulty", 1, 32, 0, run_load_save_keep, 0},
    {0x004CE480u, "loader_file_read_at (platform_file_read)", 4, 32, 0, run_file_read_at, 0},
    {0x004BD330u, "floodplain_settings_reset", 0, 0, 0x00D38360u, run_floodplain_reset, 0},
    {0x004BD360u, "floodplain_settings_copy", 0, 0, 0x00D38360u, run_floodplain_copy, 0},
    {0x004D4720u, "empire_route_rebuild (save_load.c)", 0, 8, 0, run_route_rebuild, 0},
    {0x004F6700u, "vec_reserve", 1, 8, 0, run_vec_reserve, 0},
    {0x004F6760u, "vec_reset", 0, 32, 0, run_vec_reset, 0},
    /* the loader and its steps, ported in core/io/save_file.c and save_load.c on the bytes of the
     * file (loader_read): reached only through their callers */
    {0x004D41E0u, "loader (loader_read: save_file.c, save_load.c)", 1, 0, 0, NULL, 0},
    {0x005662E3u, "CRT _lseek (an offset of platform_file_read)", 3, 32, 0, NULL, 0},
    {0x004D3C40u, "chunk 175 defaults below version 154 (save_file.c)", 0, 0, 0, NULL, 0},
    {0x004D0A40u, "shipwright queues from chunk 140 (save_load.c)", 1, 0, 0, NULL, 0},
    {0x004F67E0u, "shipwright queue reserve (save_load.c)", 1, 0, 0, NULL, 0},
    {0x004F6810u, "shipwright queue fields (save_load.c)", 1, 0, 0, NULL, 0},
    {0x004D32F0u, "loader step (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3D00u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3F60u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D4180u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004BD5D0u, "loader upgrade: floodplain +0x1C (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D1840u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D1860u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D1880u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D18B0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D18F0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D1AD0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D2FA0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3200u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D32A0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3C20u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3CC0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3DA0u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
    {0x004D3F40u, "loader upgrade (save_load.c)", 0, 0, 0, NULL, 0},
};
const int loader_ports_count = sizeof(loader_ports) / sizeof(loader_ports[0]);
