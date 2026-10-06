# Starting a new game (roadmap phase 6)

How Pharaoh.exe (GOG 2.1) turns a menu choice into a running city, what the port has to reproduce,
and how to check it. Survey dated 2026-10-06. Evidence: **dec** (Ghidra export,
`local/ghidra-export/`), **dis** (`tools/oracle/disasm.py`), **run** (the oracle, scratch scripts
described in §6). Every statement is marked *confirmed* (with its evidence) or *assumed*.

Abbreviations: `CITY` = the current city record, `0x00EA1870 + 0x49D8 × [0x00EAAC28]` (index 1 in
every start, set by `0x00540A10` / the loader); `ST(x)` = screen id `0x00E38DDA` = x;
`0x00E38DCA` := 1 (redraw) is omitted.

## 1. Summary

- **Every path that starts a city goes through `0x005405B0(name, flag)`** (callers: `0x0041A180`,
  `0x00418510`, `0x0041F5C0`; *confirmed* dec + dis). `name` is always `0x00E37078` (the map name
  buffer, char[65]). It has two branches (*confirmed* dec, dis, run):
  - **campaign record** when `0x00E38EBC == 0` (not custom) or `flag != 0`: `0x004D58B0(0, 0)` loads
    record `[0x00E38EA8]` of `mission1.pak` with the saved-game loader `0x004D41E0`, then
    `0x004BD360` and the scenario start `0x004D52A0` (the same code a save load runs);
  - **custom map** otherwise: `0x005409D0` checks that `maps\<name>.map` opens, then
    `0x00540A10` (new-game reset) → `0x004DE820` (scenario setup) → `0x004DE600` (map load
    `0x004D5820(maps\<name>.map, 0)` → `0x004D4AE0`, terrain rebuild, map points, ...).
  **Custom maps do not run `0x004D52A0`** (*confirmed* run: with `0x004D52A0` transparent in the
  recorder, none of its callees appear on any of the 33 maps); `docs/formats/campaign.md` §1 calls
  `0x004D52A0` "the scenario start used for custom maps": that is wrong for `.map` files.
- **Proposed boundary: `0x005405B0` itself**, with its inputs as globals (§3). The menus around it
  only choose those globals (§2) and are UI.
- **The oracle runs all of them headless** with two additions to the emulator (CRT `_lseek` and a
  `QueryPerformanceFrequency` stub, §5): 33/33 bundled maps and 53/53 campaign records start
  (slots 53–59 of `mission1.pak` are empty: the start returns early, as in the game).
- A **campaign start differs from loading the same record as a save** (`Game.load_save`,
  `game_load_save`, already ported) by only ~1,300 bytes on mission 0 (*confirmed* run, §5.3):
  the campaign path is mostly the save-load port plus a dozen small steps. The custom-map path is
  new code (record clears, map loader, setup, initial figures).
- The result **depends on the state before the start** (*confirmed* run, §7.1): the check must
  start both sides from the same state: the oracle's booted state (the boot is not ported).

## 2. Paths from the UI

Screens (`0x00E38DDA`, draw dispatcher `0x004D71C0`, input dispatcher `0x004D8310`; *confirmed* dec
unless marked): `0x2F` family list (`0x00516630`), `0x36` new family name (`0x00520B60`), `0x30`
main menu (`0x004CB4D0`), `0x17` campaign / period choice (`0x0041C2D0`), `0x34` mission choice
(`0x0041AAA0`), `0x35` and `0x3B` mission briefing (`0x0041A180`), `0x3D` custom-scenario list
(`0x0041E5C0`), `0x33` victory (`0x004194C0`), `4` timed fade into `[0x006FDCA4]` (`0x0041AA20`,
`0x0041AA60`, reads `timeGetTime`).

Button records are 0x1C bytes (callback, right-click callback `0x0051F780` = no-op, ..., parameter
stored in `0x00D3B198` before the call). Labels below are *assumed* (from what the handler does).

