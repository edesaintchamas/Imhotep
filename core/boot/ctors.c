/* Imhotep - the boot: the C++ static constructors (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The CRT runs the table 0x00576000 .. 0x0057607C before WinMain (_initterm, @bin 0x005646C0). Each
 * entry constructs one global object (`mov ecx, OBJECT; jmp CONSTRUCTOR`) and registers its
 * destructor with atexit (0x00561953): the destructors run when the program exits, so the
 * registration is the CRT's own state and core keeps none (tools/oracle/game.py makes atexit a no-op
 * as well). The monument objects' constructors are in monuments.c. */
#include <string.h>

#include "boot/boot_internal.h"
#include "boot/monuments.h"
#include "city/day_x87.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

static uint64_t u64_at(game_state *st, uint32_t a)
{
    return (uint64_t) U32(st, a) | (uint64_t) U32(st, a + 4) << 32;
}

/* fstp qword: the bits of a double holding the 53-bit value x (a normal number or zero) */
static uint64_t x87_to_double(x87_val x)
{
    if (x.m == 0) {
        return (uint64_t) x.neg << 63;
    }
    x = x87_norm(x); /* m in [2^52, 2^53) */
    uint64_t exp = (uint64_t) (x.e + 1075);
    return (uint64_t) x.neg << 63 | exp << 52 | (x.m & 0xFFFFFFFFFFFFFull);
}

/* 0x00561441 `eh vector constructor iterator`: `count` objects of `size` bytes from `p`, each built by
 * `ctor` (in order) */
typedef void (*elem_ctor)(game_state *st, uint32_t p);
static void vector_ctor(game_state *st, uint32_t p, uint32_t size, int32_t count, elem_ctor ctor)
{
    for (int32_t i = 0; i < count; i++) {
        ctor(st, p + (uint32_t) i * size);
    }
}

/* @bin 0x00445460 (0x144-byte records at 0x00776FB0) */
static void elem_445460(game_state *st, uint32_t p)
{
    U32(st, p + 0x134) = 0;
    U32(st, p + 4) = 0;
    U8(st, p + 0x141) = 0;
    U32(st, p + 0x138) = 0;
    U8(st, p + 0x142) = 0;
    U8(st, p) = 5;
    U16(st, p + 0x13C) = 0xFFFF;
    U16(st, p + 0x13E) = 0xFFFF;
    U8(st, p + 0x140) = 1;
}

/* @bin 0x004F6500 (0xE0-byte records at 0x00EAAF68) */
static void elem_4f6500(game_state *st, uint32_t p)
{
    U32(st, p + 8) = 0;
    U32(st, p + 4) = 0;
    U32(st, p) = 0;
}

/* @bin 0x004F6670 (0x1C-byte arrays at 0x0097844C) */
static void elem_4f6670(game_state *st, uint32_t p)
{
    U32(st, p + 4) = 1;
    U32(st, p + 8) = 0;
    U32(st, p + 0xC) = 0;
    U32(st, p + 0x14) = 0;
    U32(st, p + 0x10) = 0;
    U8(st, p) = 0;
}

/* @bin 0x004BD650 (0x1C-byte records of the floodplain object) */
static void elem_4bd650(game_state *st, uint32_t p)
{
    U32(st, p + 0x10) = 0;
    U32(st, p + 0x14) = 0;
    U32(st, p) = 0;
    U32(st, p + 0x18) = 0;
}

/* @bin 0x00448960 (0x7C-byte event records at 0x00785D74): words of -1, then 0x004489C0 */
static void elem_448960(game_state *st, uint32_t p)
{
    static const uint8_t off[20] = {6, 0xA, 8, 4, 0xE, 0x12, 0x10, 0xC, 0x16, 0x1A,
                                    0x18, 0x14, 0x1E, 0x22, 0x20, 0x1C, 0x5A, 0x5E, 0x5C, 0x58};
    for (int i = 0; i < 20; i++) {
        U16(st, p + off[i]) = 0xFFFF;
    }
    GAME_CALLT0(st, p, FN_EVENT_INIT);
}

