/* Imhotep - run ported functions on a state file, replaying the original's unported calls (the
 * port's side of tools/oracle/replay.py, used by harness.py and lockstep.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_replay IN REPLAY OUT [--replay ADDR,ADDR... | --replay-all] [--map ADDR:FILE ...]
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
 * from the game directory DIR, names matched case-insensitively as tools/oracle/game.py does.
 * Prints "return N" per top-level call, "replay ADDR" / "native ADDR" / "fallback ADDR" per
 * recorded call, "args ADDR i original port" when a replayed call's arguments differ.
 * Exit: 0, 4 on a call sequence mismatch, 5 when a replayed call wrote its caller's stack (an
 * output buffer: it needs a port), 2 on errors. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/state.h"

#define NREC_ARGS 8

typedef struct {
    uint32_t address, kind;
    int32_t eax;
    uint32_t nargs;
    const uint8_t *args;
    uint32_t this_ptr, flags, heap_used, nblocks, nranges;
    const uint8_t *blocks, *ranges;
} record;

typedef struct {
    record *rec;
    uint32_t count, next;
    int replay_all;
    uint32_t replay[1024];
    int nreplay;
    int mismatch, stack_writes;
    game_state backup;
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

static void apply(game_state *st, const record *r)
{
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
    if (c->next >= c->count || c->rec[c->next].kind != 1 || c->rec[c->next].address != address) {
        printf("mismatch: the port calls 0x%08X, the original 0x%08X (record %u)\n", address,
               c->next < c->count ? c->rec[c->next].address : 0, c->next);
        c->mismatch = 1;
        return 1;
    }
    const record *r = &c->rec[c->next++];
    if (p && p->partial == PORT_PLATFORM && !forced) {
        /* the original's own run is presentation (excluded from comparisons): the port's request */
        int32_t a[16] = {0};
        for (int i = 0; i < nargs && i < 15; i++) {
            a[i] = args[i];
        }
        a[PORT_THIS_ARG] = (int32_t) this_ptr;
        platform_record = r;
        *result = p->run(st, a);
        platform_record = NULL;
        printf("platform 0x%08X\n", address);
        return 1;
    }
    if (p && !forced) { /* partial: run it, back to the record if it reaches unported code */
        int32_t a[16] = {0};
        for (int i = 0; i < nargs && i < 15; i++) {
            a[i] = args[i];
        }
        a[PORT_THIS_ARG] = (int32_t) this_ptr;
        uint32_t before = st->unported;
        state_copy(&c->backup, st);
        *result = p->run(st, a);
        if (st->unported == before) {
            printf("native 0x%08X\n", address);
            return 1;
        }
        printf("fallback 0x%08X (reached 0x%08X)\n", address, st->unported);
        state_copy(st, &c->backup);
        st->unported = before;
    } else {
        printf("replay 0x%08X\n", address);
    }
    for (int i = 0; i < nargs && i < (int) r->nargs; i++) {
        int32_t o = (int32_t) rd32(r->args + 4 * i);
        if (o != args[i]) {
            printf("args 0x%08X %d original %d port %d\n", address, i, o, args[i]);
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

static uint32_t pixels_load(void *ctx, const char *path, uint32_t skip, uint32_t size)
{
    (void) ctx;
    (void) path;
    (void) skip;
    (void) size;
    if (platform_record && platform_record->nranges > 0 && rd32(platform_record->ranges + 4) == 4) {
        return rd32(platform_record->ranges + 8);
    }
    return 0;
}

static int32_t file_read(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    /* DIR + the game's relative path, '\\' as '/', each part matched case-insensitively */
    char full[4096];
    snprintf(full, sizeof(full), "%s", (const char *) ctx);
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
        snprintf(full + l, sizeof(full) - l, "/%s", match);
        if (d) {
            closedir(d);
        }
    }
    FILE *f = fopen(full, "rb");
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
        r->heap_used = rd32(p + 4);
        r->nblocks = rd32(p + 8);
        r->blocks = p + 12;
        p += 12 + 8 * r->nblocks;
        r->nranges = rd32(p);
        r->ranges = p + 4;
        p += 4;
        for (uint32_t k = 0; k < r->nranges; k++) {
            p += 8 + rd32(p + 4);
        }
    }
    return p == end ? 0 : -1;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: state_replay IN REPLAY OUT [--replay A,B... | --replay-all]\n");
        return 2;
    }
    replay_ctx c = {0};
    int nmaps = 0;
    if (state_create(&c.backup) != 0) {
        return 2;
    }
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--replay-all") == 0) {
            c.replay_all = 1;
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
    sim_platform files = {0};
    for (int i = 4; i < argc && nmaps; i++) {
        if (strcmp(argv[i], "--map") != 0 || i + 1 >= argc) {
            continue;
        }
        if (strncmp(argv[i + 1], "files=", 6) == 0) {
            files.ctx = argv[i + 1] + 6;
            files.file_read = file_read;
            files.pixels_load = pixels_load;
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
        state_map_external(&st, (uint32_t) strtoul(argv[i + 1], NULL, 16), buf, (uint32_t) n);
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
        if (!p || !p->run) {
            fprintf(stderr, "0x%08X is not ported\n", top->address);
            return 2;
        }
        int32_t a[16] = {0};
        for (uint32_t i = 0; i < top->nargs && i < 15; i++) {
            a[i] = (int32_t) rd32(top->args + 4 * i);
        }
        a[PORT_THIS_ARG] = (int32_t) (top->this_ptr ? top->this_ptr : p->this_ptr);
        int32_t ret = p->run(&st, a);
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
    if (c.mismatch) {
        status = 4;
    } else if (c.stack_writes && status == 0) {
        status = 5;
    }
    f = fopen(argv[3], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[3]);
        return 2;
    }
    return status;
}