| Path | Handler chain | Reaches `0x005405B0` with | Evidence |
|---|---|---|---|
| Family (player name) | `0x004CB970` / `0x00417FF0` p0 → `0x00516BB0` (clears `0x00E38E74`, lists `Save\*.dat`, ST 0x2F or 0x36) → `0x00418220` in ST '6' → `0x00516C80` (reads the typed name into `0x00E38E74`, creates `Save\<name>`, reloads `campaign.txt` `0x0041AE40`, ...) → main menu `0x004CB6A0` (ST 0x30) | — (sets `0x00E38E74`) | dec |
| Main menu buttons (table `0x005E03F4`) | `0x004CB800` continue / Pharaoh campaign, `0x004CB860` Cleopatra / mission list, `0x004CB910` load a saved game (`0x00402450` dialog), `0x004CB940` custom scenario, `0x004CB970` change family | — | dec, table dump |
| Continue | `0x004CB800`: if `0x00D3B18B` (the last save `0x00700998` exists) → `0x004D5100(0x00700998)` (a save load, not a start); else period choice ST 0x17 with `0x006FDC98` := 0 | — | dec |
| Pharaoh campaign | ST 0x17: period buttons `0x0041BB10` p0–8 (table `0x0057BE78`) set `0x007027AC`; start `0x0041BB70` (table `0x0057BE40`): `0x00E38EBC` := 0, `0x007027B4` := 1 + (`0x006FDC98` ≠ 0), period video (periods 0–4, `0x00413580`), `0x00417DD0(period)` (first mission of the period from the campaign table `0x006FDD08`: `0x00E38EA8`, `0x00E38EAB`, `0x00E38EA9`, `0x00E38EB4` := 0, `0x00E38EB8` := 0, `0x00E38EB0`), then `0x004181E0` → fade → ST 0x35 → **`0x0041A180` calls `0x005405B0(0x00E37078, 0)` once** (`0x007027A8` 0 → 1) | campaign branch, `0x007027B4` = 1 | dec, dis (`0x0041A181`–`0x0041A19D`) |
| Cleopatra campaign | `0x004CB860` → (if `0x0041BAB0`) ST 0x17 with `0x006FDC98` := 1; `0x0041C350` / `0x0041C310` switch between the period screen and the mission list; then as above with `0x007027B4` = 2 | campaign branch, `0x007027B4` = 2 | dec |
| Mission list ("choose mission") | `0x004CB860` or `0x0041C310` → `0x0041F870` (ST 0x3D, list mode `0x00704DB0` := 1) → row click `0x0041F670` (`0x00E38EA8` := mission id of the row `0x0041DFC0`; `0x004D58B0(0, 1)` loads the record for the preview, **without** the scenario start) → start `0x0041F5C0` (table `0x0057CCAC`): `0x007027B4` := 3, **`0x005405B0(0x00E37078, 1)`**, then `0x004181E0` → briefing → `0x0041A180` calls `0x005405B0(…, 0)` a second time (`0x00E38EBC` is 0 by then) | campaign branch twice | dec; run (both calls start mission 5) |
| Custom scenario | `0x004CB940`: `0x00E38EBC` := 2, `0x00605A3C` := 0, list mode 0, ST 0x3D, `0x0041F810("maps\*.map")`; row click `0x0041F670`: `0x00E37078` := file name, `0x004D5820(maps\<name>.map, 1)` (preview: map chunks only); start `0x0041F5C0`: `0x007027B4` := 0, **`0x005405B0(0x00E37078, 0)`**, then ST 1 (the city, no briefing) | custom branch | dec; run |
| Difficulty | `0x00E39080` (0–4): 2 at boot (`0x004147F0`), overwritten by `Pharaoh.inf` (settings block `0x00E38E58`, 0x234 bytes, saved at exit by `0x00414750`); ± buttons `0x00402E50` on the list screen and on the briefing: **on ST '5' (after the start) it also reloads the three model files and resets the treasury** (`docs/commands.md` §3.9, ported `cmd_difficulty`) | read by the start (`0x005D1960`, `0x005D1988`, model files) | dec |
| Replay mission | `0x00418510` (table `0x005C8F44`; thunk `0x004EA830`): custom (`0x00E38EBC` ≠ 0) → `0x007027B4` := 0, `0x005405B0(0x00E37078, 0)`, ST 1; campaign with `0x007027B4` ≠ 1 → `0x004181E0` (briefing → `0x005405B0`); **Pharaoh campaign (`0x007027B4` = 1) → loads `Save\<player>\autosave_replay.sav` with `0x004D51B0`** (a save load keeping the difficulty, written by `0x004D5C30` on the first frame of the mission) | custom: yes; campaign 2/3: yes; Pharaoh campaign: no (save load) | dec |
| Next mission after a win | victory ST 0x33 → `0x00418640`: marks the mission won (`0x006FDD44 + 64 × step`), `0x0041B920` next step, `0x004188F0` (**carry-over**: best formations and monument positions of the finished city into `0x006FDCB8`–`0x006FDD04`, `0x0070278C`–`0x007027A3`), then `0x004181E0` (briefing → `0x005405B0`), or ST 0x34 (choice `0x00418220` → `0x0041B920` → `0x004181E0`), or the next period / end videos | campaign branch; the carry-over is read by `0x00418BF0` | dec |
| Load a saved game | `0x00402720` → `0x004D5100` (ported: `game_load_save`) | — | dec |
| Scenario editor | `0x00417FF0` p2 (`0x00E38EBC` := 2, `0x00605A3C` := 1), map open `0x004099B0` → `0x004DE600` | out of scope | dec |

Campaigns: `campaign.txt` has 9 sections, `[PREDYNASTIC_PERIOD]` … `[NEW_KINGDOM]` (Pharaoh,
periods 0–4 with videos) and `[VALLEY_OF_THE_KINGS]`, `[RAMSES_II]`, `[ANCIENT_CONQUERORS]`,
`[CLEOPATRAS_ALEXANDRIA]` (Cleopatra, periods 5–8) (*confirmed* file). Both kinds load records of the
same `mission1.pak`; the campaign kind only changes `0x007027B4` (1 Pharaoh, 2 Cleopatra, 3 single
mission from the list, 0 custom) (*confirmed* dec of `0x0041BB70`, `0x0041F5C0`).

## 3. The boundary: `0x005405B0(name, flag)`

### 3.1 Inputs

