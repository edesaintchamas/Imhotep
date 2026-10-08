/* Imhotep - the game state: a byte-exact mirror of Pharaoh.exe's data segments.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original keeps its whole simulation in global variables. Imhotep keeps the same bytes at
 * the same addresses: `game_state` covers .rdata and .data (incl. the uninitialised part) of the
 * reference executable, 0x0056F000 .. 0x01267AA8, and code addresses its variables by their
 * address in Pharaoh.exe. So a ported function reads and writes what the original does, the
 * oracle (tools/oracle) compares states by address, and saved chunks load where the game loads
 * them (save_schema[].address).
 *
 * The initial contents (the exe's initialised data: static tables, defaults) come from the
 * user's own Pharaoh.exe, read by platform/exe_image.c; nothing from the game is in this
 * repository. Values are little-endian, as in the original (checked at compile time).
 * Layout: docs/formats/state.md. */
#ifndef IMHOTEP_STATE_STATE_H
#define IMHOTEP_STATE_STATE_H

#include <stddef.h>
#include <stdint.h>

#define STATE_BASE 0x0056F000u /* @bin PE section .rdata */
#define STATE_END 0x01267AA8u  /* @bin PE section .data: 0x00576000 + virtual size 0xCF1AA8 */
#define STATE_SIZE (STATE_END - STATE_BASE)

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "game_state mirrors the original's little-endian memory"
#endif

/* The original allocates some records with new / malloc (floodplain objects, route paths).
 * They live in a heap arena at fixed addresses with the oracle's allocator (tools/oracle/emu.py:
 * bump allocation, 16-byte aligned, zero-filled memory, nothing reused), so that pointers the game
 * stores in its variables are reproducible and compare equal between the port and the oracle.
 * Pointer values are not otherwise observable by the game. */
#define STATE_HEAP_BASE 0x60000000u
#define STATE_HEAP_LIMIT 0x08000000u

struct sim_env;      /* core/sim/call.h */
struct sim_platform; /* core/sim/platform.h */

typedef struct {
    uint8_t *mem;       /* STATE_SIZE bytes; mem[a - STATE_BASE] is the byte at address a */
    uint8_t *heap;      /* heap_cap bytes; heap[a - STATE_HEAP_BASE] */
    uint32_t heap_used; /* next allocation at STATE_HEAP_BASE + heap_used */
    uint32_t heap_cap;
    uint32_t *blocks;   /* (offset, size) of every allocation, in address order */
    uint32_t nblocks, blocks_cap;
    uint32_t unported;  /* first original function reached that is not ported yet (0: none) */
    const struct sim_env *env; /* who handles calls to unported functions (tests); not copied */
    const struct sim_platform *platform; /* video and sound requests (NULL: none); not copied */
    struct {
        uint32_t address, size;
        const uint8_t *data;
    } external[16]; /* game data outside the segments (state_map_external); not copied */
    int nexternal;
} game_state;

/* A ported function reached a branch that calls an original function not ported yet: recorded
 * (the first one) so that tools and tests fail with its address instead of a wrong state. */
void state_unported(game_state *st, uint32_t address);

/* Allocate a zeroed state. Returns 0, or -1 when out of memory. */
int state_create(game_state *st);
void state_free(game_state *st);

/* Copy an initialised section of the exe (address, bytes) into the state; parts outside the
 * state's range are ignored. */
void state_load_section(game_state *st, uint32_t address, const uint8_t *data, size_t len);

/* Pointer to the byte at `address`, or NULL when [address, address + len) is outside the data
 * segments and the allocated heap. Code that follows a pointer stored by the game uses this. */
uint8_t *state_ptr(const game_state *st, uint32_t address, uint32_t len);

/* Make `size` bytes of game data that the original keeps outside its segments (the text file
 * Pharaoh_Text.eng from byte 0x1F5C, at the address the state's pointer 0x00E36C70 holds) readable
 * through state_ptr, read-only (writes through P8/P16/P32 there are not allowed). data must stay
 * valid. Returns 0, or -1 when the table is full. The first mapping that holds an address wins.
 * Test tools also map the drawing buffers the original keeps outside the state (the back buffer at
 * [0x00E92E18], tools/oracle/state_replay.c --screen): those the drawing ports write. */
int state_map_external(game_state *st, uint32_t address, const uint8_t *data, uint32_t size);

/* new / malloc: n zeroed bytes in the heap arena; returns their address, or 0 when the arena is
 * full or out of memory (the original's allocation failures). */
uint32_t state_alloc(game_state *st, uint32_t n);

/* delete / free: the block is zero-filled (the oracle does the same, so dead memory compares
 * equal); nothing is reused. Unknown addresses and 0 are ignored. */
void state_dealloc(game_state *st, uint32_t address);

/* Make the heap hold `used` bytes (state files carry the heap after the data segments). Returns 0,
 * or -1 when out of memory. */
int state_heap_resize(game_state *st, uint32_t used);

/* dst := a copy of src (data segments, heap, blocks, unported); dst must have been created.
 * Returns 0, or -1 when out of memory. */
int state_copy(game_state *dst, const game_state *src);

/* Record a heap block allocated elsewhere (tests replaying the original's allocations). */
int state_heap_add_block(game_state *st, uint32_t offset, uint32_t size);

/* State files: STATE_SIZE bytes, u32 heap_used, the heap bytes, u32 block count, then (u32
 * offset, u32 size) per block. */
int state_write_file(const game_state *st, void *file);  /* FILE * */
int state_read_file(game_state *st, void *file);         /* FILE * */

/* Unaligned, aliasing-safe integer types: the original packs fields at any offset. */
typedef int16_t __attribute__((aligned(1), may_alias)) state_s16;
typedef uint16_t __attribute__((aligned(1), may_alias)) state_u16;
typedef int32_t __attribute__((aligned(1), may_alias)) state_s32;
typedef uint32_t __attribute__((aligned(1), may_alias)) state_u32;

/* Typed lvalue at a constant address inside the state (no range check: the address is part of
 * the code, the same as in the original). Little-endian host: plain loads and stores. */
#define STATE_AT(st, type, address) (*(type *) ((st)->mem + ((uint32_t) (address) - STATE_BASE)))
#define S8(st, a) STATE_AT(st, int8_t, a)
#define U8(st, a) STATE_AT(st, uint8_t, a)
#define S16(st, a) STATE_AT(st, state_s16, a)
#define U16(st, a) STATE_AT(st, state_u16, a)
#define S32(st, a) STATE_AT(st, state_s32, a)
#define U32(st, a) STATE_AT(st, state_u32, a)

/* Typed lvalue at any address of the state, data segments or heap, for addresses the game reads
 * from its own pointers. An address outside the state is a null dereference: the original would
 * fault there as well. */
#define PTR_AT(st, type, address) (*(type *) state_ptr(st, (uint32_t) (address), sizeof(type)))
#define P8(st, a) PTR_AT(st, uint8_t, a)
#define P16(st, a) PTR_AT(st, state_s16, a)
#define P32(st, a) PTR_AT(st, state_u32, a)

#endif
