# Graphics: `.sg3` index + `.555` pixel data

Reference installation: GOG Pharaoh + Cleopatra 2.1 (`docs/source-fingerprint.json`).
Corpus: all 175 `Data/*.sg3` files and the 214 `.555` files they reference (175 paired + 39 external).

**Status vocabulary.** *confirmed* = checked against the actual files, with the check stated.
Unless a row says otherwise, a check ran over **all 175 `.sg3` files** (scripts run on
2026-10-04 and recorded in this session; `tools/extract/extract_graphics.py` reimplements the
parsing). *assumed* = plausible, taken from a reference project or a partial pattern, not proven.
Where the loader in `Pharaoh.exe` was read (§10), the row says **@bin** with the address.
Otherwise "confirmed" means "consistent with the data", not "this is how Pharaoh.exe reads it".

References used as hints: Akhenaten `src/content/imagepak.cpp`, `imagepak.h`,
`graphics/image.cpp`; Julius `src/core/image.c` (Caesar III `.sg2`, same family).

All integers are little-endian.

## 1. File layout

| Offset | Size | Content | Status |
|---|---|---|---|
| 0 | 80 | Header, 20 × u32 (§2) | confirmed (fields below) |
| 80 | 600 | Group table, 300 × u16 (§3) | confirmed |
| 680 | 40000 | Bitmap records, 200 × 200 bytes (§4) | confirmed |
| 40680 | 64 × `max_entries` | Image entries (§5) | confirmed |
| 40680 + 64 × `max_entries` | 14400 | Group names, 300 × 48 bytes (§6) | confirmed |

Check: `file size == 40680 + 64 × header[3] + 14400` holds for all 175 files. There are only two
file sizes: 695080 (`max_entries` = 10000, 158 files) and 887080 (`max_entries` = 13000, 17 files:
the enemy/foreign-army sets such as `Assyrian`, `Hittite`, `Egyptian`, `Enemy_1`).

> **Mismatch with Akhenaten.** Akhenaten's comment says the 13000/10000 count leaves "225 spaces
> at the end left unused" and never reads the last 14400 bytes. The data shows that this region
> is a 300 × 48-byte table of group names whose index matches the group table (§6). The 225 figure
> is just 14400 / 64.

## 2. Header (80 bytes)

| Off | Type | Name | Meaning | Status | Evidence |
|---|---|---|---|---|---|
| 0 | u32 | `h0` | Equals the `.sg3` file size in 171 files; 74480 in `Pharaoh_General`, `Pharaoh_Terrain`, `Pharaoh_Fonts`, `Pharaoh_Unloaded` | assumed | Meaning of 74480 unknown |
| 4 | u32 | `version` | 213 in every file | value confirmed, meaning assumed | Akhenaten calls it `version` and reads 8 more bytes per entry when ≥ 214; no file here has ≥ 214 |
| 8 | u32 | `h2` | 0–25. 1 in 143 files | unknown | — |
| 12 | u32 | `max_entries` | Capacity of the entry table | confirmed | File-size equation above |
| 16 | u32 | `last_index` | Index of the last used entry (entry 0 is an empty placeholder, so the file has `last_index` images) | confirmed | Last bitmap's `last_index` field equals it in 158 files. Enemy files have no ranges in their bitmap records (§4). Data offsets end exactly at the end of the `.555` (§7) |
| 20 | u32 | `num_bitmaps` | Number of bitmap records in use | confirmed | Every entry's `bitmap_id` < this value; all records past it are zero |
| 24 | u32 | `h6` | `num_bitmaps − 1` in 158 files; other values in the 17 enemy files | assumed | — |
| 28 | u32 | `h7` | `h8 + h9` in 158 files | relation confirmed for those files, meaning unknown | — |
| 32 | u32 | `h8` | Sum of the `length` of the internal entries in 155 files (= `.555` size − 4 in those) | assumed | Fails in 20 files; never investigated |
| 36 | u32 | `h9` | Unknown size total | unknown | — |
| 40 | 10 × u32 | — | Always 0 | confirmed | All files |

## 3. Group table (offset 80, 300 × u16)

`group_first[g]` is the entry index of the first image of group `g`. Slot 0 is 0.

Status: layout **confirmed** (non-zero values are valid entry indices). Pairing with the names of
§6 checked on one case: in `Pharaoh_General`, `SPR_MENU_BLOCKS` → 201, the first image of
`paneling.bmp` per its bitmap record.
The group numbering is presumably what the engine uses as image-group IDs (like Julius'
`GROUP_*`): **assumed**.

## 4. Bitmap records (offset 680, 200 × 200 bytes)

