/* Imhotep - run ported functions on a state file, replaying the original's unported calls (the
 * port's side of tools/oracle/replay.py, used by harness.py and lockstep.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_replay IN REPLAY OUT [--replay ADDR,ADDR... | --replay-all] [--map ADDR:FILE ...]
 *        [--clock MS] [--cursor X,Y] [--keys DOWN,.../PRESSED,...] [--sound FILE]
 *
 * REPLAY (format: tools/oracle/replay.py) lists top-level calls, each followed by the calls the
 * original made inside it to functions that are not ported, in order, with what they changed and
 * returned. Each top-level call runs its port; every sim_call it makes to an unported function
 * must be the next recorded call (same address) and gets the recorded changes and eax. Partial
 * ports run natively and fall back to their record when they reach unported code. --replay
 * replays these ported functions instead (the recorder must have recorded them). --map makes a file
 * readable at ADDR (state_map_external: game data the original keeps outside its segments);
 * --map files=DIR serves the game's file reads (platform_file_read, core/sim/platform.h) and the
 * pixel loads (platform_pixels_load: the handle is the original's buffer address, from the record)
 * from the game directory DIR, names matched case-insensitively as tools/oracle/game.py does; the
 * player's files (platform_file_read, platform_file_write) from the directory IMHOTEP_PLAYER_FILES
 * first when it is set (written files go there; "file_write PATH SIZE" is printed); directory
 * listings (platform_file_list) merge the two directories, names compared case-insensitively, in
 * NTFS order (by name, upper-cased), as tools/oracle/game.py lists them for the original. The
 * window-system requests (web address, posted messages, quit, player file deletion) are printed and
 * logged to IMHOTEP_REQUEST_LOG (see request_log below).
 * --screen W,H,PITCH,FORMAT,IN,OUT gives the port a back buffer (phase 7, docs/oracle.md "Display"):
 * PITCH * H bytes read from IN, mapped writable at the address the state's pointer 0x00E92E18 holds
 * (W, H, PITCH must be the state's 0x00E38DB0, 0x00E38DAC, 0x00E38D94; FORMAT 555 or 565, the state's
 * 0x00E92DFC 0 or 2), written to OUT at the end; the recorded calls' writes into it are applied.
 * --rw ADDR,IN,OUT does the same for another buffer outside the state (the spare screen [0x00E92E1C]).
 * They are mapped before the --map files, which they may overlap (the oracle's other heap, mapped
 * read-only for its pixel data: docs/formats/state.md "Game data outside the segments").
 * --sound FILE: the original's sound requests (tools/oracle/sound.py, docs/oracle.md "Sound"): the
 * port's sound requests (printed "sound ...") must be these, in order; a mismatch exits 4.
 * A top-level call to a function with no port is replayed whole from its record when the recorder
 * recorded it so (tools/oracle/replay.py call(whole=True)); "whole ADDR" is printed.
 * Prints "return N" per top-level call, "replay ADDR" / "native ADDR" / "fallback ADDR" per
 * recorded call, "args ADDR i original port" when a replayed call's arguments differ.
 * Exit: 0, 4 on a call sequence mismatch, 5 when a replayed call wrote its caller's stack (an
 * output buffer: it needs a port), 6 when a replayed call got other arguments than the original's
 * (other than addresses in the original's stack), 2 on errors. */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "present/present.h"
#include "state/ported.h"
#include "state/state.h"

#define NREC_ARGS 8
#define STACK_LOW 0x7DF00000u  /* the oracle's stack (tools/oracle/emu.py STACK_TOP - STACK_SIZE .. STACK_TOP) */
#define STACK_HIGH 0x7E000000u

typedef struct {
    uint32_t address, kind;
    int32_t eax;
    uint32_t nargs;
    const uint8_t *args;
    uint32_t this_ptr, flags, nested, heap_used, other_top, nblocks, nranges;
    const uint8_t *blocks, *ranges;
} record;

typedef struct {
    record *rec;
    uint32_t count, next;
    int replay_all;
    uint32_t replay[1024];
    int nreplay;
    int mismatch, stack_writes, bad_args;
    game_state *backup[16]; /* a state copy per depth of partial ports run natively (fallback) */
    int depth;
} replay_ctx;

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

static int forced_replay(const replay_ctx *c, uint32_t a)
{
    if (c->replay_all) {
        return 1;
    }
    for (int i = 0; i < c->nreplay; i++) {
        if (c->replay[i] == a) {
            return 1;
        }
    }
    return 0;
}

/* the record of the platform request being run (pixels_load takes the original's buffer address
 * from it: the one value the request writes, slot +0xCC24 of 0x0053F660) */
static const record *platform_record;

/* --other-heap TOP: the platform's memory (sim_platform.memory) and display modes (set_display), at
 * the addresses the original's allocations take in the oracle's other heap, a bump allocator in
 * 16-byte steps (tools/oracle/emu.py alloc): other_top follows it (each applied record gives the top
 * after the original's call, replay.py) */
static int have_other_heap;
static uint32_t other_top;
static game_state *replay_state;
/* the records of the partial ports running natively, innermost last (set_display's buffer) */
static const record *partial_records[16];
static int npartial_records;

static void note_record(const record *r)
{
    if (have_other_heap && r->other_top > other_top) {
        other_top = r->other_top;
    }
}

