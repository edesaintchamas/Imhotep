/* Imhotep - the boot: the Cleopatra monument objects (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Eight global objects describe monuments built in parts (docs/formats/save.md: the monument
 * records): a vtable, the object's own part at +0xC (0x004E5210), then an array of parts of 0x34
 * bytes (x, y, w, h, then nine dwords of screen offsets computed by 0x004E5190) and per-part tables.
 * Their static constructors only store constants; their effect comes from tools/gen/
 * gen_boot_data.py (core/boot/boot_data.c), which runs their code and records the stores.
 * WinMain then lays out every part (0x004013F0 .. 0x0054F3D0: 0x004E52B0 over the parts). */
#include "boot/monuments.h"

#include <string.h>

#include "boot/boot_internal.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/ported.h"

/* the table entries, their objects and constructors (@bin 0x00413C80 .. 0x00413E40: mov ecx, OBJECT;
 * jmp CONSTRUCTOR) */
static const struct {
    uint32_t entry, self, ctor;
} OBJECTS[8] = {
    {0x00413C70u, 0x006FD5D8u, 0x0040D280u}, {0x00413CB0u, 0x006FD368u, 0x00416C20u},
    {0x00413CF0u, 0x006F9320u, 0x005430E0u}, {0x00413D30u, 0x006F5648u, 0x005567D0u},
    {0x00413D70u, 0x006E9188u, 0x005480E0u}, {0x00413DB0u, 0x006DE9E0u, 0x0054FEF0u},
    {0x00413DF0u, 0x006DE650u, 0x00401030u}, {0x00413E30u, 0x006DE548u, 0x004E6400u},
};

/* @bin 0x004E50F0 a part: its 13 dwords 0 */
static void part_clear(game_state *st, uint32_t part)
{
    memset(st->mem + (part - STATE_BASE), 0, 0x34);
}

/* @bin 0x004E5120 + 0x004E5140 a part's rectangle. 0x004E5140 copies 13 dwords from the stack
 * temporary that 0x004E5120 built: the 9 after the rectangle are what the stack held (not
 * reproducible); 0x004E5190 rewrites them when WinMain lays the parts out */
static void part_rect(game_state *st, uint32_t part, int32_t x, int32_t y, int32_t w, int32_t h)
{
    S32(st, part) = x;
    S32(st, part + 4) = y;
    S32(st, part + 8) = w;
    S32(st, part + 0xC) = h;
}

/* @bin 0x004E5210 the base: one part at +0xC (0, 0, 1, 7), vtable 0x0056F698, +0x40 .. +0x48 three
 * string addresses, +4 1, +8 0 */
static void monument_base(game_state *st, uint32_t self)
{
    part_clear(st, self + 0xC);
    U32(st, self) = 0x0056F698u;
    part_rect(st, self + 0xC, 0, 0, 1, 7);
    U32(st, self + 0x40) = 0x005E4578u;
    U32(st, self + 0x44) = 0x005E456Cu;
    U32(st, self + 0x48) = 0x005E4560u;
    U32(st, self + 4) = 1;
    U32(st, self + 8) = 0;
}

void monument_ctor(game_state *st, uint32_t fn)
{
    int k = 0;
    while (k < 8 && OBJECTS[k].entry != fn) {
        k++;
    }
    if (k == 8) {
        state_unported(st, fn);
        return;
    }
    uint32_t self = OBJECTS[k].self;
    const monument_def *d = &monument_defs[k];
    monument_base(st, self);
    if (d->base == 0x00545F80u) {
        U32(st, self) = 0x005701A0u; /* @bin 0x00545F88 */
    }
    /* @bin 0x00561441 the parts (0x004E50F0), then their rectangles */
    for (int32_t i = 0; i < d->nparts; i++) {
        part_clear(st, self + d->parts + 0x34u * (uint32_t) i);
    }
    for (int32_t i = 0; i < d->nparts; i++) {
        const int16_t *r = d->rects[i];
        part_rect(st, self + d->parts + 0x34u * (uint32_t) i, r[0], r[1], r[2], r[3]);
    }
    for (int32_t i = 0; i < d->nruns; i++) {
        const monument_run *r = &d->runs[i];
        for (uint32_t j = 0; j < r->count; j++) {
            if (r->size == 4) {
                U32(st, self + r->off + 4 * j) = r->value;
            } else {
                U8(st, self + r->off + j) = (uint8_t) r->value;
            }
        }
    }
    /* @bin 0x00545304 (and the same loops of the other three) one rand() % 5 per part, a byte each */
    for (int32_t i = 0; i < d->nrand; i++) {
        int32_t v = crt_rand(st);
        U8(st, self + d->rand[i]) = (uint8_t) (v % 5);
    }
}

void monument_part_layout(game_state *st, uint32_t part)
{
    /* @bin 0x004E5190 with h1 = [0x0070E45C] + 1 and l = [0x0070E44C] (32-bit products) */
    uint32_t h1 = U32(st, 0x0070E45Cu) + 1u, l = U32(st, 0x0070E44Cu);
    uint32_t x = U32(st, part), y = U32(st, part + 4);
    P32(st, part + 0x10) = x + 228u * y;
    P32(st, part + 0x14) = (x - y) * h1;
    P32(st, part + 0x24) = (x + y) * l;
    P32(st, part + 0x18) = (x + y) * h1;
    P32(st, part + 0x30) = (x - y) * l;
    P32(st, part + 0x20) = 0u - (x + y) * h1;
    P32(st, part + 0x28) = (y - x) * l;
    P32(st, part + 0x1C) = (y - x) * h1;
    P32(st, part + 0x2C) = 0u - (x + y) * l;
}