Each record describes one source bitmap that the images were cut from. Entries point at a record
through `bitmap_id` (§5).

| Off | Type | Meaning | Status | Evidence |
|---|---|---|---|---|
| 0 | char[65] | Bitmap file name, NUL-terminated (`system.bmp`, `paneling.bmp`, ...) | confirmed | Readable names in all files. For external images, the name selects the `.555` file (§7) |
| 65 | char[51] | Comment (`"menus, icons, side panels"`) | confirmed (text) | Readable text |
| 116 | u32 | Bitmap width? | assumed | 6420 of 59247 images do not fit inside (`u24 + w`, `u26 + h`) — so either not width/height or the positions are not what I think |
| 120 | u32 | Bitmap height? | assumed | same |
| 124 | u32 | Image count | confirmed | `= last − first + 1` for every record that has a range |
| 128 | u32 | First entry index | confirmed | Entries in [first, last] carry this record's `bitmap_id`. Exceptions: the Dummy files (§9) and `Pharaoh_Terrain` entry 994 |
| 132 | u32 | Last entry index | confirmed | same |
| 136 | 7 × u32 | Unknown | unknown | — |
| 164 | 3 × u32 | Size totals: value at 168 = 164 + 172 (e.g. `paneling.bmp`: 678885 + 130925 = 809810). `system.bmp` in General holds 39214, 50124, 10910 = `h8, h7, h9` of `Pharaoh_Unloaded` | relation seen on 2 records, meaning unknown | Same pattern as header h7–h9 |
| 176 | u32 | Unknown (28, 14, 23, 9 in General) | unknown | — |
| 180 | 4 × u32 | 0 in the records looked at | unknown | — |
| 196 | u32 | 1 or 131073 | unknown | — |

The 17 enemy files use their records differently: names like `ENEMY_MISSILE`, two small
integers, and no count/first/last. **Assumed**: these records are not image ranges there.

## 5. Image entry (64 bytes, at 40680 + 64 × index)

Entry 0 is always all-zero. Entries `last_index+1 … max_entries−1` are zero, except in 11
files where they hold a single byte 63 = 1 (e.g. `Empire`, `Pyramid`, `Temple_*`).

Counts below are over the 59247 entries with `length > 0`, unless stated.