/* [address, address + size) zeroed, mapped writable when nothing maps it (the oracle's other heap is
 * mapped as it is after the original's run: a block the original allocated holds its final bytes) */
static int zero_block(uint32_t address, uint32_t size)
{
    uint8_t *p = state_ptr(replay_state, address, size);
    if (!p) {
        uint8_t *buf = calloc(size ? size : 1, 1);
        if (!buf || state_map_external(replay_state, address, buf, size) != 0) {
            return -1;
        }
        return 0;
    }
    memset(p, 0, size);
    return 0;
}

static uint32_t platform_memory(void *ctx, uint32_t address, uint32_t size)
{
    (void) ctx;
    if (size == 0) {
        printf("memory free 0x%08X\n", address);
        return 0; /* the oracle's free does not reuse or clear other-heap blocks */
    }
    uint32_t p = other_top;
    other_top += (size + 15u) & ~15u;
    printf("memory 0x%08X %u\n", p, size);
    return zero_block(p, size) == 0 ? p : 0;
}

/* the original's display mode change is its DirectDraw's (emulated surfaces in the other heap): the
 * new back buffer is the one the original's set-up made (the value its record writes at 0x00E92E18),
 * zeroed, and the other heap's top is the record's (the platform's allocations are the original's) */
static uint32_t platform_set_display_replay(void *ctx, int32_t width, int32_t height, int32_t rgb565)
{
    (void) ctx;
    const record *r = npartial_records ? partial_records[npartial_records - 1] : NULL;
    uint32_t back = 0;
    const uint8_t *p = r ? r->ranges : NULL;
    for (uint32_t i = 0; r && i < r->nranges; i++) {
        uint32_t a = rd32(p), n = rd32(p + 4);
        if (a <= 0x00E92E18u && 0x00E92E18u + 4 <= a + n) {
            back = rd32(p + 8 + (0x00E92E18u - a));
        }
        p += 8 + n;
    }
    printf("set_display %dx%d %s -> 0x%08X\n", width, height, rgb565 ? "565" : "555", back);
    if (back == 0 || width <= 0 || height <= 0 || zero_block(back, (uint32_t) (2 * width * height)) != 0) {
        return 0;
    }
    if (r && r->other_top > other_top) {
        other_top = r->other_top;
    }
    return back;
}

static void apply(game_state *st, const record *r)
{
    note_record(r);
    if (r->heap_used > st->heap_used) {
        state_heap_resize(st, r->heap_used);
    }
    for (uint32_t i = 0; i < r->nblocks; i++) {
        state_heap_add_block(st, rd32(r->blocks + 8 * i), rd32(r->blocks + 8 * i + 4));
    }
    const uint8_t *p = r->ranges;
    for (uint32_t i = 0; i < r->nranges; i++) {
        uint32_t a = rd32(p), n = rd32(p + 4);
        uint8_t *dst = state_ptr(st, a, n);
        if (dst) {
            memcpy(dst, p + 8, n);
        }
        p += 8 + n;
    }
}

/* after a native run of a partial port or platform request: its nested records must all have been
 * consumed (the original made these unported calls inside it) */
/* the end of the nested records of the partial ports and platform requests running natively,
 * innermost last */
static uint32_t nest_ends[32];
static int nest_platform[32]; /* 1: a platform request (its original's calls are presentation) */
static int nnest;

/* inside a native partial port, a nested record with no effect on the state (nothing written, no new
 * block: e.g. a sprintf into a stack buffer for OutputDebugStringA, 0x004D0480) that the port does not
 * make is skipped ("skipped ADDR"); inside a platform request (the original's run is presentation:
 * core/sim/platform.h, tools/oracle/excluded.py) any nested record the port does not make is ("skipped
 * ADDR (platform)": the log buffer 0x004CFF20 of a video that does not open, ...) */
static int skip_inert(replay_ctx *c, uint32_t end, int platform)
{
    if (c->next >= end) {
        return 0;
    }
    const record *r = &c->rec[c->next];
    if (platform) {
        printf("skipped 0x%08X (platform)\n", r->address);
        c->next += 1 + r->nested;
        return 1;
    }
    if (r->kind != 1 || r->nranges != 0 || r->nblocks != 0 || r->nested != 0) {
        return 0;
    }
    printf("skipped 0x%08X (no effect)\n", r->address);
    c->next++;
    return 1;
}

static void nested_done(replay_ctx *c, uint32_t address, uint32_t end, int platform)
{
    while (!c->mismatch && c->next != end && skip_inert(c, end, platform)) {
    }
    if (!c->mismatch && c->next != end) {
        printf("mismatch: the port of 0x%08X made fewer calls than the original (next 0x%08X, record %u)\n", address,
               c->rec[c->next].address, c->next);
        c->mismatch = 1;
    }
}

static uint32_t fallback_reached;

/* a partial port run natively, its nested records consumed by its calls; 1 when it ran to its end,
 * 0 when it reached unported code (the state is then restored, c->next is left to the caller) */
