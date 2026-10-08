/* Imhotep - the platform's memory at game addresses: pixel data, drawing buffers (build/imhotep).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original keeps outside its data segments some memory that the drawing code reads and writes
 * through pointers stored in the state: the back buffer (u32 0x00E92E18, a locked system-memory
 * DirectDraw surface), the spare screen (0x00E92E1C), the pixel data of the graphics slots (the
 * .555 files read by 0x0053F660, pointed to by the slot +0xCC24 and by every image entry +0), the
 * fonts, the text files (0x00E36C70, 0x00E36C74), the sound registry's names. The ported code reaches
 * them with P8 / P16 / P32 (state_ptr), which follows the state's external mappings.
 *
 * Design: the platform reserves ONE large range of the 32-bit game address space, at the address the
 * oracle uses for the same allocations (0x68000000, tools/oracle/emu.py OTHER_HEAP_BASE), backed by
 * one lazily committed host mapping (mmap, no reserve), and maps it into the state once with
 * state_map_external. Inside it, a small allocator hands out blocks (16-byte aligned, zero-filled):
 * the platform's own buffers (back buffer, strings it gives the game) and the pixel buffers of
 * platform_pixels_load, whose handle IS the game address of the data (so the original's pointer
 * arithmetic on it, entry offsets added to the slot's buffer, works unchanged). pixels_free gives the
 * block back. Only one mapping is used, so the state's 16-entry external table is not a limit, and
 * new loads never need a new mapping. The range is writable (the original's buffers are writable);
 * nothing in it is part of the simulation state. */
#ifndef IMHOTEP_PLATFORM_MEMORY_H
#define IMHOTEP_PLATFORM_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include "state/state.h"

#define PMEM_BASE 0x68000000u /* the oracle's other heap starts here (tools/oracle/emu.py) */
#define PMEM_SIZE 0x30000000u /* 768 MB of address space, committed on use */

/* Reserve the range. Returns 0, or -1. */
int pmem_init(void);
/* Copy `size` bytes (an image of the oracle's other heap: the temporary boot path) to the start of
 * the range; the allocator starts after them. Returns 0, or -1 when too large. */
int pmem_preload(const uint8_t *data, uint32_t size);
/* Map the range into the state (state_map_external). */
int pmem_map(game_state *st);
/* A zero-filled block of n bytes: its game address, 0 when the range is full. */
uint32_t pmem_alloc(uint32_t n);
/* Give a block back (addresses not from pmem_alloc, e.g. inside the preloaded image, are ignored). */
void pmem_free(uint32_t address);
/* Host pointer of a game address in the range (NULL outside it). */
uint8_t *pmem_ptr(uint32_t address, uint32_t len);
/* Bytes in use (allocated blocks and the preloaded image). */
uint32_t pmem_used(void);

#endif
