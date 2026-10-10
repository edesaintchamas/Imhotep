/* Imhotep - named globals: the graphics object, its image slots and the image entries (core/graphics).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Evidence: docs/formats/graphics.md §5 (image entry), §11 ("What a load writes": @bin 0x0053DD90,
 * 0x0053DFF0). Conventions: docs/naming.md (owner: package P5). */
#ifndef IMHOTEP_STATE_LAYOUT_G_GRAPHICS_H
#define IMHOTEP_STATE_LAYOUT_G_GRAPHICS_H

#define G_GRAPHICS_GFX 0x00F55BB0u /* the graphics object: `this` of 0x0053DD90, which loads a file into an image slot (core/city/day2_farm.c) */
#define G_IMAGE_GROUPS 0x00F55BB4u /* i32 per sprite group, 8 bytes a group: its first image (core/map/terrain.c writes them; core/figure/actions_a.c) */
#define G_IMAGE_COUNT 0x0126517Cu /* G_GRAPHICS_GFX + GFX_IMAGE_COUNT: i32 the image ids in use (present/ui/info/military.c) */
#define G_SG3_PIXEL_CURSOR 0x00D3B5F4u /* @bin 0x0053DFF0 u32: the running pointer into a slot's pixel buffer while its entries are filled */
#define G_STR_SYSTEM_BMP 0x006018E4u /* @bin 0x0053E152 "system.bmp": a file whose first bitmap has this name skips 200 images */
#define G_STR_SG3_EXT 0x006018F0u    /* @bin 0x0053E078 ".sg3" */
#define G_STR_DATA_DIR 0x006018F8u   /* @bin 0x0053E078 "data\\" */

/* fields of the graphics object (G_GRAPHICS_GFX + ...; graphics.md §11) */
#define GFX_SLOTS 0x1F40u            /* @bin 0x0053DD90 the image slots, GFX_SLOT_SIZE bytes each (0x1E, 0x0053DA90) */
#define GFX_SLOT_SIZE 0xCC3Cu        /* @bin 0x0053DA90 */
#define GFX_ENTRIES 0x180E48u        /* @bin 0x0053DD90 the image entries, 64 bytes per image id (IMGENTRY_*) */
#define GFX_SLOT_GROUP_ENDS 0x30F548u /* @bin 0x0053DEB3 i32 per slot: its expected running group total + 1 (the
                                       * "Graphic file out of sync" check) */
#define GFX_EXTERNAL_BUF 0x30F5C0u   /* @bin 0x0053DB60 u32: the buffer holding the last external image read (present/draw/image.c) */
#define GFX_EXTERNAL_BUF2 0x30F5C4u  /* @bin 0x0053DB60 u32: the other external buffer released with it (0x0053F790) */
#define GFX_GROUP_TOTAL 0x30F5C8u    /* @bin 0x0053DD90 i32: the running total of groups */
#define GFX_IMAGE_COUNT 0x30F5CCu    /* @bin 0x0053DD90 i32: the running total of images: the image ids in use (G_IMAGE_COUNT) */
#define GFX_PIXEL_TOTAL 0x30F5D0u    /* @bin 0x0053DD90 u32: the total pixel bytes of the slots */
#define GFX_SLOT_TOTAL 0x30F5D4u     /* @bin 0x0053DD90 i32: the slots filled, the next free slot */

/* fields of an image slot (G_GRAPHICS_GFX + GFX_SLOTS + GFX_SLOT_SIZE * slot + ...): the file name at +0,
 * the bitmap names from +0x104 (0x104 bytes each) */
#define GFXSLOT_PIXELS 0xCC24u       /* @bin 0x0053DFF0 u32: the pixel buffer (graphics heap, 0x0053F660) */
#define GFXSLOT_PIXEL_BYTES 0xCC28u  /* @bin 0x0053DFF0 u32: the .555 size */
#define GFXSLOT_IMAGES 0xCC2Cu       /* @bin 0x0053DFF0 i32: image count (max of old and new), 0: empty slot */
#define GFXSLOT_GROUPS 0xCC30u       /* @bin 0x0053DFF0 i32: group count */
#define GFXSLOT_FIRST_IMAGE 0xCC34u  /* @bin 0x0053DFF0 i32: the slot's first image id */
#define GFXSLOT_FIRST_GROUP 0xCC38u  /* @bin 0x0053DFF0 i32: the slot's first group number */

/* fields of an image entry (G_GRAPHICS_GFX + GFX_ENTRIES + 64 * id + ...; graphics.md §5). Note:
 * G_IMAGE_ENTRIES / IMAGE_ENTRY_* (pending names of core/map) count from entry + 8 */
#define IMGENTRY_OFFSET 0u           /* @bin 0x0053E41E u32: pointer into the pixel buffer (internal), 1-based offset (external) */
#define IMGENTRY_LENGTH 4u           /* @bin 0x0053E41E i32: byte length of the pixel data */
#define IMGENTRY_FULL_LENGTH 8u      /* @bin 0x0053E41E i32: isometric: the footprint's byte length */
#define IMGENTRY_SLOT 12u            /* @bin 0x0053E41E i32: := the slot at load */
#define IMGENTRY_TYPE 50u            /* @bin 0x0053E41E u8: 30 isometric */
#define IMGENTRY_COMPRESSED 51u      /* @bin 0x0053E41E u8: RLE */
#define IMGENTRY_EXTERNAL 52u        /* @bin 0x0053E41E u8: data in <bitmap name>.555 */
#define IMGENTRY_HAS_TOP 53u         /* @bin 0x0053E41E u8: isometric image with an RLE top after its footprint */

/* the image entries as some ports count them, from their +8 (G_GRAPHICS_GFX + GFX_ENTRIES + 8): the
 * fields below are IMGENTRY_* - 8 */
#define G_IMAGE_ENTRIES 0x010D6A00u /* core/city/day2_farm.c: 0x40-byte records */
#define IMAGE_ENTRY_WIDTH 0x0Cu  /* @bin 0x004D4720 i16: the width (entry +20, graphics.md §5) */
#define IMAGE_ENTRY_HEIGHT 0x0Eu /* @bin 0x004D4720 i16: the height (entry +22) */

#endif