static int run_partial(replay_ctx *c, game_state *st, const ported_function *p, int32_t *a, int32_t *result,
                       const record *r)
{
    if (c->depth >= 16) {
        printf("partial ports nested too deep\n");
        c->mismatch = 1;
        return 0;
    }
    if (!c->backup[c->depth]) {
        c->backup[c->depth] = calloc(1, sizeof(game_state));
        if (!c->backup[c->depth] || state_create(c->backup[c->depth]) != 0) {
            printf("out of memory\n");
            c->mismatch = 1;
            return 0;
        }
    }
    game_state *backup = c->backup[c->depth++];
    uint32_t before = st->unported;
    state_copy(backup, st);
    uint32_t top = other_top;
    partial_records[npartial_records++] = r;
    nest_platform[nnest] = 0;
    nest_ends[nnest++] = (uint32_t) (r - c->rec) + 1 + r->nested;
    *result = p->run(st, a);
    nnest--;
    npartial_records--;
    c->depth--;
    if (st->unported == before) {
        return 1;
    }
    fallback_reached = st->unported;
    other_top = top;
    const sim_env *env = st->env;
    const sim_platform *platform = st->platform;
    state_copy(st, backup);
    st->env = env;
    st->platform = platform;
    st->unported = before;
    return 0;
}

static int external(const sim_env *env, game_state *st, uint32_t address, const int32_t *args, int nargs,
                    uint32_t this_ptr, int32_t *result)
{
    replay_ctx *c = env->ctx;
    *result = 0;
    if (c->mismatch) {
        return 1;
    }
    const ported_function *p = ported_find(address);
    int forced = forced_replay(c, address);
    if (p && !p->run) {
        printf("inline 0x%08X: ported inside its callers only, needs a port of its own\n", address);
        c->mismatch = 1;
        return 1;
    }
    if (p && !p->partial && !forced) {
        return 0; /* natively, by sim_call */
    }
    while (nnest > 0 && (c->next >= c->count || c->rec[c->next].address != address) &&
           skip_inert(c, nest_ends[nnest - 1], nest_platform[nnest - 1])) {
    }
    if (c->next >= c->count || c->rec[c->next].kind != 1 || c->rec[c->next].address != address) {
        printf("mismatch: the port calls 0x%08X, the original 0x%08X (record %u)\n", address,
               c->next < c->count ? c->rec[c->next].address : 0, c->next);
        c->mismatch = 1;
        return 1;
    }
    uint32_t index = c->next++;
    const record *r = &c->rec[index];
    uint32_t end = index + 1 + r->nested; /* its nested records: the port's own unported calls inside it */
    if (end > c->count) {
        printf("bad record %u: %u nested records past the end\n", index, r->nested);
        c->mismatch = 1;
        return 1;
    }
    if (p && p->partial == PORT_PLATFORM && !forced) {
        /* the original's own run is presentation (excluded from comparisons): the port's request */
        int32_t a[16] = {0};
        for (int i = 0; i < nargs && i < 15; i++) {
            a[i] = args[i];
        }
        a[PORT_THIS_ARG] = (int32_t) this_ptr;
        const record *outer = platform_record;
        platform_record = r;
        nest_platform[nnest] = 1;
        nest_ends[nnest++] = end;
        *result = p->run(st, a);
        nnest--;
        platform_record = outer;
        printf("platform 0x%08X\n", address);
        note_record(r); /* the platform's allocations are the original's */
        nested_done(c, address, end, 1);
        return 1;
    }
    if (p && !forced) { /* partial: run it, back to the record if it reaches unported code */
        int32_t a[16] = {0};
        for (int i = 0; i < nargs && i < 15; i++) {
            a[i] = args[i];
        }
        a[PORT_THIS_ARG] = (int32_t) this_ptr;
        int32_t r_native;
        if (run_partial(c, st, p, a, &r_native, r)) {
            *result = r_native;
            printf("native 0x%08X\n", address);
            nested_done(c, address, end, 0);
            return 1;
        }
        if (c->mismatch) {
            return 1;
        }
        printf("fallback 0x%08X (reached 0x%08X)\n", address, fallback_reached);
    } else {
        printf("replay 0x%08X\n", address);
    }
    c->next = end;
    for (int i = 0; i < nargs && i < (int) r->nargs; i++) {
        int32_t o = (int32_t) rd32(r->args + 4 * i);
        if (o != args[i]) {
            printf("args 0x%08X %d original %d port %d\n", address, i, o, args[i]);
            /* a wrong argument to a replayed call would be hidden by the replay (its recorded effects
             * are applied whatever the port passed): a failure, except an address in the original's
             * stack (a buffer of the caller's frame: the port's own buffers are elsewhere) */
            if (!((uint32_t) o >= STACK_LOW && (uint32_t) o <= STACK_HIGH)) {
                c->bad_args = 1;
            }
        }
    }
    if (r->flags & 1) {
        printf("stack 0x%08X: writes its caller's stack, needs a port\n", address);
        c->stack_writes = 1;
    }
    apply(st, r);
    *result = r->eax;
    return 1;
}

/* --video W,H: BinkOpen opens a video of W x H with no frame (tools/oracle/game.py Game.bink): the
 * platform's video_open answers the same, video_modal shows nothing */
static int have_video;
static int32_t video_w, video_h;

static int video_open(void *ctx, const char *name, int32_t *width, int32_t *height)
{
    (void) ctx;
    printf("video_open %s -> %d\n", name, have_video);
    if (!have_video) {
        return 0;
    }
    *width = video_w;
    *height = video_h;
    return 1;
}

static void video_modal(void *ctx, int32_t x, int32_t y)
{
    (void) ctx;
    printf("video_modal %d,%d\n", x, y);
}

/* --clock MS: the wall clock the original read (Game.clock: timeGetTime / GetTickCount) */
static uint32_t clock_value;

