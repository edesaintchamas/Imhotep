/* Imhotep - PKWARE DCL "implode" (binary literals), as the game's compressor 0x0055BF10 does it.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Translated from Julius (https://github.com/bvschaik/julius, src/core/zip.c), AGPL-3.0,
 * Bianca van Schaik and contributors: its pk_implode is a decompilation of the same PKWARE library
 * that Caesar III and Pharaoh link.
 * @from julius:src/core/zip.c (pk_implode and helpers, tables, verbatim but for the I/O glue)
 * @diff oracle_save_check (output byte-identical to the game compressor 0x0055BF10 on every
 * compressed chunk of 15 saves)
 */
#include "io/pkware.h"

#include <stdlib.h>
#include <string.h>

enum {
    PK_SUCCESS = 0,
    PK_INVALID_WINDOWSIZE = 1,
    PK_LITERAL_ENCODING_UNSUPPORTED = 2,
    PK_TOO_FEW_INPUT_BYTES = 3,
    PK_ERROR_DECODING = 4,
    PK_ERROR_VALUE = 774,
    PK_EOF = 773,
};

struct pk_token {
    int stop;

    const uint8_t *input_data;
    int input_ptr;
    int input_length;

    uint8_t *output_data;
    int output_ptr;
    int output_length;
};

typedef int pk_input_func(uint8_t *buffer, int length, struct pk_token *token);
typedef void pk_output_func(uint8_t *buffer, int length, struct pk_token *token);

struct pk_comp_buffer {
    pk_input_func *input_func;
    pk_output_func *output_func;
    struct pk_token *token;

    int window_size;
    int dictionary_size;
    unsigned int copy_offset_extra_mask;
    int current_output_bits_used;

    uint8_t input_data[8708];
    uint8_t output_data[2050];
    int output_ptr;

    uint16_t analyze_offset_table[2304];
    uint16_t analyze_index[8708];
    signed short long_matcher[518];

    uint16_t codeword_values[774];
    uint8_t codeword_bits[774];
};


struct pk_copy_length_offset {
    int length;
    uint16_t offset;
};

