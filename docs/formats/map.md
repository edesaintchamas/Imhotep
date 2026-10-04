# Scenario maps (`.map`)

Reference installation: GOG Pharaoh + Cleopatra 2.1. Corpus: the 33 `Maps/*.map` files.
Analysis dated 2026-10-04. Tool: `tools/extract/read_map.py` (walks and decompresses every chunk;
it fails loudly on any mismatch). Same status vocabulary as `graphics.md`: **@bin** = read in
`Pharaoh.exe` (Ghidra decompilation), *confirmed* = consistent with all files, *assumed* = hint.

Hints used: Akhenaten `src/io/gamestate/file_schema_map.cpp`, `chunks.cpp`,
`src/scenario/scenario.cpp`; Julius `src/core/zip.c` (decompressor).

## 1. Container

A `.map` file is a sequence of **chunks**, back to back, with no padding. The file starts with
three small chunks and a **schema** that describes every chunk, including itself. All integers
are little-endian.

| Chunk | Offset | Size | Content | Status |
|---|---|---|---|---|
| 0 | 0 | 4 | Magic `MAPS` | confirmed (33/33); **@bin 0x004D4AE0** compares it with the string at 0x005E17CC |
| 1 | 4 | 4 | File version (u32). Shipped maps: 144 (1), 146 (2), 149 (23), 150 (7). The game writes **160** (0xA0) | values confirmed; writer **@bin 0x004D5520** |
| 2 | 8 | 4 | Number of schema records in use (24 in every file) | confirmed |
| 3 | 12 | 6000 | Schema: 300 records × 20 bytes (§2), first `count` used | confirmed |
| 4… | 6012 | … | Data chunks, sizes and compression given by the schema | confirmed |

**Check**: walking the chunks as the schema describes them ends exactly at end of file in 33/33
maps.

Compressed chunk on disk: `u32 stored_size` then `stored_size` bytes of PKWARE DCL "implode" data
(§3), which decompress to exactly `elem_size × elem_count` bytes. Confirmed for all 165
compressed chunks in the corpus. **@bin 0x0055BCF0** (the game's reader): a `stored_size` of
0x80000000 means the raw bytes follow; **0 means no data and a zero-filled chunk**; any other
value is PKWARE data passed to `0x0055C000`, which runs the standard DCL `explode` (`0x0055CBD0`,
0x8DD8-byte work area, CRC-32 computed and not checked) and fails only on a stream error. Chunks
above 1,200,000 bytes are refused. Neither special value occurs in the corpus. Our reader is
stricter in one point: it also requires the output to fill the chunk exactly (true for all 165).

## 2. Schema record (20 bytes)

| Off | Type | Meaning | Status | Evidence |
|---|---|---|---|---|
| 0 | u32 | 1 = compressed | confirmed; **@bin 0x004D0E10** reads the file's flag | Walk + decompression (§1) |
| 4 | u32 | Memory address of the variable in the program that wrote the file | confirmed (meaning) | Values differ between file versions and from the static table in our exe (§4). They are meaningless to a reader |
| 8 | u32 | Element size in bytes | confirmed; **@bin 0x004D0E10** | Walk |
| 12 | u32 | Element count | confirmed; **@bin 0x004D0E10** | Walk |
| 16 | u32 | Pointer to a per-version converter list, or 0 (writer's memory, so stale in files) | **@bin 0x004D0E10** (used from the exe's own table) | Non-zero only for chunk 16, in every file |

The schema is what makes old maps loadable. The game reads each chunk with the **file's** flag
and sizes and copies it into its current layout (§4).

## 3. Compression

PKWARE Data Compression Library, "implode", binary literals (header byte 0 = 0), window size in
header byte 1. All 165 chunks decompress to exactly the size the schema declares, ending on the
end-of-stream code. Decompressor: `tools/extract/pkware.py`, translated from Julius.

Status: format **confirmed** on the data. The game's reader is `0x0055BCF0` (called for
compressed chunks by `0x004D0E10`; rules in §1); the exe contains the PKWARE DCL copyright strings
("Version 1.11"), and `0x0055CBD0` is the library's `explode`.

## 4. Chunk list

The exe has a static schema table at **0x005E17D8** (24 × 20 bytes), used both to write maps
(`0x004D5520`) and as the target layout when reading (`0x004D0E10`). Records 21–23 are zero in the
static data and must be filled in at run time (not found yet).

Names are **Akhenaten's** (*assumed*). Sizes and flags are from the files and the exe table.
`G` = one element per tile of the 228 × 228 grid (51,984).

