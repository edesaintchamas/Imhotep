/* Imhotep - PKWARE DCL "explode" (binary literals), used for compressed chunks.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Portions translated from Julius (https://github.com/bvschaik/julius, src/core/zip.c),
 * AGPL-3.0, Bianca van Schaik and contributors. */
#ifndef IMHOTEP_IO_PKWARE_H
#define IMHOTEP_IO_PKWARE_H

#include <stddef.h>
#include <stdint.h>

/* @diff terrain_check (every compressed chunk of the map corpus)
 * Decompress `in` into exactly `out_len` bytes. Returns 0 on success, -1 on any error
 * (bad header, overrun, missing end-of-stream marker, size mismatch). */
int pkware_explode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_len);

/* Compress `in` (binary literals, dictionary of 1024, 2048 or 4096 bytes) into `out`. Returns 0
 * and the compressed size, or -1 when `out` is too small. */
int pkware_implode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap, size_t *out_len, int dictionary_size);

#endif