/* --cursor X,Y and --keys DOWN/PRESSED (comma lists of virtual keys): the scripted input the
 * original's GetCursorPos / GetAsyncKeyState return (tools/oracle/game.py Game.cursor, keys_down,
 * keys_pressed); bit 0 ("pressed since the last call") is consumed by the first read, as there */
static int have_cursor;
static int32_t cursor_x, cursor_y;
static uint8_t keys_down[256], keys_pressed[256];

static int cursor_pos(void *ctx, int32_t *x, int32_t *y)
{
    (void) ctx;
    if (!have_cursor) {
        return 0;
    }
    *x = cursor_x;
    *y = cursor_y;
    return 1;
}

static int16_t key_state(void *ctx, int32_t vk)
{
    (void) ctx;
    if (vk < 0 || vk > 255) {
        return 0;
    }
    int16_t r = keys_down[vk] ? (int16_t) 0x8000 : 0;
    if (keys_pressed[vk]) {
        keys_pressed[vk] = 0;
        r |= 1;
    }
    return r;
}

static void key_list(const char *s, uint8_t *set)
{
    while (*s) {
        char *end;
        unsigned long vk = strtoul(s, &end, 0);
        if (end == s) {
            break;
        }
        if (vk < 256) {
            set[vk] = 1;
        }
        s = *end == ',' ? end + 1 : end;
    }
}

static uint32_t clock_ms(void *ctx)
{
    (void) ctx;
    return clock_value;
}

static uint32_t pixels_load(void *ctx, const char *path, uint32_t skip, uint32_t size, int32_t convert, int32_t rle)
{
    (void) convert;
    (void) rle;
    (void) ctx;
    (void) path;
    (void) skip;
    (void) size;
    if (platform_record && platform_record->nranges > 0 && rd32(platform_record->ranges + 4) == 4) {
        return rd32(platform_record->ranges + 8);
    }
    return 0;
}

/* DIR + the game's relative path, '\\' as '/', each part matched case-insensitively (a part that
 * matches nothing is kept as it is; with mkdirs, missing directories are created) */
static void resolve(const char *dir, const char *path, char *full, size_t cap, int mkdirs)
{
    snprintf(full, cap, "%s", dir);
    for (const char *p = path; *p;) {
        char part[256];
        size_t n = 0;
        while (*p && *p != '\\' && *p != '/' && n + 1 < sizeof(part)) {
            part[n++] = *p++;
        }
        part[n] = 0;
        while (*p == '\\' || *p == '/') {
            p++;
        }
        DIR *d = opendir(full);
        const char *match = part;
        struct dirent *e;
        while (d && (e = readdir(d)) != NULL) {
            if (strcasecmp(e->d_name, part) == 0) {
                match = e->d_name;
                break;
            }
        }
        size_t l = strlen(full);
        snprintf(full + l, cap - l, "/%s", match);
        if (d) {
            closedir(d);
        }
        if (mkdirs && *p) {
            mkdir(full, 0755);
        }
    }
}

/* --sound FILE: the original's sound requests, one per line (tools/oracle/sound.py: its Miles calls
 * translated). Each sound request of the port is printed ("sound LINE"); with --sound it must be the
 * next line (the stream's volume is compared only when the stream opened), and the port's queries get
 * the original's answers (opened, playing, volume) */
static char **sound_lines;
static int sound_count, sound_next, sound_failed, whole_replays;

static int read_sound(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0]) {
            continue;
        }
        char **grown = realloc(sound_lines, sizeof(char *) * (size_t) (sound_count + 1));
        if (!grown) {
            fclose(f);
            return -1;
        }
        sound_lines = grown;
        sound_lines[sound_count++] = strdup(line);
    }
    fclose(f);
    return 0;
}

/* the port's request `mine` against the next expected line: equal up to `n` characters (the whole
 * line when n < 0); the expected line, or NULL (printed: a mismatch) */
static const char *sound_expect(const char *mine, int n)
{
    printf("sound %s\n", mine);
    if (!sound_lines || sound_failed) {
        return NULL;
    }
    const char *want = sound_next < sound_count ? sound_lines[sound_next] : "(none)";
    int same = n < 0 ? strcmp(mine, want) == 0 : strncmp(mine, want, (size_t) n) == 0 && strlen(want) >= (size_t) n;
    if (sound_next >= sound_count || !same) {
        printf("sound mismatch: the port asks '%s', the original '%s' (request %d)\n", mine, want, sound_next);
        sound_failed = 1;
        return NULL;
    }
    sound_next++;
    return want;
}

/* "... -> R": R */
static int32_t sound_answer(const char *line)
{
    const char *r = line ? strstr(line, "-> ") : NULL;
    return r ? (int32_t) strtol(r + 3, NULL, 10) : 0;
}

static int32_t sound_stream_play(void *ctx, const char *name, int32_t channel, int32_t loops, int32_t volume)
{
    (void) ctx;
    char mine[1024];
    snprintf(mine, sizeof(mine), "stream_play %d %d %d ? %s", channel, loops, volume, name);
    printf("sound %s\n", mine);
    if (!sound_lines || sound_failed) {
        return 0;
    }
    /* expected: stream_play CH LOOPS VOL|- OPENED NAME */
    const char *want = sound_next < sound_count ? sound_lines[sound_next] : "(none)";
    int ch = -1, lp = -1, opened = -1, used = 0;
    char vol[32] = "";
    int ok = sscanf(want, "stream_play %d %d %31s %d %n", &ch, &lp, vol, &opened, &used) == 4 && ch == channel &&
             lp == loops && strcmp(want + used, name) == 0 && (opened == 0 || atoi(vol) == volume);
    if (!ok) {
        printf("sound mismatch: the port asks '%s', the original '%s' (request %d)\n", mine, want, sound_next);
        sound_failed = 1;
        return 0;
    }
    sound_next++;
    return opened ? 0x5B000000 | channel : 0;
}

