/* Imhotep - the growable ring's push and growth (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The growable ring of core/sim/queue.c (+0 a flag byte, +4 head, +8 tail, +0x0C count, +0x10
 * capacity, +0x14 slots in the heap arena), as used by the battles 0x009783A4 + 0x1C * n
 * (0x004F68F0 forwards to the push 0x004F6900). Growth goes through a temporary copy on the
 * original's stack (0x004F66C0 .. 0x004F66F0); its slots are a heap block, allocated and freed
 * here in the same order so that the arena matches the original's. */
#include "sim/queue.h"
#include "state/ported.h"
#include "state/vec.h"

typedef struct {
    uint8_t flag;
    int32_t head, tail, count, cap;
    uint32_t slots;
} ring_tmp;

static void tmp_copy_from(game_state *st, ring_tmp *t, uint32_t src)
{
    /* @bin 0x004F6790 (dst, src): dst reserved to src's capacity (0x004F6700: reset, a zeroed block
     * of n slots, head 1, tail and count 0), then count, head, tail, the flag and dst's capacity
     * of slots copied from src */
    int32_t n = S32(st, src + 0x10);
    if (n > t->cap) {
        t->slots = 0; /* the reset: no block yet */
        t->cap = n;
        t->slots = state_alloc(st, (uint32_t) n * 4u);
        t->head = 1;
        t->tail = 0;
        t->count = 0;
    }
    t->count = S32(st, src + 0x0C);
    t->head = S32(st, src + 4);
    t->tail = S32(st, src + 8);
    t->flag = U8(st, src);
    for (int32_t i = 0; i < t->cap; i++) {
        P32(st, t->slots + 4u * (uint32_t) i) = P32(st, U32(st, src + 0x14) + 4u * (uint32_t) i);
    }
}

int32_t ring_grow_4f6a50(game_state *st, uint32_t r, int32_t n)
{
    /* @bin 0x004F6A50 (n) when n exceeds the capacity: a copy of the ring (0x004F66C0), the ring
     * reserved to n (0x004F6700: its old block freed, a new zeroed one), then the copy's count,
     * head, tail and flag back, and n slots copied from the copy's block (0x004F6790: rep movsd
     * forwards, reading past the copy's block into what follows it in the heap), the copy freed
     * (0x004F66F0 -> 0x004F6760). Returns 1 */
    if (n <= S32(st, r + 0x10)) {
        return 1;
    }
    ring_tmp t = {0, 1, 0, 0, 0, 0}; /* @bin 0x004F66C0 +8, +0x0C, +0x14, +0x10 := 0, +0 := 0, +4 := 1 */
    tmp_copy_from(st, &t, r);
    vec_reserve(st, r, n);
    /* @bin 0x004F6790 (ring, copy): the ring's capacity already holds the copy's */
    if (t.cap > S32(st, r + 0x10)) {
        vec_reserve(st, r, t.cap);
    }
    S32(st, r + 0x0C) = t.count;
    S32(st, r + 4) = t.head;
    S32(st, r + 8) = t.tail;
    U8(st, r) = t.flag;
    uint32_t dst = U32(st, r + 0x14);
    for (int32_t i = 0; i < S32(st, r + 0x10); i++) {
        P32(st, dst + 4u * (uint32_t) i) = P32(st, t.slots + 4u * (uint32_t) i);
    }
    if (t.slots != 0) {
        state_dealloc(st, t.slots); /* @bin 0x004F6760 on the copy */
    }
    return 1;
}

int32_t ring_push_4f6900(game_state *st, uint32_t r, int32_t value)
{
    /* @bin 0x004F6900 a full ring (count == capacity) grows by 10 (0x004F6A50); with slots, the
     * tail + 1 wrapped (0x004F6A10) gets the value and the count grows. Returns the count (0 when
     * there are no slots) */
    if (S32(st, r + 0x0C) == S32(st, r + 0x10)) {
        if (ring_grow_4f6a50(st, r, S32(st, r + 0x10) + 10) == 0) {
            return 0;
        }
    }
    if (U32(st, r + 0x14) == 0) {
        return 0;
    }
    S32(st, r + 8) = S32(st, r + 8) + 1;
    ring_wrap(st, r, r + 8);
    P32(st, U32(st, r + 0x14) + 4u * (uint32_t) S32(st, r + 8)) = (uint32_t) value;
    S32(st, r + 0x0C) = S32(st, r + 0x0C) + 1;
    return S32(st, r + 0x0C);
}

