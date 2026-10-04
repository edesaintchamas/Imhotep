/* Imhotep - PKWARE DCL "explode" (binary literals).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Portions translated from Julius (https://github.com/bvschaik/julius, src/core/zip.c),
 * AGPL-3.0, Bianca van Schaik and contributors.
 *
 * The format is confirmed on the data (all 165 compressed chunks of the 33 maps decompress to
 * the size their schema declares; docs/formats/map.md §3). The game's own decompressor
 * (0x0055BCF0) has not been compared instruction by instruction. */
#include "io/pkware.h"

/* @from julius:src/core/zip.c pk_copy_offset_bits / pk_copy_offset_code
 * @diff terrain_check (format of every compressed map chunk) */
static const uint8_t OFFSET_BITS[64] = {
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};
static const uint8_t OFFSET_CODE[64] = {
    0x03, 0x0D, 0x05, 0x19, 0x09, 0x11, 0x01, 0x3E, 0x1E, 0x2E, 0x0E, 0x36, 0x16, 0x26, 0x06, 0x3A,
    0x1A, 0x2A, 0x0A, 0x32, 0x12, 0x22, 0x42, 0x02, 0x7C, 0x3C, 0x5C, 0x1C, 0x6C, 0x2C, 0x4C, 0x0C,
    0x74, 0x34, 0x54, 0x14, 0x64, 0x24, 0x44, 0x04, 0x78, 0x38, 0x58, 0x18, 0x68, 0x28, 0x48, 0x08,
    0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10, 0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00,
};
/* @from julius:src/core/zip.c pk_copy_length_* @diff terrain_check */
static const uint8_t LENGTH_BITS[16] = {3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7};
static const uint16_t LENGTH_BASE[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                         0x08, 0x0A, 0x0E, 0x16, 0x26, 0x46, 0x86, 0x106};
static const uint8_t LENGTH_CODE[16] = {0x05, 0x03, 0x01, 0x06, 0x0A, 0x02, 0x0C, 0x14,
                                        0x04, 0x18, 0x08, 0x30, 0x10, 0x20, 0x40, 0x00};
static const uint8_t LENGTH_EXTRA[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

#define END_OF_STREAM 519 /* @from julius:src/core/zip.c PK_EOF (773) - 254 @diff terrain_check */

typedef struct {
    const uint8_t *data;
    size_t len, pos;
    uint32_t acc;
    int n;
} bits;

static uint32_t peek(bits *b, int k)
{
    while (b->n < k) {
        uint32_t byte = b->pos < b->len ? b->data[b->pos] : 0;
        b->pos++;
        b->acc |= byte << b->n;
        b->n += 8;
    }
    return b->acc & ((1u << k) - 1);
}

static uint32_t take(bits *b, int k)
{
    uint32_t v = peek(b, k);
    b->acc >>= k;
    b->n -= k;
    return v;
}

static int overrun(const bits *b)
{
    return b->pos - (size_t) (b->n / 8) > b->len;
}

static void jump_table(const uint8_t *nbits, const uint8_t *codes, int size, uint8_t *jump)
{
    /* @from julius:src/core/zip.c pk_explode_construct_jump_table @diff terrain_check */
    for (int i = size - 1; i >= 0; i--) {
        for (int code = codes[i]; code < 0x100; code += 1 << nbits[i]) {
            jump[code] = (uint8_t) i;
        }
    }
}

int pkware_explode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_len)
{
    uint8_t length_jump[256], offset_jump[256];
    if (in_len < 4 || in[0] != 0 || in[1] < 4 || in[1] > 6) {
        return -1; /* @from julius:src/core/zip.c pk_explode header checks @diff terrain_check */
    }
    int window = in[1];
    jump_table(LENGTH_BITS, LENGTH_CODE, 16, length_jump);
    jump_table(OFFSET_BITS, OFFSET_CODE, 64, offset_jump);
    bits b = {in, in_len, 2, 0, 0};
    size_t pos = 0;
    for (;;) {
        if (overrun(&b)) {
            return -1;
        }
        if (take(&b, 1)) {
            /* copy: length symbol, then offset @from julius:src/core/zip.c @diff terrain_check */
            int idx = length_jump[peek(&b, 8)];
            take(&b, LENGTH_BITS[idx]);
            int length = LENGTH_BASE[idx] + (LENGTH_EXTRA[idx] ? (int) take(&b, LENGTH_EXTRA[idx]) : 0) + 2;
            if (length == END_OF_STREAM) {
                break;
            }
            int oidx = offset_jump[peek(&b, 8)];
            take(&b, OFFSET_BITS[oidx]);
            int low = length == 2 ? 2 : window;
            size_t offset = (((size_t) oidx << low) | take(&b, low)) + 1;
            if (offset > pos || pos + (size_t) length > out_len) {
                return -1;
            }
            for (int k = 0; k < length; k++, pos++) {
                out[pos] = out[pos - offset];
            }
        } else {
            if (pos >= out_len) {
                return -1;
            }
            out[pos++] = (uint8_t) take(&b, 8);
        }
    }
    return overrun(&b) || pos != out_len ? -1 : 0;
}