| Off | Type | Name | Meaning | Status | Evidence |
|---|---|---|---|---|---|
| 0 | i32 | `offset` | Byte offset of the pixel data. **Internal**: 0-based offset in the paired `.555`, but **ignored by the game**, which recomputes it (§10). **External**: 1-based offset in `<bitmap name>.555` | confirmed; internal recomputation **@bin 0x0053DFF0** | Internal: data is contiguous from offset 4 to EOF of the `.555` in 170 files (exceptions: §9). External: 483 entries, `offset − 1 + length` ends exactly at EOF of each of the 39 external files |
| 4 | i32 | `length` | Byte length of the pixel data | confirmed | Contiguity above, and all images decode consuming exactly `length` bytes (§8) |
| 8 | i32 | `uncompressed_length` | Isometric: byte length of the footprint part (= 1800 × n²). Plain: = `length`. RLE: = `length` for 37204 compressed entries, different in the rest (0 throughout the enemy files) | confirmed for isometric | 4943/4943 isometric images |
| 12 | i32 | — | Always 0 (all 70253 slots) | confirmed | — |
| 16 | i32 | `mirror` | If ≠ 0: the entry has no data (`length` = 0, `offset` = 0) and reuses entry `index + mirror` (always negative) | confirmed; horizontal flip **@bin 0x0053E590** | 9561/9561: target has data and the same width/height. Draw dispatcher: if byte +62 ≠ 0, image id := id + `mirror`, then right-to-left blitters (`0x0040FED0`, `0x0040FF50` plain; `0x0040FC40`, `0x0040FCF0`, `0x0040FDD0` RLE). Visual (`Mitani`): entries 5/6/7 mirror 3/2/1 in each 8-direction cycle. The whole sprite (shadow included) is flipped, since shadows are part of the image data. Akhenaten calls it `offset_mirror`, Julius has no such field |
| 20 | i16 | `width` | Pixels | confirmed | Plain: `length = 2·w·h` (8079/8079); RLE covers exactly `w·h` (46225/46225) |
| 22 | i16 | `height` | Pixels | confirmed | same. One slot in the corpus has a negative value (no data) |
| 24 | u16 | `u24` | Probably x in the source bitmap | assumed | Not checked; see §4 caveat |
| 26 | u16 | `u26` | Probably y in the source bitmap | assumed | — |
| 28 | i16 | `group_id` | In the file: the group number at the first image of each group, 0 elsewhere. Overwritten at load time with a global group number | file content confirmed; overwrite **@bin 0x0053DFF0** | 1028/1028 group starts in the 158 normal files; 0 on all 68671 other entries. The 17 enemy files leave it 0 |
| 30 | u16 | `num_sprites` | Animation frame count | assumed (Julius, Akhenaten) | Non-zero in 273 |
| 32 | i16 | — | Always 0 | confirmed | — |
| 34 | i16 | `sprite_offset_x` | Animation anchor | assumed (Julius, Akhenaten) | — |
| 36 | i16 | `sprite_offset_y` | Animation anchor | assumed | — |
| 38 | 10 bytes | — | Always 0 | confirmed | — |
| 48 | u8 | `anim_flag48` | Julius: `animation_can_reverse`. Values 0–3 here, which does not fit a boolean | assumed | 0: 59078, 3: 161, 2: 5, 1: 3 |
| 49 | u8 | — | Always 0 | confirmed | — |
| 50 | u8 | `type` | 0, 1, 10, 12, 13, 20, 30 seen. **30 = isometric** | 30 confirmed; others unknown | Every type-30 image decodes as a footprint (§8.3) |
| 51 | u8 | `compressed` | 1 = RLE (§8.2) | confirmed | Type/flag combos: (20,1) 25548, (0,1) 20634, (1,1) 43; never set on type 30 |
| 52 | u8 | `external` | 1 = data in `<bitmap name>.555` (§7) | confirmed | 483 entries in `Pharaoh_Unloaded` and `Expansion` |
| 53 | u8 | `has_top` | Isometric image has an RLE "top" after its footprint | confirmed | 4943/4943: `has_top = 1` ⇔ `length > uncompressed_length` |
| 54 | u8 | — | Always 0 | confirmed | — |
| 55 | u8 | `tile_size55` | Isometric: footprint size n | confirmed with exceptions | 4936/4943 equal `(width+2)/60`; 7 have 0 |
| 56 | u8 | `bitmap_id` | Index into bitmap records | confirmed | Always < `num_bitmaps`; matches record ranges (§4) |
| 57 | u8 | — | 0 or 1 | unknown | — |
| 58 | u8 | `anim_speed58` | Julius: `animation_speed_id` | assumed | 0 in 59138, 2–8 elsewhere |
| 59 | 3 bytes | — | Mostly 0; byte 61 takes small values | unknown | — |
| 62 | u8 | `is_mirror` | 1 = draw as horizontal flip of entry `index + mirror` | **@bin 0x0053E590** (read by the draw dispatcher) | 1 on exactly the 9561 entries with `mirror ≠ 0`, 0 on all others |
| 63 | u8 | — | 0/1 (also the only non-zero byte in unused slots, §5 intro) | unknown | — |

## 6. Group names (offset 40680 + 64 × `max_entries`, 300 × 48 bytes)

NUL-terminated name (`SPR_MISSILES`, `SPR_MENU_BLOCKS`, ...) for group slot `g`. Bytes after the
NUL are often uninitialised leftovers (`...\0GFX`). Status: layout **confirmed** (constant 14400-byte region in all files, readable names).
That slot *g* names group `g` of §3 is checked on one case only (`Pharaoh_General` slot 11
`SPR_MENU_BLOCKS` → 201, first image of `paneling.bmp`): **assumed** in general. Slot 0 is empty in some files and `SPR_` / `SPR_TAG0` in others.

## 7. Pixel data files (`.555`)

- **Paired**: `X.sg3` ↔ `X.555`. Begins with 4 zero bytes (all 175), then the image data of the
  internal entries in index order, with no gaps, up to EOF. **confirmed** (170 files, §9).
- **External**: (visually checked on `Pharaoh_Unloaded`, 2026-10-04: full-screen backgrounds and
  panels decode correctly) when `external = 1`, the data is in `Data/<stem of bitmap name>.555`. File names
  differ in case (`panelwindows.BMP` → `PanelWindows.555`), so the lookup is case-insensitive.
  Offsets are 1-based and contiguous. **confirmed** (483 entries, 39 files).
- 6 `.555` files are referenced by no `.sg3`: `ckit`, `picture0`, `picture5`–`picture8`. Format
  unknown.

### Pixel format