void ring_copy_4f6790(game_state *st, uint32_t dst, uint32_t src)
{
    /* @bin 0x004F6790 (dst, src) on two rings of the state */
    vec_reserve(st, dst, S32(st, src + 0x10));
    S32(st, dst + 0x0C) = S32(st, src + 0x0C);
    S32(st, dst + 4) = S32(st, src + 4);
    S32(st, dst + 8) = S32(st, src + 8);
    U8(st, dst) = U8(st, src);
    uint32_t d = U32(st, dst + 0x14), s = U32(st, src + 0x14);
    for (int32_t i = 0; i < S32(st, dst + 0x10); i++) {
        P32(st, d + 4u * (uint32_t) i) = P32(st, s + 4u * (uint32_t) i);
    }
}

void ring_init_copy_4f66c0(game_state *st, uint32_t dst, uint32_t src)
{
    /* @bin 0x004F66C0 (dst, src) +8, +0x0C, +0x14, +0x10 := 0, +0 := 0, +4 := 1, then 0x004F6790 */
    S32(st, dst + 8) = 0;
    S32(st, dst + 0x0C) = 0;
    U32(st, dst + 0x14) = 0;
    S32(st, dst + 0x10) = 0;
    U8(st, dst) = 0;
    S32(st, dst + 4) = 1;
    ring_copy_4f6790(st, dst, src);
}

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int in_state(game_state *st, uint32_t p, uint32_t addr)
{
    if (state_ptr(st, p, 0x18)) {
        return 1;
    }
    state_unported(st, addr); /* an object in the caller's frame */
    return 0;
}

static int32_t run_push(game_state *st, const int32_t *a)
{
    return ring_push_4f6900(st, THIS(a), a[0]);
}

static int32_t run_grow(game_state *st, const int32_t *a)
{
    return ring_grow_4f6a50(st, THIS(a), a[0]);
}

static int32_t run_copy(game_state *st, const int32_t *a)
{
    if (!in_state(st, THIS(a), 0x004F6790u) || !in_state(st, (uint32_t) a[0], 0x004F6790u)) {
        return 0;
    }
    ring_copy_4f6790(st, THIS(a), (uint32_t) a[0]);
    return 0;
}

static int32_t run_init_copy(game_state *st, const int32_t *a)
{
    if (!in_state(st, THIS(a), 0x004F66C0u) || !in_state(st, (uint32_t) a[0], 0x004F66C0u)) {
        return 0;
    }
    ring_init_copy_4f66c0(st, THIS(a), (uint32_t) a[0]);
    return (int32_t) THIS(a);
}

static int32_t run_reset(game_state *st, const int32_t *a)
{
    if (!in_state(st, THIS(a), 0x004F66F0u)) {
        return 0;
    }
    vec_reset(st, THIS(a)); /* @bin 0x004F66F0 jmp 0x004F6760 */
    return 0;
}

const ported_function ring_ports[] = {
    {0x004F6900u, "ring_push_4f6900", 1, 32, 0, run_push, 0},
    {0x004F6A50u, "ring_grow_4f6a50", 1, 32, 0, run_grow, 0},
    {0x004F6790u, "ring_copy_4f6790", 1, 0, 0, run_copy, 0},
    {0x004F66C0u, "ring_init_copy_4f66c0", 1, 32, 0, run_init_copy, 0},
    {0x004F66F0u, "ring_reset (thunk 0x004F6760)", 0, 0, 0, run_reset, 0},
};
const int ring_ports_count = sizeof(ring_ports) / sizeof(ring_ports[0]);
