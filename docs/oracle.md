# The oracle: Pharaoh.exe in a CPU emulator

`tools/oracle/` runs the reference `Pharaoh.exe` (GOG 2.1) headless under Unicorn, so its own
code produces reference data: loaded states, advanced states, saves, and the effect of any single
function. Nothing is launched as a Windows program and nothing from the game leaves `local/`.

Set up once: `make oracle-env` (a venv in `local/venv` with Unicorn and Capstone).

## Files

| File | Role |
|---|---|
| `emu.py` | `Pharaoh`: maps the exe at 0x00400000, binds every import to a stub, SEH (TIB through `fs`), stack and bump heap; `call(addr, *args, ecx=)`, `hook(addr, fn)`, memory access, `load_image` |
| `game.py` | `Game`: CRT file and heap functions over a virtual file system, `boot()`, `load_save()`, `save()` |
| `corpus.py` | reference save corpus: a state advanced month by month, saved by the game |
| `diff.py` | two states compared field by field (names from `docs/formats/save.md`) |
| `harness.py` | original functions and their ports on the same state (`build/state_replay`), unported callees replayed |
| `replay.py` | deep recorder: every outermost call to an unported function inside an original call, with its changes and return value (docs/porting.md §4) |
| `lockstep.py` | our tick against the original's, tick by tick, unported functions replayed |
| `frontier.py` | the unported functions an original call (or the tick) still reaches: what to port next |
| `coverage.py` | every function the tick enters on the corpus, with its callers (indirect calls included) |
| `attribute.py` | which call of a function wrote which bytes (snapshots at every call site) |
| `readset.py` | pages of the state the tick reads (page protection) |
| `scratch.py` | is a variable written before every read within a tick |
| `watch.py` | who writes an address |
| `excluded.py` | state the comparisons ignore, with the reason |
| `state_run.c`, `state_replay.c`, `state_load.c`, `state_save.c` | the port's side: ported functions (alone, or with a replay of the original's unported calls), load, save on state files |
| `where.py`, `disasm.py` | address → exported function; Capstone disassembly of the exe |

## What is replaced, and why it does not change the simulation

- **Imports.** Each DLL function lands on a stub. Emulated: heap and memory, directory and file
  queries, `GetSystemMetrics` (1024 × 768), `GlobalMemoryStatus` (256 MiB),
  `timeGetTime` / `GetTickCount` (0), `QueryPerformanceFrequency` / `Counter` (no counter: 0;
  read only by the profiler). Audio and video DLLs (MSS32, BINKW32, SMACKW32) return 0.
  Any other call stops the run with its name and caller.
- **Heap.** Allocations made while a function of `SIM_ALLOCATORS` is on the call stack go to the
  simulation heap (0x60000000), the others to a separate heap; both bump-allocate in 16-byte
  steps and zero-fill freed blocks, as core does (docs/formats/state.md). The file lists' buffers
  (`0x0041C740`: the names of the scenario list `0x00704C00`, which choose the map a start loads, and
  of the family list `0x00EADFA8`) are in the simulation heap: the ports allocate them with
  `state_alloc` (core/newgame/flow2.c).
- **CRT** (statically linked): `_open`, `_read`, `_write`, `_close`, `_lseek`, `_filelength`, `_chdir`,
  the streams `fopen` / `fgets` / `fread` / `fseek` / `fwrite` / `fclose` (campaign.txt and Pharaoh.inf
  at boot, the progress file),
  `malloc` / `calloc` / `realloc` / `free`, `new` / `delete`, `atexit`, replaced by Python. The C
  initialiser table (CRT internals) is not run; its effect on the simulation, the x87 precision
  (53-bit, `0x005640EC`), is set directly.
- **Hooks and translation.** Unicorn applies a new code hook only to code it translates after the
  hook is added: tools that hook after the boot or the load flush the translation cache
  (`ctl_flush_tb`, `ctl_remove_cache`).
- **Start-up.** `Game.boot()` runs the C++ static constructors (`0x00576000` … `0x0057607C`), then
  WinMain's (`0x00413ED0`) initialisation calls in order, with their `this` pointers from the
  disassembly. Skipped: the CD drive search `0x004268C0` (its result, the drive letter, is set),
  DirectDraw `0x004CF960`, sound start `0x005394F0`, the intro video and the message loop. Of
  `0x004CF960` only the Bink player's object is made (a 0x1C-byte block of the other heap, constructor
  `0x004133D0`, its pointer in `0x006FDC54`): the period videos `0x00413580` then run as in a game
  whose `BinkOpen` fails (the BINKW32 stubs return 0), which is what the port's request
  `platform_video_bink` does without a platform. States cached before this change
  (local/oracle/cache) hold 0 there.