16-bit little-endian. **X1R5G5B5** (bits 10–14 red, 5–9 green, 0–4 blue). **@bin 0x004CDF20 / 0x004CDEB0**:
when the display mode global `DAT_00e92dfc == 2`, the loader converts every pixel to RGB565 with
`(v & 0xFFE0) << 1 | (v & 0x1F)` (plain/footprint data, skipping 0xF81F) or
`(v & 0x7FE0) << 1 | (v & 0x1F)` (RLE data). The data on disk is therefore 555. Bit 15 is shifted out
(plain) or masked out (RLE): it carries no colour. The statistics below agree.

Evidence: bit 15 is set in 100 % of the plain-image pixels of `Pharaoh_General`, `Pharaoh_Terrain`
and `Pharaoh_Unloaded` (986,258 pixels), so it carries no colour. Read as 555 those files contain
39,335 + 3,975 + 3,612 exact greys, against 6,789 + 16 + 0 when read as 565. UI art is full of
greys, so 555 fits. Visual check passed (human, 2026-10-04) on `mausoleum0_extra`; colours also look natural on
`Pharaoh_Unloaded` (full-screen backgrounds) and `Mitani` (figures). Still **assumed** until the
binary's blitter is read. A few `panelwindows.BMP` images (Roman scenes, likely Caesar III
leftovers) come out red-monochrome; other images in the same bitmap and encoding have normal
colours, so the tint is presumably in the data.

Transparency key in plain images: **0xF81F**. That the value is special is **@bin 0x004CDF20**: the
555→565 conversion leaves it untouched, so it survives as RGB565 magenta. It is drawn as
transparent: **@bin** the six plain-image blitters (`0x0040FED0`, `0x0040FF50`, `0x0040FFD0`,
`0x00410090`, and the masked variants `0x004100F0`, `0x004101C0`) all skip source pixels equal to
0xF81F. The key is only tested in **plain** images: RLE encodes transparency as skips, and
footprints are opaque (§8.3). It is the second most common value in plain
images (241k pixels) and, read as 555, gives Julius' `COLOR_SG2_TRANSPARENT` = `0xF700FF`.
Expanding 5 bits to 8 (`c<<3 | c>>2`) is a display choice taken from Julius, not part of the
file format.

## 8. Image encodings

The choice is made in this order: `type == 30` → isometric; else `compressed == 1` → RLE; else
plain. **confirmed** in the sense that all 59,247 images decode with no leftover byte and no
out-of-bounds pixel under these rules: 8079 plain, 46225 RLE, 4943 isometric. Ran on every
`.sg3`, 2026-10-04.

### 8.1 Plain
`width × height` u16 pixels, row-major. `length = 2·w·h` (8079/8079).

### 8.2 RLE
Byte stream, read until `length` bytes are consumed. Pixels fill the image row-major:
- `0xFF n`: skip `n` transparent pixels;
- `c` (0–254): `c` raw u16 pixels follow.

Stream format **@bin 0x004CDEB0** (0xFF + 1 byte = skip; else count byte then count u16 pixels).
Note: the count is read as a *signed* char, so a run ≥ 128 would desynchronise the game's
converter. No such run exists: the longest run in all files is 16 pixels.
Every RLE image covers exactly `w·h` pixel positions: 46225/46225. (Akhenaten and Julius: same
scheme.)

### 8.3 Isometric (type 30)
`n = (width + 2) / 60` (exact for 4943/4943). The first `uncompressed_length = 1800·n²` bytes are
the footprint: n² diamond tiles of 58 × 30 pixels (900 pixels each; row y of a tile spans
`x ∈ [s, 58−s)` with `s = 28,26,…,2,0,0,2,…,28`). Tiles are stored row by row from the top of the
diamond: rows of 1, 2, …, n, then n−1, …, 1 tiles. Tiles in a row are 60 px apart, each row
starts 30 px further left and 15 px lower. The diamond is anchored at the bottom of the image.
If `has_top`, the rest of the data is an RLE stream (§8.2) over the full `width × height`
starting at (0, 0), drawn over the footprint.

Status: tile size and byte counts **confirmed** (4943/4943). Tile order and placement taken
from Akhenaten (`convert_isometric_footprint`). Visual check passed on `mausoleum0_extra`
(human, 2026-10-04): footprints are clean diamonds and tops sit on them. Then on `Pharaoh_General`
(2026-10-04) for n = 2 to 6 (84, 35, 10, 3, 1 images): footprints tile without seams and buildings
line up with them (e.g. entry 808, 6×6 palace).