| Input | Meaning | Status |
|---|---|---|
| arg 1 | always `0x00E37078`: map name (no extension needed; `.map` is appended from `0x00576274`) | confirmed dec, all callers |
| arg 2 | 1 = load the campaign record even when custom (mission list, first call) | confirmed dec |
| `0x00E38EBC` | custom flag (≠ 0: custom map; 2 from the menus); the campaign branch sets it to 0 | confirmed dec, run |
| `0x00E38EA8` (i8) | mission = record slot of `mission1.pak` (0x7F = none) | confirmed dec, run |
| `0x007027B4` | campaign kind 0–3 (above) | confirmed dec |
| `0x00E38EA9`, `0x00E38EAA`, `0x00E38EAB`, `0x00E38EB4`, `0x00E38EB8` | period, previous / current campaign step, kingdom carry, missions played: saved before the load and restored after it (the loader overwrites them); with kind 3, `0x00E38EAB` := `0x0041B9D0(mission)` on the campaign table (`this` = `0x006FDD08`) | confirmed dis `0x00540747`–`0x005407BB` |
| `0x00E39080` | difficulty 0–4: treasury % `0x005D1960[d]`, kingdom start `0x005D1988[d]`, model files | confirmed dec, run (Default: 4000 / 1000 / 562 at d = 0 / 2 / 4) |
| `0x00E38E74` | player name, char[32]: copied to `0x00EA184C` | confirmed dec |
| `0x00605A3C` | editor flag (0 in game) | confirmed dec |
| campaign table `0x006FDD08` (64-byte entries, count `0x006FF608`) | read only with kind 3 (`0x0041B9D0`) | confirmed dis |
| carry-over `0x006FDCB8`–`0x006FDD04`, `0x0070278C`–`0x007027A3` | start buildings of linked missions (`0x00418BF0`) | confirmed dec |
| the whole previous state | the custom branch clears records but not everything (§7.1) | confirmed run |

All inputs are in the state mirror (0x0056F000–0x01267AA8).

### 3.2 Steps (campaign branch; *confirmed* dec + dis)

1. `local = "maps\" + name`; `0x006FDCB4` := 1; `0x004DE140` (4 point pairs: 8 dwords at `0x00E94DDC` := −1; corrected by the port, §8).
2. `strcpy(0x00EA184C, 0x00E38E74)`; `0x004D58B0(0, 0)`: returns 0 when `0x00E38EA8` = 0x7F, the
   offset table entry is < 1, or the file does not open; else `_lseek` to the record, `0x004D41E0`,
   restore `0x00E39080`, `CITY+0x44A0` (cheat counter) := 0, `0x004BD360` (`this` =
   `0x00D38360`, floodplain settings), `0x004D52A0` (dis `0x004D5927`–`0x004D596B`).
3. `0x00E38EBC` := 0; treasury `CITY+0x0C` := `0x004CDE40(CITY+0x0C, 0x005D1960[d])`; the name again.
4. (custom branch instead: `0x005409D0(local)` — fails → ST 1, return; `0x00540A10(local)`.)
5. Restore the saved campaign globals (3.1); `0x00E38EB0` := `0x005D1988[d]`; `CITY+0x42D8` :=
   it; `CITY+0x4450` := `0x00E38EB4` (0 when custom); `CITY+0x444C` := (i16) `0x00784B62`;
   `CITY+0x2C9C` := min(that, 10); `CITY+0x2C98` := `0x005D0320[CITY+0x2C9C]`; `CITY+0x4720`,
   `CITY+0x4724`, `0x0078D810`–`0x0078D82C` := 1; `0x0078D830` := (campaign and mission 0),
   `0x0078D828` := (campaign and mission ≤ 4).
6. Building availability: campaign and mission < 11 → `0x007027B8[]` and `0x006FDC88[]` from the
   table `0x005794B0` (0x1C-byte entries up to `0x00579794`: enabled when entry−3 < mission);
   otherwise all enabled.
7. `0x00404350`, `0x004051A0` (ported), `0x004E1770` (message list reset; `0x004E1830` reads
   `timeGetTime`), `0x00418BF0` (mission flag string, text group 0x133 via `0x004CC110`: carried
   monuments placed with `building_create` `0x0046A400` and `building_map_add` `0x00470990`,
   formation entries cleared unless flag 2 is '1', then the terrain rebuild `0x004DEA50`;
   corrected, §8), restore `CITY+0x42D8` / `+0x4450` when
   campaign, `CITY+0x47A8` := difficulty, byte `0x00784CBD` := (`0x00784CBD` == 1), `0x00489B40`
   (profiling timers: `QueryPerformanceFrequency` into `0x00BCA848`, `0x00D05710`, clears
   `0x00BCA850`; read only by the profiler `0x00489A50` / `0x00489A80`: not simulation).

### 3.3 Custom branch: `0x00540A10` and `0x004DE820` (*confirmed* dec; areas *assumed* from names and sizes)

