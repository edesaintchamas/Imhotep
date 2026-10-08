/* Imhotep - the reference installation check of build/imhotep.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PLATFORM_FINGERPRINT_H
#define IMHOTEP_PLATFORM_FINGERPRINT_H

#include <stddef.h>

/* <game>/Pharaoh.exe against its SHA-256 in `json` (docs/source-fingerprint.json): 0 when equal,
 * -1 otherwise (msg says why) */
int fingerprint_check(const char *game, const char *json, char *msg, size_t size);

#endif