static const uint8_t pk_copy_offset_bits[64] = {
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

static const uint8_t pk_copy_offset_code[64] = {
    0x03, 0x0D, 0x05, 0x19, 0x09, 0x11, 0x01, 0x3E,
    0x1E, 0x2E, 0x0E, 0x36, 0x16, 0x26, 0x06, 0x3A,
    0x1A, 0x2A, 0x0A, 0x32, 0x12, 0x22, 0x42, 0x02,
    0x7C, 0x3C, 0x5C, 0x1C, 0x6C, 0x2C, 0x4C, 0x0C,
    0x74, 0x34, 0x54, 0x14, 0x64, 0x24, 0x44, 0x04,
    0x78, 0x38, 0x58, 0x18, 0x68, 0x28, 0x48, 0x08,
    0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10,
    0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00,
};

static const uint8_t pk_copy_length_base_bits[16] = {
    3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7,
};


static const uint8_t pk_copy_length_base_code[16] = {
    0x05, 0x03, 0x01, 0x06, 0x0A, 0x02, 0x0C, 0x14,
    0x04, 0x18, 0x08, 0x30, 0x10, 0x20, 0x40, 0x00,
};

static const uint8_t pk_copy_length_extra_bits[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8,
};

static void pk_memcpy(uint8_t *dst, const uint8_t *src, int length)
{
    for (int i = 0; i < length; i++) {
        dst[i] = src[i];
    }
}

static void pk_memset(void *buffer, uint8_t fill_byte, unsigned int length)
{
    memset(buffer, fill_byte, length);
}

static int pk_implode_fill_input_buffer(struct pk_comp_buffer *buf, int bytes_to_read)
{
    int used = 0;
    int read;
    do {
        read = buf->input_func(&buf->input_data[buf->dictionary_size + 516 + used], bytes_to_read, buf->token);
        used += read;
        bytes_to_read -= read;
    } while (read && bytes_to_read > 0);
    return used;
}

static void pk_implode_flush_full_buffer(struct pk_comp_buffer *buf)
{
    buf->output_func(buf->output_data, 2048, buf->token);
    uint8_t new_first_byte = buf->output_data[2048];
    uint8_t last_byte = buf->output_data[buf->output_ptr];
    buf->output_ptr -= 2048;
    memset(buf->output_data, 0, 2050);
    if (buf->output_ptr) {
        buf->output_data[0] = new_first_byte;
    }
    if (buf->current_output_bits_used) {
        buf->output_data[buf->output_ptr] = last_byte;
    }
}

static void pk_implode_write_bits(struct pk_comp_buffer *buf, int num_bits, unsigned int value)
{
    if (num_bits > 8) { // but never more than 16
        num_bits -= 8;
        pk_implode_write_bits(buf, 8u, value);
        value >>= 8;
    }
    int current_bits_used = buf->current_output_bits_used;
    uint8_t shifted_value = (uint8_t) (value << buf->current_output_bits_used);
    buf->output_data[buf->output_ptr] |= shifted_value;
    buf->current_output_bits_used += num_bits;
    if (buf->current_output_bits_used == 8) {
        buf->output_ptr++;
        buf->current_output_bits_used = 0;
    } else if (buf->current_output_bits_used > 8) {
        buf->output_ptr++;
        buf->output_data[buf->output_ptr] = (uint8_t) (value >> (8 - current_bits_used));
        buf->current_output_bits_used -= 8;
    }
    if (buf->output_ptr >= 2048) {
        pk_implode_flush_full_buffer(buf);
    }
}

static void pk_implode_write_copy_length_offset(struct pk_comp_buffer *buf, struct pk_copy_length_offset copy)
{
    pk_implode_write_bits(buf, buf->codeword_bits[copy.length + 254], buf->codeword_values[copy.length + 254]);

    if (copy.length == 2) {
        pk_implode_write_bits(buf, pk_copy_offset_bits[copy.offset >> 2],
                              pk_copy_offset_code[copy.offset >> 2]);
        pk_implode_write_bits(buf, 2, copy.offset & 3);
    } else {
        pk_implode_write_bits(buf, pk_copy_offset_bits[copy.offset >> buf->window_size],
                              pk_copy_offset_code[copy.offset >> buf->window_size]);
        pk_implode_write_bits(buf, buf->window_size, copy.offset & buf->copy_offset_extra_mask);
    }
}

static void pk_implode_determine_copy(struct pk_comp_buffer *buf, int input_index, struct pk_copy_length_offset *copy)
{
    uint8_t *input_ptr = &buf->input_data[input_index];
    int hash_value = 4 * input_ptr[0] + 5 * input_ptr[1];
    uint16_t *analyze_offset_ptr = &buf->analyze_offset_table[hash_value];
    uint16_t hash_analyze_index = *analyze_offset_ptr;
    int min_match_index = input_index - buf->dictionary_size + 1;
    uint16_t *analyze_index_ptr = &buf->analyze_index[hash_analyze_index];
    if (*analyze_index_ptr < min_match_index) {
        do {
            analyze_index_ptr++;
            hash_analyze_index++;
        } while (*analyze_index_ptr < min_match_index);
        *analyze_offset_ptr = hash_analyze_index;
    }

    int max_matched_bytes = 1;
    uint8_t *prev_input_ptr = input_ptr - 1;
    uint16_t *hash_analyze_index_ptr = &buf->analyze_index[hash_analyze_index];
    uint8_t *start_match = &buf->input_data[*hash_analyze_index_ptr];
    if (prev_input_ptr <= start_match) {
        copy->length = 0;
        return;
    }
    uint8_t *input_ptr_copy = input_ptr;
    while (1) {
        if (start_match[max_matched_bytes - 1] == input_ptr_copy[max_matched_bytes - 1]
            && *start_match == *input_ptr_copy) {
            uint8_t *start_match_plus_one = start_match + 1;
            uint8_t *input_ptr_copy_plus_one = input_ptr_copy + 1;
            uint16_t matched_bytes = 2;
            do {
                start_match_plus_one++;
                input_ptr_copy_plus_one++;
                if (*start_match_plus_one != *input_ptr_copy_plus_one) {
                    break;
                }
                matched_bytes++;
            } while (matched_bytes < 516);
            input_ptr_copy = input_ptr;
            if (matched_bytes >= max_matched_bytes) {
                copy->offset = (uint16_t) (input_ptr - start_match_plus_one - 1 + matched_bytes);
                max_matched_bytes = matched_bytes;
                if (matched_bytes > 10) {
                    break;
                }
            }
        }
        hash_analyze_index_ptr++;
        hash_analyze_index++;
        start_match = &buf->input_data[*hash_analyze_index_ptr];
        if (prev_input_ptr <= start_match) {
            copy->length = (uint16_t) (max_matched_bytes < 2 ? 0 : max_matched_bytes);
            return;
        }
    }
    if (max_matched_bytes == 516) {
        copy->length = max_matched_bytes;
        copy->offset--;
        return;
    }
    if (&buf->input_data[buf->analyze_index[hash_analyze_index + 1]] >= prev_input_ptr) {
        copy->length = max_matched_bytes;
        return;
    }
    // Complex algorithm for finding longer match
    short long_offset = 0;
    int long_index = 1;
    buf->long_matcher[0] = -1;
    buf->long_matcher[1] = 0;
    do {
        if (input_ptr[long_index] != input_ptr[long_offset]) {
            long_offset = buf->long_matcher[long_offset];
            if (long_offset != -1) {
                continue;
            }
        }
        long_index++;
        long_offset++;
        buf->long_matcher[long_index] = long_offset;
    } while (long_index < max_matched_bytes);
    int matched_bytes = max_matched_bytes;
    uint8_t *match_ptr = &buf->input_data[max_matched_bytes] + buf->analyze_index[hash_analyze_index];
    while (1) {
        matched_bytes = buf->long_matcher[matched_bytes];
        if (matched_bytes == -1) {
            matched_bytes = 0;
        }
        hash_analyze_index_ptr = &buf->analyze_index[hash_analyze_index];
        uint8_t *better_match_ptr;
        do {
            hash_analyze_index_ptr++;
            hash_analyze_index++;
            better_match_ptr = &buf->input_data[*hash_analyze_index_ptr];
            if (better_match_ptr >= prev_input_ptr) {
                copy->length = max_matched_bytes;
                return;
            }
        } while (&better_match_ptr[matched_bytes] < match_ptr);
        if (input_ptr[max_matched_bytes - 2] != better_match_ptr[max_matched_bytes - 2]) {
            while (1) {
                hash_analyze_index++;
                better_match_ptr = &buf->input_data[buf->analyze_index[hash_analyze_index]];
                if (better_match_ptr >= prev_input_ptr) {
                    copy->length = max_matched_bytes;
                    return;
                }
                if (better_match_ptr[max_matched_bytes - 2] == input_ptr[max_matched_bytes - 2]
                    && *better_match_ptr == *input_ptr) {
                    matched_bytes = 2;
                    match_ptr = better_match_ptr + 2;
                    break;
                }
            }
        } else if (&better_match_ptr[matched_bytes] != match_ptr) {
            matched_bytes = 0;
            match_ptr = &buf->input_data[*hash_analyze_index_ptr];
        }
        while (input_ptr[matched_bytes] == *match_ptr) {
            matched_bytes++;
            if (matched_bytes >= 516) {
                break;
            }
            match_ptr++;
        }
        if (matched_bytes >= max_matched_bytes) {
            copy->offset = (uint16_t) (input_ptr - better_match_ptr - 1);
            if (matched_bytes > max_matched_bytes) {
                max_matched_bytes = matched_bytes;
                if (matched_bytes == 516) {
                    copy->length = 516;
                    return;
                }
                do {
                    if (input_ptr[long_index] != input_ptr[long_offset]) {
                        long_offset = buf->long_matcher[long_offset];
                        if (long_offset != -1) {
                            continue;
                        }
                    }
                    long_index++;
                    long_offset++;
                    buf->long_matcher[long_index] = long_offset;
                } while (long_index < matched_bytes);
            }
        }
    }
    // never reached
}

static int pk_implode_next_copy_is_better(
    struct pk_comp_buffer *buf, int offset, const struct pk_copy_length_offset *current_copy)
{
    struct pk_copy_length_offset next_copy;
    pk_implode_determine_copy(buf, offset + 1, &next_copy);
    if (current_copy->length >= next_copy.length) {
        return 0;
    }
    if (current_copy->length + 1 == next_copy.length && current_copy->offset <= 128) {
        return 0;
    }
    return 1;
}

static void pk_implode_analyze_input(struct pk_comp_buffer *buf, int input_start, int input_end)
{
    memset(buf->analyze_offset_table, 0, sizeof(buf->analyze_offset_table));
    for (int index = input_start; index < input_end; index++) {
        buf->analyze_offset_table[4 * buf->input_data[index] + 5 * buf->input_data[index + 1]]++;
    }

    int running_total = 0;
    for (int i = 0; i < 2304; i++) {
        running_total += buf->analyze_offset_table[i];
        buf->analyze_offset_table[i] = (uint16_t) running_total;
    }

    for (int index = input_end - 1; index >= input_start; index--) {
        int hash_value = 4 * buf->input_data[index] + 5 * buf->input_data[index + 1];
        unsigned short value = --buf->analyze_offset_table[hash_value];
        buf->analyze_index[value] = (uint16_t) index;
    }
}

static void pk_implode_data(struct pk_comp_buffer *buf)
{
    int eof = 0;
    int has_leftover_data = 0;

    buf->output_data[0] = 0; // no literal encoding
    buf->output_data[1] = (uint8_t) buf->window_size;
    buf->output_ptr = 2;

    int input_ptr = buf->dictionary_size + 516;
    pk_memset(&buf->output_data[2], 0, 2048);

    buf->current_output_bits_used = 0;

    while (!eof) {
        int bytes_read = pk_implode_fill_input_buffer(buf, 4096);
        if (bytes_read != 4096) {
            eof = 1;
            if (!bytes_read && has_leftover_data == 0) {
                break;
            }
        }
        int input_end = buf->dictionary_size + bytes_read; // keep 516 bytes leftover
        if (eof) {
            input_end += 516; // eat the 516 leftovers anyway
        }

        if (has_leftover_data == 0) {
            pk_implode_analyze_input(buf, input_ptr, input_end + 1);
            has_leftover_data++;
            if (buf->dictionary_size != 4096) {
                has_leftover_data++;
            }
        } else if (has_leftover_data == 1) {
            pk_implode_analyze_input(buf, input_ptr - buf->dictionary_size + 516, input_end + 1);
            has_leftover_data++;
        } else if (has_leftover_data == 2) {
            pk_implode_analyze_input(buf, input_ptr - buf->dictionary_size, input_end + 1);
        }

        while (input_ptr < input_end) {
            int write_literal = 0;
            int write_copy = 0;
            struct pk_copy_length_offset copy;
            pk_implode_determine_copy(buf, input_ptr, &copy);

            if (copy.length == 0) {
                write_literal = 1;
            } else if (copy.length == 2 && copy.offset >= 256) {
                write_literal = 1;
            } else if (eof && input_ptr + copy.length > input_end) {
                copy.length = input_end - input_ptr;
                if (input_end - input_ptr > 2 || (input_end - input_ptr == 2 && copy.offset < 256)) {
                    write_copy = 1;
                } else {
                    write_literal = 1;
                }
            } else if (copy.length >= 8 || input_ptr + 1 >= input_end) {
                write_copy = 1;
            } else if (pk_implode_next_copy_is_better(buf, input_ptr, &copy)) {
                write_literal = 1;
            } else {
                write_copy = 1;
            }

            if (write_copy) {
                pk_implode_write_copy_length_offset(buf, copy);
                input_ptr += copy.length;
            } else if (write_literal) {
                // Write literal
                pk_implode_write_bits(buf, buf->codeword_bits[buf->input_data[input_ptr]],
                                      buf->codeword_values[buf->input_data[input_ptr]]);
                input_ptr++;
            }
        }

        if (!eof) {
            input_ptr -= 4096;
            pk_memcpy(buf->input_data, &buf->input_data[4096], buf->dictionary_size + 516);
        }
    }

    // Write EOF
    pk_implode_write_bits(buf, buf->codeword_bits[PK_EOF], buf->codeword_values[PK_EOF]);
    if (buf->current_output_bits_used) {
        buf->output_ptr++;
    }
    buf->output_func(buf->output_data, buf->output_ptr, buf->token);
}

static int pk_implode(pk_input_func *input_func, pk_output_func *output_func,
                      struct pk_comp_buffer *buf, struct pk_token *token, int dictionary_size)
{
    buf->input_func = input_func;
    buf->output_func = output_func;
    buf->dictionary_size = dictionary_size;
    buf->token = token;
    if (dictionary_size == 1024) {
        buf->window_size = 4;
        buf->copy_offset_extra_mask = 0xf;
    } else if (dictionary_size == 2048) {
        buf->window_size = 5;
        buf->copy_offset_extra_mask = 0x1f;
    } else if (dictionary_size == 4096) {
        buf->window_size = 6;
        buf->copy_offset_extra_mask = 0x3f;
    } else {
        return PK_INVALID_WINDOWSIZE;
    }

    for (int i = 0; i < 256; i++) {
        buf->codeword_bits[i] = 9; // 8 + 1 for leading zero
        buf->codeword_values[i] = (uint16_t) (i << 1); // include leading zero to indicate literal byte
    }

    // prepare copy length values right after the literal bits
    int code_index = 256;
    for (int copy = 0; copy < 16; copy++) {
        int base_bits = pk_copy_length_base_bits[copy];
        int extra_bits = pk_copy_length_extra_bits[copy];
        int base_code = pk_copy_length_base_code[copy];
        int max = 1 << extra_bits;
        for (int i = 0; i < max; i++) {
            buf->codeword_bits[code_index] = (uint8_t) (1 + base_bits + extra_bits);
            buf->codeword_values[code_index] = (uint16_t) (1 | (base_code << 1) | (i << (base_bits + 1)));
            code_index++;
        }
    }

    pk_implode_data(buf);
    return PK_SUCCESS;
}


/* buffer input / output for pk_implode */
static int buffer_input(uint8_t *buffer, int length, struct pk_token *token)
{
    if (token->stop || token->input_ptr >= token->input_length) {
        return 0;
    }
    if (token->input_length - token->input_ptr < length) {
        length = token->input_length - token->input_ptr;
    }
    memcpy(buffer, &token->input_data[token->input_ptr], (size_t) length);
    token->input_ptr += length;
    return length;
}

static void buffer_output(uint8_t *buffer, int length, struct pk_token *token)
{
    if (token->stop) {
        return;
    }
    if (token->output_length - token->output_ptr < length) {
        token->stop = 1;
        return;
    }
    memcpy(&token->output_data[token->output_ptr], buffer, (size_t) length);
    token->output_ptr += length;
}

int pkware_implode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap, size_t *out_len, int dictionary_size)
{
    struct pk_token token;
    struct pk_comp_buffer *buf = calloc(1, sizeof(struct pk_comp_buffer));
    if (!buf) {
        return -1;
    }
    memset(&token, 0, sizeof(token));
    token.input_data = in;
    token.input_length = (int) in_len;
    token.output_data = out;
    token.output_length = (int) out_cap;
    int err = pk_implode(buffer_input, buffer_output, buf, &token, dictionary_size);
    free(buf);
    if (err || token.stop) {
        return -1;
    }
    *out_len = (size_t) token.output_ptr;
    return 0;
}