| # | Name (hint) | Files (≤ v150) | Exe table (v160) | Compressed | Notes |
|---|---|---|---|---|---|
| 0 | magic | 4 × 1 | same | no | `MAPS` |
| 1 | file_version | 4 × 1 | same | no | |
| 2 | schema_count | 4 × 1 | same | no | |
| 3 | schema | 20 × 300 | same | no | §2 |
| 4 | image_grid | 4 × G | same | no | u32 global image id per tile; 0 outside the map (§5) |
| 5 | edge_grid | 1 × G | same | no | |
| 6 | terrain_grid | 4 × G | same | no | u32 flags; 0x80005 / 0x80000 outside the map |
| 7 | bitfields_grid | 1 × G | same | no | |
| 8 | random_grid | 1 × G | same | no | |
| 9 | elevation_grid | 1 × G | same | no | All zero on Small Oasis (flat map) |
| 10, 11 | random_iv | 4 × 1 each | same | no | **@bin**: the two RNG states, 10 = 0x00D3B384 (iv1), 11 = 0x00D3B380 (iv2) in the exe schema 0x005E17D8; generator 0x004D7090 (`core/random/random.c`) |
| 12, 13 | camera | 4 × 1 each | same | no | Akhenaten: one 8-byte chunk; here two |
| 14 | scenario_info | 1592 × 1 | same | no | §5 |
| 15 | soil_fertility_grid | 1 × G | same | no | Non-zero near water on Small Oasis |
| 16 | scenario_events | 18628 × 1 | same | no | Akhenaten splits it 18600 + 28. Converter list at 0x005E0960 (version 130) |
| 17 | junk11 | 32 × 40 | same | yes | |
| 18 | empire_map_objects | **76** × 200 | **98** × 200 | yes | Grows in v160 (Akhenaten: "15200 / 19600 at version 160") |
| 19 | empire_map_routes | 324 × 50 | same | yes | |
| 20 | vegetation_growth_grid | 1 × G | same | no | |
| 21 | floodplain_settings | 36 × 1 (**32** in v144/146) | runtime (0x004D4A60) | yes | 36 bytes at 0x00D38360, the floodplain manager's settings (@bin) |
| 22 | trade_prices | 8 × 36 | runtime (0x004D4A60) | no | 0x005D2C48 |
| 23 | moisture_grid | 1 × G | runtime (0x004D4A60) | yes | 0x00A509BC |

### Reading old versions (**@bin 0x004D0E10**, per chunk)

- If the file's element size and count equal the exe's (and there is no converter to run): read
  straight into the variable, using the **exe's** compression flag (the file's flag is only used
  on the migrating path). The check is skipped while the schema count global is 0, i.e. for the
  chunks before the schema, and for the schema chunk itself.