/* @bin 0x005404E0 (0xCC3C-byte graphics slots of 0x00F55BB0) */
static void elem_5404e0(game_state *st, uint32_t p)
{
    U32(st, p + 0xCC24) = 0;
    U8(st, p) = 0;
    U32(st, p + 0xCC28) = 0;
    U32(st, p + 0xCC2C) = 0;
    U32(st, p + 0xCC30) = 0;
}

/* @bin 0x0041C6D0 a file list (core/newgame/flow2.h): vtable 0x0056F548, the rest 0 */
static void file_list_ctor(game_state *st, uint32_t p)
{
    U32(st, p) = G_FILE_LIST_VTABLE;
    U32(st, p + 4) = 0;
    U32(st, p + 0x10) = 0;
    U32(st, p + 8) = 0;
    U32(st, p + 0xC) = 0;
    U32(st, p + 0x14) = 0;
    U32(st, p + 0x18) = 0;
}

/* @bin 0x0053DB60 (thiscall on the graphics object 0x00F55BB0) the slots emptied: the two external
 * buffers released (0x0053F790), each slot's pixel buffer (+0xEB64 + 0xCC3C k, operator delete),
 * name byte and counters cleared, the 30 slot image ids of the stack table into +0x30F548 */
static void graphics_reset(game_state *st, uint32_t self)
{
    static const int32_t ids[30] = {0x27,  0xFD,  0x10E, 0x1F2, 0x234, 0x235, 0x236, 0x237, 0x238, 0x239,
                                    0x240, 0x241, 0x27A, 0x282, 0x293, 0x2C7, 0x2F0, 0x2FC, 0x2FD, 0x2FE,
                                    0x2FF, 0x300, 0x301, 0x312, 0x317, 0x32C, 0x33C, 0x353, 0x370, 0x393};
    GAME_CALL(st, FN_EXTERNAL_BUFFER_FREE, (int32_t) (self + 0x30F5C4u), 0);
    GAME_CALL(st, FN_EXTERNAL_BUFFER_FREE, (int32_t) (self + 0x30F5C0u), 0);
    for (uint32_t k = 0; k < 30; k++) {
        uint32_t buf = self + 0xEB64u + 0xCC3Cu * k;
        if (U32(st, buf) != 0) {
            GAME_CALL(st, FN_OPERATOR_DELETE, (int32_t) U32(st, buf));
            U32(st, buf) = 0;
        }
        U8(st, buf - 0xCC24u) = 0;
        U32(st, buf + 4) = 0;
        U32(st, buf + 8) = 0;
        U32(st, buf + 0xC) = 0;
        S32(st, self + 0x30F548u + 4 * k) = ids[k];
    }
    U32(st, self + 0x30F5D4u) = 0;
    U32(st, self + 0x30F5C8u) = 1;
    U32(st, self + 0x30F5CCu) = 1;
    U32(st, self + 0x30F5D0u) = 0;
}