Footprint drawing **@bin**: the dispatcher `0x0053E590` picks a routine by width (58 → `0x00427A20`,
118 → `0x0042C740`, 178 → `0x0042C9A0`, 238 → `0x0042CEE0`, 298 → `0x0042D810`, 358 → `0x0042E680`).
The 2×2 routine draws 4 tiles from byte offsets 0, 1800, 3600, 5400 at (x, y), (x−30, y+15),
(x+30, y+15), (x, y+30), which is the order and spacing above. The single-tile routine
`0x00427A20` is an unrolled copy whose row spans match the `s = 28, 26, 24, …` table (row 0: 2 px at
x = 28; row 1: 6 px at x = 26; row 2: 10 px at x = 24). It copies pixels with **no key test**.
Tile layout is therefore **@bin** for n ≤ 2; the n = 3…6 routines were not read, but they are
visually consistent.

Transparency key in footprints: the game draws footprint pixels opaque (see above), and so does
the extractor. 4 footprints are
filled entirely with 0xF81F (`Pharaoh_General` 1349–1352, groups `SPR_B_BATHS2` /
`SPR_EMPTY_BATHS2`, no top, likely Caesar III leftovers) and render as magenta diamonds; 43 others
contain 1–74 key pixels. The game would draw those as magenta too.

## 9. Anomalies

- **Dummy files** (`CaesareumDummy`, `TombRamDummy`, `TombSetiDummy`, `TombThutDummy`,
  `TombTutDummy`): entries after the system images have `offset = 0` and `bitmap_id = 0` even
  though their bitmap record (`dummy.bmp`) claims them. **Resolved**: the game ignores stored
  internal offsets and lays the data out sequentially (§10), so these files load correctly. The
  extractor now does the same.
- `Pharaoh_Terrain` entry 994 has `bitmap_id = 5` inside the range of record 4.
- `h8` (§2) is off by −2 / −10 bytes from the `.555` size in 3 files (`LibMaina3`, `LibMainb2`,
  `sphinx4a`).

## Open questions

1. ~~Locate the loader~~: done (§10). Still to do: recover the loader's stack frame in Ghidra (the
   header and record offsets below are inferred from stack offsets) and read the blitter.
2. ~~Header and entry fields~~: §12. The loader reads only header +16/+20; h0 and h2 remain
   unexplained but unused. Entry +24, +26, +30, +48, +55, +57, +58, +61, +63 are never read.
3. ~~`type` values~~: §12 (dispatch @bin 0x0053E590).
4. ~~`mirror`~~: horizontal flip, @bin (§5).
5. ~~Dummy files' offsets~~: resolved (§9, §10).
6. ~~The 6 unreferenced `.555` files~~: never opened by the exe (§12).
7. ~~The 7 isometric images with `tile_size55 = 0`~~: +55 is never read (§12).
8. ~~0xF81F in footprints~~: drawn opaque, @bin (§8.3). Are the bath images ever drawn?
9. ~~Pixel format, display mode, colour key~~: @bin (§7, §10).
10. `DAT_00d3b531` / `DAT_00d3b530` are clip states, not draw modes: `0x004CDAD0` sets the
    horizontal one (0 none, 1 left, 2 right, 5 off-screen), `0x004CDB80` the vertical one (0, 3 top,
    4 bottom, 5). `DAT_0070e120` is a **tint mask** (@bin): when non-zero, footprint blitters (`0x00428C00`,
    `0x0042AA00`, `0x0042BCF0`) write `pixel & mask` (the mask repeated in both halves of a
    dword), and sprite draws (`0x0053E590`, `0x0053FA40`, `0x0053F250`) run a second masked pass
    (`0x004100F0`, `0x004101C0`). Values (RGB565): **0xF863** keeps red (set for a building whose
    byte +0xBF ≠ 0 in the city draw `0x00430EF0`, or whose u16 +0x4A ≠ 0 in `0x00439980`, and in
    `0x004C0160`, `0x0043F5B0`); **0x1FE3** keeps green (construction previews `0x0043D030`,
    `0x0043B630`, `0x0043D720`, …). `0x004C2F50` also uses building u16 +0x3A as the mask. The
    game logic behind those building bytes (e.g. marked for deletion) is *assumed*.

## 10. Loader in `Pharaoh.exe` (2.1.0.0)

Found from the only code references to the strings `".sg3"` (VA 0x006018F0, referenced at
0x0053E0D5) and `".555"` (VA 0x00601900, referenced at 0x0053F6C2). Read from Ghidra 12.1.4
decompilation (`make analyze`, output in `local/ghidra-export/`, not committed). Ghidra did not
recover the 0xD8A00-byte stack frame of `0x0053DFF0`. Below, the mapping of stack offsets to file
offsets is my reconstruction: header at frame offset 0x114, so group table at 0x164 (= +80),
bitmap records at 0x3BC (= +680), entries at 0x9FFC (= +40680). It fits all of the following.

