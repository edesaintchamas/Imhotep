# Saved games (`.sav`) and campaign records

Reference: GOG Pharaoh + Cleopatra 2.1. Analysis dated 2026-10-04 from the loader
`0x004D41E0` and the 53 campaign records of `mission1.pak` (`campaign.md`). No player save was
available; the layout below is the game's own schema.

## 1. Container

Same self-describing chunk container as a `.map` (`map.md` §1–§3) without the `MAPS` magic:
four 1-byte chunks, file version (u32 at offset 4), schema count (u32 at offset 8), schema
(20-byte records), then the chunks. Written by the game at version **160** with **181** chunks;
older versions have 175 (campaign records: 147, 149, 150).

## 2. Loader `0x004D41E0` (**@bin**, decompiled)

1. Version global := 160; schema count := 0. Saves copies of four 24-byte-record tables
   (`0x005E45B0` ×10, `0x005E46A0` ×18, `0x005C1140` ×22, `0x005C1350` ×13).
2. Frees per-entry allocations of the table at `0x00776FB4` (stride 0x144).
3. Reads chunks 0…180 with the per-chunk reader `0x004D0E10` against the static schema
   `0x005E09A0` (the same schema-migrating reader as maps, `map.md` §4). For file versions below
   154 (0x9A) it stops before chunk 175 and calls `0x004D3C40` to set defaults.
4. Rebuilds the `0x00776FB4` entries (`0x004D4720`) or resets them.
5. Version upgrades, each a function or a few stores, applied when the file version is below
   117, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 132, 133, 134, 136, 139, 143, 144, 146,
   147, 148, 149, 150, 151, 152, 153, 155, 156, 157, 158, 159, 160 (0x75 … 0xA0), plus
   `0x004D0A40(1)` and `0x004D32F0` unconditionally. Ported in `core/io/save_load.c` (§4).
6. Scans the buildings (chunk 32, type at +0x10): if no Caesareum (0xE3) / Pharos Lighthouse
   (0xE4) / Alexandria's Library (0xE1) / Abu Simbel (0xE6) / Royal Burial Tomb (0xE5, 0xEA–0xEC)
   exists, calls `0x00420D10`, `0x00421010`, `0x004206A0`, `0x004209E0`, `0x004212F0`
   respectively — per-monument clean-ups for the Cleopatra monuments (not read).
7. Restores the first dword of every record of the four tables from step 1 (run-time pointers
   that the file must not overwrite).
8. `DAT_00eaaf5c := DAT_0078d7a4 > 0`.

Campaign records are then started with `0x004BD360` and the scenario start `0x004D52A0`
(`0x004BD360` is called with `ECX = 0x00D38360`, the floodplain settings of run-time record 21:
it copies fields 0–2 to 3–5, sets field 7 to 2 and clears field 6 — the current flood parameters
restart from the scenario's base ones; the daily flood tick `0x004BE770` reads fields at +0x0C,
+0x10, +0x14, +0x1C. Field meanings beyond that are not read.)
(`campaign.md` §1), which rebuilds every terrain image (`map.md` §6).

## 3. Schema

Records 0–146 are static data at `0x005E09A0`; records 147–180 are filled by an initialiser at
`0x004D1350` (not recognised as a function by Ghidra; read from its disassembly). The map schema's
run-time records 21–23 are filled the same way at `0x004D4A60`. The static and run-time records
together equal the schema stored in the version-160 campaign records (sizes and compression,
181/181; record 147's compression flag is stored outside the initialiser's range and was taken
from the records).

Columns: chunk, variable in Pharaoh.exe, element size × count, compressed, role (**@bin** where
read by code analysed so far), Akhenaten's name where the size lines up (*hint*), converter list.