/* @bin 0x0055E38B the sound object (0x00E92BF8, the Miles wrapper: core/sim/platform.h) */
static void sound_object_ctor(game_state *st, uint32_t self)
{
    /* @bin 0x0055DBC0 its registry: three dwords 0; an instance counter 0x012663B0 += 0xC */
    U32(st, G_REGISTRY_INSTANCES) += 0xC;
    U32(st, self + 8) = 0;
    U32(st, self + 4) = 0;
    U32(st, self) = 0;
    U32(st, self + 0x200) = 0;
    U32(st, self + 0x19C) = 0;
    U32(st, self + 0x1A0) = 0;
    U32(st, self + 0x1A8) = 0;
    U32(st, self + 0x1AC) = 0;
    U32(st, self + 0x1F0) = 0;
    U32(st, self + 0x1F4) = 0;
    U32(st, self + 0x1F8) = 0;
    U32(st, self + 0x18C) = 0x5622; /* 22050 Hz */
    U32(st, self + 0x190) = 0x10;   /* 16 bits */
    U32(st, self + 0x194) = 2;      /* stereo */
    memset(st->mem + (self + 0x1B0 - STATE_BASE), 0xFF, 0x28); /* @bin 0x0055E439 memset(-1, 0x28) */
    U32(st, self + 0x14C) = 0x64;
    U32(st, self + 0x150) = 0x64;
    U32(st, self + 0x154) = 0x64;
    U32(st, self + 0x158) = 0x64;
    U32(st, self + 0x1EC) = 0;
    U32(st, self + 0x1FC) = 0;
    for (uint32_t i = 0; i < 3; i++) {
        U32(st, self + 0x1D8 + 4 * i) = 0;
    }
    /* @bin 0x0055E4CC 26 records of 3 dwords from the table 0x00604628 into +0xC */
    for (uint32_t i = 0; i < 0x1A; i++) {
        U32(st, self + 0xC + 0xC * i) = U32(st, G_SOUND_INIT_RECORDS + 0xC * i);
        U32(st, self + 0x10 + 0xC * i) = U32(st, G_SOUND_INIT_RECORDS + 4u + 0xC * i);
        U32(st, self + 0x14 + 0xC * i) = U32(st, G_SOUND_INIT_RECORDS + 8u + 0xC * i);
    }
    U32(st, self + 0x148) = 0xFFFFFFFFu;
    U32(st, self + 0x144) = 1;
    U32(st, self + 0x168) = 0;
    U32(st, self + 0x16C) = 0x3F000000u; /* 0.5f */
    U32(st, self + 0x170) = 0x3D75C28Fu; /* 0.06f */
    U32(st, self + 0x174) = 0x3F000000u;
    U32(st, self + 0x164) = 5;
    U32(st, G_SOUND_DRIVER) = self; /* @bin 0x0055E57D the sound object's address */
}

/* @bin 0x00539550 (thiscall on 0x00E39180) the music and sound tables cleared: 9 dwords at
 * +0x59A50, then 0x35 records of 0x1B10 bytes (0x578 dwords from the record's start, 0x118 from
 * +0x15E0, 0x32 from +0x1A40, and +0x1B08 first) */
static void music_tables_ctor(game_state *st, uint32_t self)
{
    for (uint32_t i = 0; i < 9; i++) {
        U32(st, self + 0x59A50 + 4 * i) = 0;
    }
    uint32_t edx = self + 0x15E0;
    for (int k = 0; k < 0x35; k++) {
        U32(st, edx + 0x528) = 0;
        memset(st->mem + (edx - 0x15E0 - STATE_BASE), 0, 0x578 * 4);
        memset(st->mem + (edx - STATE_BASE), 0, 0x118 * 4);
        memset(st->mem + (edx + 0x460 - STATE_BASE), 0, 0x32 * 4);
        edx += 0x1B10;
    }
}

static uint8_t mem8(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 1);
    return p ? *p : 0;
}

/* @bin 0x00562370 _stricmp ("C" locale: ASCII letters) of two strings of the state or mapped memory */
static int stricmp_at(game_state *st, uint32_t a, uint32_t b)
{
    for (;; a++, b++) {
        int x = mem8(st, a), y = mem8(st, b);
        x = x >= 'A' && x <= 'Z' ? x + 0x20 : x;
        y = y >= 'A' && y <= 'Z' ? y + 0x20 : y;
        if (x != y || x == 0) {
            return x - y;
        }
    }
}

static uint32_t strlen_at(game_state *st, uint32_t a)
{
    uint32_t n = 0;
    while (mem8(st, a + n)) {
        n++;
    }
    return n;
}

#define B(i) (*(uint8_t *) state_ptr(st, buf + (uint32_t) (i), 1))

/* @bin 0x004CBA60 (thiscall on 0x00784F20, file name, key table) the event message phrases: the
 * key table {?, name}[] (ending with a 0 name), an array of string pointers (one per key, outside the
 * state: +4), and the strings (+8) read from the file: `KEY "text"` pairs, ';' comments to the line
 * end. Two passes: the first measures, the second (with the string block) copies. A key that is not in
 * the table ends the read. */
