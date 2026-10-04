/* Imhotep - run game ticks on a state file, replaying what the original did in the calls that are
 * not ported (the port's side of tools/oracle/lockstep.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/state_tick IN REPLAY OUT [--replay-all | --replay ADDR,ADDR...]
 *
 * REPLAY lists, in order, every call the original's tick skeleton made (0x004D7090, 0x00451C50,
 * 0x00452370) with what that call changed in the state:
 *   u32 count, then per call: u32 address, i32 eax, u32 heap_used after, u32 new blocks,
 *   (u32 offset, u32 size) per new block, u32 ranges, (u32 address, u32 length, bytes) per range.
 * game_tick (core/sim/tick.c) runs once per tick marker in the file (address 0); each call it makes
 * must be the next recorded one. A ported function runs natively unless replayed by the options;
 * the others get the recorded change and return value. Prints one line per call
 * ("native" / "replay" ADDR) and exits 0, or 4 on a call sequence mismatch, 3 when a native port
 * reached unported code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/tick.h"
#include "state/ported.h"
#include "state/state.h"

typedef struct {
    uint32_t address;
    int32_t eax;
    uint32_t heap_used, nblocks, nranges;
    const uint8_t *blocks, *ranges;
} record;

typedef struct {
    record *rec;
    uint32_t count, next;
    int replay_all;
    uint32_t replay[64];
    int nreplay;
    int mismatch;
    game_state backup;     /* the state before a native call */
    const record *native;  /* the record of the native call in progress */
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
    (void) args;
    (void) nargs;
    (void) this_ptr;
    replay_ctx *c = env->ctx;
    if (c->mismatch) {
        *result = 0;
        return 1;
    }
    if (c->next >= c->count || c->rec[c->next].address != address) {
        printf("mismatch: the port calls 0x%08X, the original 0x%08X (call %u)\n", address,
               c->next < c->count ? c->rec[c->next].address : 0, c->next);
        c->mismatch = 1;
        *result = 0;
        return 1;
    }
    const record *r = &c->rec[c->next++];
    const ported_function *p = ported_find(address);
    if (p && !forced_replay(c, address)) {
        /* run the port here, so that a port that reaches unported code falls back to the replay */
        int32_t a[16] = {0};
        for (int i = 0; i < nargs && i < 16; i++) {
            a[i] = args[i];
        }
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
    }
    printf("replay 0x%08X\n", address);
    apply(st, r);
    *result = r->eax;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: state_tick IN REPLAY OUT [--replay-all | --replay A,B...]\n");
        return 2;
    }
    replay_ctx c = {0};
    if (state_create(&c.backup) != 0) {
        return 2;
    }
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--replay-all") == 0) {
            c.replay_all = 1;
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            for (char *s = argv[++i]; *s && c.nreplay < 64;) {
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
    f = fopen(argv[2], "rb");
    if (!f) {
        fprintf(stderr, "cannot read %s\n", argv[2]);
        return 2;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t) len);
    if (!data || fread(data, 1, (size_t) len, f) != (size_t) len) {
        return 2;
    }
    fclose(f);
    c.count = rd32(data);
    c.rec = calloc(c.count ? c.count : 1, sizeof(record));
    const uint8_t *p = data + 4;
    for (uint32_t i = 0; i < c.count; i++) {
        record *r = &c.rec[i];
        r->address = rd32(p);
        r->eax = (int32_t) rd32(p + 4);
        r->heap_used = rd32(p + 8);
        r->nblocks = rd32(p + 12);
        r->blocks = p + 16;
        p += 16 + 8 * r->nblocks;
        r->nranges = rd32(p);
        r->ranges = p + 4;
        p += 4;
        for (uint32_t k = 0; k < r->nranges; k++) {
            p += 8 + rd32(p + 4);
        }
    }
    sim_env env = {external, &c};
    int status = 0;
    while (c.next < c.count && !c.mismatch) {
        if (c.rec[c.next].address != 0) {
            printf("mismatch: a tick starts with the original's call 0x%08X\n", c.rec[c.next].address);
            c.mismatch = 1;
            break;
        }
        c.next++; /* the tick marker */
        game_tick(&st, &env);
        if (st.unported) {
            printf("unported: 0x%08X\n", st.unported);
            status = 3;
            break;
        }
        if (!c.mismatch && c.next < c.count && c.rec[c.next].address != 0) {
            printf("mismatch: the original made more calls (next 0x%08X)\n", c.rec[c.next].address);
            c.mismatch = 1;
        }
    }
    if (c.mismatch) {
        status = 4;
    }
    f = fopen(argv[3], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[3]);
        return 2;
    }
    return status;
}