- **Display-only game functions**, replaced by no-ops (`game.py` `PRESENTATION`): the loading
  screen text `0x004CF7B0` (GDI text only) and the screen fade / present `0x004D0280` (writes
  only its fade counters `0x00E92E08`, `0x00E38DC8`).
- **Game log** `0x004CE6E0`: collected in `Game.log`.
- **Modal message box** `0x00538CD0` (trade refusals and the like): its Win32 message loop waits for
  the player. `game.py` `_message_box` presses the first button at once and makes the same state
  writes as the port's `platform_message_box` (button count, pressed flag and index, redraw flag;
  docs/commands.md §7). Other `MessageBoxA` calls still stop the run: they report errors.
- **Game files on the port's side.** The ports read game data files through the platform
  (`platform_file_read`, e.g. the `.sg3` indexes of `0x0053DFF0`). `replay.py` `external_maps`
  passes `--map files=DIR` (the game directory, `IMHOTEP_GAME_DIR`) to `build/state_replay`, which
  then serves those reads from DIR, names matched case-insensitively and `\` read as `/`, as
  `game.py` resolves the original's file opens: both sides read the same bytes. The `.555` pixel
  load `0x0053F660` is a platform request (`platform_pixels_load`): on the port's side
  `state_replay` returns the original's buffer address from the recorded call, so the stored
  handle (slot `+0xCC24` and the image entries derived from it) is the original's.
- **Player files.** The campaign progress file `Save\<family>.dat` is written with the CRT stream
  `fopen(path, "wb")` / `fwrite` (`0x00561F5F`) / `fclose` (`0x0041B8A0`): `game.py` keeps a "wb"
  stream as a new file of `Game.files` (lower-case name, `/`). On the port's side such writes are a
  platform request (`platform_file_write`, core/sim/platform.h); `state_replay` prints
  `file_write PATH SIZE` and, when `IMHOTEP_PLAYER_FILES` names a directory, writes the file there and
  serves the platform's reads from it before the game directory (saved games such as
  `Save\<family>\autosave_replay.sav`). `tests/differential/oracle_newgame_check.py` gives player
  files to both sides and checks that every file the original writes is written by the port with
  the same bytes. The progress file is read back with `fopen(path, "rb")` / `fread` / `fseek`
  (`0x0041B680`): `game.py` hooks `fread` and `fseek` on its streams.
- **Directory listings.** `FindFirstFileA` / `FindNextFileA` / `FindClose` (the file lists
  `0x0041C8F0`: `maps\*.map`, `Save\*.dat`, `save\<family>\*.sav`) list, in `game.py`, the files of
  `Game.files` in the pattern's directory (and the directories their paths imply) then those of the
  game directory, names compared case-insensitively (`Game.names` keeps a player file's name as it was
  written); `state_replay`'s `platform_file_list` lists `IMHOTEP_PLAYER_FILES` then the game
  directory the same way. Both give the names in NTFS order: by name, upper-cased, byte by byte
  (*assumed*: the order of `FindNextFileA` on the GOG install's NTFS volume; the game sorts the
  scenario and family lists itself with `_stricmp`, so the order matters only to names equal but for
  case, which NTFS does not hold, and to the main menu's count). Patterns are matched on the long
  name only, `*` and `?` (*assumed*: Windows also matches 8.3 short names, so `*.sav` would list
  `x.save`; not reproduced).
- **Player commands.** `lockstep.py --commands FILE` applies commands between ticks: each line
  names a UI handler (docs/commands.md) and the globals to set first (hovered tile, menu item,
  button parameters, confirmation answer); the handler runs in the original and in the port from
  the same state. `tests/differential/oracle_command_check.py` holds the command tests.

## Known differences from a real run

- `timeGetTime` returns 0. The game reseeds the CRT `rand` with it at some points (`0x004EAEC0`,
  `0x004EB000`), so a real run is not reproducible there and the oracle's run is; the port takes
  the seed as an input.
- No player input except the scripted commands: without them the oracle advances a city exactly as
  an idle player would.

## Checks so far

- Our loader against the original's (`tests/differential/oracle_load_check.py`): all 181 saved
  variables equal on the 53 `mission1.pak` records (version upgrades included) and the 7 player
  saves, from the same CRT `rand` state.

- Desirability (`0x0046A0C0`, `0x0046A140`) after the original's load, on Iken and sais 6374:
  stored grid = emulated original = port, on all 51,984 tiles.
- A month advanced by the original and saved by its saver reads back with `save_file_read`;
  desirability and the loader checks pass on it.