| Address | Role (our name) | What it shows |
|---|---|---|
| 0x0053DD90 | `sg3_load_family` (caller) | Wraps the loader. Checks the group count against an expected table and shows "Graphic file out of sync" if they differ |
| 0x0053DFF0 | `sg3_load_index` | Builds `<dir><name>.sg3` and reads the whole file in one go (`0x004CEB60`). If the first bitmap is `system.bmp` (and a flag argument is set), skips the first 200 images. Copies `last_index − skipped` entries of 64 bytes starting at entry 1 + skipped (§2 `last_index`, §5 entry size). Walks the 300 group slots (§3), and for each non-zero one writes a global group number into field +28 of that group's first entry (§5). Copies `num_bitmaps` names from 200-byte records (§4). Calls `0x0053F660` for the `.555`. Then, for each entry: if `external == 0` (+52), sets `offset` = running pointer starting at buffer + 4 and advances by `length` (+4), ignoring the stored offset; if external and `offset == 0`, sets it to 1. Converts the pixels: type 30 (+50) → `0x004CDF20` on the footprint, plus `0x004CDEB0` if `has_top` (+53). Other types → `has_top` forced to 0, then `0x004CDEB0` if `compressed` (+51) else `0x004CDF20` |
| 0x0053F660 | `sg3_load_pixels` | Builds `<dir><name>.555`, gets the size (`0x004CEA30`) unless given, allocates and reads (`0x004CEB60`) |
| 0x004CDF20 | `pixels_555_to_565` | Plain/footprint conversion when `DAT_00e92dfc == 2`; skips 0xF81F |
| 0x004CDEB0 | `rle_555_to_565` | Same for RLE streams; parses the RLE format of §8.2 |
| 0x004CEB60 | `file_read_at` | Opens a file (retries via `0x004CE9E0`, probably the CD path), seeks and reads |
| 0x00426FF0 | `dd_create_surfaces` | DirectDraw surface setup. Reads the primary surface's pixel format and sets `DAT_00e92dfc`: red mask 0xFC00 → 3 (655), green mask 0x07E0 → 2 (565), blue mask 0x003F → 1 (556), else 0 (555). Modes 1 and 3 are logged as unsupported and forced to 2. So the game runs in 555 (data used as stored) or 565 (converted at load) |
| 0x0053E590 | `image_draw` | Draw dispatcher, 205 callers. Looks up the 64-byte runtime entry at `this + 0x180E48 + id·64`, handles mirroring (+62/+16), picks a blitter by type (+50), `compressed` (+51) and, for type 30, width |
| 0x0040FFD0, 0x00410090 | `blit_plain`, `blit_plain_clipped` | Plain image, left to right, skips 0xF81F |
| 0x0040FED0, 0x0040FF50 | `blit_plain_mirrored`, `..._clipped` | Same, right to left |
| 0x004100F0, 0x004101C0 | `blit_plain_mask` | ANDs a colour mask into the destination where the source is not 0xF81F |
| 0x00427A20 | `draw_footprint_tile` | One 58×30 tile, opaque |
| 0x0042C740 | `draw_footprint_2x2` | 4 tiles at 1800-byte steps |

## 11. Slot reloads after startup (**@bin**, decompiled)

The 30 startup slots (`0x00414420`) fix the global id layout. Later loads go through the per-slot
loader **`0x0053DD90`** → `0x0053DFF0`:

- If the slot already holds a file with the same name (case-insensitive), nothing happens.
- Otherwise the new file is copied in at the slot's stored image base and group base. The running
  totals move only the first time a slot is filled, so **global image ids and group numbers never
  change**; only the entries and pixels do. The slot's image count becomes max(old, new).
- The new file's group count is checked against a per-slot table. A mismatch shows "Graphic file
  out of sync" (what fills that table was not traced).
- File data: every variant has the same group count as its startup file, and the same number of
  images or fewer, with one exception. `TombSetiFin` has 226 images against 225 for
  `TombSetiDummy` in slot 28, so its last entry would land on slot 29's first id (*assumed*, not
  tested).

**Scenario start `0x004D52A0` and save load `0x004D5820`** both call these three functions
(offsets are into scenario_info, 0x007848D8):