static void phrases_ctor(game_state *st, const boot_host *host, uint32_t self, uint32_t name, uint32_t table)
{
    U32(st, self + 0xC) = 0;
    U32(st, self + 4) = 0;
    U32(st, self) = table;
    U32(st, self + 8) = 0;
    U32(st, self + 0x10) = 0;
    if (!table) {
        return;
    }
    for (uint32_t p = table + 4; U32(st, p) != 0; p += 8) {
        U32(st, self + 0xC) += 1;
    }
    int32_t count = S32(st, self + 0xC);
    if (count == 0) {
        return;
    }
    uint32_t arr = boot_alloc(st, host, (uint32_t) count << 2, 0x004CBAB3u);
    U32(st, self + 4) = arr;
    if (!arr) {
        U32(st, self + 0xC) = 0;
        return;
    }
    for (int32_t i = 0; i < count; i++) {
        P32(st, arr + 4 * (uint32_t) i) = 0;
    }
    int32_t key_index = count; /* @bin 0x004CBAE8 [esp + 0x10] */
    if (!name) {
        return;
    }
    char path[260];
    uint32_t n = strlen_at(st, name);
    if (n >= sizeof(path)) {
        return;
    }
    for (uint32_t i = 0; i <= n; i++) {
        path[i] = (char) mem8(st, name + i);
    }
    /* @bin 0x004CBAFE fopen(name, "rb"), _filelength, a block of length + 1, fread, a 0 after */
    int32_t len = platform_file_read(st, path, 0, NULL, 0);
    if (len < 0) {
        return;
    }
    uint32_t buf = boot_alloc(st, host, (uint32_t) len + 1, 0x004CBB2Bu);
    if (!buf) {
        return;
    }
    uint8_t *data = state_ptr(st, buf, (uint32_t) len + 1);
    if (!data) {
        state_unported(st, buf);
        return;
    }
    platform_file_read(st, path, 0, data, (uint32_t) len);
    data[len] = 0;
    uint32_t used = 0;
    for (int pass = 0; pass < 2; pass++) {
        uint32_t strings = 0;
        if (pass) {
            strings = boot_alloc(st, host, used, 0x004CBB7Bu);
            U32(st, self + 8) = strings;
            if (!strings) {
                break;
            }
        }
        int32_t key = -1;
        used = 0;
        int32_t i = 0;
        if (len < 0) {
            continue;
        }
        while (i <= len) {
            uint8_t c = B(i);
            if (c == 0x3B) { /* ';': to the line's end */
                for (;;) {
                    if (i > len) {
                        goto next_pass;
                    }
                    if (B(i) == 0xD || B(i) == 0xA) {
                        break;
                    }
                    i++;
                }
                c = B(i);
                if (B(i + 1) != c && (c == 0xD || c == 0xA)) {
                    if (i >= len) {
                        if (i > len) {
                            goto next_pass;
                        }
                        continue; /* @bin 0x004CBBE5 the flags of that compare: i == len, again */
                    }
                    i++;
                }
            } else if (c <= 0x20) {
                i++;
            } else if (key >= 0) {
                if (c != 0x22) {
                    goto end; /* a value that is not a quoted string */
                }
                i++;
                if (pass) {
                    P32(st, U32(st, self + 4) + 4u * (uint32_t) key) = U32(st, self + 8) + used;
                }
                c = B(i);
                while (c != 0x22) {
                    if (c == 0x5C) {
                        /* @bin 0x004CBC2E a backslash: blanks after it up to a line end are skipped (the
                         * scan starts from the key index [esp + 0x10], not from the backslash) */
                        int32_t e = key_index + 1;
                        int line_end = 0;
                        while (e <= len) {
                            uint8_t d = B(e);
                            if (d == 0xD || d == 0xA) {
                                line_end = 1;
                                break;
                            }
                            if (d > 0x20) {
                                break;
                            }
                            e++;
                        }
                        if (line_end) {
                            uint8_t d = B(e);
                            if (B(e + 1) != d && (d == 0xD || d == 0xA) && e < len) {
                                e++;
                            }
                            i += e;
                            c = B(i);
                            continue;
                        }
                        if (pass) {
                            P8(st, U32(st, self + 8) + used) = 0x5C;
                        }
                    } else if (pass) {
                        P8(st, U32(st, self + 8) + used) = c;
                    }
                    used++;
                    i++;
                    c = B(i);
                }
                if (pass) {
                    P8(st, U32(st, self + 8) + used) = 0;
                }
                used++;
                i++;
                key = -1;
            } else {
                if (pass == 0) { /* the key's end: its first blank becomes 0 */
                    for (int32_t e = i; e <= len; e++) {
                        if (B(e) <= 0x20) {
                            B(e) = 0;
                            break;
                        }
                    }
                }
                /* @bin 0x004CBCE1 the key's index ([esp + 0x10]: count when it is not in the table) */
                key_index = 0;
                while (key_index < count &&
                       stricmp_at(st, U32(st, table + 4 + 8 * (uint32_t) key_index), buf + (uint32_t) i) != 0) {
                    key_index++;
                }
                int32_t k = key_index;
                if (k >= count) {
                    goto end; /* an unknown key */
                }
                i += (int32_t) strlen_at(st, U32(st, table + 4 + 8 * (uint32_t) k));
                key = k;
            }
        }
    next_pass:;
    }
end:
    boot_release(st, host, buf); /* @bin 0x004CBD54 operator delete */
}

