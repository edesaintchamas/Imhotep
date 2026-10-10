/* Imhotep - named globals: the scenario and the city as a whole (the city record: layout/city.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_CITY_H
#define IMHOTEP_STATE_LAYOUT_G_CITY_H

/* the scenario */
#define G_DIFFICULTY 0x00E39080u   /* @bin 0x004DE820 i32: 0..4 (chunk 174, save.md §5) */
#define G_MISSION 0x00E38EA8u      /* @bin 0x004F46D0 i8: the campaign mission (0x7F: none) */
#define G_CUSTOM 0x00E38EBCu       /* @bin 0x004E8840 i32: != 0 a custom map */
#define G_PYRAMID_KIND 0x005E4EF4u /* @bin 0x004F30B0 i32: the scenario's pyramid kind 1..5 (5: the other tables) */
#define G_FLOODPLAIN 0x00D38360u   /* @bin 0x004BD330 the floodplain settings (`this`; docs/formats/state.md) */
#define G_REQUEST_EVENT 0x006057E4u /* @bin 0x004E0F90 i32: the request passed to 0x0044C1A0 */

/* sentiment (0x004E8840) */
#define G_SENTIMENT_BASE 0x005D199Cu /* @bin 0x004E8840 u8 by difficulty, 4 bytes each: an empty house's base */
#define G_SENTIMENT_TABLE 0x00EA1384u /* @bin 0x004E8840 i32[11] per CITY_TAX_RATE (loaded by 0x00541160) */

/* the gods */
#define G_GOD_MODE 0x007848E2u     /* @bin 0x004C47A0 i16 per god: 0 not in the city */

#endif