static void sound_stream_stop(void *ctx, int32_t channel)
{
    (void) ctx;
    char mine[64];
    snprintf(mine, sizeof(mine), "stream_stop %d", channel);
    sound_expect(mine, -1);
}

static int sound_stream_playing(void *ctx, int32_t channel)
{
    (void) ctx;
    char mine[64];
    snprintf(mine, sizeof(mine), "stream_playing %d ->", channel);
    return sound_answer(sound_expect(mine, (int) strlen(mine))) != 0;
}

static int32_t sound_stream_volume(void *ctx, int32_t channel)
{
    (void) ctx;
    char mine[64];
    snprintf(mine, sizeof(mine), "stream_volume %d ->", channel);
    return sound_answer(sound_expect(mine, (int) strlen(mine)));
}

static void sound_stream_set_volume(void *ctx, int32_t channel, int32_t volume)
{
    (void) ctx;
    char mine[64];
    snprintf(mine, sizeof(mine), "stream_set_volume %d %d", channel, volume);
    sound_expect(mine, -1);
}

static void sound_sample_play(void *ctx, int32_t sound, const char *name, int32_t volume, int32_t pan, int32_t loops)
{
    (void) ctx;
    (void) sound;
    char mine[1024];
    snprintf(mine, sizeof(mine), "sample_play %d %d %d %s", volume, pan, loops, name ? name : "?");
    sound_expect(mine, -1);
}

/* the player's files (saved games, the campaign progress): the directory IMHOTEP_PLAYER_FILES when
 * set, looked in before the game's; file writes go there (or are only counted without it) */
static const char *player_dir(void)
{
    const char *d = getenv("IMHOTEP_PLAYER_FILES");
    return d && *d ? d : NULL;
}

static int32_t file_write(void *ctx, const char *path, const uint8_t *data, uint32_t size)
{
    (void) ctx;
    printf("file_write %s %u\n", path, size);
    if (!player_dir()) {
        return (int32_t) size;
    }
    char full[4096];
    resolve(player_dir(), path, full, sizeof(full), 1);
    FILE *f = fopen(full, "wb");
    if (!f) {
        return -1;
    }
    size_t n = fwrite(data, 1, size, f);
    return fclose(f) == 0 && n == size ? (int32_t) size : -1;
}

/* the window-system requests (platform_open_url, platform_post_message, platform_quit,
 * platform_file_delete): printed ("open_url URL", "post_message MSG WPARAM LPARAM", "quit CODE",
 * "file_delete PATH R") and appended to the file IMHOTEP_REQUEST_LOG names, when set, for a check to
 * compare with the original's (tools/oracle/game.py Game.posted, Game.deleted); file_delete removes
 * the player file (or the empty directory) from IMHOTEP_PLAYER_FILES: 0, or -1 when there is none */
static void request_log(const char *line)
{
    printf("%s\n", line);
    const char *path = getenv("IMHOTEP_REQUEST_LOG");
    FILE *f = path && *path ? fopen(path, "a") : NULL;
    if (f) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
}

static void open_url(void *ctx, const char *url)
{
    (void) ctx;
    char line[1024];
    snprintf(line, sizeof(line), "open_url %s", url);
    request_log(line);
}

static void post_message(void *ctx, uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    (void) ctx;
    char line[128];
    snprintf(line, sizeof(line), "post_message 0x%X 0x%X 0x%X", msg, wparam, lparam);
    request_log(line);
}

static void quit_request(void *ctx, int32_t code)
{
    (void) ctx;
    char line[64];
    snprintf(line, sizeof(line), "quit %d", code);
    request_log(line);
}

static int32_t file_delete(void *ctx, const char *path)
{
    (void) ctx;
    int32_t r = -1;
    if (player_dir()) {
        char full[4096];
        resolve(player_dir(), path, full, sizeof(full), 0);
        r = remove(full) == 0 ? 0 : -1;
    }
    char line[1024];
    snprintf(line, sizeof(line), "file_delete %s %d", path, r);
    request_log(line);
    return r;
}

/* only a regular file opens, as with the original's _open: an empty name resolves to the
 * directory itself, which fopen would open on POSIX systems */
static FILE *open_regular(const char *full)
{
    struct stat sb;
    return stat(full, &sb) == 0 && S_ISREG(sb.st_mode) ? fopen(full, "rb") : NULL;
}

