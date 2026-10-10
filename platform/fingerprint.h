/* Imhotep - the reference installation check of build/imhotep.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PLATFORM_FINGERPRINT_H
#define IMHOTEP_PLATFORM_FINGERPRINT_H

#include <stddef.h>

enum { FINGERPRINT_MISSING = -1, FINGERPRINT_MISMATCH = -2 };

/* the reference Pharaoh.exe's SHA-256 (docs/source-fingerprint.json), lower-case hex */
extern const char FINGERPRINT_EXE_SHA256[];

/* <game>/Pharaoh.exe (the name in any case) against the reference's SHA-256: 0 when equal,
 * FINGERPRINT_MISSING (no such file, or unreadable) or FINGERPRINT_MISMATCH (another version);
 * msg says why */
int fingerprint_check(const char *game, char *msg, size_t size);

#endif
