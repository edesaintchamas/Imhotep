# Text resource (`Pharaoh_Text.eng`)

Reference: GOG Pharaoh + Cleopatra 2.1 (`Pharaoh_Text.eng`, 269,804 bytes; its source is
`Pharaoh_Text.txt`, compiled by the developers' `csr2text`). Tool: `tools/extract/read_text.py`.
Analysis dated 2026-10-04.

## Layout

| Offset | Size | Content | Status |
|---|---|---|---|
| 0 | 16 | `Pharaoh textfile` | confirmed |
| 16 | 4 | 315 = highest group in use (314) + 1 | confirmed |
| 20 | 4 | 6,859 (number of strings, one less than the NUL count 6,860) | assumed |
| 24 | 4 | 43,210 | unknown |
| 28 | 8 000 | 1,000 × {u32 offset into the data, u32 in use} | confirmed (layout of Julius's `c3.eng`, *hint*; 314 groups in use; groups 186 and 187 share one offset) |
| 8,028 | … | NUL-terminated strings, group after group | confirmed |

**Lookup — @bin 0x004CCBB0**: start = data + offset of the group (table at 0x00E34C38, 8 bytes
per group), then skip `index` strings. A NUL counts as a terminator only if the byte before it is
NUL or ≥ 0x20 (control bytes 1–31 may precede an embedded NUL); leading control bytes of the
result are skipped. The reference file has no NUL after a control byte, so splitting on NUL gives
the same numbering. Check: groups 1 and 2 equal sections `*1`, `*2` of `Pharaoh_Text.txt` line
for line.

## Groups used for names

| Group | Indexed by | Evidence |
|---|---|---|
| 41 | building type (record +0x10) | **@bin 0x004FAB20** (info panels: `0x004D62F0(0x29, type, …)`) |
| 64 | figure type (record +0x0A) | **@bin 0x004FAB20** (`0x004D62F0(0x40, figure type, …)`) |
| 28 | building type, construction-menu wording ("House1"…) | **@bin 0x00520000** (build menus: table 0x005CE3BC, 30 building types per menu, item names drawn from group 28 by type) |
| 29 | house levels ("Crude Hut"…) | *assumed* |

## Message file (`Pharaoh_MM.eng`)

590,127 bytes. **Loader @bin 0x004CEC40** (also loads `Pharaoh_Text.eng`: 28 + 8,000 bytes then the
data): reads 0x13898 = 80,024 bytes to 0x00E209A0 (header + entries), then the rest of the file
as the string data (pointer 0x00E36C74). Tool: `read_text.py --messages`.

| Offset | Size | Content | Status |
|---|---|---|---|
| 0 | 16 | `Pharaoh MM file.` | confirmed |
| 16 | 4 | 1,000 = number of entries | confirmed (entries fill exactly 80,000 bytes) |
| 20 | 4 | 497 = entries in use | confirmed (497 non-zero entries) |
| 24 | 1,000 × 80 | message entries | confirmed size |
| 80,024 | … | NUL-terminated strings | confirmed |

Entry fields (entry base 0x00E209B8). *Confirmed* fields are read by the game at that offset;
names are Julius's (`lang.c`, *hint*):

| Offset | Type | Julius name | Status |
|---|---|---|---|
| 0 | i16 | type | read by 0x004C6C70 (meaning *assumed*) |
| 2 | i16 | message_type | read by 0x004C6C70, 0x004E0CF0, 0x004E1A50 (meaning *assumed*) |
| 10, 12 | i16 | width_blocks, height_blocks | read by 0x004C6C70 (meaning *assumed*) |
| 56 | i32 | urgent | read by 0x004E0CF0, 0x004E1320 (meaning *assumed*) |
| 60 | i32 | video text offset | **confirmed**: data offset (0x004C6C70, 0x004E0CF0, 0x004E1320) |
| 68 | i32 | title offset | **confirmed**: data offset (0x004C6C70, 0x004E1A50); entry 1 = "Potter" |
| 76 | i32 | content offset | **confirmed**: data offset (0x004C6C70) |
| others | | x, y, image, title/subtitle/video positions, subtitle offset (72) | *assumed* (Julius) |

Every non-zero offset points into the data and ends at a NUL (497/497 entries).

Text drawing: `0x004D62F0` / `0x004D6250` (position from the current screen layout) →
`0x004CCBB0` / `0x004CCB30` (group, index, x, y, …).
