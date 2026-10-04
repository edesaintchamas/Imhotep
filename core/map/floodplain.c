/* Imhotep - the floodplain manager (Pharaoh.exe object at 0x00D05720) and its objects.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/floodplain.h"

#include <string.h>

/* @bin 0x004BD6A0 (this = object) release the object's list: elements in list order; each
 * element's node (element +4) is unlinked (head +16, tail +20), cleared and freed, then the
 * element; +24 holds the element being released; finally +0 := 0. Nodes: +0 next, +4 previous,
 * +12 element. */
void floodplain_list_release(game_state *st, uint32_t obj)
{
    if (S32(st, obj) == 0) {
        return;
    }
    uint32_t node = U32(st, obj + 16);
    uint32_t elem = node ? P32(st, node + 12) : 0;
    node = node ? P32(st, node) : 0;
    U32(st, obj + 24) = elem;
    while (elem != 0) {
        uint32_t next_elem = node ? P32(st, node + 12) : 0;
        node = node ? P32(st, node) : 0;
        uint32_t cur = U32(st, obj + 24);
        uint32_t n = P32(st, cur + 4), prev = P32(st, n + 4), next = P32(st, n);
        if (prev == 0) {
            U32(st, obj + 16) = next;
        } else {
            P32(st, prev) = next;
        }
        if (next == 0) {
            U32(st, obj + 20) = prev;
        } else {
            P32(st, next + 4) = prev;
        }
        P32(st, n + 4) = 0;
        P32(st, n) = 0;
        state_dealloc(st, n);
        P32(st, cur + 4) = 0;
        state_dealloc(st, cur); /* (0x0051F780 called before: an empty function) */
        U32(st, obj + 24) = next_elem;
        elem = next_elem;
    }
    S32(st, obj) = 0;
}

/* @bin 0x004BE0F0 (this = 0x00D05720) floodplain manager: grid of 0xCB10 dwords cleared, 30
 * objects of 7 dwords from +0x32C64 reset (id 1..30, owner = the manager), counters */
void floodplain_manager_reset(game_state *st)
{
    const uint32_t m = 0x00D05720u;
    memset(state_ptr(st, m, 0xCB10u * 4), 0, 0xCB10u * 4);
    for (uint32_t i = 0; i < 0x1E; i++) {
        uint32_t obj = m + 4 * (0xCB1Au - 1) + 28 * i;
        floodplain_list_release(st, obj);
        U32(st, obj + 4) = 0;
        U32(st, obj + 8) = i + 1;
        U32(st, obj + 12) = m;
        U32(st, obj + 24) = 0;
        U32(st, obj) = 0;
    }
    U32(st, m + 4 * 0xCBEBu) = 0x1E;
    U32(st, m + 4 * 0xCBECu) = 0;
    U8(st, m + 4 * 0xCBEFu) = 0;
}

/* @bin 0x004BD740 (this = list): object (new 0x1C, 0x004BD610), then node (new 0x10) keyed by the
 * random grid at the tile, inserted before the first node with a key >= it (front when the list
 * is empty or the key <= head's; back when >= tail's); count + 1 */
uint32_t floodplain_object_create(game_state *st, uint32_t list, int32_t offset, int32_t x, int32_t y,
                                  int32_t depth, uint8_t flag)
{
    uint32_t obj = state_alloc(st, 0x1C);
    if (obj) { /* @bin 0x004BD610 */
        P32(st, obj + 8) = (uint32_t) offset;
        P32(st, obj + 12) = (uint32_t) x;
        P32(st, obj + 16) = (uint32_t) y;
        P32(st, obj + 20) = (uint32_t) depth;
        P32(st, obj + 4) = 0;
        P32(st, obj) = list;
        P8(st, obj + 24) = flag;
    }
    int32_t key = U8(st, 0x00ACF85Cu + (uint32_t) offset);
    uint32_t node = state_alloc(st, 0x10);
    if (node) {
        P32(st, node + 4) = 0;
        P32(st, node) = 0;
        P32(st, node + 12) = obj;
        P32(st, node + 8) = (uint32_t) key;
    }
    uint32_t head = U32(st, list + 16);
    if (head == 0 || key <= (int32_t) P32(st, head + 8)) {
        P32(st, node) = head;
        if (U32(st, list + 16) != 0) {
            P32(st, U32(st, list + 16) + 4) = node;
        }
        P32(st, node + 4) = 0;
        U32(st, list + 16) = node;
        if (U32(st, list + 20) == 0) {
            U32(st, list + 20) = node;
        }
    } else if (key < (int32_t) P32(st, U32(st, list + 20) + 8)) {
        uint32_t p = head;
        while ((int32_t) P32(st, p + 8) < key) {
            p = P32(st, p);
        }
        P32(st, node) = p;
        P32(st, node + 4) = P32(st, p + 4);
        P32(st, P32(st, p + 4)) = node;
        P32(st, p + 4) = node;
    } else {
        P32(st, node + 4) = U32(st, list + 20);
        if (U32(st, list + 20) != 0) {
            P32(st, U32(st, list + 20)) = node;
        }
        P32(st, node) = 0;
        U32(st, list + 20) = node;
        if (U32(st, list + 16) == 0) {
            U32(st, list + 16) = node;
        }
    }
    P32(st, obj + 4) = node;
    S32(st, list) += 1;
    return obj;
}