`0x00540A10`: inline flags, `0x004E7D10` (city records cleared and initialised, `0x004E9EE0`),
`0x004E7E10`, `0x00EAAC28` := 1, `0x004E1770`, `0x00540B50` (view and UI reset), `0x0040F250`,
`0x0041D730` / `0x0041DF60` (ambient sound), `0x00404330`, record clears `0x0046A230` (buildings
`0x007999D8`, 0x101D00 bytes), `0x0046B0E0`, `0x0046EFB0`, `0x0046F120`, `0x0046A260`,
`0x004B4760` (figures), `0x004B8C90`, `0x004B58D0`, `0x004B8290` (formations), `0x004B32A0`,
`0x00451BE0` (date and counters), [editor only: `0x004DE430`, `0x00443EA0`, ...], then
`0x004DE820(local)`, then the player name, `CITY` flags and the treasury rescale.

`0x004DE820`: `0x00EAAC28` := 1, **`0x004DE600`** (map load: `0x0046FA40` pre-load clear (ported),
`0x004D5820(maps\<name>.map, 0)` → `0x004D4AE0` (chunks 1–23 of the `MAPS` container through the
chunk reader `0x004D0E10`, version upgrades `0x004D17E0` …), the graphics slots `0x004F4520`,
`0x0053D970`, `0x004BC720`; map size, `0x0046F7B0`, the 16 map points, `0x00430530`, `0x004BBC10`,
`0x004BB3F0`, `0x004D5980` (image grid, floodplain, vegetation := −1, `0x004860D0`, ...),
`0x004DEA50` terrain rebuild, `0x0046D480` (buildings for tiles flagged as buildings),
`0x004DDF90`, `0x004DE080`, the five routing grids), the scenario info copied over itself
(`0x0040F860`), `0x004BBD40(x, y, flag)` (initial figures, uses CRT `rand` `0x00562348`), routing
grids again, entry / exit points into `CITY+0x2778`…`+0x2784`, treasury `CITY+0x0C` and
`CITY+0x2CBC`, year `0x0078D7BC`, deadline `0x0078D7B8`, `0x00784F10`, `0x00444720`,
`0x00444B10` (trade / empire), camera `0x00775910` / `0x0077590C` and `0x00442800`, `0x00404350`,
`0x004051A0` (save.md §5 has the city fields).

## 4. Files read on the start path (*confirmed* run: file opens logged in the oracle)

| Path | File | Original function | Port today |
|---|---|---|---|
| custom | `maps\<name>.map` (open/close check, then the whole file) | `0x004CE3F0` (CRT `_open`), `0x004D5820` → `0x004D4AE0` (`_read` / compressed `0x0055BCF0`) | `core/io/map_file.c` parses the container into chunks; not yet onto the state |
| campaign | `mission1.pak`: 4 bytes at `4 × mission`, then the record from its offset | `0x004CE480` (`_open`, `_lseek` `0x005662E3`, `_read`; on failure the CD-drive retry `0x004CE9E0`), `0x004D58B0` (`_open`, `_lseek`, `0x004D41E0`) | `core/io/campaign_pak.c` (record bytes), `game_load_save` |
| campaign | `Pharaoh_Model_<D>.txt`, `Figure_model_<D>.txt`, `Tax_Sentiment_Model_<D>.txt` | `0x00540C90`, `0x00540ED0`, `0x00541160` in `0x004D52A0` | ported (`scenario_start`, `game_env.file`) |
| both | `data\PYRAMID.sg3/.555`, `data\LibExtraDummy.sg3/.555`; enemy graphics on some missions (e.g. `data\CANAANITE.sg3/.555`) | slots `0x004F4520`, `0x004BC720` → `0x0053DD90` → `0x0053DFF0` (`0x004CEB60`), pixels `0x0053F660` | `0x0053DD90`, `0x0053DFF0` ported (`platform_file_read`); `0x0053F660` platform request |
| replay (Pharaoh) | `Save\<player>\autosave_replay.sav` | `0x00418510` → `0x004D51B0` | a player file, not a game data file |

Not read by the start: `Pharaoh2.emp`, `campaign.txt`, the `.txt` next to the maps (*confirmed* run
for Default and mission 0; the `.txt` files and `Pharaoh2.emp` are read by other screens or not at
all, *assumed*). `campaign.txt` is read at boot (`0x0041AE40`, `this` = `0x006FDD08`) and when a
family is created.