| Function | Slot | Files | Selected by |
|---|---|---|---|
| `0x004BC720` | 2 | 12 enemy files, table `0x005DEF84` (HYKSOS … PERSIAN) | i32 +0x24, clamped to 11 |
| `0x0053D970` | 9 | SPHINX / OBELISK / SUN_TEMPLE / MAUSOLEUM0–2 `_EXTRA` | name +0x3C = "The Glory of Ptolemy" → mausoleum; else the monument list at +0x508, through the type table `0x005D19C8` |
| `0x004F4520` | 17–29 | byte +0x3E4: < 2 → slot 17 gets a pyramid file (`0x004F44E0`); 2 → Tomb{Thut,Tut,Seti}[Fin] in slots 25/27/28 (Fin by mission index `0x00E38EA8`), TombRam in 29; 4 → AbuSimbel in 26; other → Caesareum1, LtHouse1, LibMain{a,b,c}1, LibExtra, MausCarryover in 18–24 | +0x3E4 and the mission index |

**While drawing**, some slots are reloaded per building, so they hold the file of whatever was drawn
last:
- Slots 5–8: SPHINX1A–2B (`0x0053D850`), maus* (`0x004DF360`), Obelisk* (`0x004E5840`,
  `0x004E5950`) and suntemple1–3 (`0x005414B0`).
- Slot 10: TEMPLE_NILE / RA / PTAH / SETH / BAST (table `0x005E018C`, `0x00470990`,
  `0x004C0F30`).

**What a load writes in the game state** (**@bin** `0x0053DD90`, `0x0053DFF0`, asm; measured in the
oracle on Sais, slot 5 maus0_1a → maus0_2a: 26 bytes; slot 29 TombRamDummy → TombRam: 1,265 bytes).
`this` = 0x00F55BB0:

| Where | Written | From |
|---|---|---|
| `this + 8·(first group + k)` | group record {slot, first image id + first index − 1} for the k-th non-zero group, in group order | `.sg3` group table (+80), minus 200 when the first bitmap is `system.bmp` |
| `this + 0x1F40 + 0xCC3C·slot` | file name, bitmap names (+0x104 each, 200-byte records), +0xCC28 `.555` size, +0xCC2C max(old, new) image count, +0xCC30 group count, +0xCC34 / +0xCC38 first image / group | `.sg3` header (+16, +20), bitmap records (+680); `.555` file size (−0x992E with `system.bmp`) |
| `this + 0x180E48 + 64·id` | the file's entries 1 + skip …; +12 := slot, +0 := pointer into the pixel buffer (internal images) or 1 (external, when 0), +53 := 0 (internal, not type 30, length > 0), +28 := group number on each group's first entry | `.sg3` entries (+40680) |
| +0xCC24, 0x00D3B5F4 | the pixel buffer (graphics heap) and the running pointer into it | `0x0053F660` |
| `this + 0x30F5D0` | total pixel bytes (− old size + new size) | |
| `this + 0x30F5C8 / CC / D4` | running totals of groups, images, slots (first fill of a slot only) | |