| # | Variable | Size | Compr. | Role | Akhenaten (hint) |
|---|---|---|---|---|---|
| 0 | 0x00E38EA8 | 1 × 1 | no | mission number (i8; campaign table +0x0C), **@bin 0x00417DD0, 0x0041A850** | mission index (4 bytes) |
| 1 | 0x00E38EA9 | 1 × 1 | no | campaign id, **@bin 0x00417DD0, 0x0041B920** |  |
| 2 | 0x00E38EAA | 1 × 1 | no | previous campaign step (0xFF = none), **@bin 0x00418220** |  |
| 3 | 0x00E38EAB | 1 × 1 | no | current campaign step (table 0x006FDD08, 64-byte entries), **@bin 0x0041B920** |  |
| 4 | 0x00E945C0 | 4 × 1 | no | file version (0xA0 when written), **@bin 0x004D41E0, 0x004D4E20** | file_version |
| 5 | 0x00E945C4 | 4 × 1 | no | schema record count (181 saves, 24 maps), **@bin 0x004D0E10** | schema count |
| 6 | 0x00E92E50 | 20 × 300 | no | schema | schema |
| 7 | 0x00B7482C | 4 × 51984 | yes | image grid | image_grid |
| 8 | 0x00B67D1C | 1 × 51984 | yes | edge grid | edge_grid |
| 9 | 0x00B4E6FC | 2 × 51984 | yes | building grid | building_grid |
| 10 | 0x00AF598C | 4 × 51984 | yes | terrain grid | terrain_grid |
| 11 | 0x00AE8E7C | 1 × 51984 | yes | canal water grid (flood-fill from canal tiles, through Water Lifts), **@bin 0x00468590** | aqueduct_grid |
| 12 | 0x00B350DC | 2 × 51984 | yes | figure grid (first figure per tile), **@bin 0x00439740** | figure_grid |
| 13 | 0x00B285CC | 1 × 51984 | yes | bitfields grid | bitfields_grid |
| 14 | 0x00ADC36C | 1 × 51984 | yes | per-tile sprite overlay (image group at 0x00F56D34 + v − 1; bit 0x80), **@bin 0x00436B60, 0x0040F2C0** | sprite_grid |
| 15 | 0x00ACF85C | 1 × 51984 | no | random grid | random_grid |
| 16 | 0x00AC2D4C | 1 × 51984 | yes | desirability grid, i8 (`desirability.md`), **@bin 0x0046A0C0, 0x0046A140** | desirability_grid |
| 17 | 0x00AB623C | 1 × 51984 | yes | elevation level grid (editor ±1, max 5; terrain 0x200/0x400), **@bin 0x004793E0** | elevation_grid |
| 18 | 0x00A9CC1C | 2 × 51984 | yes | per-tile damage counter (i16; attackers; destroy past 10/200/400), **@bin 0x004690F0** | building_damage_grid |
| 19 | 0x009D1B1C | 1 × 51984 | yes | backup of the canal grid (build previews), **@bin 0x00440700, 0x004728A0** | aqueduct_backup_grid |
| 20 | 0x009C500C | 1 × 51984 | yes | backup of the sprite overlay grid, **@bin 0x00484F60** | sprite_backup_grid |
| 21 | 0x00BCD5F8 | 388 × 2000 | yes | figures (§5), **@bin 0x004B4800** | figures |
| 22 | 0x00D04E58 | 2 × 1000 | yes | path owners: 1000 × u16 figure id (figure +0x3A = path slot), **@bin 0x004B3250** | route_figures |
| 23 | 0x00C8AD38 | 1 × 500000 | yes | paths: 1000 × 500 direction codes (figure +0x3C step, +0x3E length), **@bin 0x004B33D0** | route_paths |
| 24 | 0x00BCB9C8 | 144 × 50 | yes | formations, 50 × 144 (§5), **@bin 0x004B6E10** | formations |
| 25 | 0x00BCB9BC | 4 × 1 | no | highest formation id in use, **@bin 0x004B6E10** |  |
| 26 | 0x00BCB9B8 | 4 × 1 | no | last player legion found by the recount, **@bin 0x004B77D0** |  |
| 27 | 0x00BCB9B4 | 4 × 1 | no | number of player legions, **@bin 0x004B77D0** |  |
| 28 | 0x00EA1870 | 18904 × 2 | yes | city data, one record per player (§5), **@bin 0x004DE820** |  |
| 29 | 0x00EA186C | 2 × 2 | no |  |  |
| 30 | 0x00EA182C | 32 × 2 | no | player names, 2 × 32 (slot 1 from 0x00E38E74), **@bin 0x00540A10** |  |
| 31 | 0x00EAAC28 | 4 × 1 | no | current player index, **@bin 0x00540A10** |  |
| 32 | 0x007999D8 | 264 × 4000 | yes | buildings (§5), **@bin 0x0046A400** |  |
| 33 | 0x00E94E04 | 4 × 1 | no | city view orientation 0/2/4/6, **@bin 0x00430B20** |  |
| 34 | 0x0078D7CC | 4 × 1 | no | tick within the day, 0–50, **@bin 0x00451C50** |  |
| 35 | 0x0078D7C4 | 4 × 1 | no | day within the month, 0–15, **@bin 0x00452370** |  |
| 36 | 0x0078D7C0 | 4 × 1 | no | month, 0–11, **@bin 0x00452370** |  |
| 37 | 0x0078D7BC | 4 × 1 | no | year (scenario info +0 at start), **@bin 0x00452370, 0x004DE820** |  |
| 38 | 0x0078D7C8 | 4 × 1 | no | total days elapsed, **@bin 0x00452370** |  |
| 39 | 0x0078D78C | 4 × 1 | no | highest building id ever, **@bin 0x0045AA30** |  |
| 40 | 0x0078D79C | 4 × 1 | no | locust plague timer (−1 armed; 2632 ticks), **@bin 0x004E6EB0, 0x00451C50** |  |
| 41 | 0x00D3B384 | 4 × 1 | no | RNG iv1, **@bin 0x004D7090** | RNG iv1 (@bin 0x004D7090) |
| 42 | 0x00D3B380 | 4 × 1 | no | RNG iv2, **@bin 0x004D7090** | RNG iv2 (@bin 0x004D7090) |
| 43 | 0x00BCA2BC | 4 × 1 | no | city camera x, **@bin 0x00430CD0** |  |
| 44 | 0x00BCA2C0 | 4 × 1 | no | city camera y, **@bin 0x00430CD0** |  |
| 45 | 0x00605A40 | 4 × 1 | no | population advisor chart selection (UI), **@bin 0x00525CB0** |  |
| 46 | 0x0078D7A0 | 4 × 1 | no | hailstorm timer (408 ticks), **@bin 0x004E6ED0** |  |
| 47 | 0x00775910 | 4 × 1 | no | empire map scroll x, **@bin 0x00442730** |  |
| 48 | 0x0077590C | 4 × 1 | no | empire map scroll y, **@bin 0x00442730** |  |
| 49 | 0x00784F14 | 4 × 1 | no | selected empire object (1-based, chunk 142), **@bin 0x0052DA40** |  |
| 50 | 0x0077DE00 | 106 × 61 | yes | empire cities, 61 × 106 (§5), **@bin 0x00443950** |  |
| 51 | 0x0078D6DC | 4 × 36 | no | industry building count per resource (36 × i32), **@bin 0x004677C0** |  |
| 52 | 0x0078D64C | 4 × 36 | no | same, buildings with u16 +0x40 > 0, **@bin 0x004677C0** |  |
| 53 | 0x005D2C48 | 8 × 36 | no | trade prices, 36 × {buy, sell}, **@bin 0x00445980, 0x00445E50** |  |
| 54 | 0x00D056E0 | 4 × 1 | no | figure name counter for text group 254 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 55 | 0x00D056DC | 4 × 1 | no | figure name counter for text group 255 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 56 | 0x00D056D8 | 4 × 1 | no | figure name counter for text group 256 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 57 | 0x00D056D4 | 4 × 1 | no | figure name counter for text group 257 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 58 | 0x00D056D0 | 4 × 1 | no | figure name counter for text group 258 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 59 | 0x00D056CC | 4 × 1 | no | figure name counter for text group 259 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 60 | 0x00D056C8 | 4 × 1 | no | figure name counter for text group 260 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 61 | 0x00D056C4 | 4 × 1 | no | figure name counter for text group 261 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 62 | 0x00D056C0 | 4 × 1 | no | figure name counter for text group 262 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 63 | 0x00D056BC | 4 × 1 | no | figure name counter for text group 263 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 64 | 0x00D056B8 | 4 × 1 | no | figure name counter for text group 264 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 65 | 0x00D056B4 | 4 × 1 | no | figure name counter for text group 265 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 66 | 0x00D056B0 | 4 × 1 | no | figure name counter for text group 266 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 67 | 0x00D056AC | 4 × 1 | no | figure name counter for text group 267 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 68 | 0x00D056A8 | 4 × 1 | no | figure name counter for text group 268 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 69 | 0x00D056A4 | 4 × 1 | no | figure name counter for text group 269 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 70 | 0x00D056A0 | 4 × 1 | no | figure name counter for text group 270 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 71 | 0x00D0569C | 4 × 1 | no | figure name counter for text group 271 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 72 | 0x00D05698 | 4 × 1 | no | figure name counter for text group 272 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 73 | 0x00D05694 | 4 × 1 | no | figure name counter for text group 273 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 74 | 0x00D05690 | 4 × 1 | no | figure name counter for text group 274 (seeded rand() & 0xF), **@bin 0x004B58D0, 0x004B5AF0** |  |
| 75 | 0x007848D8 | 1592 × 1 | no | scenario info |  |
| 76 | 0x0078D7B8 | 4 × 1 | no | deadline year (start + time limit, or +1,000,000), **@bin 0x004DE820** |  |
| 77 | 0x00E95228 | 48 × 1000 | yes | messages, 1000 × 48 (§5), **@bin 0x004E0CF0** |  | converter 0x005E0980
| 78 | 0x00E951F0 | 4 × 1 | no | next message sequence number, **@bin 0x004E0CF0** |  |
| 79 | 0x00E951EC | 4 × 1 | no | message count, **@bin 0x004E0CF0** |  |
| 80 | 0x00E951E8 | 4 × 1 | no | newest message index (−1 none), **@bin 0x004E0CF0** |  |
| 81 | 0x00E951C4 | 1 × 10 | no | population milestone flags (+1…+9: 500 … 25000), **@bin 0x00458540** |  |
| 82 | 0x00E95174 | 4 × 20 | no | per-category message counters/timers, 20 × i32, **@bin 0x00468B70** (partial) |  |
| 83 | 0x00E95124 | 4 × 20 | no | per-category message delays, 20 × i32, monthly −1, **@bin 0x004E1850** |  |
| 84 | 0x0078D804 | 4 × 1 | no | burning ruins found by the last scan, **@bin 0x00469730** |  |
| 85 | 0x0078D800 | 4 × 1 | no | burning-ruin list size (chunk 96), **@bin 0x00469A50** |  |
| 86 | 0x00D056FC | 4 × 1 | no | figure creation sequence number, **@bin 0x004B4800** |  |
| 87 | 0x00E38EB0 | 4 × 1 | no | campaign carry-over rating (table 0x005D1988 by difficulty; city data +0x42D8), **@bin 0x00417DD0** |  |
| 88 | 0x00E38EB4 | 4 × 1 | no | campaign carry-over savings (city data +0x4450), **@bin 0x00418640** |  |
| 89 | 0x00E38EB8 | 4 × 1 | no | missions completed in this campaign, **@bin 0x00418640** |  |
| 90 | 0x0077D160 | 32 × 101 | yes | no reference in the code (unused?) |  |
| 91 | 0x00E38EBC | 4 × 1 | no | custom-scenario flag (≠0 skips tutorial / mission checks; meaning *assumed*), **@bin 0x005405B0, 0x00404350** |  |
| 92 | 0x00702900 | 128 × 70 | no | ambient city-sound channels, 70 × 128 (uses timeGetTime: not simulation state), **@bin 0x0041D730** |  |
| 93 | 0x0078D790 | 4 × 1 | no | highest building id in use, **@bin 0x0046A400** |  |
| 94 | 0x0077AF00 | 88 × 100 | no | trader records, 100 × 88 (bought / sold per resource; figure +0x7F = index), **@bin 0x00444B30** |  |
| 95 | 0x0077AEFC | 4 × 1 | no | next trader record index, **@bin 0x00444B30** |  |
| 96 | 0x0078DD90 | 2 × 500 | yes | burning-ruin building ids, u16 × 500 (count 0x0078D7FC), **@bin 0x00469730** |  |
| 97 | 0x0078D9A8 | 2 × 500 | yes | scratch building-id list, u16 × 500, **@bin 0x00455430** |  |
| 98 | 0x0078E178 | 2 × 4000 | yes | scratch building-id list, u16 × 4000, **@bin 0x004583F0** |  |
| 99 | 0x0078D830 | 4 × 1 | no | set to 1 at start of mission index 0; no reader found, **@bin 0x005405B0** |  |
| 100 | 0x0078D828 | 4 × 1 | no | set at start of campaign missions 0–4, blocks screen 0x14 (empire map *assumed*), **@bin 0x005405B0, 0x00409FA0** |  |
| 101 | 0x00D0564C | 4 × 1 | no | figure-name counter, enemy class 8 (text group 300), **@bin 0x004B5AF0** |  |
| 102 | 0x00D05648 | 4 × 1 | no | figure-name counter, enemy class 9 (text group 301), **@bin 0x004B5AF0** |  |
| 103 | 0x00D05644 | 4 × 1 | no | figure-name counter, enemy class 10 (text group 302), **@bin 0x004B5AF0** |  |
| 104 | 0x00D05640 | 4 × 1 | no | figure-name counter, enemy class 11 (text group 303), **@bin 0x004B5AF0** |  |
| 105 | 0x0078D7A8 | 4 × 1 | no | Plague of Frogs countdown, **@bin 0x004E70B0** |  |
| 106 | 0x0078D7A4 | 4 × 1 | no | god-curse countdown from Pyramid progress (flag 0x00EAAF5C), **@bin 0x004F3AA0** |  |
| 107 | 0x0078D7AC | 4 × 1 | no | River of Blood duration, **@bin 0x004E7200** |  |
| 108 | 0x00D0563C | 4 × 1 | no | enemy formations, **@bin 0x004B7C80** |  |
| 109 | 0x00D05638 | 4 × 1 | no | enemy soldiers, **@bin 0x004B7C80** |  |
| 110 | 0x00D05634 | 4 × 1 | no | own formations, **@bin 0x004B7C80** |  |
| 111 | 0x00D05630 | 4 × 1 | no | own soldier strength, **@bin 0x004B7C80** |  |
| 112 | 0x00D0562C | 4 × 1 | no | counter 0–4 (wrap calls 0x004BA090), **@bin 0x004B8CF0** |  |
| 113 | 0x007900B8 | 196 × 200 | no | storage settings, 200 × 196 (building +0xC1 = index), **@bin 0x00456760** |  |
| 114 | 0x0077646C | 144 × 20 | yes | trade-route limits, i32[20][36], **@bin 0x00444720** |  |
| 115 | 0x0077592C | 144 × 20 | yes | traded this period, i32[20][36], **@bin 0x00445980** |  |
| 116 | 0x00BCB9AC | 4 × 1 | no | tower sentry request, **@bin 0x00462180, 0x004B7400** |  |
| 117 | 0x0078D7B4 | 4 × 1 | no | building creation sequence number, **@bin 0x0046A400** |  |
| 118 | 0x00EADFDC | 4 × 1 | no | no code reference |  |
| 119 | 0x00EADFD8 | 4 × 1 | no | enemy routing calls (debug 'barb'), **@bin 0x0051A420** |  |
| 120 | 0x00EADFD4 | 4 × 1 | no | routing calls (debug 'routings'), **@bin 0x005181C0** |  |
| 121 | 0x00EADFD0 | 4 × 1 | no | no code reference |  |
| 122 | 0x00BCA318 | 4 × 1 | no | entry-flag x, **@bin 0x0047D870** |  |
| 123 | 0x00BCA31C | 4 × 1 | no | entry-flag y, **@bin 0x0047D870** |  |
| 124 | 0x00BCA320 | 4 × 1 | no | exit-flag x, **@bin 0x0047D870** |  |
| 125 | 0x00BCA324 | 4 × 1 | no | exit-flag y, **@bin 0x0047D870** |  |
| 126 | 0x00775900 | 2 × 1 | no | invasion id counter (formation +0x88), **@bin 0x00446A30** |  |
| 127 | 0x0078D5C4 | 4 × 1 | no | buildings repaired against the grid (count), **@bin 0x0046B740** |  |
| 128 | 0x0078D5C0 | 4 × 1 | no | buildings not found on the grid (count), **@bin 0x0046B740** |  |
| 129 | 0x00E37078 | 65 × 1 | no | scenario / map file name, char[65], **@bin 0x005405B0** |  |
| 130 | 0x00E94DDC | 8 × 4 | no | map view bookmarks, 4 × {position, i32}, **@bin 0x00416B10** |  |
| 131 | 0x0078D7B0 | 4 × 1 | no |  |  |
| 132 | 0x00BC96C4 | 4 × 1 | no |  |  |
| 133 | 0x00BC96C0 | 4 × 1 | no |  |  |
| 134 | 0x00E38EF4 | 4 × 99 | no | last dword: victory video toggle; rest unreferenced, **@bin 0x004EA2C0** |  |
| 135 | 0x00A5D4CC | 1 × 51984 | no | soil fertility grid |  |
| 136 | 0x00785D70 | 18628 × 1 | no | scenario events |  | converter 0x005E0960
| 137 | 0x00EAAF68 | 224 × 50 | no | ferry-landing queues, 50 × 224 (building +0xCA), **@bin 0x0046EFD0** |  |
| 138 | 0x009784D8 | 44 × 50 | no | ferry-boat passenger lists, 50 × 44 (figure +0x11C), **@bin 0x0046F140** |  |
| 139 | 0x00D056E4 | 4 × 4 | no | no code reference |  |
| 140 | 0x009763B8 | 820 × 10 | no | shipwright repair queues, 10 × 820 (building +0xCB), **@bin 0x004D0A40, 0x0046A280** |  |
| 141 | 0x007843D8 | 32 × 40 | yes | empire directory, 40 × 32 (first 0x500 bytes of `Pharaoh2.emp`), **@bin 0x004444E0** |  |
| 142 | 0x0077F748 | 98 × 200 | yes | empire map objects, 200 × 98, **@bin 0x004426C0** |  |
| 143 | 0x00776FB0 | 324 × 50 | yes | empire map routes, 50 × 324 (heap pointer at +4 rebuilt on load), **@bin 0x004D4720** |  |
| 144 | 0x00A43EAC | 1 × 51984 | no | vegetation grid |  |
| 145 | 0x00978D88 | 20 × 1 | no | vegetation growth state, **@bin 0x004860D0** |  |
| 146 | 0x005C1140 | 24 × 22 | no | Sphinx (0xD2) work-step records, 22 × 24, **@bin 0x00420350** |  |
| 147 | 0x00D38360 | 36 × 1 | yes | floodplain settings (manager 0x00D05720 + 0x32C40) |  |
| 148 | 0x00978D9C | 4 × 51984 | yes | walker tile-visit counters (ten 3-bit counters per tile), **@bin 0x0047E250** |  |
| 149 | 0x005C1350 | 24 × 13 | no | Sun Temple work-step records, 13 × 24, **@bin 0x00420240** |  |
| 150 | 0x00D0568C | 4 × 1 | no | figure-name counter: Architect, **@bin 0x004B5AF0** |  |
| 151 | 0x00D05688 | 4 × 1 | no | figure-name counter: Constable, **@bin 0x004B5AF0** |  |
| 152 | 0x00D05684 | 4 × 1 | no | figure-name counter: Tax collector, **@bin 0x004B5AF0** |  |
| 153 | 0x00D05680 | 4 × 1 | no | figure-name counter: Magistrate, **@bin 0x004B5AF0** |  |
| 154 | 0x00D0567C | 4 × 1 | no | figure-name counter: Ferry boat, **@bin 0x004B5AF0** |  |
| 155 | 0x00D05678 | 4 × 1 | no | figure-name counter: Sentry, **@bin 0x004B5AF0** |  |
| 156 | 0x00D05674 | 4 × 1 | no | figure-name counter: enemy class 7, **@bin 0x004B5AF0** |  |
| 157 | 0x00D05670 | 4 × 1 | no | figure-name counter: enemy class 6, **@bin 0x004B5AF0** |  |
| 158 | 0x00D0566C | 4 × 1 | no | figure-name counter: enemy class 5, **@bin 0x004B5AF0** |  |
| 159 | 0x00D05668 | 4 × 1 | no | figure-name counter: enemy class 4, **@bin 0x004B5AF0** |  |
| 160 | 0x00D05664 | 4 × 1 | no | figure-name counter: enemy class 0, **@bin 0x004B5AF0** |  |
| 161 | 0x00D05660 | 4 × 1 | no | figure-name counter: enemy class 2, **@bin 0x004B5AF0** |  |
| 162 | 0x00D0565C | 4 × 1 | no | figure-name counter: enemy class 1, **@bin 0x004B5AF0** |  |
| 163 | 0x00D05658 | 4 × 1 | no | figure-name counter: enemy class 3, **@bin 0x004B5AF0** |  |
| 164 | 0x00D05654 | 4 × 1 | no | figure-name counter: ships, **@bin 0x004B5AF0** |  |
| 165 | 0x00D05650 | 4 × 1 | no | figure-name counter: Bedouin infantry, **@bin 0x004B5AF0** |  |
| 166 | 0x007027B8 | 1 × 26 | no | tutorial trigger flags, u8[26], **@bin 0x00417AE0** |  |
| 167 | 0x006FDC88 | 1 × 15 | no | 15 availability flags (advisors *assumed*), **@bin 0x004179F0** |  |
| 168 | 0x00A9010C | 1 × 51984 | yes | rubble building-type grid, **@bin 0x0047C480** |  |
| 169 | 0x007027B4 | 1 × 1 | no | game / campaign mode byte 0–3, **@bin 0x004D5C30, 0x00418640** |  |
| 170 | 0x00A509BC | 1 × 51984 | yes | moisture grid |  |
| 171 | 0x005E45B0 | 24 × 10 | no | small obelisk (0xCF) work-step records, 10 × 24, **@bin 0x004E5B90** |  |
| 172 | 0x005E46A0 | 24 × 18 | no | large obelisk (0xD0) work-step records, 18 × 24, **@bin 0x004E5B90** |  |
| 173 | 0x0078D798 | 4 × 1 | no | mummy-attack parameter (default 10), **@bin 0x004E7AC0** |  |
| 174 | 0x00E39080 | 4 × 1 | no | difficulty 0–4, **@bin 0x004DE820** |  |
| 175 | 0x006FDCE0 | 12 × 1 | no | army company carried over from the previous mission, **@bin 0x00418BF0, 0x00470990** | formations_info |
| 176 | 0x005C1F80 | 24 × 27 | no | Alexandria's Library work-step records, 27 × 24, **@bin 0x004206A0** |  |
| 177 | 0x005C1488 | 24 × 27 | no | Caesareum work-step records, 27 × 24, **@bin 0x00420D10** |  |
| 178 | 0x005C1E18 | 24 × 15 | no | Pharos Lighthouse work-step records, 15 × 24, **@bin 0x00421010** |  |
| 179 | 0x005C2208 | 24 × 56 | no | Abu Simbel work-step records, 56 × 24, **@bin 0x004209E0** |  |
| 180 | 0x005C1710 | 24 × 75 | no | Royal Burial Tombs work-step records, 75 × 24, **@bin 0x004212F0** |  |