**Is `platform_file_read` enough?** It gives (path, offset, size) reads relative to the game
directory, case-insensitive, `\` as `/` (*confirmed*, core/sim/platform.h). That covers every
read above: the CRT `_open` / `_lseek` / `_read` sequences become offset reads (the port keeps the
position), the existence checks `0x004CE3F0` / `0x005409D0` a size query (−1 = missing). The CD
retry `0x004CE9E0` (a missing `mission1.pak`) is an error path: the port should report the file
as missing (*assumed* acceptable: it waits for a CD). `autosave_replay.sav` is a player file: it
belongs with the save-file request of the platform (a save load, already ported), not with game
data.

## 5. The oracle

### 5.1 What had to be added (scratch scripts only; to move into `tools/oracle`)

1. **CRT `_lseek` `0x005662E3`** is not replaced by `game.py`: `0x004CE480` and `0x004D58B0` seek
   in `mission1.pak` with it. Without a hook the campaign start fails silently (`0x004D58B0`
   returns 0, nothing loaded: year 0, treasury 0) (*confirmed* run). The corpus never needed it:
   `corpus.py --pak` extracts a record with `build/save_dump` and loads it as a save
   (`Game.load_save` → `0x004D5100`), which is **not** the mission start (§5.3).
2. **`QueryPerformanceFrequency`** (KERNEL32) stops the run at `0x00489B4C`; a stub returning 0
   (no counter) is enough: only the profiler reads its results (*confirmed* dec).
3. **CRT streams** (`fopen` `0x00561A0A`, `fgets` `0x00561E4D`, `fread` `0x00561CA1`, `fclose`
   `0x00561994`) are not replaced: `fopen` returns 0 in the emulator (*confirmed* run). So the
   oracle's boot has **no campaign table** (`0x006FF608` = 0, *confirmed* run), no `Pharaoh.inf`
   (empty in the GOG install anyway) and no family files. The start itself reads only `_open`
   files, but kind 3 (`0x0041B9D0`) and every campaign-progress handler (`0x00417DD0`,
   `0x0041B920`, `0x00418640`) read the campaign table: hook the stream functions like `_open`.

### 5.2 Results (*confirmed* run; boot, set the globals of §3.1, call `0x005405B0`)

| Set | Started | Notes |
|---|---|---|
| `Maps/*.map` (33 files) | 33 / 33 | `0x00E38EBC` = 2, `0x007027B4` = 0, flag 0; 0.3–9 s each |
| `mission1.pak` records 0–52 | 53 / 53 | `0x00E38EBC` = 0, `0x007027B4` = 1; 0.6–9 s each |
| `mission1.pak` slots 53–59 | — | empty slots: `0x004D58B0` returns 0 and the start returns early (as the game does) |
| mission list (kind 3, flag 1, then flag 0) | mission 5 | both calls start it; `0x00E38EAB` = 0 because the oracle has no campaign table |
| difficulty 0 / 4 on Default | yes | treasury 4000 / 562 (1000 at 2) |

No run stopped on a missing stub, a message box or a video after the two additions. Map start
values (year, treasury, map width) are in the scratch logs (§6).

### 5.3 Campaign start against a save load of the same record (*confirmed* run, mission 0)

Booted twice; one side `Game.load_save(record 0)` (`0x004D5100`), the other the mission start.
1,275 bytes of the data segments differ (heaps equal in size): start flags `0x0078D810`–`0x0078D830`,
the availability tables `0x007027B4`… and `0x006FDC88`…, `0x00D3B1B8` (set by `0x004D5100` only),
build-menu flags `0x0070AD14`…, the text cursor `0x00E20768` (`0x00418BF0`), and 2-byte values every
0x40 bytes from `0x011829F8` (*assumed* image entries of a graphics slot). So the campaign start is
the ported save load plus the start-only steps of §3.2.

## 6. Frontier (*confirmed* run)

Recorded with `DeepRecorder(g, native)`, native = the ports of `build-final/state_run --list` plus
the start's own dispatchers made transparent (`0x00540A10`, `0x004DE820`, `0x004DE600`,
`0x004D5820`, `0x004D58B0`, `0x004D52A0`, `0x00418BF0`, `0x004E1770`), so the list shows the
unported functions under them; bytes = bytes written in the state (segments and heap), summed over
the runs. 

| Function | Code bytes | Maps: runs / calls / bytes written | Missions: runs / calls / bytes written | Area (*assumed* unless in a port) |
|---|---|---|---|---|
| `0x004D41E0` | 1334 | — | 53 / 53 / 209,748,203 | loader (ported as save_file_load_state) |
| `0x0046A230` | 41 | 33 / 33 / 34,848,264 | — | reset: buildings clear |
| `0x004B4760` | 58 | 33 / 33 / 25,608,264 | — | reset: figures clear |
| `0x004D5980` | 329 | 33 / 33 / 19,044,838 | — | map setup: grids, floodplain, vegetation |
| `0x004B32A0` | 29 | 33 / 33 / 16,566,000 | — | reset: clear 0x00C8AD38 + 0x00D04E58 |
| `0x00540B50` | 309 | 33 / 33 / 14,124,165 | — | reset: view / UI |
| `0x004D4AE0` | 667 | 33 / 33 / 3,228,194 | — | map load: MAPS chunks + upgrades |
| `0x004BD740` | 264 | — | 41 / 55693 / 1,745,026 | scenario start: floodplain objects (sim heap) |
| `0x004F4520` | 1013 | 33 / 33 / 547,883 | 53 / 53 / 1,046,823 | graphics slots (monuments) |
| `0x0046B0E0` | 21 | 33 / 33 / 1,293,600 | — | reset: storage clear |
| `0x004E7D10` | 156 | 33 / 33 / 1,247,862 | — | reset: city records init |
| `0x0041D730` | 2088 | 33 / 33 / 295,812 | 53 / 53 / 475,092 | ambient sound (presentation) |
| `0x0046EFB0` | 21 | 33 / 33 / 369,600 | — | reset: clear 0x00EAAF68 |
| `0x00489B40` | 38 | 33 / 33 / 116,193 | 53 / 53 / 186,613 | profiler timers (not simulation) |
| `0x00444720` | 434 | 33 / 33 / 295,700 | — | setup: trade-route limits |
| `0x00444B10` | 31 | 33 / 33 / 290,532 | — | setup: clear 0x0077AF00 |
| `0x004B8290` | 68 | 33 / 33 / 237,996 | — | reset: formations clear |
| `0x00404330` | 20 | 33 / 33 / 150,480 | — | reset: build menu flags := 1 |
| `0x004BBD40` | 1013 | 33 / 33 / 118,578 | — | initial figures (rand) |
| `0x004BB3F0` | 2075 | 33 / 33 / 96,636 | — | initial figures / formations |
| `0x0046F120` | 21 | 33 / 33 / 72,600 | — | reset: clear 0x009784D8 |
| `0x00485F70` | 176 | — | 53 / 53 / 65,267 | scenario start: building images / grid |
| `0x004BBC10` | 303 | 33 / 33 / 48,432 | — | initial figures (rand) |
| `0x0053D970` | 224 | 33 / 33 / 24,199 | 53 / 53 / 22,305 | graphics |
| `0x00451BE0` | 106 | 33 / 33 / 16,500 | — | reset: date and counters |
| `0x00444A80` | 143 | 33 / 33 / 14,229 | — | setup: empire / trade |
| `0x0040F250` | 32 | 33 / 33 / 8,415 | — | reset: clear 0x00605A6C |
| `0x004B58D0` | 534 | 33 / 33 / 5,544 | — | reset: figure name counters (rand) |
| `0x004426C0` | 102 | — | 53 / 53 / 4,602 | scenario start |
| `0x004DE140` | 24 | 33 / 33 / 1,056 | 53 / 53 / 1,696 | start: map point pairs := -1 |
| `0x00442660` | 96 | — | 53 / 53 / 2,664 | scenario start |
| `0x004CC560` | 87 | 33 / 66 / 2,178 | — | UI text boxes |
| `0x004E1830` | 29 | 33 / 66 / 792 | 53 / 106 / 1,272 | message timer (timeGetTime) |
| `0x004E59A0` | 20 | — | 53 / 53 / 2,052 | scenario start |
| `0x0046A260` | 23 | 33 / 33 / 1,650 | — | reset: clear 0x009783C0 |
| `0x004B8C90` | 82 | 33 / 33 / 1,419 | — | reset: queue release |
| `0x004BD360` | 28 | — | 53 / 53 / 1,060 | floodplain settings copy |
| `0x004BD330` | 46 | 33 / 33 / 1,056 | — | floodplain settings (ported inline in load_game) |
| `0x00476750` | 125 | — | 53 / 53 / 792 | scenario start |
| `0x004D5AD0` | 236 | — | 53 / 53 / 710 | scenario start |
| `0x0041DF60` | 13 | 33 / 33 / 264 | 53 / 53 / 424 | ambient sound |
| `0x004D5BC0` | 110 | — | 53 / 53 / 424 | scenario start |
| `0x0053F660` | 303 | 29 / 29 / 116 | 50 / 50 / 200 | pixels (platform request) |
| `0x004E7E20` | 30 | — | 53 / 53 / 212 | city flag (inline in scenario_start) |
| `0x00417E70` | 11 | — | 53 / 53 / 212 | scenario start (inline in scenario_start) |
| `0x004BCA90` | 66 | — | 53 / 53 / 212 | scenario start |
| `0x004CE3C0` | 46 | 33 / 33 / 165 | — | file name helper |
| `0x004E7E10` | 11 | 33 / 33 / 132 | — | reset: 0x00EA1820 := 0 |
| `0x0046D480` | 953 | 33 / 33 / 132 | — | setup: buildings for flagged tiles |
| `0x005409D0` | 63 | 33 / 33 / 99 | — | map file exists |
| `0x004CE3A0` | 21 | 33 / 66 / 33 | — | file name helper |
| `0x004CE2D0` | 50 | 33 / 33 / 0 | 53 / 53 / 0 | working directory |
| `0x00568EAB` | 23 | 33 / 33 / 0 | 53 / 53 / 0 | CRT _open |
| `0x005647EB` | 179 | 33 / 33 / 0 | 53 / 53 / 0 | CRT _close |
| `0x00442800` | 106 | 33 / 33 / 0 | — | camera clamp |
| `0x004CE480` | 602 | — | 53 / 53 / 0 | file read at offset |
| `0x005662E3` | 154 | — | 53 / 53 / 0 | CRT _lseek |
| `0x004CE6E0` | 374 | — | 53 / 159 / 0 | game log |
| `0x004771C0` | 76 | — | 53 / 53 / 0 | scenario start |
| `0x004B32C0` | 81 | — | 53 / 53 / 0 | scenario start |
| `0x0046F190` | 42 | — | 53 / 53 / 0 | scenario start |
| `0x004A3560` | 128 | — | 53 / 53 / 0 | scenario start |

Runs: 33 maps, 53 records. "Code bytes" = the function's own size (Ghidra), not its callees.
Three entries are mostly ported code underneath (a third run with them transparent, Default
map): `0x004D5980` leaves only `0x004771C0`, `0x004BD360`, `0x004E59A0`, `0x00442660`,
`0x004426C0`, `0x0046F190`, `0x004D4040` (< 0.5 KB written; the grids are written by ported
`0x0040F8F0`, `0x00486E70`, `0x004860D0`); `0x00540B50` leaves `0x004DDF20`, `0x004CDD60`,
`0x004CC560` (×6), `0x004E2AB0` (< 1 KB; the view tables are ported `0x00430530`, `0x00481F20`);
`0x004E7D10` leaves `0x004E9EE0` and `0x004E7E20`.

Already covered by the save-load port: `0x004D41E0` (as `save_file_load_state`, from parsed
chunks: `game_load_save` already is the "record bytes → state" entry the campaign path needs),
`0x004BD330` (`floodplain_settings_reset`), `0x004E7E20` and `0x00417E70` (inline in
`scenario_start`), the five routing grids, the terrain rebuild, the models, `0x0053DD90` /
`0x0053DFF0`. The rows marked "scenario start" are the `not_ported` calls of
`core/state/scenario_start.c`: they block the save load's state check too. New for 6.1: the
whole custom-map column (reset, map load, setup, initial figures) and, on the campaign side,
only `0x004DE140`, `0x004E1770` / `0x004E1830`, the unported placers of `0x00418BF0`
(`0x004DF2C0`, `0x0040D7F0`, `0x004E65A0`, `0x00547AD0`, reached only with a carry-over block)
and `0x00489B40`.

Scratch scripts (not part of the repository):
`/private/tmp/claude-501/-Users-doud-Dev-imhotep/4186130b-ae89-4a73-856e-c263b52356f1/scratchpad/newgame/`
`start.py` (boot + start, with the two additions), `front.py` (frontier), `agg.py`, `prior.py`
(state dependence), `loadvsstart.py`, `files.py` (file opens).

## 7. The check

### 7.1 Starting state

- The start reads more than its inputs: custom-map starts clear the record arrays but leave parts
  of the previous city. Starting Default after Alexandria instead of after the boot differs in
  1,852 segment bytes (row-range tables `0x00BC9A34`…, `0x0096E6B4`, `0x009720F8`, `0x00D3B1C8`,
  the empire-route table `0x00776xxx`) and in the heap (400 vs 73,744 bytes used: the bump
  allocator never reuses); mission 0 after mission 51: 38,706 bytes (`0x00F21D04`: 66 KB)
  (*confirmed* run). Whether the tick reads these bytes is open (their pages are in the tick's
  read sets, a byte-level answer needs `tools/oracle/scratch.py`).
- So both sides start **from the same state file**: the oracle's booted state (`Game.boot()` then
  `state_bytes()`), as `oracle_state_check.py` does for loads. The boot itself is not ported
  (`platform/exe_image.c` gives only the exe's initial data segments; the boot runs ~30 functions,
  reads the text, 30 graphics slots, the models): porting it is a separate item, not needed for
  6.1.
- Add the menu inputs on top: `0x00E37078` name, `0x00E38EBC`, `0x00E38EA8`, `0x007027B4`,
  `0x00E39080`, `0x00E38E74`, optionally the carry-over block.

### 7.2 Proposed test (`tests/differential/oracle_newgame_check.py`)

For every bundled map and every record 0–52, at difficulty 2 (and 0 and 4 for a few):
1. Oracle: boot, write the inputs, `state_bytes()` = S0; run `0x005405B0(0x00E37078, 0)`
   (with the deep recorder while ports are partial); `state_bytes()` = S1.
2. Port: `build/state_start S0 OUT name|mission` (new tool beside `state_load`, with
   `--map files=DIR` for the game files) → S1'.
3. Compare S1 and S1' byte for byte (excluded.py, plus the profiler block of `0x00489B40`).
4. Then lockstep: `lockstep.py` from a state file (it now takes a save: add a `--state` or
   `--start map:NAME|mission:N` option) for N ticks (a month: ~1,550 ticks), so the initial
   figures (`0x004BBD40`, CRT `rand`) and the first-tick reactions are covered.
5. Variants: mission list (flag 1 then 0), a carry-over block set by hand (no won state in the
   corpus: `0x004188F0` runs only after a victory), and the briefing difficulty change
   (`0x00402E50` on ST '5', already a command test).

The first frame in the city (`0x004D6B60` → `0x004D5C30`) writes the autosaves and `CITY+0x471C`;
it is outside the tick (*confirmed* dec), so it stays out of the comparison unless the frame is
ported.

## 8. Proposed packages

1. **Oracle and check** (tools only): `_lseek` hook, `QueryPerformanceFrequency` stub, CRT
   stream hooks (campaign table at boot), a start driver, `state_start` on the port side,
   `oracle_newgame_check.py`, lockstep from a state file. Unblocks everything else.
2. **Campaign start**: `0x005405B0` shell, `0x004D58B0` on `game_load_save`'s pieces, the
   start-only steps (`0x004DE140`, `0x004E1770` + `0x004E1830`, availability tables, CITY fields,
   `0x00418BF0`, `0x00489B40` as a no-op on profiler state), and the scenario-start steps still
   `not_ported` that the records reach (`0x00485F70`, `0x004E59A0`, `0x00442660`, `0x004426C0`,
   `0x004BD740` floodplain objects, `0x004BD360`, `0x00417E70`, `0x004BCA90`, `0x004D5AD0`,
   `0x004D5BC0`, `0x00476750`, ...): shared with save loading.
3. **New-game reset** (`0x00540A10` without the setup): `0x004E7D10` / `0x004E9EE0`, `0x00540B50`
   (`0x004DDF20`, `0x004CDD60`, `0x004CC560`, `0x004E2AB0`), the record clears (`0x0046A230`,
   `0x0046B0E0`, `0x0046EFB0`, `0x0046F120`, `0x0046A260`, `0x004B4760`, `0x004B8C90`, `0x004B58D0`,
   `0x004B8290`, `0x004B32A0`), `0x00451BE0`, `0x0040F250`, `0x00404330`, `0x0041D730` /
   `0x0041DF60`. Mostly memsets: large in bytes, small in code.
4. **Map load and scenario setup**: `0x004DE820`, `0x004DE600`, `0x004D5820`, `0x004D4AE0` (the
   `MAPS` chunk reader and its version upgrades onto the state, reusing `core/io/map_file.c` and
   the save reader's chunk code), `0x004D5980`, `0x00444A80`, `0x00444720`, `0x00444B10`,
   `0x00442800`, `0x0046D480`.
5. **Initial figures and graphics slots**: `0x004BBD40`, `0x004BB3F0`, `0x004BBC10` (CRT `rand`),
   `0x004F4520`, `0x0053D970` (both paths and the save load).
6. *(Optional, UI flow)* the menu handlers of §2 as commands (`0x0041BB70`, `0x00417DD0`,
   `0x00418640` + `0x004188F0`, `0x00418220`, `0x0041F5C0` / `0x0041F670`, `0x00418510`) and the
   campaign table loader `0x0041AE40`: needed for campaign progression, not for 6.1's check.

## 8. Results of the ports, and corrections

Ported in core/newgame/ (start, reset, setup, map load, initial figures, graphics slots) and
core/state/loader.c (campaign records, saved games, scenario start `0x004D52A0` and its steps).
`tests/differential/oracle_newgame_check.py`: every bundled map and every mission1.pak record,
difficulties 0 / 4, the mission-list path, a carry-over block, starts from a running city: the
state after `0x005405B0` equals the original's, and a day of ticks after it too.
`tests/differential/oracle_state_check.py`: every saved game of the corpus loads equal.

The campaign flow (core/newgame/flow.c, flow2.c): campaign.txt, the progress file written and read
back (`0x0041B680`: only the mission records `+0x2CD4`, the string `+0x2C90` and the words `+0x4A84`
come from the file; the steps' won flags `+0x3C` follow the records), the family list and a new family
(`0x00516990` … `0x00516C80`), the main menu, the scenario list with the maps directory's names
(FindFirstFileA as `platform_file_list`), the period start `0x0041BB70` (its Bink video
`0x00413580` a platform request), the next mission, replay, the mission choice `0x00418220`.

Corrections from the ports (dis):
- `0x004DE140` sets 4 point pairs (8 dwords at `0x00E94DDC`), not 16.
- `0x00418BF0` copies the carry-over block `0x006FDCB8` to `0x006FDCE0`, sets `0x00E38EB4` from
  flag 0 ('1' keeps the previous `0x006FDCB8`), places the carried monuments ('A': 0xDE, 0xE1 / 0xE4
  on mission 0x33; 'V': 0xE5, 0xEA after mission 0x27, 0xEB after 0x28) and clears 3 formation
  entries unless flag 2 is '1'; it places no formations.
- The record load overwrites `0x0070278C`–`0x00702797`: 'V' monument positions set before a start
  are lost there, `0x00702798`… survives (as in the original). How they reach the next mission in
  play (the progress file written by `0x0041B8A0`?) is open.
- `0x004D3C40` (record versions below 154) also writes the six carry-over records around chunk 175
  (`0x006FDCBC`–`0x006FDCD8` := 0xFF / −1).
- The file maps' chunk 18 is 76 × 200 bytes against the exe's 98 × 200: the chunk reader
  `0x004D0E10` migrates it on every map.
- `0x005405B0` passes the path it builds on its stack to `0x00540A10`, which only forwards it to
  `0x004DE820`, which never reads it.
- The ambient-sound slots `0x00702900` (0x2300 bytes, cleared and filled by `0x0041D730` in the
  reset) are save record 92: state, read only by drawing and the frame's sound step.
- The campaign flow is ported too (core/newgame/flow.c, flow2.c): the progress file is read by
  `0x0041B680` (fopen "rb", 0x4A9C bytes into `0x006FDD08`; called from `0x00516AB0`), not by
  `0x0041B920` (the next campaign step), and written by `0x0041B8A0` (a platform file write); the
  Pharaoh-campaign replay `0x00418510` loads `Save\<player>\autosave_replay.sav`; the main menu
  and the scenario list list files through a platform listing request (FindFirstFileA order);
  the Bink videos `0x00413580` are a platform request. The check covers next mission, replay of
  each campaign kind, families and progress files, the main menu, the period choice.
