# Campaign scenarios (`missionN.pak`)

Reference installation: GOG Pharaoh + Cleopatra 2.1; only `mission1.pak` exists (17,187,872 bytes).
Analysis dated 2026-10-04. Readers: `core/io/campaign_pak.c`, `core/io/map_file.c`
(`chunk_file_read`), `core/map/terrain.c` (`terrain_init_campaign`). Test: `build/terrain_check --campaign`.

## 1. Archive

| Offset | Size | Content | Status |
|---|---|---|---|
| 0 | 240 | 60 × u32: file offset of record *i*, 0 = no record | **@bin 0x004D58B0** reads the u32 at `mission × 4` of `mission1.pak` … `mission5.pak` (names at 0x005E1B98, 20 bytes apart); a value < 1 means no record |
| 240 | … | 53 records back to back (slots 0–52; 53–59 empty) | confirmed: each record's chunk walk ends exactly at the next record's offset (or end of file) |

Loading a record (**@bin 0x004D58B0**): seek to its offset, load it with the saved-game loader
`0x004D41E0`, then `0x004BD360` and **`0x004D52A0`** (the scenario start). Custom maps do not run
`0x004D52A0`: they go through the reset `0x00540A10` and the setup `0x004DE820`, which rebuild the
terrain images with the same function (`0x004DEA50`) (run on all 33 bundled maps, docs/newgame.md).

## 2. Record

A record is a saved game in the same self-describing chunk container as a `.map`
(`map.md` §1–§3), without the `MAPS` magic: four 1-byte chunks (`00 00 FF FF`), then the file
version (u32 at offset 4), the record count (u32 at offset 8) and the schema. Versions in the
archive: 147 (18 records), 149 (17), 150 (1), 160 (17); 175 chunks, or 181 at version 160.

Chunk indices used by the terrain rebuild (same in every version). Identified by mapping each
record's schema addresses onto Pharaoh.exe's variables with one constant delta per record
(e.g. +0x8B78 for record 31): **confirmed** on records of every version.

| Chunk | Content | Pharaoh.exe |
|---|---|---|
| 7 | image grid (u32) | 0x00B7482C |
| 8 | edge grid | 0x00B67D1C |
| 9 | building grid (u16) | 0x00B4E6FC |
| 10 | terrain grid (u32) | 0x00AF598C |
| 13 | bitfields grid | 0x00B285CC |
| 15 | random grid | 0x00ACF85C |
| 16 | desirability grid | 0x00AC2D4C |
| 75 | scenario info (1592 bytes) | 0x007848D8 |
| 135 | soil fertility grid | 0x00A5D4CC |
| 144 | vegetation grid | 0x00A43EAC |
| 170 | moisture grid | 0x00A509BC |

The rest of the saved-game layout (figures, buildings, city data, …) is not analysed yet.

## 3. What the records contain for terrain

| Terrain | Records | Tiles |
|---|---|---|
| cliffs (`0x20000002`) | 38, 39, 40, 43, 44 (all version 160) | 33,464 |
| `0x100` | 25 | 20, all also water (`0x10104`), so handled by the water pass |
| buildings (flag 8) | 11, 23, 25, 26, 32, 34, 40, 42, 44, 46, 49–51 | 2,281 |

No tile reaches the canal sub-pass `0x0047BAE0` (it needs `0x100` without water or building
flags): the canal pass is ported from the binary (`map.md` §6) but has no differential coverage.

## 4. Differential results (`build/terrain_check --campaign`)

The stored image grids were computed by the game when the records were written. Records are
saved states, so tiles whose images depend on saved buildings, desirability or flood progress
can differ legitimately; the test gates on cliffs and rocks only.

- **Image numbering.** Version-160 records number `SPR_NEW_CLIFF` images 152 ids later than the
  reference files (every one of the 33,464 cliff tiles differs by exactly +152; other groups
  match), i.e. they were written with an `expansion.sg3` holding 152 more images before that
  group. Version-147 records use an older numbering (mostly +539) and are skipped.
- **Cliffs** (`0x00475FD0`, ported): 33,464 / 33,464 tiles match (with the +152 shift).
- **Rocks**: 14,729 / 14,729 tiles match over the 35 records of version ≥ 149.
- Other categories: 46–100 % per record; records 20 and 37 match 100 % on every category.
  The differences are saved state, not rule errors:
  - floodplain tiles: stored − ours is exactly the flood progress at save time (+1, +2, +3 in
    records 31, 38, 15). Progress (floodplain row field 1) is not saved and the scenario start
    resets it (`0x004BE0F0`), so the game shows our images (`save.md`);
  - deep water: differences are multiples of 15, the `variant × 15` term of the deep-water image
    (`map.md` §6): the tile renderer **@bin 0x00430EF0** animates deep water by +15 per frame over
    6 frames (wrapping by −90) and floodplain coast by +1 over 6 frames, and the game writes the
    animated id back to the image grid, so saved grids hold a frame of the animation;
  - tiles next to buildings (roads, walls, banks) depend on saved buildings and desirability.
