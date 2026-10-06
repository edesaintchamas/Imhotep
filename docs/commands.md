# Player commands (roadmap phase 5)

Reference: GOG Pharaoh + Cleopatra 2.1. This file lists, for every player action that changes the
simulation state, the function of `Pharaoh.exe` that applies it (the *command function*: the lowest
function that takes the player's decision as input and does all the state change, below mouse and
window handling), its inputs, and how the UI reaches it. It is a survey: nothing here is ported yet.

Evidence marks used below:

- **dis**: read in the disassembly (`tools/oracle/disasm.py`) or the instruction-level xref of all
  exported functions (capstone over `local/ghidra-export/index.json`).
- **dec**: read in the Ghidra decompilation only (layout, conditions); not yet checked instruction
  by instruction. Good enough to locate; a port re-reads the disassembly.
- **run**: the original function was called in the oracle (`tools/oracle/game.py`, save
  `local/saves/downloaded/1592/39-Actium-Dec30c.sav` unless said otherwise; Iken =
  `local/saves/downloaded/1564/CaesarAlan-Iken.sav`) and the bytes it changed were compared with the
  save layout (`diff.schema()`): the changes listed are the observed ones.
- **assumed**: an interpretation (usually the player-facing meaning of a field) not established by
  the binary. Building-type names come from Akhenaten's `building_type.h` and are hints only.

Conventions: every command function found is **cdecl** (`ret` without operand, checked for all of
them with the xref tool, dis); "no args" means the inputs are globals. `P` is the current player
index `u32 0x00EAAC28` (1 in all saves seen) and `CITY` the current player's city record
`0x00EA1870 + P·0x49D8` (save chunk 28); city fields are given as `CITY+off`. All the globals named
below are inside the state mirror (0x0056F000–0x01267AA8: the whole `.data`), so a port and a test
can set them in both the oracle and core. Whether the *tick* reads a given UI global was not
established (the readset is page-granular and every page involved is read by the tick for other
variables).

## 1. How the UI reaches the simulation

### 1.1 Input dispatch (dis/dec)

- `0x004D6B60` (per-frame input) calls `0x004D8310`, a 22 KB dispatcher on the current window id
  `u8 0x00E38DDA` (dec; names from what each branch calls: 1 = city view, 3 = city view with an extra menu
  (*assumed*), 6 = build menu, 7 = advisors, 9 = building/figure info, 0x11 = message list,
  0x1E/0x1F/0x20 = choose a target for a legion / transport ship / warship (corrected, §7), 0x3F = yes/no popup;
  other ids are menus, editor, videos).
  `u8 0x00E38DCA` = 1 asks for a redraw (UI only).
- Mouse state: x `0x00E38E34`, y `0x00E38E30`; `u8 0x00E38E03` left button pressed this frame,
  `0x00E38E00` left button held (drag), `0x00E38E04` left button released, `0x00E38DF8` right
  click (dis/dec: their use in the city-view branch below and in the button handlers).
- Map tile under the mouse: `0x00405F80` computes it every frame from `0x00430790(0)`:
  `0x00BCA2C4` grid offset (0 = off map), `0x00BCA2C8` x, `0x00BCA2CC` y; copied to
  `0x00BCA300`/`0x00BCA304`/`0x00BCA308` unless `u8 0x00605A5C` is set (dec). Grid offset =
  `u32 0x00BCA2D8` (map start) + y·0xE4 + x (dis: used in every command below).
- City view (window 1), dec: left press → `0x0040B630`, `0x0040B790`, `0x0040B6D0` (clicks on
  buildings / figures that open info windows), else construction start `0x00405C20`; held →
  `0x00405D90` (drag preview); released → `0x00405E90` (place); right click → `0x00405BA0`
  (cancel construction).
- Gate: many commands return immediately if `u8 0x00E38E60` ≠ 0. Only writes found: three stores of
  0 (`0x004147F0`, `0x00417FF0`, `0x004D52A0`; dis), so it is always 0 in single-player.

### 1.2 Button tables and callback parameters (dis/dec)

Windows hand a table of buttons to one of three handlers. Each entry carries a callback and two
parameters; before calling the callback the handler stores them in **`0x00D3B198` (param 1)** and
**`0x00D3B194` (param 2)**. Every callback below reads its parameters from there.

| Handler | Entry size | Layout | Notes |
|---|---|---|---|
| `0x00425990` text buttons | 0x24 | +0 x, +2 y, +4 w, +6 h (i16), +8 kind (i16), +0x10 left callback, +0x14 right callback, +0x18 enabled (u8), +0x1C param 1, +0x20 param 2 | left press, right click, or held for kind 6 |
| `0x004261D0` image buttons | 0x1C | +0 x, +2 y, +4 x end, +6 y end (i16), +8 left callback, +0xC right callback, +0x13 kind (1–4), +0x14 param 1, +0x18 param 2 | |
| `0x00425C40` arrow buttons | 0x18 | +0 x, +2 y, +6 size, +8 callback, +0xF kind (2, 4 auto-repeat, 5), +0x10 param 1, +0x14 param 2 | auto-repeat reads `timeGetTime` |

The tables are in `.data` (e.g. `0x005C4F00`, `0x005C68C8`); some are chosen at run time
(`0x005C81F8`: storage limits; `PTR 0x005F78F0[15]`: advisor shortcuts). Table → callback lists
were extracted by scanning these layouts (§6).

### 1.3 Confirmation popup (dis)

`0x004264D0(text_id, callback, kind)` saves the window id in `0x0070570C`, sets window 0x3F and
stores `callback` in `0x00705704`, `text_id` in `0x00705710`, `kind` in `0x00705708` (2 = yes/no,
0/1 = OK only), and clears `u8 0x00D3B439`. The popup's buttons (table `0x005C8168`, handled by
`0x00426530`) are `0x00426590` (yes: `0x00D3B439` := 1) and `0x004265B0` (no: := 0); both call
`*0x00705704` and restore the window. **Confirmed commands are therefore small callbacks that test
`0x00D3B439 == 1`** (trade route `0x004022D0`, request `0x00402340`, clearing `0x004021D0`…).

### 1.4 Building / figure info window (window 9, `0x004F96E0`, dec)