static int32_t file_read(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    char full[4096];
    FILE *f = NULL;
    if (player_dir()) {
        resolve(player_dir(), path, full, sizeof(full), 0);
        f = open_regular(full);
    }
    if (!f) {
        resolve((const char *) ctx, path, full, sizeof(full), 0);
        f = open_regular(full);
    }
    if (!f) {
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    if (buf && size && (long) offset < n) {
        fseek(f, (long) offset, SEEK_SET);
        size_t want = (size_t) (n - (long) offset) < size ? (size_t) (n - (long) offset) : size;
        if (fread(buf, 1, want, f) != want) {
            n = -1;
        }
    }
    fclose(f);
    return (int32_t) n;
}

/* '*' and '?' of a FindFirstFileA pattern, ASCII letters in any case */
static int wild(const char *m, const char *n)
{
    if (*m == 0) {
        return *n == 0;
    }
    if (*m == '*') {
        for (;; n++) {
            if (wild(m + 1, n)) {
                return 1;
            }
            if (*n == 0) {
                return 0;
            }
        }
    }
    if (*n == 0) {
        return 0;
    }
    if (*m != '?' && toupper((unsigned char) *m) != toupper((unsigned char) *n)) {
        return 0;
    }
    return wild(m + 1, n + 1);
}

/* NTFS order: by name, upper-cased (ASCII), byte by byte */
static int ntfs_cmp(const void *a, const void *b)
{
    const unsigned char *x = *(const unsigned char *const *) a, *y = *(const unsigned char *const *) b;
    for (;; x++, y++) {
        int u = toupper(*x), v = toupper(*y);
        if (u != v || u == 0) {
            return u - v;
        }
    }
}

static int32_t file_list(void *ctx, const char *pattern, int32_t index, char *name, uint32_t size)
{
    char folder[1024], full[4096];
    const char *slash = NULL;
    for (const char *p = pattern; *p; p++) {
        if (*p == '\\' || *p == '/') {
            slash = p;
        }
    }
    size_t n = slash ? (size_t) (slash - pattern) : 0;
    if (n >= sizeof(folder)) {
        return -1;
    }
    memcpy(folder, pattern, n);
    folder[n] = 0;
    const char *mask = slash ? slash + 1 : pattern;
    char *names[4096];
    int count = 0;
    const char *dirs[2] = {player_dir(), (const char *) ctx};
    for (int k = 0; k < 2; k++) {
        if (!dirs[k]) {
            continue;
        }
        resolve(dirs[k], folder, full, sizeof(full), 0);
        DIR *d = opendir(full);
        struct dirent *e;
        while (d && (e = readdir(d)) != NULL && count < 4096) {
            int seen = strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0 || !wild(mask, e->d_name);
            for (int i = 0; i < count && !seen; i++) {
                seen = strcasecmp(names[i], e->d_name) == 0;
            }
            if (!seen) {
                names[count++] = strdup(e->d_name);
            }
        }
        if (d) {
            closedir(d);
        }
    }
    qsort(names, (size_t) count, sizeof(names[0]), ntfs_cmp);
    int32_t len = -1;
    if (index >= 0 && index < count) {
        snprintf(name, size, "%s", names[index]);
        len = (int32_t) strlen(names[index]);
    }
    for (int i = 0; i < count; i++) {
        free(names[i]);
    }
    return len;
}

static int parse(replay_ctx *c, const uint8_t *data, size_t len)
{
    c->count = rd32(data);
    c->rec = calloc(c->count ? c->count : 1, sizeof(record));
    const uint8_t *p = data + 4, *end = data + len;
    for (uint32_t i = 0; i < c->count; i++) {
        record *r = &c->rec[i];
        if (p + 12 > end) {
            return -1;
        }
        r->address = rd32(p);
        r->kind = rd32(p + 4);
        if (r->kind == 0) {
            r->nargs = rd32(p + 8);
            r->args = p + 12;
            p += 12 + 4 * r->nargs;
            r->this_ptr = rd32(p);
            p += 4;
            continue;
        }
        r->eax = (int32_t) rd32(p + 8);
        r->nargs = rd32(p + 12);
        r->args = p + 16;
        p += 16 + 4 * r->nargs;
        r->flags = rd32(p);
        r->nested = rd32(p + 4);
        r->heap_used = rd32(p + 8);
        r->other_top = rd32(p + 12);
        r->nblocks = rd32(p + 16);
        r->blocks = p + 20;
        p += 20 + 8 * r->nblocks;
        r->nranges = rd32(p);
        r->ranges = p + 4;
        p += 4;
        for (uint32_t k = 0; k < r->nranges; k++) {
            p += 8 + rd32(p + 4);
        }
    }
    return p == end ? 0 : -1;
}

/* a buffer outside the state that the port writes (the back buffer, the spare screen) */
typedef struct {
    uint32_t address, size;
    uint8_t *data;
    const char *out;
} rw_buffer;

static uint8_t *read_whole(const char *path, uint32_t want)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    uint8_t *buf = calloc(want ? want : 1, 1);
    size_t n = buf ? fread(buf, 1, want, f) : 0;
    fclose(f);
    if (!buf || n != want) {
        free(buf);
        return NULL;
    }
    return buf;
}

static int file_size(const char *path, uint32_t *size)
{
    struct stat sb;
    if (stat(path, &sb) != 0) {
        return -1;
    }
    *size = (uint32_t) sb.st_size;
    return 0;
}

