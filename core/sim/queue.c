/* Imhotep - small container classes of the game (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Two thiscall classes, on objects that vary (the wrappers take `this` from args[PORT_THIS_ARG]):
 * - a fixed ring of ints: +4 head, +8 tail, +0x0C size, +0x14 the slots (inline): the ferry /
 *   building figure lists 0x00EAAF68 + 0xE0 * n (push 0x004F65B0, length 0x004F6630 are in
 *   core/figure/actions_a3.c);
 * - a growable ring: +4 head, +0x0C count, +0x10 capacity, +0x14 pointer to the slots (heap):
 *   the lists 0x009783C0 + 0x1C * n. */
#include "sim/queue.h"

#include "state/ported.h"

void queue_clear(game_state *st, uint32_t q)
{
    /* @bin 0x004F6560 tail, then head := 0 */
    S32(st, q + 8) = 0;
    S32(st, q + 4) = 0;
}

int32_t queue_contains(game_state *st, uint32_t q, int32_t value)
{
    /* @bin 0x004F6570 a slot from head to tail (wrapping at size) holds value */
    int32_t i = S32(st, q + 4), tail = S32(st, q + 8);
    while (i != tail) {
        if (S32(st, q + 0x14 + 4u * (uint32_t) i) == value) {
            return 1;
        }
        i++;
        if (i >= S32(st, q + 0x0C)) {
            i = 0;
        }
    }
    return 0;
}

int32_t queue_next(game_state *st, uint32_t q, uint32_t cursor)
{
    /* @bin 0x004F65F0 (cursor) the slot at *cursor (0 when it is the tail), *cursor advances and
     * wraps at size */
    int32_t i = S32(st, cursor);
    if (i == S32(st, q + 8)) {
        return 0;
    }
    int32_t v = S32(st, q + 0x14 + 4u * (uint32_t) i);
    S32(st, cursor) = i + 1;
    if (i + 1 >= S32(st, q + 0x0C)) {
        S32(st, cursor) = 0;
    }
    return v;
}

int32_t queue_pop(game_state *st, uint32_t q)
{
    /* @bin 0x004F6620 next(&head) */
    return queue_next(st, q, q + 4);
}

int32_t ring_wrap(game_state *st, uint32_t r, uint32_t index)
{
    /* @bin 0x004F6A10 (index pointer) brings *index into 0 .. capacity - 1 (al: 0 when the
     * capacity +0x10 is 0) */
    if (S32(st, r + 0x10) == 0) {
        return 0;
    }
    while (S32(st, index) < 0) {
        S32(st, index) = S32(st, index) + S32(st, r + 0x10);
    }
    while (S32(st, index) >= S32(st, r + 0x10)) {
        S32(st, index) = S32(st, index) - S32(st, r + 0x10);
    }
    return 1;
}

int32_t ring_pop(game_state *st, uint32_t r)
{
    /* @bin 0x004F68B0 (0x004F68A0 jumps here) 0 when empty; else head + 1 wrapped, count - 1,
     * returns the slot at the old head */
    if (S32(st, r + 0x0C) == 0) {
        return 0;
    }
    int32_t old = S32(st, r + 4);
    S32(st, r + 4) = old + 1;
    ring_wrap(st, r, r + 4);
    S32(st, r + 0x0C) = S32(st, r + 0x0C) - 1;
    return (int32_t) P32(st, U32(st, r + 0x14) + 4u * (uint32_t) old);
}

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int in_state(game_state *st, uint32_t p, uint32_t addr)
{
    if (state_ptr(st, p, 4)) {
        return 1;
    }
    state_unported(st, addr); /* a pointer into the caller's frame */
    return 0;
}

static int32_t run_clear(game_state *st, const int32_t *a)
{
    queue_clear(st, THIS(a));
    return 0;
}

static int32_t run_contains(game_state *st, const int32_t *a)
{
    return queue_contains(st, THIS(a), a[0]);
}

static int32_t run_next(game_state *st, const int32_t *a)
{
    return in_state(st, (uint32_t) a[0], 0x004F65F0u) ? queue_next(st, THIS(a), (uint32_t) a[0]) : 0;
}

static int32_t run_pop(game_state *st, const int32_t *a)
{
    return queue_pop(st, THIS(a));
}

static int32_t run_wrap(game_state *st, const int32_t *a)
{
    return in_state(st, (uint32_t) a[0], 0x004F6A10u) ? ring_wrap(st, THIS(a), (uint32_t) a[0]) : 0;
}

static int32_t run_ring_pop(game_state *st, const int32_t *a)
{
    return ring_pop(st, THIS(a));
}

const ported_function bigb2_queue_ports[] = {
    {0x004F6560u, "queue_clear", 0, 0, 0, run_clear, 0},
    {0x004F6570u, "queue_contains", 1, 32, 0, run_contains, 0},
    {0x004F65F0u, "queue_next", 1, 32, 0, run_next, 0},
    {0x004F6620u, "queue_pop", 0, 32, 0, run_pop, 0},
    {0x004F6A10u, "ring_wrap", 1, 8, 0, run_wrap, 0},
    {0x004F68B0u, "ring_pop", 0, 32, 0, run_ring_pop, 0},
    {0x004F68A0u, "ring_pop (thunk)", 0, 32, 0, run_ring_pop, 0},
};
const int bigb2_queue_ports_count = sizeof(bigb2_queue_ports) / sizeof(bigb2_queue_ports[0]);