The group records are simulation state: image ids taken from them (0x00F56B84, 0x00F57544 …) are
stored in the saved grids. A reload changes them when the new file's group table differs from the
slot's current file: `AbuSimbel` / `AbuSimbelDummy` (slot 26), `LibExtra(2)` / `LibExtraDummy`
(slot 23), `Tomb*` / `Tomb*Dummy` (slots 25, 27–29). The files of the other reload families (maus*,
sphinx*, obelisk*, suntemple*, temples, caesareum*, LtHouse*, LibMain*) all share one group table
per family. The rest (entries, names, pixel pointers and sizes) is read only by drawing and by
`0x0053DD90` itself (tick read set, `tools/oracle/readset.py`: no read of these pages in a month on
three cities, except the font entries' widths, `0x004CBF30`, whose slot is never reloaded).

So `0x0053DFF0` is not presentation only. It is ported (`core/graphics/sg3.c`): the `.sg3` bytes
come from the platform (`platform_file_read`, docs/formats/state.md "Game data outside the
segments"); the group list nodes (`0x00540540`, `0x005404B0`, `0x00540570`, `0x00540590`, graphics
heap) are kept on the host; the `.555` read `0x0053F660` stays an original call (it allocates the
pixel buffer in the graphics heap and writes only its address, +0xCC24); the 565 conversions
`0x004CDF20` / `0x004CDEB0` act on that buffer only. `0x0053DD90` (the slot bookkeeping) is ported
in `core/graphics/last_graphics.c`.

**For `render_map.py`:** terrain groups are not in these slots, so terrain renders are correct with
the startup files. Enemy, monument and temple images would need the selections above.

## 12. Fields the game reads, `type` dispatch, leftovers (**@bin**, decompiled and asm)

Statistics are over all 175 `.sg3` files (`parse_sg3`).

**Header.** `0x0053DFF0` reads only `last_index` (+16) and `num_bitmaps` (+20): the only reads in
the header range are at `0x0053E19F` and `0x0053E394`. The `.555` loader `0x0053F660` gets size 0
(`0x0053E3F2`) and takes the size from the file system. File relations (unused by the game,
*assumed* meaning):

| Field | Values |
|---|---|
| h0 | 695,080 (154 files), 887,080 (17 enemy files), 74,480 (General, Terrain, Fonts, Unloaded) |
| h2 | 0 in the 17 enemy files, 1 in 143, otherwise 2–25; matches no count we tried |
| h6 | `num_bitmaps − 1` (158 files); 0 in enemy files |
| h7 | decoded size of the internal images: 2·w·h, or 2·w·(h+1) for type-30 images with a top (158/158) |
| h8 | `.555` size − 4; enemy files: the full `.555` size (17/17) |
| h9 | h7 − h8 (bytes saved by compression); 0 in enemy files |

**Entries.** Nine functions use the runtime entry table (`this+0x180E48`): `0x0053E590`,
`0x0053FA40`, `0x0053F250`, `0x0053F4F0`, `0x0053F4A0`, `0x0053E530`, `0x00438180`, `0x0053DD90`,
`0x0053DFF0`.

| Off | Use |
|---|---|
| +0 offset | must be > 0 (all) |
| +4 length | byte count for loading an external image (`0x0053E590`) |
| +8 uncompressed_length | the isometric top's RLE starts at `offset + this` (`0x0053F250`) |
| +12 | **written** at load (`0x0053E44E`): the family slot index; used with +56 to find the external file name (`family·0xCC3C + 0x2044 + bitmap_id·0x104`) |
| +20, +22 | width, height |
| +34, +36 | anchor: x = +34 + x0, y = +36 − height + 30 + y0 (`0x00436B60`, `0x00439F10`) — confirmed |
| +50–53 | type, compressed, external, has_top |
| +56 | bitmap_id (external `.555` name) |

Never read: +24, +26, +30 (`num_sprites`), +48, +55, +57, +58, +61, +63. Frame counts and
animation speeds therefore come from the code, not the file.

**`type` dispatch (`0x0053E590`).** Values in the files: 0, 1, 10, 12, 13, 20, 30. The code also
handles 2, which no file uses.

| type | Entries | Drawing |
|---|---|---|
| 0 | 23,161 | the only type where +51 selects RLE or plain; both skip the 0xF81F key |
| 30 | 4,943 | isometric footprint chosen by width (§10). Clipped width-58 tiles: `0x0042A160` / `0x0042B440`. Widths 26 and 10 call the empty `0x0051F780` |
| 20 | 25,548 (`system.bmp`, `fonts.bmp`) | glyph mask: walks the RLE stream and writes one colour (`DAT_00d3b51c`) per pixel (`0x00410B10`, `0x00410DE0`, `0x00410E80`) |
| 10, 12, 13 | 1,634 / 1,925 / 158 | always 16×16, 24×24, 32×32 |
| 1 | 1,878 | full-screen and panel art |

Types 1, 2, 10, 12 and 13 share one path (`0x0053F0FC`–`0x0053F228`):
- fully on screen: opaque copy `0x0040FB60`, no key test;
- clipped only vertically: `0x0040FFD0`, which skips the key;
- clipped horizontally: type 1 goes to `0x00410090` (keyed). Types 2, 10, 12 and 13 go to
  fixed-size opaque copies (`0x00410FB0`, `0x004114A0`, `0x00411FD0`, `0x00410F70`) that do not
  clip.

The data fits "type ≠ 0 is opaque": no type-10/12/13 image contains 0xF81F, and only 20 of 1,835
plain type-1 images do. Type 1 never looks at +51, so the 43 type-1 entries flagged RLE would be
copied as raw pixels (*assumed* never drawn). External images wider than the screen are cropped,
centred, to 1024×768 (`0x0053F7D0`).

**Isometric images with `tile_size55` = 0** (AbuSimbel 210, 237; TombRam 319, 323, 327, 331,
349): +55 is never read. The footprint and the top are placed by width, and n = (w + 2) / 60
agrees with `uncompressed_length = 1800·n²` for all 7. They draw like any other image;
`extract_graphics.py` already derives n from the width.

**Unreferenced `.555` files (`Data/`).** The exe has no `ckit` or `picture` string (ASCII or
UTF-16), and no bitmap record names these files: they are never opened (*assumed* build
leftovers).
- `Picture5`–`8`: 0 bytes.
- `Ckit` (1024×768, plain): an older `FE_Map Editor.555`, which the game uses through
  `Pharaoh_Unloaded` entry 505.
- `Picture0`: an exact prefix (840,200 bytes) of `History1A.555`.