int main(int argc, char **argv)
{
    present_register(); /* the presentation code's ports (present/) */
    if (argc < 4) {
        fprintf(stderr, "usage: state_replay IN REPLAY OUT [--replay A,B... | --replay-all]\n");
        return 2;
    }
    replay_ctx c = {0};
    int nmaps = 0;
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--replay-all") == 0) {
            c.replay_all = 1;
        } else if (strcmp(argv[i], "--sound") == 0 && i + 1 < argc) {
            if (read_sound(argv[++i]) != 0) {
                fprintf(stderr, "cannot read %s\n", argv[i]);
                return 2;
            }
            if (!sound_lines) {
                sound_lines = calloc(1, sizeof(char *)); /* none expected */
            }
        } else if (strcmp(argv[i], "--video") == 0 && i + 1 < argc) {
            have_video = sscanf(argv[++i], "%d,%d", &video_w, &video_h) == 2;
        } else if (strcmp(argv[i], "--other-heap") == 0 && i + 1 < argc) {
            have_other_heap = 1;
            other_top = (uint32_t) strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--clock") == 0 && i + 1 < argc) {
            clock_value = (uint32_t) strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--cursor") == 0 && i + 1 < argc) {
            have_cursor = sscanf(argv[++i], "%d,%d", &cursor_x, &cursor_y) == 2;
        } else if (strcmp(argv[i], "--keys") == 0 && i + 1 < argc) {
            char *slash = strchr(argv[++i], '/');
            key_list(argv[i], keys_down);
            if (slash) {
                key_list(slash + 1, keys_pressed);
            }
        } else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc) {
            nmaps++; /* applied after the state is read */
            i++;
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            for (char *s = argv[++i]; *s && c.nreplay < 1024;) {
                c.replay[c.nreplay++] = (uint32_t) strtoul(s, &s, 16);
                if (*s == ',') {
                    s++;
                }
            }
        }
    }
    game_state st;
    FILE *f = fopen(argv[1], "rb");
    if (state_create(&st) != 0 || !f || state_read_file(&st, f) != 0) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);
    rw_buffer rw[8];
    int nrw = 0;
    for (int i = 4; i < argc; i++) {
        int screen = strcmp(argv[i], "--screen") == 0;
        if ((!screen && strcmp(argv[i], "--rw") != 0) || i + 1 >= argc || nrw >= 8) {
            continue;
        }
        char spec[4096], *field[6] = {0};
        snprintf(spec, sizeof(spec), "%s", argv[++i]);
        int nf = 0;
        for (char *t = strtok(spec, ","); t && nf < 6; t = strtok(NULL, ",")) {
            field[nf++] = t;
        }
        rw_buffer *b = &rw[nrw];
        const char *in;
        if (screen) {
            if (nf != 6) {
                fprintf(stderr, "bad --screen %s\n", argv[i]);
                return 2;
            }
            int32_t w = atoi(field[0]), h = atoi(field[1]), pitch = atoi(field[2]), format = atoi(field[3]);
            if (w != S32(&st, 0x00E38DB0u) || h != S32(&st, 0x00E38DACu) || pitch != S32(&st, 0x00E38D94u) ||
                (format == 565) != (S32(&st, 0x00E92DFCu) == 2) || (format != 565 && format != 555) || h <= 0 ||
                pitch < 2 * w) {
                fprintf(stderr, "--screen %dx%d pitch %d format %d: the state holds %dx%d pitch %d format %d\n", w, h,
                        pitch, format, S32(&st, 0x00E38DB0u), S32(&st, 0x00E38DACu), S32(&st, 0x00E38D94u),
                        S32(&st, 0x00E92DFCu));
                return 2;
            }
            b->address = U32(&st, 0x00E92E18u);
            b->size = (uint32_t) pitch * (uint32_t) h;
            in = field[4];
            b->out = argv[i] + (field[5] - spec);
        } else {
            if (nf != 3) {
                fprintf(stderr, "bad --rw %s\n", argv[i]);
                return 2;
            }
            b->address = (uint32_t) strtoul(field[0], NULL, 16);
            in = field[1];
            b->out = argv[i] + (field[2] - spec);
            if (file_size(in, &b->size) != 0) {
                fprintf(stderr, "cannot read %s\n", in);
                return 2;
            }
        }
        b->data = read_whole(in, b->size);
        if (!b->data || state_map_external(&st, b->address, b->data, b->size) != 0) {
            fprintf(stderr, "cannot map %s at 0x%08X\n", in, b->address);
            return 2;
        }
        nrw++;
    }
    sim_platform files = {0};
    for (int i = 4; i < argc && nmaps; i++) {
        if (strcmp(argv[i], "--map") != 0 || i + 1 >= argc) {
            continue;
        }
        if (strncmp(argv[i + 1], "files=", 6) == 0) {
            files.ctx = argv[i + 1] + 6;
            files.file_read = file_read;
            files.file_write = file_write;
            files.file_delete = file_delete;
            files.open_url = open_url;
            files.post_message = post_message;
            files.quit = quit_request;
            files.file_list = file_list;
            files.pixels_load = pixels_load;
            files.clock_ms = clock_ms;
            files.cursor_pos = cursor_pos;
            files.key_state = key_state;
            files.stream_play = sound_stream_play;
            files.stream_stop = sound_stream_stop;
            files.stream_playing = sound_stream_playing;
            files.stream_volume = sound_stream_volume;
            files.stream_set_volume = sound_stream_set_volume;
            files.sample_play = sound_sample_play;
            files.video_open = video_open;
            files.video_modal = video_modal;
            if (have_other_heap) {
                files.memory = platform_memory;
                files.set_display = platform_set_display_replay;
            }
            st.platform = &files;
            continue;
        }
        char *colon = strchr(argv[i + 1], ':');
        FILE *m = colon ? fopen(colon + 1, "rb") : NULL;
        if (!m) {
            fprintf(stderr, "bad --map %s\n", argv[i + 1]);
            return 2;
        }
        fseek(m, 0, SEEK_END);
        long n = ftell(m);
        fseek(m, 0, SEEK_SET);
        uint8_t *buf = malloc((size_t) n + 1);
        if (!buf || fread(buf, 1, (size_t) n, m) != (size_t) n) {
            return 2;
        }
        fclose(m);
        if (state_map_external(&st, (uint32_t) strtoul(argv[i + 1], NULL, 16), buf, (uint32_t) n) != 0) {
            fprintf(stderr, "too many --map\n");
            return 2;
        }
    }
    f = fopen(argv[2], "rb");
    if (!f) {
        fprintf(stderr, "cannot read %s\n", argv[2]);
        return 2;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t) len);
    if (!data || fread(data, 1, (size_t) len, f) != (size_t) len || parse(&c, data, (size_t) len) != 0) {
        fprintf(stderr, "cannot parse %s\n", argv[2]);
        return 2;
    }
    fclose(f);
    replay_state = &st;
    sim_env env = {external, &c};
    st.env = &env;
    int status = 0;
    while (c.next < c.count && !c.mismatch) {
        const record *top = &c.rec[c.next++];
        if (top->kind != 0) {
            printf("mismatch: the original made more calls (0x%08X, record %u)\n", top->address, c.next - 1);
            c.mismatch = 1;
            break;
        }
        const ported_function *p = ported_find(top->address);
        int32_t a[16] = {0};
        for (uint32_t i = 0; i < top->nargs && i < 15; i++) {
            a[i] = (int32_t) rd32(top->args + 4 * i);
        }
        int32_t ret;
        int whole = c.next < c.count && c.rec[c.next].kind == 1 && c.rec[c.next].address == top->address;
        if (whole && p && p->run && p->partial == PORT_PARTIAL && !forced_replay(&c, top->address)) {
            /* a partial port recorded whole: natively with its nested records, else the record */
            uint32_t index = c.next++, end = index + 1 + c.rec[index].nested;
            a[PORT_THIS_ARG] = (int32_t) (top->this_ptr ? top->this_ptr : p->this_ptr);
            if (run_partial(&c, &st, p, a, &ret, &c.rec[index])) {
                printf("native 0x%08X\n", top->address);
                nested_done(&c, top->address, end, 0);
                printf("return %d\n", ret);
                continue;
            }
            if (c.mismatch) {
                break;
            }
            printf("fallback 0x%08X (reached 0x%08X)\n", top->address, fallback_reached);
            c.next = index;
        }
        if ((!p || p->partial == PORT_PARTIAL || forced_replay(&c, top->address)) && whole) {
            /* recorded whole (replay.py call(whole=True)): the original's changes */
            printf("whole 0x%08X\n", top->address);
            whole_replays++;
            const record *r = &c.rec[c.next++];
            c.next += r->nested;
            if (r->flags & 1) {
                c.stack_writes = 1;
            }
            apply(&st, r);
            ret = r->eax;
            printf("return %d\n", ret);
            continue;
        }
        if (!p || !p->run) {
            fprintf(stderr, "0x%08X is not ported\n", top->address);
            return 2;
        }
        a[PORT_THIS_ARG] = (int32_t) (top->this_ptr ? top->this_ptr : p->this_ptr);
        /* a platform request called at the top: the original's calls inside it are the platform's
         * work, as inside a recorded one (nested records): those with no effect may be left out */
        uint32_t top_end = c.next;
        while (top_end < c.count && c.rec[top_end].kind != 0) {
            top_end++;
        }
        int platform_top = p->partial == PORT_PLATFORM;
        if (platform_top) {
            nest_platform[nnest] = 1;
            nest_ends[nnest++] = top_end;
        }
        ret = p->run(&st, a);
        if (platform_top) {
            nnest--;
            nested_done(&c, top->address, top_end, 1);
        }
        printf("return %d\n", ret);
        if (st.unported) {
            printf("unported: 0x%08X\n", st.unported);
            status = 3;
            break;
        }
        if (!c.mismatch && c.next < c.count && c.rec[c.next].kind != 0) {
            printf("mismatch: the original made more calls (next 0x%08X, record %u)\n", c.rec[c.next].address,
                   c.next);
            c.mismatch = 1;
        }
    }
    if (sound_lines && !sound_failed && sound_next < sound_count && whole_replays == 0) {
        printf("sound mismatch: the original made more requests: '%s' (request %d of %d)\n", sound_lines[sound_next],
               sound_next, sound_count);
        sound_failed = 1;
    }
    if (c.mismatch || sound_failed) {
        status = 4;
    } else if (c.stack_writes && status == 0) {
        status = 5;
    } else if (c.bad_args && status == 0) {
        printf("args: a replayed call got other arguments than the original's (see the args lines)\n");
        status = 6;
    }
    f = fopen(argv[3], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[3]);
        return 2;
    }
    for (int i = 0; i < nrw; i++) {
        f = fopen(rw[i].out, "wb");
        if (!f || fwrite(rw[i].data, 1, rw[i].size, f) != rw[i].size || fclose(f) != 0) {
            fprintf(stderr, "cannot write %s\n", rw[i].out);
            return 2;
        }
    }
    return status;
}