Context globals, set when the window is opened and by its drawing code: `0x00EADD6C` building id,
`0x00EADCCC` formation id, `0x00EADD50` sub-kind (2 building, 4 legion, 5 transport ship, 6
warship), `0x00EADCC8` / `0x00EADCC4` transport / warship figure id (corrected, §7), `0x00EADCE4` selected row
of a figure list. Storage windows compute the resource lists shown: `0x00F53DD8` count +
`0x00F53DDC[]` (granaries and storage yards' resource ids for the order rows), `0x00F53E6C`
count + `0x00F53E70[]` (storage yard list), and keep a scroll row in building `+0xE0`. These lists
are built by the window code, not by the simulation: a test must set them.

### 1.5 Proposed test boundary (general)

For each command: set the listed input globals (and `0x00D3B198`/`0x00D3B194`) in the oracle and in
the port, call the command function, compare the whole state (`tools/oracle/harness.py` already
does this for `ADDR:ARGS`; the inputs need a `--fill`-like "set globals" step). The port's entry
point is the same function, registered in the ported table, so `GAME_CALL` reaches it. UI-only
effects inside a command (`0x00E38DDA`, `0x00E38DCA`, sounds `0x0047D350`, popups `0x004264D0`,
city notices `0x004E2970`, message box `0x00538CD0`) stay in the port as calls / stores: they are
cheap to reproduce and keep the byte comparison exact; sounds and message boxes are platform calls.

## 2. Construction and clearing (roadmap 5.1)

### 2.1 Sequence (dis for the calls and arguments, dec for the conditions)

| Step | UI trigger | Function | What it does |
|---|---|---|---|
| choose a building | build menu button (window 6, table at `0x005C28A8`, 0x1C entries; `0x00403E00` / `0x00403D70`) | `0x00405200` (no args; reads submenu `0x007056EC`, item `0x00D3B198`) | **set type**: `0x006059E8` := type (game menus: `0x005CE3B8[submenu·30 + item]`; monuments menu 0x1C: from the scenario list `0x00784DDE`), clears and sets the placement flags (§2.3), sets the window to 1. If a ferry is half placed (`0x00BCA310` = 1) it deletes it: `0x00486060(0x00BC9800)` sets building state := 6. Also the editor's tool menus (submenus 0xE–0x13, 0x1A, 0x1B, 0x25). |
| mouse down | city view, left press | `0x00405C20` (no args) | **construction start**: needs `0x00BCA300` ≠ 0 and `0x00E38E60` = 0. Start = end = hovered tile: `0x006059E0` x1, `0x006059D8` y1, `0x006059DC` x2, `0x006059D4` y2, `0x006059D0` start grid offset, `0x00BC96CC` := elevation grid byte at the start, `0x00605798` := 0. Except for the first landing of a ferry (type 0x88 with `0x00BCA310` = 0), calls `0x00484F60` (undo snapshot, below); if it returns 0 the construction does not start. Then `0x006059E4` := 1 (in progress); roads (5), canals (8) and water lifts (7) call `0x0051B680(1, x, y, flag)`, walls (6, 0xA8, 0xA9) `0x0051C790(1, x, y)` (drag path start). |
| drag | left held | `0x00405D90` → `0x0046FEF0(x1, y1, x2, y2, type)` | end := hovered tile (unless `0x00605A5C`), `0x00BCA2E4` := its grid offset, `0x00605798` := 1; **preview**: places a ghost into the real grids (restored by the place step from the backups) and computes ferry state `0x00BCA310` / `0x00BCA315` / `0x00BC9800`. |
| mouse up | left released | `0x00405E90` (no args) | **place**: if `0x006059E4` = 0, nothing. `0x006059E4` := 0. If the mouse is off the map (`0x00BCA2C4` = 0) uses `0x00BCA2E4` when a drag happened (`0x00605798`), else cancels. Sound `0x0047D350(4)` if type > 0. Calls **`0x004728A0(P, u32 0x00BCA2E0, x1, y1, x2, y2, type)`** (push order checked at 0x00405F2B–0x00405F54). Editor-only: map-editor flag `0x00605A3C` = 1 → `0x005178B0` first; types 0x7B, 0x7C, 0x81 → `0x004795E0`. |
| cancel | right click | `0x00405BA0` | clears type and tiles; deletes a half-placed ferry (`0x00486060`). |
| undo | sidebar button (table `0x005C7370` entry 12) | `0x00406010` → **`0x00485230`** (no args) | restores the last construction (§2.6). |

`0x00BCA2E0` is the city view orientation (copied from `0x00E94E04` by the rotate command
`0x00409EB0`, §3.11); `0x004728A0` uses it as "param_2" to anchor multi-tile buildings.

### 2.2 The place function `0x004728A0(player, orientation, x1, y1, x2, y2, type)`

cdecl, 7 args, no return value (dis: prologue, `ret`). Reads (dis, direct memory operands):
placement flags `0x006059B4`–`0x006059CC`, `0x00605A48`, building orientation `0x00BC9714`, pyramid
variant `0x005E6970`, hovered tile `0x00BCA300`, map size `0x00BCA2D0`/`0x00BCA2D4`, map start
`0x00BCA2D8`, ferry state `0x00BCA310`/`0x00BCA314`/`0x00BCA315`, debug "free monuments" flag
`0x00605A0C` (written only by `0x00413ED0`, start-up; 0 in normal play), state counters
`0x00705C1C`, `0x00705C6C`, `0x00BCB9B0`, `0x00BCB9B4`, `0x00BC96F8`, `0x00BC971C`, and city data.

Order of work (dec, main path confirmed by the runs in §2.7):

1. `0x0046FDE0(type)`: 1 for "drag on roads allowed" types (5, 8, 9, 0x5A, 0x64–0x69, 0xC4,
   0xE0), used as the last argument of the land check.
2. Clears `0x00E94E20`, cost `0x00BC9728`, refund `0x00BC9718`. Type 0 → return.
3. Treasury `CITY+0xC` < −4999 → restore bitfields grid (`0x0040F920`), notice 3, return.
4. Type ≠ 9 and `0x004B5830(x1, y1, x2, y2)` ≠ 0 (figures in the way, *assumed*): restore grids from
   the backups (`0x0040F860` copies) for walls / roads / canals / plaza / gardens, notice 0x32,
   return.
5. Per-type checks and work (§2.4). Failures post a city notice `0x004E2970(id, 0, 0)` and return
   (some also restore the bitfields grid with `0x0040F920(0x00B285CC, 0xCB10, 0xAF)`).
6. Single buildings: `0x00470900(x1, y1, x1+s−1, y1+s−1)` refuses a rectangle that covers a
   scenario special point (table `0x00784B64`… / `0x00784CC0`…; notice 0x3D). The anchor tile is
   moved by `orientation` (2, 4, 6 shift by size − 1). Land check
   `0x0047C880(x, y, size, size, terrain_mask, 0, allow_roads)` (ported: `land_block_fits`); mask
   −1 by default, −0x41 for gatehouses / temple-complex parts, −0xC04001 for towers, 8 for 0x8A.
   Then the required-terrain check selected by the flags (§2.3).
7. Per-type follow-ups `0x004E3330`, `0x004E2D30`, `0x004E3100` … `0x004E5070`, `0x004E2B10`
   (~30 calls with the type; *assumed* advisor / tutorial triggers; they wrote 0 bytes for a well,
   frontier run) and `0x0051F780` (empty).
8. Create: `0x0046A400(player, type', x, y, subtype, 0)` (ported: `building_create`; forts 0x28,
   0x2C, 0x2D create type 0x39 first; pyramid 0xB7 with subtype 0x90; monuments with their slot
   counts: 0xD5 100, 0xE6 0x10, 0xE1 0x19, 0xE3 9, 0xE5 0xA7, 0xEA 0x9E, 0xEB 0x1B5, 0xEC 0x201,
   0xE4 2, 0xDE 0x74). Non-monuments: `0x00485100(id)` (ported: `building_keep_removed`; records the
   building for undo). Then `0x00470990(player, type, id, x, y, size, 1, 0)` (map add; also used
   by the loader) and for a ferry `0x00486020` (two-step placement).
9. Common tail: routing rebuilds `0x00517370`, `0x00517660`, `0x005178E0` (ported),
   `0x004B9EE0(x, y)`; ferry with `0x00BCA310` = 0 costs 0; cost −= `0x00BC9718`;
   **`0x004E9680(player, cost)`**: if `u8 CITY+0x2` = 1 and not in the editor, treasury
   `CITY+0xC` −= cost and `CITY+0x2C8C` += cost (dis: 0x004E9680 decompiled in full);
   `0x00485200()` (undo bookkeeping) except types 0x38 and 9; `0x004F61E0()`.

Cost (dec): `0x00BC9728` = model record field 0, `i32 0x00705EA4 + type·0x28` (model.md), times
the count of tiles/buildings for drag types (`0x00BC971C`) or the bridge length (`0x00BC96F8`).
Some monuments also consume goods through `0x0045B4D0(player, resource, amount)`: 0x35 needs
city stock `CITY+0x494C` ≥ 500 and takes 500 of resource 0x17; obelisks 0xCF / 0xD0 need
`CITY+0x4958` ≥ 10000 / 20000 of resource 0x1A; 0xD5 and 0xDE need `CITY+0x4968` ≥ 22000 / 24000
of resource 0x1E (all skipped when `0x00605A0C` = 1). The three fields are
`CITY+0x48F0 + 4·res` for those resources, the per-resource city stock that the grave-goods
window also reads (§3.6); amounts in hundredths of a unit (*assumed*). The
consumed amounts are accumulated in `0x00BC9650`, `0x00BC965C`, `0x00BC966C` (undo).

Writes (run, well 0x5C on Actium): building record (chunk 32), building creation sequence
(chunk 117), terrain / building / image grids (chunks 10, 9, 7), treasury `CITY+0xC`,
`CITY+0x2C8C`; outside the save: undo list `0x00BA7888…`, `0x00BC95F0…`, notices `0x00E94E20…`.
Storage buildings also get a storage-settings record (chunk 113, run: bazaar 0x46).

UI side effects: notices (`0x004E2970`, text only, ported as `notice_add_4e2970`), place sound
`0x0047D350(4)` in `0x00405E90`.

### 2.3 Placement flags set by `0x00405200` (dec)

Cleared on every selection, then set by type; `0x004728A0` and the preview `0x0046FEF0` read them.

| Global | Set for types | Check in `0x004728A0` (notice on failure) |
|---|---|---|
| `0x006059CC` | 0x64–0x69, 0xC4, 0xE0 (farms, *hint*) | `0x00482650(x, y, 3, 1, 0x10800)` (0x14) |
| `0x00605A48` (code pointer) | 0x6A → `0x00405B80`; 0xDD, 0x6B, 0xD8, 0xA2 → `0x00405B60`; 0xA1, 0xD9 → `0x00405B40` | `0x00482720(x, y, 2, 1, predicate)` (0x16) |
| `0x006059C8` | (not seen set in the game branch) | `0x00482650(x, y, 2, 1, 1)` (0x17) |
| `0x006059C0` | (not seen) | `0x00482650(x, y, 2, 1, 0x40000)` (0x3A) |
| `0x006059C4` | 0x6D | `0x00482650(x, y, 2, 3, 0x10004)` (0x15) |
| `0x006059BC` | 0x3B, 0xAC, 0xAD (towers, *hint*) | `0x004828E0(x, y, 2, 0, 0x4000)` (0x31) |
| `0x006059B8` | 0x8A | `0x004828E0(x, y, 1, 0, 0x40)` and sprite grid = 0 (0x42 / 2) |
| `0x006059B4` | 0x5C, 0xB4, 0x4D–0x4F, 0xBB–0xBD | `0x004829B0(x, y, size, 0, 0x80, flag)` (0x4C) |
| `0x00BC9714` (orientation) | 1 for 0x38, 0x3A, 0xAA, 0xAB, 0xC8–0xCA, 0x8C–0x95; view orientation `0x00BCA2E0` for temple complexes 0x41–0x45 | gatehouses / complexes |
| `0x005E6970` | pyramid 0xB7 variant (monuments menu) | `0x004ECEC0` |

`0x00605A48` holds the address of a predicate in the exe: a port maps the three addresses to C
functions (the stored value itself must stay the original address, it is in the state).
`0x00BC9714` is also changed by the **R key** while the ghost is drawn: `0x00441F80` (called from
the city-view draw `0x0043B630`) does `GetAsyncKeyState('R')` and increments it modulo 16 (dis/dec).
`0x004BFCD0` and `0x004C0F30` (also under `0x0043B630`) set it for gatehouses and temple complexes.

### 2.4 Per-type work in `0x004728A0` (dec; costs by count unless noted)

| Type (hint name) | Work | Checks / notices |
|---|---|---|
| 9 clear land | `0x004743F0(0, x1, y1, x2, y2)` (§2.5) | — |
| 5 road | `0x00474F50(0, x1, y1, x2, y2)` | special point 0x3D |
| 6, 0xA8, 0xA9 walls | `0x00475220(0, x1, y1, x2, y2, 0x4000 / 0x404000 / 0x804000)` | 0x3D |
| 8 canal (irrigation ditch) | `0x00475090(0, x1, y1, x2, y2)` | — |
| 10 houses (vacant lots) | `0x00474130(0, x1, y1, x2, y2)` | 0x3D |
| 0x26 plaza | `0x004754A0(0, …)` | needs road tiles (run: 0 cost on grass) |
| 0x27 gardens | `0x004759E0(0, …)`, routing fills `0x00516F90`, `0x005173A0` | 0x3D |
| 0x1E, 0x1F, 0x21 bandstand, booth, pavilion | `0x00440850` / `0x00440700` / `0x00440B10(0, x1, y1, x2, y2)` | 0x4D |
| 0xD1 festival square | `0x00440ED0` | one per city (`CITY+0x4814`; 4), 0xD7 |
| 0x52, 0x53 bridges | `0x0047F530(x, y, 0 / 1)`, cost × `0x00BC96F8` | length ≥ 2 (0x19) |
| 7 water lift | creates type 0x5A via `0x0046A400`, `0x00485100`, canal grid, routing | `0x0047CC70` (0x19) |
| 0x88 ferry | two placements: `0x00486020` (first: notice 0x43, `0x00BCA310` := 1) | `0x00BCA314` (0x44) |
| 0xAB, 0xCA gatehouses | `0x004BFF60(x, y, 0x00BC9714)` | 2 |
| 0x4A, 0xB6 shipwright, warship wharf | `0x0047D060`, `0x00457AC0` | 0x19, 0x38; 0xB6 max 6 (0x54) |
| 0x4B, 0xC0 docks | same, max 10 (`0x00705C1C`; 0xCE) | |
| 0x4C, 0xB5 wharves | `0x0047CC70` | 0x19 |
| 0x28, 0x2C, 0x2D forts | parade ground `0x0047C880(x+3, y−1, 4, 4, …)`, max 6 forts (`0x00BCB9B4`; 0x34) | 2 |
| 0x41–0x45 temple complexes | none built (`CITY+0x4478`), `0x004C3730(x, y)` | 4, 2, 0x3D |
| 0xD3, 0xD4 complex add-ons | on a complex (`0x00475B50` of the hovered building), `0x004C52D0` | 0x49, 0x4B, 0x4E, 0x4A |
| 0xD2 Sphinx | one per city (i16 `CITY+0x433A`), two 6×6 checks | 4, 2, 0x3D |
| 0xB7 pyramid | `0x004ECEC0(offset, 0x005E6970, &reason)` | 0xD5 / 0xD6 |
| 0xCF, 0xD0 obelisks | goods (above) | `0x004E59C0` > 0 → 0x58 |
| 0xD5, 0xDE, 0xE1, 0xE3, 0xE4, 0xE5, 0xE6, 0xEA, 0xEB, 0xEC (Cleopatra monuments, *hint*) | each type's pair "fits at hovered tile" / "already exists" (`0x00541AE0`/`0x00542EE0`, `0x004DEF00`, `0x0040D850`/`0x0040D880`, `0x00416F70`/`0x00416FA0`, `0x004E65F0`/`0x004E6650`, `0x005456C0`/`0x00545700`, `0x00401400`/`0x00401430`, `0x00558B60`/`0x00558BA0`, `0x00555E00`/`0x00555E40`, `0x0054F3F0`/`0x0054F430`) | 2, 0x5B, 0xF3–0xF5, 0x110/0x111 |
| 0x35 | goods (above) | 0x4F |
| 0x5F | one ever (`0x00705C6C` := 1) | 4 |
| 0x5E | `CITY+0x49D0` = 0 | 4 |
| 0x56 | `CITY+0x4486` ≠ 0 | 0x59 |
| 0xBB–0xBD, 0x4D–0x4F (palaces, mansions, *hint*) | one each (`CITY+0x4486`, `CITY+0x499C`) | 4 |
| 0xA1 | predicate + `0x00459FA0()` | 0x16, 0x3C |
| other single buildings | generic path 6 / 8 | 2 |

### 2.5 Clearing land: `0x004743F0(preview, x1, y1, x2, y2)` and `0x00474700(preview, …)` (dis for the confirmation path, dec for the rest)

- `0x004743F0` with preview = 1 (called by the drag preview `0x0046FEF0`) goes straight to
  `0x00474700(1, …)`. With preview = 0 it scans the rectangle; if it holds a monument
  (`0x00474380(type)`: 0xB7, 0xDE, 0xD5, 0xCF, 0xD0, 0xD2, 0xE6, 0xE1, 0xE3, 0xE5, 0xEA, 0xEB,
  0xEC, 0xE4), a temple complex (0x41–0x45, 0x61, 0x9C–0xA0), a fort part (0x39, 0x36) or a bridge
  (sprite overlay with terrain bit 4, no building), it stores the rectangle in `0x00BC96A4`,
  `0x00BC96A0`, `0x00BC969C`, `0x00BC9698` and opens a yes/no popup: text 0x68 → `0x004021D0`, 0x6A
  → `0x00402210`, 0x10 → `0x00402250`, 0x14 → `0x00402290`. Otherwise calls `0x00474700(0, …)`.
- The popup callbacks (dis) set `0x00BC96AC` / `0x00BC96A8` / `0x00BC96B4` / `0x00BC96B0` to 1 (yes)
  or −1 (no) and call `0x00474700(0, x1, y1, x2, y2)` from the stored rectangle. Several
  confirmations can chain (the next category's popup is opened by a later call); the flags are
  reset at the next `0x004743F0`.
- `0x00474700` clears terrain, canals, buildings (state := 6 for the building and its linked parts
  `+0x38` / `+0x3A`, with `0x00485100` and a sound `0x0053B090(2)`), storage / ferry / shipwright
  queue links, pyramids `0x004F4930` + `0x004F5E70`, refunds a house's stored goods
  `0x00459020`, then rebuilds images and grids over the rectangle (`0x004767D0`, `0x00477500`,
  `0x004776F0`, `0x004777C0`, `0x0047BAE0`, `0x0047B480`, `0x00477890`, `0x00476530`, `0x00475720`,
  `0x0047AFD0`, `0x00475CC0`) and, when not a preview, `0x0047BD50`, `0x0047B590`, routing
  `0x005178B0`, `0x00517370`, `0x00517660`, `0x005178E0`. `0x00BC971C` counts the cleared items:
  the caller charges `count × cost(9)`.
- **Possible quirk (dec, to check by a run):** when a popup is opened, `0x004728A0` still charges
  `0x00BC971C × cost(9)` with the count left by the previous `0x00474700` call (the preview's),
  and the confirmed clear (`0x004021D0` …) charges nothing.

### 2.6 Undo: `0x00484F60` (snapshot) and `0x00485230` (restore) (dec, run)

- `0x00484F60` (called by the construction start): zeroes `0x00BC95F4` (0x24 dwords),
  `0x00BC968C`, `0x00BC9688`, `0x00BC9684`, `0x00BC95F0`, sets `0x00BC9694` := 1 (undo possible),
  `0x00BC9690` := type, clears `0x00BA7888` (0x21D68 bytes) and `0x00BA746C` (0x41A), then
  scans the buildings: any with state 6 sets `0x00BC9694` := 0 (no undo), any with state 2 makes it
  return 0 (`0x00BC9694` := 0; the construction does not start); else copies the grids to their
  backups: image `0x00B7482C` → `0x00A1126C`, terrain `0x00AF598C` → `0x009DE62C`, canal
  `0x00AE8E7C` → `0x009D1B1C`, bitfields `0x00B285CC` → `0x009B84FC`, edge `0x00B67D1C` →
  `0x009AB9EC`, sprite `0x00ADC36C` → `0x009C500C` (chunks 19 and 20 are two of these backups)
  and returns 1. State 6 is what clearing sets (`0x00474700`); the meaning of state 2 is not
  established.
- `0x00485230` (1523 bytes) restores grids and buildings and refunds the treasury (it is in the
  list of treasury writers). Run (Actium; start + place + undo): for a well, a 4-tile road and 9
  vacant lots, the state after undo equals the state before except the removed buildings' records
  (left with their fields, state cleared) and the creation sequence number.

### 2.7 Oracle runs (Actium, area at (99, 69), orientation 0)

| Command | Result |
|---|---|
| `0x004728A0(1, 0, 97, 69, 97, 69, 0x5C)` well, no start | building 64 created; treasury −5; 19 saved bytes |
| same with drag types (0x26, 0x27, 6, 8, 9) and **no** `0x00405C20` before | terrain grid overwritten (117–133 KB): the place step restores the grids from backups that only the start step fills |
| `0x00405C20` then `0x004728A0` for road 5 (4 tiles), houses 10 (3×3), walls 6, canal 8, gardens 0x27, clear 9, well 0x5C | each builds as expected: treasury −16, −90, −48, −32, −48, 0, −5 |
| bazaar 0x46, firehouse 0xA7 (no start) | built; bazaar gets a storage record (chunk 113) |

### 2.8 Proposed boundary for 5.1

- **Command = "start + place"**: inputs `0x006059E8` (type) and the placement flags as
  `0x00405200` leaves them (§2.3), `0x00BC9714`, `0x005E6970`, `0x00BCA2E0`, the start tile
  (`0x00BCA300`/`0x00BCA304`/`0x00BCA308` at the start), then the end tile. The test calls
  `0x00405C20` with the start tile hovered, then `0x004728A0(P, 0x00BCA2E0, x1, y1, x2, y2, type)`.
  The port provides both (`0x00405C20` → `0x00484F60`, and `0x004728A0`); the flags may be set by
  calling the original `0x00405200` (submenu `0x007056EC`, item `0x00D3B198`) in the oracle and
  copying the resulting globals, or by porting the flag part of `0x00405200` as
  `construction_set_type(type)`.
- The drag preview `0x0046FEF0` should not be needed for the result (the place step restores the
  grids first), **except** for the ferry (0x88) and the clearing preview count (§2.5). Checked for
  roads, walls, canals, gardens, houses, clearing (start + place gives the expected result without
  the preview, run).
- Undo: `0x00485230` after a "start + place".
- Not applicable as is: **ferry** placement validity `0x00BCA314` is computed by the city-view draw
  code `0x0043D300`, and the R-key rotation by `0x00441F80` in the draw; both must be moved into a
  core function (or passed as command inputs) before a ferry or a rotated building can be tested.

## 3. Settings and orders (roadmap 5.2)

All are button callbacks (no args, cdecl; parameters in `0x00D3B198` / `0x00D3B194`). The "table"
column gives the button table (entry index: param 1, param 2).

### 3.1 Finance and personal savings

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| tax rate +1 / −1 | `0x0040A260` (arrows `0x005C8D08` / Treasury advisor) | p1: 0 = +1, 1 = −1 | `CITY+0x8` (tax rate, clamped 0–25); then `0x00459D20`, `0x0045A990` (ported), `0x0045A7B0` (ported) | run (+0x8 changed), dec |
| wages +1 / −1 | `0x0040A1D0` | p1 as above | `CITY+0x2C08` (clamped 0–100); `0x0045A8C0`, `0x0045A990`, `0x0045A7B0` | run (+0x2C08, +0x46A0) |
| governor salary rank | `0x0040AF30` (table `0x005C5468`, entries 1–11: p1 = 0–10) | p1 = rank | only if `CITY+0x44D4` < 1: `CITY+0x2C9C` := rank, `CITY+0x2C98` := `i32 0x005D0320[rank]`; `0x0045A970`, `0x0046EC70` (ported) | run |
| donation amount | `0x0040AFF0` (p1 0–4: 0, 500, 2000, 5000, all), `0x0040B0B0` (p1 0: +10, 1: −10), `0x0040AD00` (open) | p1 | `CITY+0x445C`, clamped to savings `CITY+0x4450` | run (`0x0040AFF0`) |
| donate to the city | `0x0040AF90` | `CITY+0x445C` | treasury `CITY+0xC` += amount, `CITY+0x4458` += amount, savings `CITY+0x4450` −= amount; `0x0045A990`, `0x0045A7B0` | run |
| gift size | `0x0040B140` (p1 1–3) | savings vs costs `CITY+0x4574`/`+0x4578`/`+0x457C` | `CITY+0x4558` := p1 − 1 if affordable | dec |
| send gift to the king | `0x0040B1C0` → **`0x0046EDD0`** (no args) | `CITY+0x4558` size, costs | savings −= cost, gift counters `CITY+0x4560`/`+0x4568`…, `CITY+0x455C`, rating via `0x004E96C0` (ported: `city_add_0x42d8`) | run (`0x0046EDD0`) |

### 3.2 Labour

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| set a category's priority | `0x0040A6C0` (row: `0x00605988` := p1, opens chooser window 0x2A) then **`0x0040A730`** (table `0x005C4BB8`, p1 = priority 0–9) | `0x00605988` category, p1 | `CITY+0x2B3C + cat·0x14` and the renumbering of the 9 categories (scratch `0x00605768[9]`); `0x00459400`, `0x004595A0`, `0x00459670` (ported) re-allocate workers (building `+0x1A`) | run |

### 3.3 Festivals (Temples / Entertainment advisor, festival square)

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| open the festival window | `0x0040AA50` (festival square button, advisor) | | none (popups 0x58 / 0x60 when impossible) | dec |
| choose the god | `0x0040AC50` (table `0x005C51A8`, p1 = god) | `0x007848E2[god]` ≠ 0 | `CITY+0x44F0` := god | dec |
| choose the size | `0x0040AC80` (p1 1–3) | treasury > −5000, `CITY+0x4514`, `CITY+0x47A0`/`+0x47A4` ≠ −1 | `CITY+0x44F4` := size, `0x00775678` := size·10 | dec |
| hold the festival | **`0x0040AB00`** (table `0x005C7BC8`, p1 = 1, p2 0/1 = return window) | `CITY+0x44F0`, `+0x44F4`, costs `+0x4504`/`+0x4508`/`+0x450C`, beer `+0x4510` | `CITY+0x4500`, `+0x44F8`, `+0x44FC`, months `+0x4528`/`+0x452C`, treasury −= cost, `CITY+0x4758` += cost; size 3 consumes beer `0x0045B4D0(P, 0xF, …)` | run |

### 3.4 Trade

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| open a trade route | empire window button `0x00409490` → popup (text 2 or 4) → **`0x004022D0`** | `0x00D3B439` = 1, selected empire city `0x00784F18` | `0x004E9680(P, cost)` with cost = empire city `+0x4E` (i16): treasury, `CITY+0x2C8C`; empire city `+0x05` := 1; `0x00404350`, `0x004051A0` | dis, run (city 1: +0x05, treasury −675) |
| select a resource (opens window 0x28) | `0x00559820` | `CITY+0x4834` scroll, `0x00F53E6C[]` | `0x0060598C` := resource | dec |
| import on / off | **`0x00559A60`** | `0x0060598C` | `CITY+0x2826 + 2·res` cycles 0/2/4 → 3, 3 → 1 (+ `0x00443330`, limit `CITY+0x286E + 2·res` := `0x00775810[res]` / 100), 1 → 0; refuses with a message box (`0x004CC110`, `0x004CC160`, `0x00538CD0`, platform) when no open route sells it (`0x004439C0`, `0x00443950`) | dec; run: message box reached for an export good |
| export on / off | **`0x00559B50`** | `0x0060598C` | same field, 0/1/3 → 4 (+ stockpile flag := 0), 4 → 2 (+ limit), 2 → 0 | run |
| cycle trade status | `0x005598F0` | `0x0060598C` | same field 0 → 1 → 2 → 0 with `0x00443950` / `0x00443B20` checks | dec |
| trade limit ± | `0x00559CD0` (arrows `0x00603580`, p1 0 = +1, 1 = −1) | `0x0060598C` | `CITY+0x286E + 2·res`, capped by `0x00559C60` (ported: `value_cap`) | run |
| stockpile toggle | `0x00559D70` (image `0x00603560`) | `0x0060598C` | `CITY+0x2A88 + 4·res` toggled; when set, an export status 2/4 becomes 0 | run |
| industry on / off (mothball) | `0x005598A0` | `0x0060598C` (9 → 1), industry count `0x0078D6DC[res]` > 0 | `CITY+0x28B6 + 2·res` toggled | run |

### 3.5 Storage yards, granaries, bazaars (building window, `0x00EADD6C` = building)

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| cycle a resource's order | `0x00406300` (table `0x005C68C8`, p1 = row 1…) | building, row → resource via `0x00F53DD8[]` (other buildings), `0x00F53E68[+0xE0 scroll + row]` (yards 0x48 / 0xC1) or the filtered `0x00F53E70[]` list (0x46 / 0xBE) | storage record `0x007900C2 + 0xC4·(building+0xC1) + res`: 0 → 2 (or 1 for bazaars), 1 → 0, 2 → 3, 3 → 1 | run (yard 129) |
| fill limit ± | `0x00406460` (table `0x005C81F8`: p1 1 = −800 / 0 = +800, p2 = row) | building, row lists | `0x007900EC + …` (accepting goods) or `0x00790134 + …` (getting goods), i16, 800–3200 | run |
| all to state 1 | `0x00406620` (image `0x005C5BA8`) | `0x00F53DD8`, `0x00F53E70[]` | every listed resource's order := 1 | run |
| `0x00406680` p1 = 1 / 2 (image `0x005C5BC8`, p1 = 2) | `0x00406680` | p1 | p1 = 1: `CITY+0x4674` := building id; p1 = 2: every listed resource := 1 | run (p1 = 2), dec |
| toggle `+0xDC` 1 ↔ 0x10 | `0x0040C630` | building | building `+0xDC` | dec, meaning *assumed* (special orders) |

`0x0040C6B0` (scroll in the yard list, building `+0xE0`) and `0x0040C760` (resource list scroll
`CITY+0x4834`) also change saved bytes but only as view state.

### 3.6 Kingdom (political advisor), requests and the royal tombs

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| dispatch a request | advisor row `0x0040AE10` (table entries 3–7, p1 = row) → `0x0044D520(row, &id)` → popup 6 / 0xE → **`0x00402340`** → **`0x0044C1A0(event)`** | `0x00D3B439` = 1, `0x006057E4` = event index | event `+0x30` status, `+0x2E`; goods `0x0045B4D0(P, res, amount·100)` (storage buildings), money requests (res 0x24) treasury −= amount and `CITY+0x4758`; troop requests (0x25) `0x0044C110` (ported), `0x004B84B0`; **advances the game RNG** (`0x00D3B384`/`0x00D3B380`, 31 steps) | dis (`0x00402340`), run on Iken (event 16: RNG, buildings, `CITY+0x48D4`, `+0x4964`) |
| dispatch via message options | `0x004E0F90` (message post) calls `0x0044C1A0(0x006057E4)` when option bit 8 of `0x00E38EF0` is set | `0x00E38EF0` | as above | dec |
| `0x0040A450` | | `0x005F8518`, `0x00521AA0` outputs, `CITY+0x4665`/`+0x4666`/`+0x4669` | dispatches the rows whose id matches (military requests, *assumed*) via `0x0044C1A0` | dec |
| grave goods: choose a resource | `0x0040CD70` (table `0x005C5798`, p1 = row) | `0x00784DE8[res]` needed, `0x00784E78[res]` sent, stock `CITY+0x48F0+4·res` | `CITY+0x297C` := res, `CITY+0x2980` := sent, `CITY+0x2984` := 0 | dec, meaning *assumed* |
| grave goods: amount | `0x0040B3B0` (max), `0x0040B420` (+1), `0x0040B4B0` (−1) | | `CITY+0x2984` | dec |
| grave goods: send | `0x0040B1F0` (table `0x005C5710` entry 1) | `CITY+0x297C`, `+0x2984` | `0x00784E78[res]` += amount, `0x0045B4D0(P, res, amount·100)`; when `0x004EA930` (ported: `goals_counters_met`), marks tombs 0xE5, 0xEA–0xEC complete (`+0x160` := 1) and `0x005468D0` (ported), `0x004DEA50` (ported) | dec |
| grave goods: cancel | `0x0040B380` | | `0x00784E78[res]` := `CITY+0x2980` | dec |

### 3.7 Military (legions = formations, 0x90 bytes at `0x00BCB9C8 + id·0x90`; ships = figures)

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| legion formation / stance | `0x00406720` (table `0x005C66D8`, p1 0–3) | `0x00EADCCC` formation (needs `+0x82` < 0) | `+0x2A` layout (p1 0: 1/2, 1: 3/4 or 0 for type 0xD, 2: 0xF, 3: 6 and `+0x62` := old), `+0x8C` := −1, `0x00BCB9C0` := id; companies of type 0xD send their figures back (`+0x38` := 0xC); sound `0x0047D350("wavs/cohortN.wav")`; window 0x1E (choose destination) or 9 / 0x49 | run (p1 1–3: formation +0x2A, +0x62) |
| legion toggle `+0x77` | `0x00407110` (image `0x005C6768`) | `0x00EADCCC` / `0x00BCB9C0` | formation `+0x77` := 1 − value; p2 = 1 also applies `0x00406720` | run |
| legion return to fort | `0x00406FB0` (table `0x005C66D8` entry 4) | `0x00EADCCC`, needs `+0x82` < 0, `+0x03` ≠ 1 | `0x004B6C20(id, 0xE)`; sound | dec (no change on Actium: gate) |
| **legion move / attack** | window 0x1E, left click → **`0x0040C440`** | `0x00BCB9C0` formation, target tile `0x00BCA2C4` (≠ 0), `0x00BCA2C8`, `0x00BCA2CC`; figure under the tile (`0x004B5640`) | own fort tile: `0x004B6C20(id, 0xE)`; empty tile: `0x004B6400(x, y, id, 0)` (ported: `formation_move_to`); enemy: `0x004B66E0(x, y, id, target)`; sound | run (formation 2 to (99, 69): formation +0x03, +0x32, +0x34, 29 figures' action state `+0x38`, routing counter chunk 120) |
| legion return (military advisor row) | `0x0040A350` (p1 = row → `0x004B78B0`) | | `0x004B6C20(id, 0xE)` | run (16 figures) |
| legion go to (advisor) | `0x0040A2F0` | | camera only (`0x00430CD0`) | dec |
| legion kingdom service toggle | `0x0040A390` | row → formation, `0x00521A60` outputs | formation `+0x67` toggled, `0x004B7C80` (ported) or popup 0x24 / 0x26 / 0x28 | run (popup) |
| transport ship orders (corrected, §7) | `0x00406B40` (table `0x005C6788`, p1 0–3; jump table `0x00406C7C`) | `0x00EADCC8` figure | figure `+0x38` := 0x14 / 0xA (+ `+0x36` := 0x4B, `+0x15C` := 0x10) / 0xC or 0xD; `0x00D05708`; p1 3: `0x004A45D0` (ported: `boat_seek_dock`); window 0x1F | dis |
| transport return (corrected, §7) | `0x00406E80` (entry 4) | `0x00EADCC8` | `+0x38` := 9, destination from the wharf (`0x004806C0`, ported), `0x004B3250` (ported) | dec |
| warship orders (corrected, §7) | `0x00406C90` (table `0x005C6818`, p1 0–3) | `0x00EADCC4` | figure `+0x38` := 0x14 / 0x17 / 0xA, `+0x15C`; p1 3 `0x004A45D0` | dec |
| warship return (corrected, §7) | `0x00406DE0` (entry 4) | `0x00EADCC4` | `0x004BC670(id)` | dec |
| transport / warship move | window 0x1F → `0x0040BA40`, window 0x20 → `0x0040B830` | `0x00D05700` / `0x00D05704`, target tile as for legions | figure destination `+0x24`/`+0x26`, `+0x38`, `0x004B3250`, `0x004B6400` (boarding) | dec |
| ship return / kingdom service (advisor) | `0x0040A5C0` (`0x004BC670`), `0x0040A600` (figure `+0x02` toggled, `0x004B8AA0`) | p1 = row → `0x004BC620` | | dec |
| figure order selection | `0x0053D730` (image `0x005C5CB0`) | `0x00EADCE4` row → `0x00EADD24[]` figure | figure `+0x7D`, `+0x131`, `0x00F55BA4[]` | dec, meaning *assumed* |

### 3.8 Messages and scenario-event replies

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| delete one message | `0x004E2300` (right click, table `0x005C4830`, p1 = row) | scroll `0x00E95120` | message `+0xA` := 0, `0x004E1640` (ported: `message_sort`), newest index `0x00E951E8` | run (chunk 77, 80) |
| delete read messages | `0x004E2290` (image `0x005C4810`) | | messages read and not kind 4 → type 0; `0x004E1640` | run |
| open a message | `0x004E21D0` (left click) | | message `+0xE` (read) := 1, `0x00E951E4`/`0x00E951E8` | run (1 byte) |
| event reply buttons | `0x0044EEA0`, `0x0044EEF0`, `0x0044EE30` (popup table `0x005E02C0`) | message `0x00E951E4`, allowed replies `u8 0x00D3B186` bits 1 / 2 / 4 | event status (`+0x30`) 7, 2 / 4 / 8, or 6; `0x0044E890` (ported: `event_festival_request`); `0x0044EE30` also sets `u8 CITY+0x49A4 + event+4` and calls `0x005596D0` (ported) | dec, meaning *assumed* |

### 3.9 Game-level settings that change state

| Command | Function | Inputs | Writes | Evidence |
|---|---|---|---|---|
| difficulty ± | `0x00402E50` (options, table `0x0057CD70`) | p1 0 = +1, 1 = −1 | `0x00E39080` (chunk 174, 0–4), reloads the three model files `0x00540C90`, `0x00540ED0`, `0x00541160`, `0x004BC720`; in window '5' also the treasury `CITY+0xC` := `0x004CDE40(0x007848F8, 0x005D1960[d])` | run |
| lower difficulty after a defeat | `0x004EA840` (table `0x005C6648`) | `CITY+0x47A8` | `0x00E39080` and `CITY+0x47A8` := min(old − 1, 1) | dec |
| keep governing 2 / 5 years | `0x004EA770` (table `0x005C65D0`, p1 1 / 2) | | `CITY+0x44D0` := 1, `CITY+0x44D4` += 24 / 60, `CITY+0x44D8`, salary `CITY+0x2C9C`/`+0x2C98` := 0, `0x0045A970` | run, meaning *assumed* |
| stop governing | `0x004EA7F0` | | `CITY+0x44D0`/`+0x44D4`/`+0x44D8` := 0, `0x00BC9694` := 0, `0x004CB6A0` | dec |
| message / auto-dispatch options | `0x00402CC0` (table `0x005C2EB0`, p1 1–…) | | bit toggles in `0x006DE510`, applied to `0x00E38EF0`, which `0x004E0F90` (message post, simulation) reads | dec |
| rotate the map | `0x00409EB0` (table `0x005C7980` entry 2) | p1 0 / 1 / 2 | orientation `0x00E94E04` (chunk 33) via `0x00430B20` / `0x00430BE0`, `0x00BCA2E0`, `0x00BC96C8`; image rebuild `0x0047D6E0`, `0x004DE080` | dec |
| cheat codes | `0x0041D3F0` → `0x0041D050(text)` (match) → **`0x0041D010(index)`** | `u8 0x007027E0` (cheats enabled, set by `0x0041D310`) | `CITY+0x44A0` += 1, then the table entry's function | run (index 1: treasury +1000) |

Cheat table `0x0057C3C8`: 34 entries {name XOR 3, function, flag} (dis). Index 0 `0x0041CFC0`,
1 "Treasure Chest" `0x0041CFD0` (+1000 treasury and `CITY+0x44A0`… while < 15000), 2
`0x0041CCF0`, 3 `0x0041CD00`, 4–5 `0x00446530`/`0x004465A0` (mock attacks), 6 frame counter, 7–33
`0x0041CA80`…`0x0041CCB0` (god and disaster triggers, *assumed* from their names).

### 3.10 Not state-changing (checked, excluded)

Window opening and scrolling (`0x0040A0E0`, `0x0040B520`, `0x0040B4E0`, `0x00409510`,
`0x0040AD30`, …), camera moves (`0x0040A560`, `0x0040A2F0`, `0x004E2380`, `0x0040CF20`), options
without a simulation reader (speed, scrolling, sound: `0x00403130`, `0x004031A0`, `0x00403320`,
`0x00403390`, `0x00402F70`…), the population-advisor graph selection `0x0040A8A0` (chunk 45 is
saved but UI), house window toggles (`0x004062D0`, `0x004062F0`), save / load / new game / campaign
screens (`0x00402720`, `0x004099B0`, `0x00418220`, `0x0041BB70`…: phase 6), and the scenario
editor (`0x00409AC0`, `0x00407370`–`0x004090E0`, `0x00448520`–`0x0044AA30`, `0x00408C30`–
`0x00408CB0` floodplain settings, empire editor on chunk 142): editor commands change saved
state but are not player commands.

## 4. Summary: commands and proposed boundaries

| # | Command | Boundary to reproduce | Inputs to set (besides the state) |
|---|---|---|---|
| 1 | place building / drag (all types) | `0x00405C20` + `0x004728A0` | type `0x006059E8`, flags §2.3, `0x00BC9714`, `0x005E6970`, `0x00BCA2E0`, hovered tile `0x00BCA300/304/308` at start, end tile |
| 2 | clear land (+ confirmations) | as 1 with type 9; confirmations `0x004021D0`, `0x00402210`, `0x00402250`, `0x00402290` | `0x00D3B439`, stored rectangle `0x00BC9698…0x00BC96A4` |
| 3 | undo | `0x00485230` | snapshot from 1 |
| 4 | set type (flags) | `0x00405200` (or a ported `construction_set_type`) | `0x007056EC`, `0x00D3B198` |
| 5 | cancel construction | `0x00405BA0` (ferry deletion `0x00486060`) | |
| 6 | tax rate | `0x0040A260` | p1 |
| 7 | wages | `0x0040A1D0` | p1 |
| 8 | salary | `0x0040AF30` | p1 |
| 9 | donation amount / donate | `0x0040AFF0`, `0x0040B0B0`, `0x0040AF90` | p1, `CITY+0x445C` |
| 10 | gift | `0x0040B140`, `0x0046EDD0` | p1 |
| 11 | labour priority | `0x0040A730` | `0x00605988`, p1 |
| 12 | festival | `0x0040AC50`, `0x0040AC80`, `0x0040AB00` | p1 |
| 13 | open trade route | `0x004022D0` | `0x00D3B439` = 1, `0x00784F18` |
| 14 | import / export / status / limit / stockpile / industry | `0x00559A60`, `0x00559B50`, `0x005598F0`, `0x00559CD0`, `0x00559D70`, `0x005598A0` | `0x0060598C`, p1 |
| 15 | storage orders | `0x00406300`, `0x00406460`, `0x00406620`, `0x00406680`, `0x0040C630` | `0x00EADD6C`, lists `0x00F53DD8…`, `0x00F53E6C…`, p1, p2 |
| 16 | dispatch request | `0x0044C1A0(event)` | event index |
| 17 | grave goods | `0x0040CD70`, `0x0040B3B0`, `0x0040B420`, `0x0040B4B0`, `0x0040B1F0`, `0x0040B380` | p1 |
| 18 | legion orders | `0x00406720`, `0x00407110`, `0x00406FB0`, `0x0040A350`, `0x0040A390` | `0x00EADCCC` or row, p1 |
| 19 | legion move / attack | `0x0040C440` | `0x00BCB9C0`, `0x00BCA2C4/C8/CC` |
| 20 | ship orders and moves | `0x00406B40`, `0x00406E80`, `0x00406C90`, `0x00406DE0`, `0x0040BA40`, `0x0040B830`, `0x0040A5C0`, `0x0040A600`, `0x0053D730` | `0x00EADCC8`/`0x00EADCC4`/`0x00D05700`/`0x00D05704`, tile, p1 |
| 21 | messages | `0x004E2300`, `0x004E2290`, `0x004E21D0` | `0x00E95120`, p1 |
| 22 | event replies | `0x0044EEA0`, `0x0044EEF0`, `0x0044EE30` | `0x00E951E4`, `0x00D3B186` |
| 23 | difficulty, keep governing, options | `0x00402E50`, `0x004EA840`, `0x004EA770`, `0x004EA7F0`, `0x00402CC0` (+ its apply) | p1 |
| 24 | rotate map | `0x00409EB0` | p1 |
| 25 | cheats | `0x0041D010(index)` | `0x007027E0` |

## 5. Open problems

1. **Ferry placement** (type 0x88) depends on `0x00BCA314`, written only by the city-view draw
   `0x0043D300`, and on the preview `0x0046FEF0`. Needs a decision: port the validity check into
   core as part of the command, or treat it as a command input.
2. **R-key rotation** (`0x00BC9714`) is read with `GetAsyncKeyState` inside the ghost drawing
   (`0x00441F80`); gatehouse / temple-complex orientation is also computed there (`0x004BFCD0`,
   `0x004C0F30`). The port should take the orientation as an input of the place command.
3. `0x00605A48` stores a code address (terrain predicate). It is state; the port must map the three
   values (`0x00405B40`, `0x00405B60`, `0x00405B80`) to C functions.
4. The clearing cost when a confirmation popup opens (§2.5) looks like it uses a stale count: to
   confirm with a run on a save with a monument.
5. Meanings marked *assumed* (city fields of §3.5, §3.6 grave goods, §3.8 event replies, §3.9
   keep governing, building `+0xDC`, figure `+0x7D`/`+0x131`) need the text strings or a run on a
   save where the situation occurs.
6. Not surveyed: keyboard shortcuts other than the cheats and R (the key handler under
   `0x004D6B60`), the overlay / building-menu right-click paths, the multiplayer-looking gate
   `0x00E38E60`, and kingdom requests handled from the message list rather than the advisor.
7. Many command functions call unported helpers that write state (from the well frontier:
   `0x00470990` 368 bytes, `0x004E32E0`, `0x004F61E0`, `0x00485200`, `0x004B9EE0`, `0x004E9680`;
   and per area: `0x00474700`, `0x00474F50`, `0x00475220`, `0x00475090`, `0x00474130`, `0x004754A0`,
   `0x004759E0`, `0x00440700`/`0x00440850`/`0x00440B10`/`0x00440ED0`, `0x0047F530`, `0x00485230`,
   `0x00484F60`, `0x0045B4D0`, `0x00459D20`, `0x0045A8C0`, `0x0045A970`, `0x004B66E0`,
   `0x004B6C20`, `0x004BC670`, `0x004B84B0`): they can be replayed (`GAME_CALL`) while the commands
   are ported top-down.

## 6. Method (reproducible)

- Button tables: all calls to `0x00425990`, `0x004261D0`, `0x00425C40` were collected from the
  decompiled functions (170 call sites, 20 with run-time tables), the tables decoded with the
  layouts of §1.2, and `.data` scanned for further entries with the same layouts. This gave 275
  candidate callbacks.
- Each callback was called in the oracle on Actium with its table parameters, and the bytes it
  changed in saved chunks listed; callbacks needing a context (building, formation, resource,
  confirmation) were then run with that context set (`0x00EADD6C`, `0x00EADCCC`, `0x0060598C`,
  `0x00D3B439`…). Construction was run through `0x004728A0` with and without `0x00405C20`.
- Global writers were found with an instruction-level xref over every exported function
  (capstone, memory operands).
- The scripts were throwaway (not in the repository); `tools/oracle/game.py` (`Game.call`,
  `state_bytes`, `load_state`) and `diff.schema()` / `diff.name_address()` are all they need.

## 7. Results of the ports, and corrections

Every command of §4 is ported (core/command/, docs/provenance/ported.md) and checked by
tests/differential/oracle_command_check.py (chained lockstep with the commands applied between
ticks). What the ports established, beyond §1–§6 (dis = disassembly, run = oracle run):

**Ships (corrects §3.7).** `0x00EADCC8` / `0x00D05700` / window 0x1F / `0x00406B40` /
`0x0040BA40` are the **transport** (figure type 0x4D); `0x00EADCC4` / `0x00D05704` / window 0x20 /
`0x00406C90` / `0x0040B830` the **warship** (type 0x4E) (dis: `0x0040B790` uses `0x004B5570`, type
0x4D; `0x0040B6D0` uses `0x004B55B0`, type 0x4E). `0x00406E80` and `0x004BC670` release the route of
the current figure `0x00D056F8`, not the ship's (`0x004B3250`; kept). In mode `0x00D05708`,
`0x0040BA40` compares the cursor `0x00E390DC` with `0x00E390C8` (board) / `0x00E390C4` (unload):
the cursor is a command input.

**Military commands the survey missed:** `0x004071C0` (closing the legion window re-applies
`0x00406720`), `0x0040C570` (target click in window 0x49, charge), `0x0040A450` / `0x0040A550`
(dispatch companies to a kingdom troop request, through `0x0044C1A0`), `0x0040B630`, `0x0040B6D0`,
`0x0040B790` (left press on a soldier / warship / transport: the selection the move windows read).
Recruiting and company creation are simulation, not commands.

**Clearing (answers §5 open problem 4, run on Iken and Actium).** When `0x004743F0` opens a
confirmation, the mouse-up still charges `0x00BC971C` × cost(9) with the count the last
`0x00474700` left (the drag preview's, or after a single click the previous clearing's), whatever
the answer; the confirmed clear in the callback charges nothing. Undo after a clearing never
happens in play: `0x004728A0` skips `0x00485200` for type 9 (dis 0x00474105), so `0x00BC968C`
stays 0 and `0x00485230` returns at once. The undo list (`0x00BA746C`) holds 525 entries (dis
0x00485132). Its Cleopatra-monument branches are dead in play (monuments are never recorded by
`0x00485100`).

**The confirmation popup `0x004264D0` is state:** it sets the window id `0x00E38DDA` := 0x3F and the
popup globals `0x00705704`–`0x00705718`, clears the answer `0x00D3B439`; tick functions read the
window id (`0x00451C50` at 0x00452012, `0x004E0CF0`, `0x004E2970`), so ticks with the popup open
differ from ticks without. Ported as state.

**Message box `0x00538CD0`** (trade refusals, ...): a modal Win32 loop. It is a platform request
(core/sim/platform.c `platform_message_box`): the platform returns the pressed button; the state
keeps the button count `0x00F535C8`, the pressed flag / index `0x00F535C0` / `0x00F535C4`
(`0x00538920`; the button table `0x005F8910` gives each button its index) and the redraw flag
`0x00E38DCA` := 1; the window id is restored. Headless, the first button is pressed
(tools/oracle/game.py does the same in the original). The button rectangles drawn by `0x00538940`
are excluded (tools/oracle/excluded.py).

**Values computed by drawing code.** The ferry validity `0x00BCA314`, the ferry reach
`0x00BC96E4`–`0x00BC96F0` (`0x0047F3E0`, called only from the draw `0x0043D300`), the bridge
measure `0x00BC96F4`–`0x00BC9704` (`0x0047F200`, from the draw `0x0043B630`) and the R-key rotation
`0x00BC9714` are read by the place step but written while the city view draws the hovered tile.
The map rules among them are ported as core "hover" functions that the platform calls for the
hovered tile, as the original's draw does; the rotation key stays a command input.

**Settings (corrects §3, run).**
- Salary: refused while `CITY+0x44D4` > 0. Donation p1 = 4 sets 1,000,000, clamped to the savings.
  Gift size: an unaffordable p1 1–3 is refused, any other p1 sets size := p1 − 1; send
  (`0x0040B1C0`) checks only the smallest cost (`+0x4574`). Gift effect on `+0x42D8` by counter
  `+0x4560` (≤ 0, 1, 2, 3) and size: {3, 5, 10}, {1, 3, 5}, {0, 1, 3}, {0, 0, 1}; the counter stops
  at 4.
- Labour priorities: 9 bubble passes (scratch `0x00605768`), then `0x00459400`, `0x004595A0`,
  `0x00459670`.
- Festival: `+0x4514` ≠ 0 refuses size 3 only; hold reads p2 and sets `+0x452C`; size 3 takes good
  0xF in the amount `+0x4510`.
- Trade: import cycles 0/2/4 → 3, 3 → 1 (default limit `0x00775810[r]` / 100), 1 → 0; export
  0/1/3 → 4 (stockpile cleared), 4 → 2 (default limit), 2 → 0; the export check is `0x00443A20`.
  Open route also sets `0x0070570C` := 0x26 and calls `0x00404350`, `0x004051A0`.
- Storage: "all to 1" `0x00406620` counts from `0x00F53DD8` and lists from `0x00F53E70`.
- Request dispatch `0x0044C1A0`: money / goods (type 0x24) take one RNG step, then
  `+0x2E` := 1 + (r & 3); troops (0x25) set status 5 and the months (`0x0044C110`, 12 when ≤ 0),
  then `0x004B84B0` sends formations 1–49 by land or sea (army kind byte `0x007770F0` + a·0x144).
- Grave goods: `+0x297C` resource, `+0x2980` sent count, `+0x2984` amount; the minimum stock is 1
  for resources 0xA, 0x18–0x1A, 0x1C, 0x1E, 0x23, 0x24, else 100; the tomb flag is building
  **+0x58** (not +0x160), set only when `+0x12` = 0 and the tomb's progress function returns 100.
- Messages: `0x00E951EC` is the message count (chunk 79); `0x00E9511C` the window's row count.
  Replies find their event with `0x0044EDD0` (kind 0x17, active, matching the message).
- Difficulty: window "5" is 0x35; the change reloads the model files (`0x00540C90`, `0x00540ED0`,
  `0x00541160`, `0x004BC720`). Options: p1 1–12 toggle bit p1 of `0x006DE510`, 13 copies it to
  `0x00E38EF0`.
- Rotate (`0x00409EB0`): p1 2 = +2, 1 = −2, 0 = back to 0. Not view-only: `0x0047D6E0` rewrites the
  edge-grid bit 0x40, bridge sprites, routing and the positions of type-0x2A figures.
- Cheats (`0x0041D010`, table `0x0057C3CC`, the index is not checked against `0x007027E0`): 6 empty,
  3 herd of 8 animals at the mouse tile (its offset adds x and y unscaled, as the original), 7–26
  god actions (god (i − 7) / 4, kind (i − 7) % 4 + 1), 27–30 plagues, 31 pyramid speed-up,
  32 `0x00469C60(3)`, 33 `0x004E7AC0(10)`.

**Construction quirks kept as in the binary:** the sphinx's special-point rectangle uses y1 + 5;
`0x004E5790` scans buildings 1..n − 1; `0x004E3F60` reads the trade status of resource 2 where 8
would be expected; the water lift calls `0x00485100` twice; the last argument `0x0046FEF0` passes
to `0x0047D360` is y1's stack slot with its low byte replaced. `0x00447530` uses x87 only as
`ftol(a / 10.0 + b)`, exact in integers.

**Not reached in play:** the "Graphic file out of sync" box of `0x0053DD90` (only a monument the
scenario does not offer, forced in a test); the map editor's drag `0x00478440` and mouse-up
`0x004795E0` (editor flag `0x00605A3C`; the editor is out of scope).