void monument_parts_layout(game_state *st, uint32_t parts, int32_t count)
{
    /* @bin 0x004E52B0 count <= 0: nothing */
    for (int32_t i = 0; i < count; i++) {
        monument_part_layout(st, parts + 0x34u * (uint32_t) i);
    }
}

void boot_monument_setup(game_state *st, int k)
{
    /* @bin 0x004141D3 .. 0x0041421E WinMain's set-ups, in its order, and their parts (0x004013F0
     * .. 0x0054F3D0: lea eax, [ecx + PARTS]; push COUNT; push eax; call 0x004E52B0) */
    static const struct {
        uint32_t self, parts;
        int32_t count;
    } SETUP[8] = {
        {0x006DE650u, 0x4C, 0x10},     {0x006FD5D8u, 0x4C, 0x19},     {0x006FD368u, 0x4C, 9},
        {0x006DE548u, 0x4C, 2},        {0x006F9320u, 0x1E5C, 0xA7},   {0x006F5648u, 0x1CBC, 0x9E},
        {0x006DE9E0u, 0x4EE0, 0x1B5},  {0x006E9188u, 0x5C88, 0x201},
    };
    monument_parts_layout(st, SETUP[k].self + SETUP[k].parts, SETUP[k].count);
}

static int32_t run_part_layout(game_state *st, const int32_t *a)
{
    monument_part_layout(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_parts_layout(game_state *st, const int32_t *a)
{
    monument_parts_layout(st, (uint32_t) a[0], a[1]);
    return 0;
}

static int32_t run_setup(game_state *st, const int32_t *a, uint32_t parts, int32_t count)
{
    monument_parts_layout(st, (uint32_t) a[PORT_THIS_ARG] + parts, count);
    return 0;
}

static int32_t run_4013f0(game_state *st, const int32_t *a) { return run_setup(st, a, 0x4C, 0x10); }
static int32_t run_40d7e0(game_state *st, const int32_t *a) { return run_setup(st, a, 0x4C, 0x19); }
static int32_t run_416f60(game_state *st, const int32_t *a) { return run_setup(st, a, 0x4C, 9); }
static int32_t run_4e6590(game_state *st, const int32_t *a) { return run_setup(st, a, 0x4C, 2); }
static int32_t run_5456a0(game_state *st, const int32_t *a) { return run_setup(st, a, 0x1E5C, 0xA7); }
static int32_t run_558b40(game_state *st, const int32_t *a) { return run_setup(st, a, 0x1CBC, 0x9E); }
static int32_t run_555de0(game_state *st, const int32_t *a) { return run_setup(st, a, 0x4EE0, 0x1B5); }
static int32_t run_54f3d0(game_state *st, const int32_t *a) { return run_setup(st, a, 0x5C88, 0x201); }

const ported_function boot_monument_ports[] = {
    {0x004E5190u, "monument_part_layout", 0, 0, 0, run_part_layout, 0},
    {0x004E52B0u, "monument_parts_layout", 2, 0, 0, run_parts_layout, 0},
    {0x004013F0u, "monument_setup_4013f0", 0, 0, 0x006DE650u, run_4013f0, 0},
    {0x0040D7E0u, "monument_setup_40d7e0", 0, 0, 0x006FD5D8u, run_40d7e0, 0},
    {0x00416F60u, "monument_setup_416f60", 0, 0, 0x006FD368u, run_416f60, 0},
    {0x004E6590u, "monument_setup_4e6590", 0, 0, 0x006DE548u, run_4e6590, 0},
    {0x005456A0u, "monument_setup_5456a0", 0, 0, 0x006F9320u, run_5456a0, 0},
    {0x00558B40u, "monument_setup_558b40", 0, 0, 0x006F5648u, run_558b40, 0},
    {0x00555DE0u, "monument_setup_555de0", 0, 0, 0x006DE9E0u, run_555de0, 0},
    {0x0054F3D0u, "monument_setup_54f3d0", 0, 0, 0x006E9188u, run_54f3d0, 0},
    /* the constructors, run by game_boot from the CRT's table (boot_ctor) */
    {0x0040D280u, "monument_ctor_40d280 (boot)", 0, 32, 0x006FD5D8u, NULL, 0},
    {0x00416C20u, "monument_ctor_416c20 (boot)", 0, 32, 0x006FD368u, NULL, 0},
    {0x005430E0u, "monument_ctor_5430e0 (boot)", 0, 32, 0x006F9320u, NULL, 0},
    {0x005567D0u, "monument_ctor_5567d0 (boot)", 0, 32, 0x006F5648u, NULL, 0},
    {0x005480E0u, "monument_ctor_5480e0 (boot)", 0, 32, 0x006E9188u, NULL, 0},
    {0x0054FEF0u, "monument_ctor_54fef0 (boot)", 0, 32, 0x006DE9E0u, NULL, 0},
    {0x00401030u, "monument_ctor_401030 (boot)", 0, 32, 0x006DE650u, NULL, 0},
    {0x004E6400u, "monument_ctor_4e6400 (boot)", 0, 32, 0x006DE548u, NULL, 0},
    {0x004E5210u, "monument_base_ctor (boot)", 0, 32, 0, NULL, 0},
    {0x00545F80u, "monument_base2_ctor (boot)", 0, 32, 0, NULL, 0},
};
const int boot_monument_ports_count = sizeof(boot_monument_ports) / sizeof(boot_monument_ports[0]);