#undef B

void boot_ctor(game_state *st, const boot_host *host, uint32_t fn)
{
    switch (fn) {
    case FN_CTOR_0X00605760_401000: {
        /* @bin 0x00401010 __ftol(the double 0x0056F340) * 200 into 0x00605760 */
        int32_t v = x87_ftol(x87_from_double(u64_at(st, G_DBL_10_56F340)));
        S32(st, G_U605760) = (int32_t) ((uint32_t) v * 200u);
        break;
    }
    case FN_CTOR_FILE_LIST_0X006057C8_402360:
        file_list_ctor(st, G_FILES_LIST); /* @bin 0x00402370 */
        break;
    case FN_CTOR_MONUMENT_0X006FD5D8_413C70:
    case FN_CTOR_MONUMENT_0X006FD368_413CB0:
    case FN_CTOR_MONUMENT_0X006F9320_413CF0:
    case FN_CTOR_MONUMENT_0X006F5648_413D30:
    case FN_CTOR_MONUMENT_0X006E9188_413D70:
    case FN_CTOR_MONUMENT_0X006DE9E0_413DB0:
    case FN_CTOR_MONUMENT_0X006DE650_413DF0:
    case FN_CTOR_MONUMENT_0X006DE548_413E30:
        monument_ctor(st, fn);
        break;
    case FN_CTOR_0X006DE518_413E70:
        /* @bin 0x00413E80 */
        U32(st, G_SURFACE_LOCK + 0x10u) = 0xFFFFFFFFu;
        U32(st, G_SURFACE_LOCK) = 0;
        U32(st, G_SURFACE_LOCK + 0x20u) = 0;
        U32(st, G_SURFACE_LOCK + 0x1Cu) = 0;
        U32(st, G_SURFACE_LOCK + 0x28u) = 0;
        U32(st, G_SURFACE_LOCK + 0x24u) = 0;
        break;
    case FN_CTOR_CAMPAIGN_TABLE: {
        /* @bin 0x00417990 the campaign table 0x006FDD08 (0x0041AC50): 100 steps (0x0041AC00), counts 0,
         * the 0x41 bytes at +0x2C90 */
        for (uint32_t k = 0; k < 100; k++) {
            GAME_CALLT0(st, G_CAMPAIGN_TABLE + 0x40 * k, FN_CAMPAIGN_ENTRY_INIT);
        }
        U32(st, G_CAMPAIGN_TABLE + 0x1900) = 0;
        U32(st, G_CAMPAIGN_TABLE + 0x2C8C) = 0;
        memset(st->mem + (G_CAMPAIGN_TABLE + 0x2C90 - STATE_BASE), 0, 0x41);
        break;
    }
    case FN_CTOR_SCENARIO_LIST_0X00704C00_41E050:
        /* @bin 0x0041E0A0 the scenario list 0x00704C00: a file list, vtable 0x0056F54C */
        file_list_ctor(st, G_LIST_OBJECT);
        U32(st, G_LIST_OBJECT) = G_VTBL_SCENARIO_LIST;
        break;
    case FN_CTOR_RECORDS_0X00776FB0_4424A0:
        vector_ctor(st, G_ROUTES, 0x144, 0x32, elem_445460); /* @bin 0x004424B0 */
        break;
    case FN_CTOR_KEYWORD_TABLE:
        U32(st, G_KEYWORDS_THIS) = G_KEYWORD_TABLE; /* @bin 0x004482D0 */
        break;
    case FN_CTOR_EVENT_PHRASES_0X00784F20:
        /* @bin 0x004482F0 the event phrases: "EVENTMSG.TXT" (0x005D9A2C), keys 0x005D3010 */
        phrases_ctor(st, host, G_STRINGS_THIS, G_STR_EVENTMSG_TXT, G_EVENT_PHRASE_KEYS);
        break;
    case FN_CTOR_EVENTS_0X00785D70: {
        /* @bin 0x0044BA20 the events 0x00785D70: 0x96 records of 0x7C at +4 (0x00448960), count 0,
         * +0x48AC -1, +0x48B0 0, each record's first word its index, 0x00448A60 on +0x43D4 */
        uint32_t self = G_EVENTS;
        vector_ctor(st, self + 4, 0x7C, 0x96, elem_448960);
        U32(st, self) = 0;
        U32(st, self + 0x48AC) = 0xFFFFFFFFu;
        U32(st, self + 0x48B0) = 0;
        for (uint32_t i = 0; i < 0x96; i++) {
            U16(st, self + 4 + 0x7C * i) = (uint16_t) i;
        }
        GAME_CALLT0(st, self + 0x43D4, FN_EVENT_FIELDS_CLEAR);
        U8(st, self + 0x43D6) = 2;
        U8(st, self + 0x48B4) = 0;
        U8(st, self + 0x48B5) = 0;
        break;
    }
    case FN_CTOR_SHIPWRIGHT_QUEUES:
        /* @bin 0x00451A70 the shipwright queues 0..4 (arrays of 20, 0x004F6690: 0x004F6700 allocates
         * them in the state's heap), 0x23 dwords at 0x0097844C cleared, 5 queues there (0x004F6670) */
        for (uint32_t q = 0; q < 5; q++) {
            uint32_t v = G_SHIPWRIGHT_QUEUES + G_SHIPWRIGHT_QUEUE_STRIDE * q;
            U32(st, v + 8) = 0;
            U32(st, v + 0xC) = 0;
            U32(st, v + 0x14) = 0;
            U32(st, v + 0x10) = 0;
            U8(st, v) = 0;
            U32(st, v + 4) = 1;
            GAME_CALLT(st, v, FN_VEC_RESERVE, 0x14);
        }
        memset(st->mem + (G_SHIPWRIGHT_QUEUES + 5u * G_SHIPWRIGHT_QUEUE_STRIDE - STATE_BASE), 0, 0x23 * 4);
        vector_ctor(st, G_SHIPWRIGHT_QUEUES + 5u * G_SHIPWRIGHT_QUEUE_STRIDE, 0x1C, 5, elem_4f6670);
        break;
    case FN_CTOR_0X00978D88_486080:
        /* @bin 0x00486090 */
        U32(st, G_VEGETATION) = 0x19;
        U32(st, G_VEGETATION + 4u) = 0x23;
        U32(st, G_VEGETATION + 8u) = 0;
        U32(st, G_VEGETATION + 0xCu) = 0;
        U8(st, G_VEGETATION + 0x10u) = 0;
        U8(st, G_VEGETATION + 0x12u) = 0;
        U8(st, G_VEGETATION + 0x11u) = 0;
        U8(st, G_VEGETATION + 0x13u) = 0;
        break;
    case FN_CTOR_0X00BCA34C_489940: {
        /* @bin 0x00489950 __ftol(the double 0x0056F5A8) * 200 into 0x00BCA34C */
        int32_t v = x87_ftol(x87_from_double(u64_at(st, G_DBL_10_56F5A8)));
        S32(st, G_HARVEST_SIZE) = (int32_t) ((uint32_t) v * 200u);
        break;
    }
    case FN_CTOR_0X00BCA350_487F50:
        break; /* @bin 0x00487F60 returns 0x00BCA350: nothing to construct */
    case FN_CTOR_FLOODPLAIN_OBJECT_0X00D05720:
        /* @bin 0x004BD2D0 the floodplain object 0x00D05720: its settings (+0x32C40, 0x004BD330), 0x1E
         * records of 0x1C at +0x32C64 (0x004BD650) */
        GAME_CALLT0(st, G_UD05720 + 0x32C40u, FN_FLOODPLAIN_SETTINGS_RESET);
        vector_ctor(st, G_UD05720 + 0x32C64u, 0x1C, 0x1E, elem_4bd650);
        break;
    case FN_CTOR_0X00D05718_4BD300: {
        /* @bin 0x004BD310 the double 0x0056F680 / 0x0056F678 (fdiv, 53 bits) into 0x00D05718 */
        uint64_t q = x87_to_double(x87_divv(x87_from_double(u64_at(st, G_DBL_9792)),
                                            x87_from_double(u64_at(st, G_DBL_25))));
        U32(st, G_DAY_FLOOD_PERIOD) = (uint32_t) q;
        U32(st, G_DAY_FLOOD_PERIOD + 4u) = (uint32_t) (q >> 32);
        break;
    }
    case FN_CTOR_SOUND_OBJECT_0X00E92BF8:
        sound_object_ctor(st, G_SOUND); /* @bin 0x004CDF80 */
        break;
    case FN_CTOR_MUSIC_TABLES_0X00E39180:
        music_tables_ctor(st, G_MUSIC); /* @bin 0x004CDFC0 */
        break;
    case FN_CTOR_0X004D1350:
        boot_ctor_4d1350(st); /* @bin 0x004D1350 */
        break;
    case FN_CTOR_0X004D4A60:
        boot_ctor_4d4a60(st); /* @bin 0x004D4A60 */
        break;
    case FN_CTOR_RECORDS_0X00EAAF68_4F64A0:
        vector_ctor(st, G_REC88, 0xE0, 0x32, elem_4f6500); /* @bin 0x004F64B0 */
        break;
    case FN_CTOR_FAMILY_LIST_0X00EADFA8:
        file_list_ctor(st, G_FAMILY_LIST); /* @bin 0x00516580 the family list */
        break;
    case FN_CTOR_GRAPHICS_OBJECT_0X00F55BB0: {
        /* @bin 0x0053DA90 the graphics object 0x00F55BB0: 0x1E slots of 0xCC3C at +0x1F40
         * (0x005404E0), the external buffers +0x30F5C4 / +0x30F5C0 0, then 0x0053DB60 */
        uint32_t self = G_GRAPHICS_GFX;
        vector_ctor(st, self + 0x1F40, 0xCC3C, 0x1E, elem_5404e0);
        U32(st, self + 0x30F5C4u) = 0;
        U32(st, self + 0x30F5C0u) = 0;
        graphics_reset(st, self);
        break;
    }
    default:
        state_unported(st, fn);
        break;
    }
}