- Otherwise: read `file_size × file_count` bytes (file's compression flag) into a temporary
  buffer. Then copy element by element into the exe layout: elements are truncated or
  zero-padded to the new element size, and elements beyond the file's count are zeroed. If the
  exe record has a converter list (5th dword), the first entry whose version threshold is above
  the file version provides a function, called on each element (flag 1 for elements that came from
  the file, 0 for new ones).
- File version < 115 (0x73): the load fails.

After all chunks, `0x004D4AE0` runs upgrade steps keyed on the file version (thresholds 117, 118,
125, 127, 144, 151, 152, 153, 155, 156, 157, 158). These are **game rules**: to be read before
implementing the loader in `core/`.

## 5. Geometry

`scenario_info` (chunk 14) at offsets 44, 48, 52, 56 (i32): `width`, `height`, `start_offset`,
`border_size` (offsets from Akhenaten, *assumed*). Consistent in 33/33 maps:

- `width == height` ∈ {84, 140, 170, 226};
- `start_offset = off × 228 + off`, with `off = (228 − width) / 2` (the map is centred in the grid);
- `border_size = 228 − width`.

**Playable area: the diamond inside the square — @bin.** `0x0046F7B0` computes, per map row, the
x range of tiles that belong to the map (globals `DAT_00bca2e8..fc`: origin, far corner, centre),
and `0x0046F930` is the "tile is on the map" test with the same diamond. With
`x0 = y0 = (228 − w)/2`, `x1 = x0 + w − 1`, `cx = (x0 + x1)/2`, `cy = (y0 + y1)/2`, row `y` runs
from `sx = cy + x0 − y` (if `y ≤ cy`) or `x0 − cy − 1 + y` (otherwise) up to (excluding)
`2·cx + 2 − sx`. Check: this range set equals the non-zero tiles of `Default.map` and
`Small Oasis.map` exactly (9,940 and 14,620 tiles).

Image ids stored outside the diamond (40,531 tiles over the corpus) are never shown: the game
zeroes the whole image grid when a scenario starts (§6).

## 6. Image ids, terrain images and rendering

`tools/extract/render_map.py` renders a map with the original graphics; tile images come from the
core rebuild through `build/terrain_dump` (`make tools`). Every rule below is
read in `Pharaoh.exe`.

**Global image ids.** At startup `0x00414420` loads 30 graphics families into slots 0–29, in this
order: `Pharaoh_Unloaded, SprMain, Hyksos, Pharaoh_General, Pharaoh_Terrain, SPHINX1A–1D,
SPHINX_EXTRA, TEMPLE_NILE, empire, SprAmbient, Pharaoh_Fonts, Egyptian, SprMain2, expansion,
PyramidDummy, CaesareumDummy, LtHouseDummy, LibMainDummy ×3, LibExtra, MausCarryoverDummy,
TombThutDummy, AbuSimbelDummy, TombTutDummy, TombSetiDummy, TombRamDummy`. Each slot gets the next
id range (`0x0053DD90` keeps the running total). Slots other than 0 skip the 200 `system.bmp`
images (`0x0053DFF0`), so `global id = base(slot) + local index − skip − 1`, with base(slot 0) = 1 (`0x0053DB60`). Missions later reload
some slots (monuments, temples, enemies) at the same base (`0x0053DD90` with an explicit slot);
the Dummy files reserve those ranges. Check: every image id drawn on the 33 maps resolves.

**Image groups.** Group numbers run from 1 over the non-empty group slots of each family, in
slot order (`0x0053DFF0`); the table at `0x00F55BB0` holds (slot, first global id) per group.
Check: the groups used by the terrain code resolve to the expected names (505 `SPR_IMPASSABLE`,
510 `SPR_DUNES`, 539 `SPR_ROCK_GOLD`, 502 `SPR_WATER`, 528 `SPR_FLOODPLAIN`, 743 `SPR_NEW_CLIFF`,
719 `SPR_BLOOD_COAST`…). Group names in the files are partly Caesar III leftovers (land is 507,
named `SPR_IRRIGATION`).

**Projection** (`0x00430530`, orientation 0): tile (x, y) is at view row `1 + x + y` (15 px) and
half-column `230 + x − y` (30 px); tiles with image id < 6 are not in the view. **Draw tile**:
edge bit 0x40; a block of n×n tiles is drawn from its tile (0, n−1) (`0x0047BF50`), and the
edge byte holds `dx | dy << 3` (table `0x005CC8C0`).

**The stored image grid is not what the game shows.** `0x004D52A0` (scenario start) zeroes the
image grid, runs the rebuild `0x004DEA50`, then the floodplain build `0x004BE160`, which ends by
running `0x004DEA50` again. The rebuild derives every tile image from the terrain flags, the
random, moisture, fertility and vegetation grids, and (second run) the floodplain objects.
Reimplemented in `core/map/terrain.c`, every rule tagged with its address; the neighbour
pattern tables are read from `.data` and their counters and the matcher globals (`0x00BC9740/44/48`)
are carried through the whole sequence as in the game.

| Sub-pass (in order) | Terrain | Rule (summary) |
|---|---|---|
| 0x004770F0 → 0x004798E0 | rocks with bitfields 0x80 | table 0x005CBEA8, group 503 |
| 0x00475DC0 | rocks 2, gold 0x100002 | 3×3 / 2×2 / 1×1 blocks by fit (0x0047CAE0) and random |
| 0x00475BF0 → 0x00479770 → 0x00486600 | trees 1 | group 509 + 8·(moisture//34), or group 501 + ring depth (0x00481490, rings 0x0046F8D0) when vegetation = 0xFF |
| 0x00476530 | 0x20 | 3×3 / 2×2 / parity singles, group 328 |
| 0x0047D870 | entry/exit signposts | only for points on the map square's edge — none in the corpus |
| 0x004767D0 | land, grass 0x80 | land: blocks up to 4×4 of sub-images; grass: moisture level (disassembly) |
| 0x004777C0 → 0x00479E70 (×2) | floodplain 0x10000 | group 528 + flood progress + 6·((trunc(fertility·0.08) & ~1) \| random & 1) |
| 0x00477500 → 0x00479B70 | meadow 0x800 | a = trunc((fert−1)·3/99), b = trunc((moist−1)·12/96) → group 534 + 12a + b, special cases 563/552/551 |
| 0x004776F0 → 0x0047AB10 | 0x40000 | shore table, group 508; fully surrounded → 529/508 |
| 0x00477210 | dunes 0x2000000 | as rocks, group 510 |
| 0x00475CC0 → 0x0047A220 | water 4, deep 0x4000000 | shore / deep tables; floodplain banks use the neighbours' floodplain objects |
| 0x00477890 → 0x0047A120 | rubble 0x1000 | group 531 + random |
| 0x0047B570 → 0x0047B590 → 0x0047B670 | roads 0x40 | table 0x005CC268, group 540 (548 on floodplain) |
| 0x00475720 | plaza (0x40 + bitfields 0x80) | 2×2 (0x00475670) or parity singles, group 437 |
| 0x0047B1A0 | walls 0x4000 | table 0x005CB9D8, group 407 |
| 0x0047BAE0 → 0x0047E870 / 0x0047EB60 | canals 0x100 | table 0x005CC678, group 506 + variant + offset (+0x30 unless the tile already shows a group-506 image 0–47; +0x15 on floodplain, 0x0051C620); a road across adds 0xF, or 0x2A if paved (0x0047B920); only tiles with aqueduct grid (0x00AE8E7C) < 0x10, whose value becomes the matcher extra. Neighbour flags: N/E/S/W canal links (road rules in 0x0047EB60), water on floodplain, adjacent Water Lifts (type 0x5A). Ported, **@bin only** (no tile reaches it, maps or campaign; `campaign.md` §3). Roads on canals (0x0047B670 0x100 branch) likewise |
| 0x00475FD0 (from rocks) | cliffs 0x20000002 | 8-neighbour cliff pattern → group 743 + index (tables 0x005DBCC8–0x005DBD98); verified on 33,464 campaign tiles (`campaign.md` §4) |

**Floodplain build** (`0x004BE160`): repeated passes until nothing changes; a tile on or next to
floodplain (0x10000) gets depth 30 if it touches water, else (neighbour depth − 1, never using a
depth assigned in the same pass); its terrain becomes `t & 0xFDFBF7FF | 0x80`, and a floodplain
object (`0x004BD740`: tile, x, y, depth, row = depth−1, flag = tile had 0x10000) goes into the
per-tile pointer grid of the floodplain manager at `0x00D05720`. Row field 1 is the flood
progress, advanced by the daily flood tick `0x004BE770`; it is 0 at scenario start.

**Verification against the game** (`make differential`, i.e. `build/terrain_check`): the map
editor stores the
image grid it computed with this same code, so a correct rebuild reproduces the stored ids. Over
the 29 comparable maps (file version ≥ 147, Gateway to Atlantis excluded) it matches 589,509 of
589,524 tiles: 100 % for water, floodplain, rocks, dunes, trees, meadow, roads, walls, 0x40000;
15 tiles differ (8 grass, 7 land) in four small clusters where a neighbour's terrain changed after
the editor last refreshed them (e.g. Hostile Nations: water tiles next to grass whose stored image
is the "no non-plain neighbour" variant). Gateway to Atlantis differs on half of its land tiles
exactly along the bitfields 0x20 bit (alternate land group), i.e. that bit was edited after its
images were computed. Maps of versions 144/146 (Bridges, NAFTA, Warfare) use an older image
numbering and match 0 %, as expected.

**x87 precision**: the integer replacements of the three floating-point formulas (grass level,
floodplain fertility, meadow) assume 53-bit precision; **@bin 0x005640EC** (CRT startup) sets
`_controlfp(0x10000, 0x30000)` = `_PC_53`, and nothing else in the exe sets the precision bits.

Global image ids start at **1**: `0x0053DB60` initialises both the image and the group running
totals to 1 (so `global id = 1 + images in earlier slots + local index − skip − 1`). This was
found through the comparison (every tile was off by exactly one before).

The "blood river" variants (expansion groups) are selected by a region (`DAT_0078d7b0`) that is
not part of a map and is zero after loading.

## Open questions

1. ~~Playable area~~: diamond, @bin (§5).
2. ~~Global image ids~~: §6.
3. Canals (`0x0047BAE0`): ported @bin; needs test data (a saved game with canals).
8. Drawing (@bin 0x00430EF0): cliff images 48–71 of group 743 are not drawn; deep water and
   floodplain coast are animated (`campaign.md` §4). The renderer applies the cliff skip.
4. Terrain flag bits, edge/bitfields/random grid encodings, `scenario_info` beyond offset 60,
   `scenario_events`, empire chunks: all from Akhenaten names only.
5. ~~Run-time schema records 21–23~~: filled by `0x004D4A60` — 21 floodplain settings (36 bytes at
   0x00D38360 = floodplain manager + 0x32C40), 22 trade prices (0x005D2C48, 8 × 36), 23 moisture
   (0x00A509BC). See `save.md` §3.
6. ~~Compare `0x0055BCF0` with `pkware.py`~~: §1.
7. ~~`random_iv`~~: RNG state, §4. Not read yet: who fills the pool at load/start (the step is
   also inlined twice in 0x004860D0) and every caller's use of the derived values.