## 3a. Reader (`core/io/save_file.c`, **@bin 0x004D0E10** per chunk)

- **Target layout:** the exe's schema (`core/io/save_schema.c`, generated by
  `tools/gen/gen_save_schema.py`: records 0–146 from `0x005E09A0`, 147–180 from `0x004D1350`). It
  equals the stored schema of all 17 version-160 records.
- **Chunks read:** 0…180, or 0…174 below version 154. In that case `0x004D3C40` (ported) sets
  chunk 175 to {u32 0, byte 0xFF, u32 −1}; its bytes +5…+7 and chunks 176–180 keep whatever the
  game held before the load (zero here; on a fresh start that would be the exe's initial data).
  `0x004D3C40` also resets neighbouring globals 0x006FDCBC–0x006FDCDF and 0x006FDCEC–0x006FDD07,
  which are not in the save. Files below version 115 are refused, as the game does.
- **Same size and count:** a direct copy. A chunk stored with another compression flag than the
  exe's would be misread by the game, so the reader refuses it.
- **Different size or count:** element-wise migration, reproducing the game's descending
  `memmove` order:
  - the data goes through a temporary buffer when the file data is larger than the variable, and
    is moved in place otherwise;
  - each element is zero-padded to the new size;
  - elements beyond the file's count are zeroed.

  Writes the game would make past the variable's end, and reads past the file data, are not
  reproduced; the read bytes are zeroed afterwards in every case.
- **Converters:** only chunks 77 (below version 136) and 136 (below version 130) have converter
  functions. They are game code and not ported: such files are refused (`SAVE_ERR_CONVERTER`).
- **Then** `save_file_load` (`core/io/save_load.c`) runs the loader's later steps (§4): the route
  rebuild, version upgrades, figure names, monument clean-ups and restores. The result is the
  state the game holds after loading.

Check: `build/save_check` (`tests/differential/README.md`). Dump: `build/save_dump`.

## 4. Version upgrades (§2 step 5) — **@bin**, ported in `core/io/save_load.c`

Applied in this order after all chunks are read, when the file version is below the threshold
(the version global is then set to 160). Chunk names are Akhenaten's (*hint*) only where the
total size matches exactly: **figures** = chunk 21 (388 × 2000), **formations** = 24 (144 × 50),
**city data** = 28 (18,904 × 2, one record per player), **buildings** = 32 (264 × 4000),
**scenario events** = 136 (18,628), **scenario info** = 75. Offsets are within one record.

| Version < | Function | Data | Effect |
|---|---|---|---|
| 117 | (inline) | scenario info +0x408, 100 × u16 | entries 0–49 := 1, 50–99 := 0 |
| 151 | (inline) | scenario info +0x2BC (0x00784B94) | u16 := 0 (word store at 0x004D4370) |
| 118 | 0x004D17E0 | chunk 142 (98-byte records) | if +0 == 1 and +1 ≠ 0: remap byte +0x18: 0,3,5 → 4; 1 → 0; 2 → 1; 4,6 → 6 |
| 120 | 0x004D1840 | formations | u16 +0x82 := 0xFFFF |
| 121 | 0x004D1860 | chunk 77 (48 × 1000) | u16 +0x10, +0x12, +0x14 := 0xFFFF |
| 122 | 0x004D1880 | city data | copy 7 dwords +0x2900 → +0x483C |
| 123 | 0x004D18B0 | city data | widen 30 i16 at +0x2796 to i32 at +0x4860, and 30 i16 at +0x27DE to i32 at +0x48F0 |
| 124 | 0x004D18F0 | chunk 50 (106-byte records) | record relayout (fields moved, new fields zeroed) |
| 124 | 0x004D1AD0 | city data | record relayout (5.3 KB of field moves; blocks at +0x2796… +0x430C) |
| 124 | 0x004D2FA0 | buildings | record relayout; houses (10–29), Pyramid (0xB7) and Sphinx (0xD2) keep a different block |
| 124 | 0x004D3200 | chunk 113 (196-byte records) | record relayout |
| 125 | 0x004BD330 | floodplain settings (chunk 147; `MOV ECX,0xD38360` at 0x004D43EB) | 8 dwords := {180, 70, 100, 180, 70, 100, 0, 100} |
| 132 | 0x004BD5D0 | floodplain settings (`MOV ECX,0xD38360` at 0x004D4403) | field 7 := field 2 |
| (all) | 0x004D0A40(1) | chunk 140 (820-byte records, 10) | rebuilds the live shipwright queues (0x009783A4) from each record's header (`0x004F67E0`: capacity, head, tail, count); the item arrays are not copied, chunk 140 itself is unchanged |
| 127 | 0x004D32D0 | figures (from figure 1) | byte +0x02 := 0; u16 +0x132 := 0xFFFF |
| 128 | (inline) | chunk 148 (u32 grid 0x00978D9C) | := 0 |
| 129 | 0x004D32A0 | chunk 77 | u16 +0x0A := 0x004E1600(value), until the first 0 |
| (all) | 0x004D32F0 | figures, chunks 101–104 and 150–165 | per figure type (byte +0x0A) set i32 +0x98 (0xFE, 0xFF, 0x100–0x115, or −2 with u16 +0x70 := 0xFFFF) and reset per-type counters past their limits |
| 133 | (inline) | chunk 169 (0x007027B4) | byte := 0 |
| 134 | (inline) | city data of the current player | one field := chunk 174 (0x00E39080) |
| 136 | 0x004D3C20 | — | if 0x004E59C0() ≠ 0, call 0x004E5B90 on it |
| 139 | 0x004D3CC0 | figures | Warships (type 0x4E) alive and +0x38 ≠ 4: copy position +0x14, +0x16 to +0x14C, +0x14E |
| 143 | 0x004DE140 | chunk 130 (8-byte records) | both dwords := −1 |
| 144 | 0x004D3D80 | scenario events (124-byte events) | 4 bytes at +0x6E… := 6 |
| 146 | 0x004D3DA0 | buildings, city data | per building in use, by type: clear one byte (0xB7, 0xCF, 0xD0: +0x80; 0xD5, 0xE1, 0xE3, 0xE4: +0x68; 0xDE: +0x74; tombs 0xE5/0xEA–0xEC: +0x9D; 0xE6: +0x60), then skip the monument's following slots (0xB7: table[+0x54] − 1, table 0x0056F7D8 or 0x0056F7EC by global 0x005E4EF4; 0xD5: 99; 0xDE: 115; 0xE1: 24; 0xE3: 8; tombs: 166/157/436/512; 0xE6: 15). Sphinx 0xD2: current player's city data +0x2988 := 0 |
| 146 | 0x004D3F40 | formations 1–49 | byte +0x68 := 0 (byte store) |
| 147 | (inline) | floodplain settings | +0x20 := +0x14 |
| 148 | 0x004D3D00 | formations, figures | formation +0x84 cleared unless it points to a Transport (0x4D), Enemy transport (0x5C) or Egyptian transport (0x65); then set from Transports |
| 149 | 0x004D3F60 | buildings | per building type, move a byte between +0xC1, +0xCA, +0xCB |
| 150 | 0x004D4040 | city data of the current player | +0x298C … +0x29B9 := 0 |
| 152 | 0x004D40C0 | scenario events, chunk 53 | events start at chunk +4, 124 bytes, count = i16 chunk +0. For events whose kind (byte +2) is flagged in 0x005D9C78 (`0x004493C0`): the i16 at +4, +6, +8, +0xA get +5 when ≥ 31, then the non-negative ones among +6, +8, +0xA are packed first (asm 0x004D411C–0x004D4148). Then 5 price pairs 0x005D2E60 → 0x005D2D40 (chunk 53 entries 31–35) |
| 153 | 0x004D19E0 | chunk 142 | three u32 bit masks (+0x40/+0x44/+0x48) become a 32-byte array (3/2/1/0 per bit), trailing bytes := 2 |
| 155 | 0x004D4080, 0x004D40A0, (inline) | buildings, city data, scenario info | buildings u16 +0x86 := 0; city data +0x44C8/+0x44CC := 0; scenario info +0x462 (110 bytes) := 0 |
| 156 | 0x004D1A70 | city data | widen 20 i16 at +0x291C to i32 at +0x3C64 and 20 at +0x2944 to i32 at +0x3D04; then zero the upper 20 entries of both 40-entry arrays (+0x3CB4…+0x3D03, +0x3D54…+0x3DA3) |
| 157 | (inline) | 0x0078D7A4, 0x0078D7A8, 0x0078D7B0 ('blood river' region), scenario info +0x3E4 | := 0, 0, 0; byte := 1 |
| 158 | (inline) | 0x0078D7AC | := 0 |
| 159 | (inline) | scenario info +0x3E5 | := 0 |
| 160 | (inline), 0x004D4180 | chunk 173 (0x0078D798), 0x0078D79C, 0x0078D7A0, buildings | := 10, 0, 0; statues (0x29–0x2B): orientation byte (+0xAA) rotated by one quarter, flag 0x40 → 0x10. The flag is a local that is **never reset**: after the first statue with 0x40, every later statue gets 0x10 too (reproduced) |
| 154 (read) | 0x004D3C40 | chunk 175 (0x006FDCE0) and neighbours | defaults for the chunks a pre-154 file lacks |

**Version-124 relayouts** (@bin, asm checked). Each zeroes a buffer, copies the old record into
it, moves blocks, and writes it back over the same slot ("kept" = the copy's stale bytes stay):

- **chunk 50** (`0x004D18F0`): +0x00–0x23 same; new +0x24–0x29 := 0; old +0x24–0x41 → +0x2A–0x47;
  +0x48–0x4D := 0; old +0x42–0x5D → +0x4E–0x69; old +0x5E–0x69 dropped.
- **chunk 113** (`0x004D3200`): +0x00–0x27 same; +0x28–0x2D := 0; old +0x28–0x69 → +0x2E–0x6F;
  +0x70–0xC3 := 0.
- **buildings** (`0x004D2FA0`): houses (10–29), 0xB7, 0xD2 keep +0x00–0xB7. Other types: +0x00–0x91
  same, +0x92–0x9D := 0, old +0x92–0x9E → +0x9E–0xAA, old +0xA0–0xAB → +0xAC–0xB7 (old +0x9F
  dropped; +0xAB kept). All: old +0xAC–0xBD → +0xB8–0xC9, +0xCA–0xCB kept, old +0xC0–0xF3 →
  +0xCC–0xFF, +0x100–0x107 := 0. So houses/0xB7/0xD2 end up with old +0xAC–0xB7 twice.
- **city data** (`0x004D1AD0`, both records; derived by stepping through its 854 instructions):

```
+0x0000..+0x27D1  same
+0x27D2..+0x27DD  0       | +0x27DE..+0x2819 <- old +0x27D2..+0x280D
+0x281A..+0x2825  0       | +0x2826..+0x2861 <- old +0x280E..+0x2849
+0x2862..+0x286D  0       | +0x286E..+0x28A9 <- old +0x284A..+0x2885
+0x28AA..+0x28B5  0       | +0x28B6..+0x28F1 <- old +0x2886..+0x28C1
+0x28F2..+0x28FD  0       | +0x28FE..+0x28FF kept
+0x2900..+0x2943 <- old +0x28C4..+0x2907
+0x2944..+0x296B <- old +0x2930..+0x2957
+0x296C..+0x2977 <- old +0x2980..+0x298B
+0x2978..+0x2A87  kept
+0x2A88..+0x2AFF <- old +0x2A9C..+0x2B13  | +0x2B00..+0x2B17 0
+0x2B18..+0x3C63 <- old +0x2B14..+0x3C5F  | +0x3C64..+0x3DA3 kept
+0x3DA4..+0x3F83 <- old +0x3C60..+0x3E3F
+0x3F84..+0x42DF <- old +0x3F80..+0x42DB  | +0x42E0..+0x42E3 0
+0x42E4..+0x449B <- old +0x42E0..+0x4497  | +0x449C..+0x449F 0
+0x44A0..+0x44C7 <- old +0x449C..+0x44C3  | +0x44C8..+0x44CF kept
+0x44D0..+0x465F <- old +0x44CC..+0x465B  | +0x4660..+0x4666 kept
+0x4667 <- old +0x4663                    | +0x4668..+0x466B kept
+0x466C..+0x472F <- old +0x4668..+0x472B  | +0x4730..+0x47B7 kept
+0x47B8..+0x4813 <- old +0x472C..+0x4787
+0x4814..+0x48D7 <- old +0x4810..+0x48D3  | +0x48D8..+0x48EF 0
+0x48F0..+0x4967 <- old +0x48D4..+0x494B  | +0x4968..+0x497F 0
+0x4980..+0x4989 <- old +0x494C..+0x4955  | +0x498A..+0x49D7 0
```

  Five 60-byte blocks from +0x27D2 each gain 12 zero bytes (15-entry arrays growing to 18 is
  *assumed*). The < 123 step runs before this one (call order checked in `0x004D41E0`), so its
  i32 arrays at +0x4860/+0x48F0 are then moved by this relayout. That is the game's behaviour.

**Monument clean-ups** (§2 step 6, @bin): `0x00420D10` (Caesareum, chunk 177, 27 records),
`0x00421010` (Pharos, 178, 15), `0x004206A0` (Library, 176, 27), `0x004209E0` (Abu Simbel, 179,
56), `0x004212F0` (Royal Burial Tomb, 180, 75). Each walks a table of 16-byte entries
`{record pointer, value, …}` (0x005AD218, 0x005B0060, 0x0059EDF0, 0x005A8148, 0x005BE510). For
each 24-byte record it sets u16 +0x12 := 0, calls `0x0041FCA0(record, 3)`, then sets i32 +0x0C :=
value. `0x0041FCA0` sets u16 +0x10 := 3 and picks one of up to 4 options of the record's
descriptor (0x005BE9B8 + id·0x28) with **CRT `rand() % count`**, then writes +0x04, +0x15 and
+0x08. A load without these monuments therefore advances the CRT `rand` state (§5, figures). The
same five functions run when such a monument is placed (`0x00470990`). The records' meaning
(animation or work stages) is *assumed*.

**Port and checks.** `core/io/save_load.c` runs every step in the order of `0x004D41E0`, writing
through the exe address of each variable. The four relayouts are tables generated from their
disassembly (`tools/gen/gen_relayouts.py`, a symbolic x86 interpreter for their MOV / REP MOVSD
code). The data they read from the exe is generated by `tools/gen/gen_save_tables.py`: clean-up
programs, option descriptors, restores, event flags, prices and pyramid tables.

The campaign records only exercise versions 147–160 (steps ≥ 148). The other steps are ported
from the binary without test data.

The load needs three inputs that are not in the file:
- the CRT `rand` state;
- the previous scenario's pyramid kind 0x005E4EF4 (versions below 146 only);
- the startup image sizes (route endpoints).

**Route rebuild** (`0x004D4720`, every load, before the upgrades): for each route of chunk 143
with a null point buffer (saved games store null), the estimate, point count, buffer capacity and
endpoint objects are recomputed. The heap pointer is not reproduced.

The point count uses `float(sqrt(dx² + dy²))` (`FSQRT`, then `FSTP float`), compared with
k · step. For segments shorter than 2,896 no float rounds sqrt(m² − 1) up to m, so the integer
form `isqrt / step + 1` is exact; the longest saved segment is 625. Endpoints are the last empire
object of kind 1 whose image rectangle (startup image sizes) holds the first / last point, and
the object scan runs only for routes with at least one point.

Check (`build/save_check`, version-160 records):
- recomputed route lengths: 850 / 850 equal to the stored ones;
- recomputed endpoints: 850 / 850;
- figure name groups (`0x004D32F0`): 308 / 308.

The other 89 figures still hold the constructor's 0 where a load writes −2. The records were
saved from a fresh scenario, never loaded.

None of these steps touches the terrain inputs of the scenario start (`map.md` §6). Two set
defaults the flood simulation reads: floodplain settings (< 125, < 132, < 147) and the
'blood river' region (< 157).

## 5. Buildings, figures, city data

Evidence: the code paths read so far (**@bin**), and the 35 campaign records of version ≥ 149
(`campaign.md`): 430 buildings and 776 figures in use. Field names are ours; offsets are within one
record. Akhenaten's field lists (`city/city_buildings.cpp` `iob_buildings`, `figure/figure.cpp`
`figure::bind`, `city/city.cpp` `iob_city_data`) are *hints* for the remaining bytes; they are
wrong for buildings at +0x0A–0x0F (no orientation/counters there: the data holds the grid offset).

### Buildings — chunk 32, 0x007999D8, 4000 × 264 bytes

Record 0 is unused (ids start at 1; the building grid, chunk 9, holds ids). Constructor
**@bin 0x0046A400** (owner, type, x, y, size hint, id or 0): takes the first free record from id
1 (state 0 and not held by the undo list 0x00BA746C, `0x004851B0`), fails with message 0x41 when
all 4,000 are used, and raises the highest-id global 0x0078D790.

| Off | Type | Field | Status | Evidence |
|---|---|---|---|---|
| +0x00 | u8 | state (1 = in use; 3 when created) | **@bin 0x004D41E0, 0x0046A400** | loader scans `+0 == 1`; the constructor writes 3 |
| +0x01 | u8 | owner (player index) | **@bin 0x0046A400** | constructor argument 1 |
| +0x02 | u8 | copied from city data[owner] +0x05 | **@bin 0x0046A400** | meaning unknown |
| +0x03 | u8 | size (footprint n×n) | confirmed, **@bin 0x0046A400** | = building property table 0x005D06DC[type] +0 (20 bytes per type); building grid covers ≥ size² tiles, 430/430 |
| +0x05 | u8 | house size class: 1 for types 10–19, 2 for 20–23, 3 for 24–27, 4 for 28–29, else 0 | **@bin 0x0046A400** | |
| +0x06 | u16 | x (map-relative) | confirmed | +0x0C = start + y·228 + x, 430/430 |
| +0x08 | u16 | y | confirmed | same |
| +0x0C | u32 | grid offset of the building's tile | confirmed | building grid holds the id there for every building with size > 0 (428/428; the 2 others are size-0 types 211, 212) |
| +0x10 | u16 | type | **@bin 0x004D41E0, 0x004D2FA0, 0x0046A400** | loader tests types 0xE1–0xEC; version-124 relayout branches on types 10–29, 0xB7, 0xD2 |
| +0x12 | u16 | subtype: house level (type − 10) for houses, else a per-type value (e.g. 0x6E → 9, 0xCC → 5) or 0 | **@bin 0x0046A400** | meaning of the non-house values unknown |
| +0x16 | u16 | creation sequence number (global 0x0078D7B4, incremented) | **@bin 0x0046A400** | |
| +0x20 | u16 | := 0 at creation | **@bin 0x0046A400** | |
| +0x34 | u8 | random grid (chunk 15) & 6 at the building's tile | **@bin 0x0046A400** | |
| +0x43 | u8 | per-type category (0x65 → 0x10, 0x66 → 1, …, 0 for most types) | **@bin 0x0046A400** | Julius `labor_category` is a *hint* |
| +0x4C | u8 | building property table 0x005D06DC[type] +4 | **@bin 0x0046A400** | |
| +0x4D | u8 | random grid & 0x7F at the building's tile | **@bin 0x0046A400** | |
| +0x56 | u16 | 3200 for types 0x47 (Granary) and 0xBF at creation | **@bin 0x0046A400** | |
| +0xAA | u8 | orientation (statues 0x29–0x2B: rotation + 16 × variant; types 0x8C–0x95, 0xCA: global 0x00BC9714; Water Lift 0x5A: bit 0 tested by canals) | **@bin 0x004D4180, 0x0046A400, 0x0047E870** | version-160 upgrade rotates the statues (the earlier "+0x9A" was relative to +0x10) |
| +0xC0 | u8 | 1 if a tile of the building's surrounding ring has water (flag 4) | **@bin 0x0046A400 → 0x00481000** | ring offsets table 0x005CC9E0, 0x60 bytes per size |
| +0xC2 | u8 | := 50 at creation | **@bin 0x0046A400** | meaning unknown |
| +0xC4 | i32 | Ferry Landing (0x88) partner building id | **@bin 0x0046A400** | pairs with the previous landing (global 0x00BC9800) |
| +0xCE | u16 | per-type value (0xCC → 13, 0xE8 → 24, 0xE7 → 5, 0x5F → 14, 0xE2 → 13), else 0 | **@bin 0x0046A400** | |
| +0xE4 | i32 | houses: house property table 0x0070CE0C[level] (88 bytes per level) +0 | **@bin 0x0046A400** | then 0x004E7F00(id, 1) |
| +0xE8, +0xFC | i32 | houses: := 0 | **@bin 0x0046A400** | |

**Further building fields.** The original's "Building info (F4)" debug overlay `0x00535080` prints
fields next to labels stored in the exe (`pop`, `goods`, `acc`, `rel`, `immigrant`, `agent`,
`SUB agent`, `rome`, `roadnet`, `spare room`, `damage`, `water sup`, `well sup`, `water dep`,
`no contact with rome`, `employees`, `tax cvr`, `max pop`, `shows`, `raw materials`, `food`).
Main readers:
- tick dispatcher `0x00451C50`;
- figure spawn per type `0x00462180`;
- fire and collapse `0x00468B70`;
- house evolution `0x00454190`;
- service effects `0x004BAA10`;
- problem flags `0x00432920`.

Per-type tables: 0x00705EB8 + type·0x28 (+0 labourers, +4 fire-risk step, +8 damage-risk step);
house levels 0x0070CDCC + level·0x58.

| Off | Type | Field | Evidence |
|---|---|---|---|
| +0x04 | u8 | house merged (max population ×4) | `0x00458440` |
| +0x14 | u8 | road network id | 'roadnet' |
| +0x18 | i16 | service / houses-covered counter (cap 300; < 51 spawns a labour seeker) | `0x004BAA10`, `0x00462180` |
| +0x1A | i16 | 'rel' (< 1 → problem 0x1000) | `0x00535080`, `0x00432920` |
| +0x1C | i16 | house population | 'pop', `0x00458440` |
| +0x1E | i16 | room left | 'spare room' |
| +0x20 | i16 | distance from the entry (> 0 reachable) | 'rome', `0x00458950` |
| +0x22 | i16 | highest population reached | 'max pop' |
| +0x24 | i16 | 'no contact with rome' | label only |
| +0x26, +0x28 | i16 | road access x, y | `0x00462180` |
| +0x2A | i16 | main walker figure id | 'agent' |
| +0x2C | i16 | second walker id | 'SUB agent' |
| +0x2E | i16 | immigrant figure id | 'immigrant', `0x00458D00` |
| +0x30 | i16 | tower walker id | `0x00462180` |
| +0x32 | u8 | spawn delay counter | `0x00462180` |
| +0x34 | u8 | roam direction (random & 6) | `0x0046A400` |
| +0x35 | u8 | water supply | 'water sup', `0x00454190` |
| +0x36 | u8 | 'water dep' | label only |
| +0x38 | i16 | previous part id (0 = main part) | `0x00475B50` |
| +0x3A | i16 | next part id (storage-yard spaces) | `0x0045D510` |
| +0x3C | i16 | stored amount (raw material, yard space, …) | 'goods' / 'raw materials', `0x00455C30` |
| +0x3F | u8 | well access | 'well sup' |
| +0x40 | i16 | workers | 'employees', `0x00462180` |
| +0x43 | u8 | output resource | `0x00462180` |
| +0x46 | i16 | collapse risk: += table +8; ≥ 1000 → state 4; architect lowers it | 'damage', `0x00468B70`, `0x00482C10` |
| +0x48 | i16 | fire risk: += table +4 × LFSR value (1…5); > 999 catches fire | `0x00468B70`, `0x00483000` |
| +0x4A | i16 | burning duration (also the red tint, graphics.md) | `0x00468B20`, `0x00439980` |
| +0x4E | u8 | tax coverage (tax collector sets 50) | 'tax cvr', `0x00484A90` |
| +0x54 | i16 | industry progress (done at 200, or 400 when +0x12 ≠ 0) | `0x00455C30` |
| +0x54 | i16[12] | houses: inventory (0–7 foods, 8 beer, 9 pottery, 10 linen, 11 luxury) | `0x00454190`, `0x00483910` |
| +0x56 | i16[] | bazaar inventory (shifted by one); granary per resource, [0] = free space | `0x00462180`, 'food' |
| +0x58–0x68 | | monuments: construction phase and done flags | `0x004E65A0`, `0x0046F200` |
| +0x6C–0x78, +0x86 | u8 | house access: juggler, musician, physician, dancer, magistrate, senet, teacher, librarian, (+0x74 ?), dentist, herbalist, water carrier, embalmer; +0x86 zookeeper | `0x004833C0` … `0x00484780`, `0x00483800` |
| +0x79–0x7D | u8 ×5 | priest access per god | `0x00483FB0` |
| +0x80–0x83 | i8 | entertainment, education, health, religion levels | `0x00454190` |
| +0x85 | u8 | evolve-status code | `0x00454190` |
| +0x87 | u8 | no room (1 → problem 0x20, 2 → 0x2000) | `0x00458440`, `0x00432920` |
| +0x9E–0xA4 | i16 | venues: performer figure ids; bazaar: demand per good | `0x00462180`, `0x00483910` |
| +0xA6–0xA9 | u8 | 'shows' flags; workers out for wood cutters / guilds | `0x00535080`, `0x00462180` |
| +0xAE–0xB2 | i16 ×3 | docker figure ids | `0x004B4980` |
| +0xB4, +0xB6 | i16 | second trader / farm labourer id; second cart id | `0x00462180`, `0x00488400` |
| +0xB8 | i32 | value from `0x0045A140`, partly lost on destruction | `0x004684E0`, `0x00469140` |
| +0xBE | i8 | desirability (house thresholds) | `0x00454190` |
| +0xC1 | u8 | storage settings index (0x007900C2 + id·0xC4) | `0x00462180` |
| +0xC3 | u8 | problem overlay code (1 problem, 2 no coverage) | `0x00432920` |
| +0xCC | i16 | second raw-material stock | `0x00455C30` |
| +0xCE | i16 | second input code | `0x004B4980` |
| +0xE4 | i32 | house crime risk (> 800 → problem 0x40) | `0x00483260`, `0x00432920` |
| +0xE8 | i32 | house malaria risk 0–1000 (from water distance) | `0x004E7F00`, `0x00482DD0` |
| +0xF4, +0xF8 | i32 | distances to terrain 4 / 0x40000 (**x87 sqrt + `__ftol`**) | `0x004E7F00` |
| +0xFC | i32 | house disease risk 0–1000 (> 800 → problem 4) | `0x004E7F00`, `0x00482EF0` |
| +0x100 | u32 | problem flags: 1 collapse, 2 fire, 8 empty, 0x80 cannot evolve, 0x100 no raw material, 0x200 no workers, 0x400 too few, 0x1000 no labour | `0x00432920` |
| +0x104, +0x106 | i16 | incoming carts / amount (granary); input counts (workshops) | `0x004BC7A0` |

All are **@bin** at the cited code. Names that rest on a debug label or one use are tentative.
Still unknown: +0x02, +0x0E–0x0F, +0x33, +0x37, +0x3E, +0x42, +0x44, +0x45, +0x4C, +0x4F–0x53,
+0x74, +0x7E–0x7F, +0x84, +0x88–0x9D (monument code only), +0xAC–0xAD, +0xBA–0xBD, +0xBF, +0xC2,
+0xC4–0xCB beyond the ferry partner, +0xD0–0xE3, +0xEC, +0xF0.

Type names: text group 41 indexed by type (**@bin 0x004FAB20**, `text.md`). Types seen in the
campaign records: 10–14 houses (Crude Hut … Rough Cottage; houses are 10–29, hence the special
block in the version-124 relayout), 41–43 statues, 46 Apothecary, 55 Police Station, 57 Forts,
60–66 temples and temple complexes, 70 Bazaar, 71 Granary, 72/73 Storage Yard(s), 74 Shipwright,
75 Dock, 76 Fishing Wharf, 78/79 mansions, 81 Architect's Post, 94 Academy, 95 Recruiter,
104/105 farms, 108 Wood Cutter, 112 Weaponsmith, 115 Hunting Lodge, 136 Ferry Landing,
138 Road Block, 141–143 shrines, 189 City Palace, 202 Gatehouse, 211/212 Temple Complex upgrade
(size 0). Other types named in code: 0xB7 Pyramid, 0xD2 Sphinx, 0xE1 Alexandria's Library,
0xE3 Caesareum, 0xE4 Pharos Lighthouse, 0xE5/0xEA/0xEB/0xEC Small/Medium/Large/Grand Royal
Burial Tomb, 0xE6 Abu Simbel.

### Figures — chunk 21, 0x00BCD5F8, 2000 × 388 bytes

Record 0 unused. The figure grid (chunk 12, 0x00B350DC, u16 per tile) holds the first figure on
each tile; the others follow through +0x08. Constructor **@bin 0x004B4800** (p1, type, x, y, p5):
first record from id 1 with state 0 (and not reserved, `0x004B47A0`), zeroed (388 bytes), then the
fields below; links the figure into the grid chain (`0x004B52F0`), raises the highest-id global
0x00BCD5F0.

**Determinism note (@bin):** the constructor draws `rand() & 3` (MSVC CRT `rand`, `0x00562348`,
state 0x00604950: `s = s·214013 + 2531011`, result `(s >> 16) & 0x7FFF`) for +0x7C/+0x7E. `rand` has
26 callers (figures, `0x004B92E0`, `0x004BBC10`, …, the tick `0x004D6B60` for sound) and is
**reseeded with `timeGetTime()`** by the figure actions `0x004EAEC0` and `0x004EB000`; its state is
not in the save schema. Results that depend on it are not reproducible from a save, even by the
original game.

| Off | Type | Field | Status | Evidence |
|---|---|---|---|---|
| +0x08 | u16 | next figure on the same tile | **@bin 0x00439740** | city draw loop walks the chain from the figure grid. Every chain entry is a live figure with that tile's +0x20 (60 saves). Live figures can be off the grid: 16 in the downloaded saves, walkers and caravans in action states 6 and 8 |
| +0x0A | u8 | type (names: text group 64) | **@bin 0x004D32F0, 0x004D3CC0, 0x004FAB20, 0x004B4800** | upgrades switch on it; the info panel prints group 64 at this index |
| +0x0C, +0x0D | u8 | := 0, := 1 at creation | **@bin 0x004B4800** | |
| +0x0E | u8 | state (1 = alive) | **@bin 0x004D3CC0, 0x004D3D00, 0x004B4800** | tested `== 1` before using a figure |
| +0x0F | u8 | constructor argument 1 | **@bin 0x004B4800** | meaning unknown |
| +0x11 | u8 | constructor argument 5 | **@bin 0x004B4800** | meaning unknown |
| +0x14 | u16 | x (map-relative) | confirmed | figure is in the grid chain of start + y·228 + x, 776/776 |
| +0x16 | u16 | y | confirmed | same |
| +0x18, +0x1A | u16 | x, y (copy) | **@bin 0x004B4800** | |
| +0x20 | u32 | grid offset | confirmed | = start + y·228 + x, 776/776 |
| +0x24, +0x26 | u16 | x, y (copy; overwritten with a target position by 0x004EAEC0) | **@bin 0x004B4800** | |
| +0x2C, +0x2E | u16 | x, y (copy) | **@bin 0x004B4800** | |
| +0x38 | u8 | action state (0xF at creation; 0x10, 0x15 … set by actions) | **@bin 0x004B4800, 0x004EB000** | |
| +0x4A, +0x4C | u16 | x · 15, y · 15 (position in 1/15 tile) | **@bin 0x004B4800** | |
| +0x70 | u16 | per-type sequence number (global 0x00D056E0) for some types | **@bin 0x004B5AF0** | |
| +0x7C, +0x7E | u8 | `rand() & 3` (same value) | **@bin 0x004B4800** | see the determinism note |
| +0x86 | u16 | creation sequence number (global 0x00D056FC) | **@bin 0x004B4800** | |
| +0x8A | u8 | index in the tile's figure chain | **@bin 0x004B52F0** | |
| +0x92 | u8 | := 0 at creation (and when the action state is set to 0xF) | **@bin 0x004B4800, 0x004EB000** | |
| +0x132 | u16 | := 0xFFFF at creation | **@bin 0x004B4800** | |

**Further figure fields.** Main readers:
- figure tick `0x00489B70` (per-type action table 0x005DC28C; state 2 deleted by `0x004B4980`);
- movement `0x004B27E0`;
- routing `0x004B28F0`, `0x004B2D20`;
- roaming `0x004B1EF0`;
- cross-country `0x004B3FF0`, `0x004B4220`;
- combat `0x004B1210`, `0x004B17D0`.

Figure property table: 0x007090CC + type·0x38 (+4 max damage, +8 attack, +0xC defence).

| Off | Type | Field | Evidence |
|---|---|---|---|
| +0x0B | u8 | resource carried | `0x00462180` |
| +0x0F | u8 | owner (compared with chunk 31) | `0x004B17D0` |
| +0x10 | u8 | action state before an attack | `0x004B1210` |
| +0x11 | i8 | direction 0–7 (8 arrived, 9 reroute, 10 lost) | `0x004B27E0` |
| +0x12 | i8 | previous direction | `0x004B27E0` |
| +0x13 | i8 | attack direction | `0x004B1210` |
| +0x18, +0x1A | i16 | previous tile | `0x004B23C0` |
| +0x1C | u8 | missile damage | `0x004B3FF0` |
| +0x1D | u8 | damage (> max → dying) | `0x004B17D0` |
| +0x24, +0x26 | i16 | destination x, y | `0x004B28F0` |
| +0x2C, +0x2E | i16 | source x, y | `0x004B23C0` |
| +0x36 | i16 | wait ticks | `0x00458D00` |
| +0x38 | u8 | action state (4 dying, 5 fighting) | |
| +0x39 | i8 | progress on tile 0–15 | `0x004B27E0` |
| +0x3A | i16 | path id (chunk 22) | `0x004B28F0` |
| +0x3C | i16 | path step | `0x004B27E0` |
| +0x3E | i16 | path length | `0x004B28F0` |
| +0x40 | u8 | in-building wait | `0x004B3FF0` |
| +0x41 | u8 | on road | `0x004B23C0` |
| +0x42, +0x44 | i16 | max roam length, roam length | `0x004B1EF0` |
| +0x46–0x49 | u8 | roam state | `0x004B1EF0` |
| +0x4A, +0x4C | i16 | cross-country position (1/15 tile) | `0x004B3FF0` |
| +0x4E, +0x50 | i16 | cross-country destination | `0x004B4220` |
| +0x52–0x56 | i16 | cross-country deltas | `0x004B4220` |
| +0x58 | u8 | cross-country direction | `0x004B3FF0` |
| +0x5A | i16 | home building | `0x00462180` |
| +0x5C | i16 | immigrant's target house | `0x00458D00` |
| +0x5E | i16 | destination building | `0x004BC7A0` |
| +0x64 | u8 | migrant people | `0x00458D00` |
| +0x65 | u8 | invisible | `0x004B23C0` |
| +0x66 | u8 | service max value | `0x004BAA10` |
| +0x6A | u8 | attack image offset | `0x004B1210` |
| +0x72 | u8 | terrain usage (routing algorithm) | `0x004B28F0` |
| +0x74 | i16 | amount carried | `0x004BC7A0` |
| +0x76 | u8 | boat mode | `0x004B28F0` |
| +0x77–0x79 | u8 | height adjust ticks, current, target | `0x004B23C0` |
| +0x7A | u8 | resource being collected | `0x00462180` |
| +0x82, +0x84 | i16 | target figure, targeted by | `0x00489B70` |
| +0x8B | u8 | attacker count | `0x004B1210` |
| +0x8C, +0x8E | i16 | attackers 1, 2 | `0x004B1210` |
| +0x90 | i16 | opponent | `0x004B1210` |
| +0x9C, +0xAC, +0xBC | i32[4] | bazaar buyer lists | `0x00462180` |
| +0x120 | i16 | follow link | `0x004B23C0` |

Tentative (one use or a name hint only): +0x30/+0x32 formation position, +0x60 formation id, +0x68
leader id, +0x15C ferry state. Unknown: +0x00–0x07, +0x0C–0x0D, +0x34–0x35, +0x59, +0x62–0x63,
+0x67, +0x6B–0x6F, +0x7B, +0x7E–0x81, +0x88–0x89, +0x92–0x9B, +0xCC–0x11F, +0x122–0x180 (military
and boat code `0x004A5680`–`0x004AE2C0`, UI).
| +0x77 | u8 | (tested by the draw loop before drawing) | **@bin 0x00439740** | meaning unknown |
| +0x98 | i32 | name text group (0xFE–0x122; +0x70 is the index in it, from the chunk 54–74 / 101–104 / 150–165 counters), reassigned on every load | **@bin 0x004D32F0, 0x004B5AF0** | §4 |

### City data — chunk 28, 0x00EA1870, 2 × 18,904 bytes

One record per player; the current player index is chunk 31 (0x00EAAC28). **The human player
is record 1**: chunk 31 is only ever assigned 1, the init `0x004E7D10` clears both records and
writes defaults into record 1 only, and several functions address record 1 directly
(`0x004E8610`, `0x004E9EE0`, `0x004E7E40`). All 53 campaign records have player 1 and an empty
record 0. Written at scenario
start by **0x004DE820** (called from the scenario setup 0x00540A10):

| Off | Type | Field | Status | Evidence |
|---|---|---|---|---|
| +0x000C | i32 | treasury | **@bin 0x004DE820** | := initial funds (scenario info +32) × pct[difficulty] / 100; equal in 22/35 records (the others were saved after spending) |
| +0x2778 | u16 | entry point x | **@bin 0x004DE820** | = scenario info +0x38C (low byte), 35/35 |
| +0x277A | u16 | entry point y | **@bin 0x004DE820** | 35/35 |
| +0x277C | u32 | entry point grid offset | **@bin 0x004DE820** | = start + y·228 + x, 35/35 |
| +0x2780 | u16 | exit point x | **@bin 0x004DE820** | from scenario info +0x390 (entry point if unset) |
| +0x2782 | u16 | exit point y | **@bin 0x004DE820** | |
| +0x2784 | u32 | exit point grid offset | **@bin 0x004DE820** | |
| +0x2CBC | i32 | balance at the end of last year (:= treasury at year end, `0x0045A5D0`; also set at setup) | **@bin 0x004DE820, 0x0045A5D0** | 25/35 equal to +0x0C |

**Field map** (read-only scan of the 490 directly addressed offsets in 407 functions, then the
main readers; i32 unless stated). Fields reached only through computed pointers may be missing.
Helpers: `0x004CDE40(a, b) = a·b/100`, `0x004CDE60(a, b) = a·100/b` (0 when b = 0).

*General.* +0x02 u8 player active (`0x00540A10` sets 1; construction charged only if 1,
`0x004E9680`) · +0x03 u8 := 1 at scenario start · +0x08 tax rate (init 9; `0x004599D0`) · +0x10
city sentiment, average house happiness, 60 without houses (`0x004E8840`) · +0x14/+0x18 health
target / health (init 50, ±2 toward the target, `0x004E81D0`) · +0x47A8 difficulty copy
(`0x004194C0`).

*Population.* +0x24 population = Σ age array (`0x00451C50`) · +0x28 population at last year end ·
+0x34/+0x38 house capacity / room left (`0x00458440`) · +0x3C i32[2400] monthly population
history, index +0x25BC, months +0x25C0 (`0x004E9700`) · +0x25C4 i16[100] population by age
(`0x004E8650`) · +0x268C i32[20] population per house type (type − 10) · +0x26DC/+0x26E0 births /
deaths at the yearly ageing (death % table 0x005E49D4 by health/10) · +0x26E8/+0x26EC immigrants /
emigrants to process (`0x004E8F80` → `0x00458540`) · +0x2700/+0x2704/+0x2710 average yearly
population / sum / years (`0x004E87A0`) · +0x2714 yearly-ageing flag · +0x2720/+0x2724 immigrated /
emigrated this cycle · +0x272C migration % from sentiment (−50…+100) · +0x273C/+0x475C immigrants
this / last month · +0x469C highest population.

*Finance* (labels: text group 60 in `0x0052A250`). This year / last year: taxes +0x2C4C/+0x2C50 ·
exports +0x2C64/+0x2C68 · gifts +0x4458/+0x4454 · mined gold +0x47B0/+0x47AC · income +0x2CA8/+0x2CA4
· imports +0x2C6C/+0x2C70 · wages +0x2C18/+0x2C1C (accumulator +0x2C14) · construction
+0x2C8C/+0x2C88 · interest +0x2C7C/+0x2C78 · salary +0x2C94/+0x2C90 · theft +0x2C84/+0x2C80 ·
tribute +0x44E0/+0x44E4 · requests and festivals +0x4758/+0x4754 · expenses +0x2CB0/+0x2CAC · net
+0x2CB8/+0x2CB4 · balance +0x2CC0/+0x2CBC. Year end: `0x0045A5D0` (its expense total leaves out
theft and requests). Also: +0x2C20…+0x2C60 taxed / untaxed population and tax due by class,
+0x2C30…+0x2C38 % registered (60:5), +0x45EC estimated tax, +0x2C98 salary, +0x2C9C salary rank,
+0x444C player rank, +0x4450 personal savings, +0x499C mansion built.

*Labour* (`0x00458880`, `0x00459100`, `0x00459400`): +0x2B24 working-age population × 60 % ·
+0x2B28 labour force · +0x2B2C + 20·i (i = 0…9) category {needed, allocated, Σ, buildings,
priority} (names *assumed*) · +0x2BF4 employed · +0x2BF8 unemployed · +0x2BFC unemployment % ·
+0x2C04 shortage · +0x2C08 wage rate (init 30; monthly cost = employed × rate / 120) · +0x2C0C
kingdom wage reference.

*Ratings* (win test `0x004EA2C0` against scenario globals 0x00784C24–C30): +0x42CC culture
(`0x004269B0`) · +0x42D0 prosperity (`0x0046E1E0`) · +0x42D4 monument (`0x0046EF50`) · +0x42D8
kingdom (`0x0046E970`) · +0x46A8 prosperity cap · +0x4434…+0x4440 explanation ids.

*Religion and festivals* (`0x004E9890`, `0x004EA000`): +0x4390 u8[5] god target mood · +0x439A
u8[5] god mood · +0x43A4, +0x4816 u8[5] counters (wrath / blessing *assumed*) · +0x43E0 i32[5]
months since festival per god · +0x44F8/+0x44FC/+0x4500 planned festival size / months / god ·
+0x4504…+0x450C festival costs.

*Food, storage, trade:* +0x483C i32[9] granary stock by food · +0x296C total food · +0x2970 food
types · +0x45DC food eaten this month · +0x4860 / +0x48F0 i32[36] storage-yard free space / stock
per resource (`0x0045AAE0`) · +0x2826 i16[36] trade status · +0x286E i16[36] trade threshold ×100
(`0x004434E0`) · +0x28B6 i16[36] industry switched off (*assumed*) · +0x2A88 i32[36] stockpiled
(*assumed*) · +0x3C64 / +0x3D04 i32[40] stock / capacity of input-holding buildings.

*Unique buildings* (`0x00470990`, `0x0046AC50`): +0x2788…+0x2790 palace x, y, offset, id ·
+0x44A4…+0x44AC recruiter · +0x45F4…+0x4600 type 0x32 building · +0x49D0 academy exists.

*Services:* entertainment coverage +0x4360…+0x437C, +0x44C8/+0x44CC (`0x004550E0`); house
evolution blocker counters +0x4304…+0x435C, +0x4388, +0x4748, +0x474C (`0x00453B40`, house model
table 0x0070CDD4 + model·0x58); +0x4750 houses at disease level 1000.

*Unknown:* +0x2CC4–0x3C63 (3.9 KB, no direct reference except the version-124 relayout: computed
access or obsolete), +0x3DA4–0x42A3, +0x2978–0x2A87, and small fields found by the scan
(+0x00–0x07, +0x1C, +0x2C, +0x30, +0x2740, +0x2794, +0x4410–0x442B, +0x4460–0x44A0,
+0x4564–0x45A4, +0x4664–0x4744, +0x4768–0x47A4, +0x4820–0x4838, +0x4980–0x49D7). +0x2796–0x2825
and +0x291C–0x296B are touched only by version-upgrade code (obsolete layouts).

Difficulty: chunk 174 (0x00E39080), 0 … 4; funds percentage table 0x005D1960 = 200, 100, 100,
100, 75 (**@bin**, used by 0x004CDE40 = a·b/100). All campaign records are at difficulty 2.
If the scenario's entry point is unset (0xFFFF), 0x004DE820 uses (width − 1, height / 2); an
unset exit point takes the entry point.

### Other records (**@bin**, fields seen in code; meanings beyond them open)

**Formations, chunk 24 (0x00BCB9C8, 50 × 144)**, read from `0x004B6E10` and `0x004B77D0`:

| Off | Field |
|---|---|
| +0x00 | in use |
| +0x01 | owner |
| +0x02 | legion index 0–7 (the first free one) |
| +0x04 | u16 copied from the fort's building +0x12 |
| +0x06 | u16 fort building id |
| +0x08 | 16 × u16 figure ids |
| +0x2A | := 1 at creation |
| +0x2C | := 50 at creation |
| +0x2E, +0x30 | x, y |
| +0x40 | u16 standard-bearer figure (type 0x0E) |
| +0x42 | player-legion flag |

**Empire cities, chunk 50 (0x0077DE00, 61 × 106)**, read from `0x00443950` and `0x00444920`:

| Off | Field |
|---|---|
| +0x00 | in use |
| +0x02 | kind (1, 3 and 5 are the trading kinds tested) |
| +0x04 | trade route id; indexes chunks 114/115 as route·36 + resource |
| +0x05 | flag returned by `0x00444980` |
| +0x06–0x29 | 36 "sells" flags |
| +0x2A–0x4D | 36 "buys" flags |
| +0x56 | u16 empire object index (chunk 142) |

**Messages, chunk 77 (0x00E95228, 1000 × 48)**, read from `0x004E0CF0`:

| Off | Field |
|---|---|
| +0x00, +0x04 | i32 parameters |
| +0x08 | u16 year |
| +0x0A | u16 message type (0 = free slot) |
| +0x0C | u16 sequence number |
| +0x0E | read flag |
| +0x0F | month |
| +0x10… | 30 bytes of optional data; when absent, +0x10/+0x12/+0x14 := 0xFFFF |

**Monument work-step records** (chunks 146, 149, 171, 172, 176–180; 24 bytes, **@bin
0x0041FCA0, 0x004207E0**):

| Off | Field |
|---|---|
| +0x00 | i32 animation-script index (table 0x005BE9C8, stride 0x28). Static data: loader step 7 restores the exe's value for chunks 146/149/171/172 |
| +0x04 | i32 chosen option (−1 none) |
| +0x08 | i32 value from the script |
| +0x0C | i32 remaining work units |
| +0x10 | u16 state |
| +0x12 | u16 assigned worker figure |
| +0x15 | u8 flag |

Each monument has a 16-byte exe table pointing to its records: {building +0x58 stage key, worker
figure type (79 Carpenter, 81 Stone mason, 108 Tomb artisan), record, initial count}.

**Game time:** 51 ticks per day (chunk 34), 16 days per month (35), 12 months (36), the year
(37), total days (38) (`0x00451C50` → `0x00452370`).

## 6. Open questions

1. Fields: buildings, figures, city data, formations, empire cities, messages and the monument
   records are mapped (§5) and every schema chunk has a role except 29, 99, 118, 121, 139 and most
   of 134. What remains is listed under "Still unknown" / "Unknown" in each table of §5, plus the
   scenario events (chunk 136) and scenario info (chunk 75) layouts beyond the known offsets.
2. ~~0x004D3DA0, the version-124 relayouts, the clean-ups~~: §4. Meanings of the fields they move
   are still open.
3. ~~What `0x004BD360` sets up~~: the floodplain settings reset (§2). Meaning of each field of
   record 21: open.
4. CRT `rand()` reseeded from the clock (§5, figures): decide how differential tests handle it
   (exclude affected figure types, or treat those outcomes as inputs).
5. Floating point in the simulation: `0x004E7F00` (house malaria / disease risk) computes
   distances with x87 `sqrt` and `__ftol` (53-bit precision, truncation). Porting it to integers
   needs the same proof as the terrain formulas (map.md §6), or a bit-exact reproduction.
6. ~~Version upgrades and per-load steps~~: ported (§4). Open:
   - chunks 176–180 of files below version 154 keep what the game held before the load; the
     port assumes a freshly started game (the exe's initial data);
   - the live objects rebuilt at load (route point arrays, shipwright queues) are runtime data,
     left to the empire and shipyard simulation;
   - no save writer.