static int32_t run_graphics_reset(game_state *st, const int32_t *a)
{
    /* thiscall: `this` in a[PORT_THIS_ARG], the graphics object 0x00F55BB0 by default */
    graphics_reset(st, a[PORT_THIS_ARG] ? (uint32_t) a[PORT_THIS_ARG] : G_GRAPHICS_GFX);
    return 0;
}

const ported_function boot_ctor_ports[] = {
    /* the constructors are run by game_boot (boot_ctor) from the CRT's table; registered so that the
     * oracle's recorder treats them as ported (they need the boot's host for their outside memory) */
    {0x004CBA60u, "event_phrases_read (boot ctors)", 2, 32, 0, NULL, 0},
    {0x0055E38Bu, "sound_object_ctor (boot ctors)", 0, 32, 0, NULL, 0},
    {0x00539550u, "music_tables_ctor (boot ctors)", 0, 32, 0, NULL, 0},
    {0x0053DA90u, "graphics_object_ctor (boot ctors)", 0, 32, 0, NULL, 0},
    {0x0053DB60u, "graphics_reset", 0, 0, G_GRAPHICS_GFX, run_graphics_reset, 0},
    {0x0044BA20u, "events_ctor (boot ctors)", 0, 32, 0, NULL, 0},
    {0x00451A70u, "shipwright_queues_ctor (boot ctors)", 0, 0, 0, NULL, 0},
    /* the table's entries 0x00576000 .. 0x0057607C and the constructors they run (`mov ecx, OBJECT;
     * jmp CONSTRUCTOR` stubs, thunks), ported in boot_ctor (their atexit registrations are the CRT's:
     * none kept) */
    {0x00401000u, "ctor 0x00605760 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00401010u, "ctor 0x00605760 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00402360u, "ctor file list 0x006057C8 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00402370u, "ctor file list 0x006057C8 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413C70u, "ctor monument 0x006FD5D8 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413C80u, "ctor monument 0x006FD5D8 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413CB0u, "ctor monument 0x006FD368 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413CC0u, "ctor monument 0x006FD368 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413CF0u, "ctor monument 0x006F9320 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413D00u, "ctor monument 0x006F9320 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413D30u, "ctor monument 0x006F5648 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413D40u, "ctor monument 0x006F5648 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413D70u, "ctor monument 0x006E9188 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413D80u, "ctor monument 0x006E9188 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413DB0u, "ctor monument 0x006DE9E0 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413DC0u, "ctor monument 0x006DE9E0 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413DF0u, "ctor monument 0x006DE650 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413E00u, "ctor monument 0x006DE650 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413E30u, "ctor monument 0x006DE548 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413E40u, "ctor monument 0x006DE548 (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413E70u, "ctor 0x006DE518 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00413E80u, "ctor 0x006DE518 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00417980u, "ctor campaign table (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00417990u, "ctor campaign table 0x006FDD08 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x0041E050u, "ctor scenario list 0x00704C00 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x0041E060u, "ctor scenario list (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x0041E0A0u, "ctor scenario list 0x00704C00 (body) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004424A0u, "ctor records 0x00776FB0 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004424B0u, "ctor records 0x00776FB0 (body) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004482C0u, "ctor keyword table (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004482D0u, "ctor keyword table 0x00784F34 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004482E0u, "ctor event phrases 0x00784F20 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004482F0u, "ctor event phrases (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00448390u, "ctor events 0x00785D70 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004483A0u, "ctor events (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00451A60u, "ctor shipwright queues (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00486080u, "ctor 0x00978D88 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00486090u, "ctor 0x00978D88 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00487F50u, "ctor 0x00BCA350 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00487F60u, "ctor 0x00BCA350 (nothing) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00489940u, "ctor 0x00BCA34C (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00489950u, "ctor 0x00BCA34C (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004BD270u, "ctor floodplain object 0x00D05720 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004BD280u, "ctor floodplain object (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004BD2D0u, "ctor floodplain object (body) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004BD300u, "ctor 0x00D05718 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004BD310u, "ctor 0x00D05718 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004CDF70u, "ctor sound object 0x00E92BF8 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004CDF80u, "ctor sound object (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004CDFB0u, "ctor music tables 0x00E39180 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004CDFC0u, "ctor music tables (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004D1340u, "ctor 0x004D1350 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004D1350u, "ctor boot_ctor_4d1350 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004D4A50u, "ctor 0x004D4A60 (thunk) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004D4A60u, "ctor boot_ctor_4d4a60 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004F64A0u, "ctor records 0x00EAAF68 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x004F64B0u, "ctor records 0x00EAAF68 (body) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00516570u, "ctor family list 0x00EADFA8 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x00516580u, "ctor family list (call) (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x0053DA50u, "ctor graphics object 0x00F55BB0 (boot_ctor)", 0, 0, 0, NULL, 0},
    {0x0053DA60u, "ctor graphics object (call) (boot_ctor)", 0, 0, 0, NULL, 0},
};
const int boot_ctor_ports_count = sizeof(boot_ctor_ports) / sizeof(boot_ctor_ports[0]);
