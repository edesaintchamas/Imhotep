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
| `screen.py` | back buffer → PNG (into `local/oracle/screens`), pixel differences as rectangles; `screen.py SAVE --display WxHxF --window ID` draws a frame |
| `framestep.py` | frame lockstep: the original's frame `0x004D6B60` (and its window procedure) against the port's, frame by frame, state and back buffer |
| `sound.py` | a working stand-in for the Miles sound driver (`Game(sound=True)`) and the translation of its call log into the port's sound requests |

## What is replaced, and why it does not change the simulation

- **Imports.** Each DLL function lands on a stub. Emulated: heap and memory, directory and file
  queries, `GetSystemMetrics` (1024 × 768), `GlobalMemoryStatus` (256 MiB),
  `timeGetTime` / `GetTickCount` (`Game.clock`, 0 unless a tool sets it), `QueryPerformanceFrequency` / `Counter` (no counter: 0;
  read only by the profiler). Audio and video DLLs (MSS32, BINKW32, SMACKW32) return 0 (with sound,
  MSS32 is `sound.py` FakeMiles: "Sound").
  Any other call stops the run with its name and caller.
- **Heap.** Allocations made while a function of `SIM_ALLOCATORS` is on the call stack go to the
  simulation heap (0x60000000) — unless a function of `NOT_SIM_ALLOCATORS` is nearer on the stack (the
  spare screen made again by `0x00414BD0` after a display mode change, e.g. the 640 × 480 switch around
  a period video inside a mission start: platform memory in the port) — the others to a separate heap; both bump-allocate in 16-byte
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
  disassembly. Skipped (without a display, the default; see "Display"): the CD drive search `0x004268C0` (its result, the drive letter, is set),
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
  the player. `game.py` `_message_box` presses the first button at once (or the next of
  `Game.box_answers`, a test's button indexes, as `build/imhotep`'s script command `answer N`) and makes
  the same state writes as the port's `platform_message_box` (button count, pressed flag and index, redraw flag;
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
- **Deleting player files; window-system requests.** The CRT `remove` `0x00562BB5` (a saved game
  `0x004D4D80`, a family `0x00516DD0`) calls `DeleteFileA`, the family deletion also
  `RemoveDirectoryA` on the family's directory. In `game.py` `DeleteFileA` removes a file of
  `Game.files` (logged in `Game.deleted`) and fails with `GetLastError` = 2 (ERROR_FILE_NOT_FOUND)
  for any other; `RemoveDirectoryA` succeeds when no file of `Game.files` is left in the directory
  (directories are implied by the files), else fails with 145. With a display, `ShellExecuteA` (the
  main menu's web site) is logged in `Game.posted` as `('url', file)`, next to `PostMessageA` /
  `PostQuitMessage`. The port's requests are `platform_file_delete` (files and the emptied directory),
  `platform_open_url`, `platform_post_message`, `platform_quit` (core/sim/platform.h): `state_replay`
  removes the file or empty directory from `IMHOTEP_PLAYER_FILES`, prints `file_delete PATH R`,
  `open_url URL`, `post_message MSG WPARAM LPARAM`, `quit CODE`, and appends these lines to the file
  `IMHOTEP_REQUEST_LOG` names when set. `tests/differential/oracle_frontend_check.py` compares them with
  `Game.posted` (same requests, in order) and checks that the port deleted exactly the player files
  the original deleted.
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

## Display (phase 7)

`Game(display=(W, H, FORMAT))` (or `display='1024x768x565'`; resolutions 640×480, 800×600, 1024×768,
formats 565 / 555) boots with WinMain's DirectDraw set-up `0x004CF960`, inserted after the settings
read `0x004149D0` (the `video` step is dropped: `0x004CF960` makes the Bink object). The resolution
`0x00E38E68` (1/2/3) and fullscreen `0x00E38E5C` = 1 come from the options file the settings read
(`0x004147F0` reads `Pharaoh.inf` over `0x00E38E58`, `core/boot/settings.c`), as in the game: a display
boot gives the game a player file `Pharaoh.inf` (`Game.files`, `game.py` `display_options`, unless a test
put its own there) of 0x14 bytes, the settings' defaults with fullscreen 1 and the display's resolution
(the GOG install's own `Pharaoh.inf` is empty: with it a 1024-wide screen would get the default 640 × 480).
The resulting state is the one the boot made before, when it set the two globals itself before `0x004CF960`
(nothing between the settings and the set-up reads them). After the set-up, the boot delivers
`WM_SIZE` and `WM_MOVE` to the window procedure `0x00414CD0`, as Windows does when the window is
shown (assumed order), which sets the client rectangle `0x00E39130` (the cursor's scaling in
`0x004CF210`: without it every cursor position scales to 0, 0). `game.py` then emulates:
- window, GDI and cursor calls (`GetDC`, `RegisterClassA`, `CreateWindowExA`, `LoadCursorA`,
  `SetRect`, `GetClientRect`, `IsIconic`, `DefWindowProcA`, …: fixed handles and trivial results;
  `TextOutA` / `GetTextExtentPointA` record the loading screen's text in `Game.text`);
  `GetSystemMetrics` returns the display size; `GetCursorPos` returns `Game.cursor`,
  `GetAsyncKeyState` `Game.keys_down` / `Game.keys_pressed` (bit 15 / bit 0);
- DirectDraw: `DirectDrawCreate` returns a COM object whose vtable methods are stubs past the
  imports (`emu.py` `com_object`): IDirectDraw (`CreateClipper`, `CreateSurface`,
  `SetCooperativeLevel`, `SetDisplayMode`), IDirectDrawSurface (`Blt`, `BltFast` copy the whole
  source, `GetPixelFormat`, `GetSurfaceDesc`, `Lock` fill the descriptor with the chosen masks,
  `IsLost`, `Restore`, …), IDirectDrawClipper. A surface is a block of the other heap, pitch 2·W.
  `Blt` with `DDBLT_COLORFILL` fills the whole surface with `DDBLTFX.dwFillColor` (the clearing
  `0x00426F60`). Any other method stops the run.
- the Bink player (BINKW32): `BinkOpen` fails (0), as every other Bink function does nothing, unless a
  test sets `Game.bink = (W, H)`: it then opens a video of W × H with no frame (a BINK structure in the
  other heap), so that the player `0x00413690` skips its frame loop and makes only its state effects
  (`tests/differential/oracle_video_check.py`; the port's side: `state_replay --video W,H`, passed by
  `replay.external_maps`).
- With a display the present `0x004D0280` and the loading-screen text `0x004CF7B0` run as the
  original (they stay no-ops without one): the present copies the back buffer to the emulated primary
  surface every 50 ms of clock (`Game.blits`).

The back buffer: `Game.screen()` (bytes, `pitch · height`), `Game.screen_info()` (address, width,
height, pitch, format), `Game.screen_png(path)`, `Game.message(msg, wparam, lparam)` (a window
message through `0x00414CD0`). PNGs are game output: `local/oracle/screens/` only.

**The default stays no display.** Measured (Actium, the boot and the loaded save, every resolution):
the simulation heap is identical; in the segments a display boot changes the resolution globals
(`0x00E38D8C`–`0x00E38DB8`, `0x00E38E68`: the no-display boot keeps the default 1 = 640×480), the
city viewport `0x0070E430`–`0x0070E46C`, the sidebar x `0x00F53F60`/`64`, the clip rectangle and
blitter globals `0x00D3B4C0`–`0x00D3B5E0`, `0x00D3B1C8` (400 bytes) and `0x00D3B364`, the DirectDraw
objects and surfaces (`0x006DE520`, `0x00E39140`–`0x00E39178`, `0x00E92DFC`, `0x00E92E18`), the 19
cursors `0x00E39090`, the client rectangle, `0x005E06B0`/`0x005E06D4` (DirectDraw ready),
`0x005E44A8`, `0x00E94E28`.. (four values derived from the width); before a load also the camera x
`0x00BCA2BC`; and about 24,700 dwords that point into the other heap (image entries, pixel buffers),
which moves because the display's allocations come first. One byte of each campaign record
(`0x006FDD08` + 0x40·k + 0x2B) differs too: `0x0041AC90` copies a 64-byte stack record whose padding
byte is never written, so it holds what earlier calls left on the emulated stack (0 without a display,
0x7D with one). Every phase 1–6 check passes under both boots (no display, and 1024×768 565 with
`IMHOTEP_ORACLE_DISPLAY`): load 53 + 111, function 834, save 15, state 31, newgame 160, hover 175,
command 203, lockstep Actium and Iken 51 ticks chained. The default stays no display anyway, so that
the existing reference states and the harness cache stay valid and the old checks do not pay for the
display; the frame checks use a display boot. `IMHOTEP_ORACLE_DISPLAY=1024x768x565` makes the display boot the default of `Game()`
for any tool (the harness's state cache is keyed by the display). Both sides of a comparison always
come from the same boot.

**Comparisons with a display.** The deep recorder (`replay.py`) also records what the unported calls
write into the drawing buffers outside the state, the back buffer `[0x00E92E18]` and the spare screen
`[0x00E92E1C]`: during a recorded call their pages are write-protected; the first write to a page
saves it and unprotects it (the hook performs the write, as Unicorn drops a write it reports on a
protected page); at the end the saved pages are compared with the new bytes. `state_replay`
`--screen W,H,PITCH,FORMAT,IN,OUT` and `--rw ADDR,IN,OUT` give the port its own copies of those
buffers at the original's addresses (writable), apply the recorded writes and write them out;
`external_maps` maps the oracle's other heap read-only for the pixel data (docs/formats/state.md
"Drawing buffers and pixel data"). `harness.compare` compares the back buffer too (not the spare
screen, a work buffer that the ported file readers do not use)
(`compare.screen` = differing pixels and rectangles; `harness.py --display WxHxF [--png]`), and
`compare(..., whole=True)` replays a top-level function with no port whole (its own record, printed
"whole ADDR" by `state_replay`) instead of failing.

**Mode changes in a comparison.** A call that changes the display mode (the resolution keys F6..F9, the
640 × 480 switch around a video: DirectDraw's set-up `0x004D0060` makes a new back buffer and
`0x00414BD0` a new spare screen) leaves the state pointing at buffers that did not exist before it.
`replay.buffer_args(g, tmp, before, info)` gives the port the input state's buffers (`info`: the screen
size and address before the run, which `--screen` must match) and, for each drawing buffer the original
made during the run, a zeroed writable buffer at its address (`--rw`); the recorder records such a
buffer whole where it is not zero at the end of the recorded call that made it (and tracks its pages
from then on). `harness.compare` and `framestep.py` then compare the back buffer the state points to
after the run, on both sides (`tests/differential/oracle_draw_check.py` "mode change", "directdraw
setup"; `oracle_frame_check.py` play-resolution-*).

**Platform memory and display modes on the port's side.** With a display, `external_maps` passes
`--other-heap TOP` (the oracle's other heap's top before the run, `mark_inputs`): `state_replay` then
gives the ports the platform's `memory` (blocks bump-allocated from TOP in 16-byte steps, as
`emu.py` `alloc` does, so the port's allocations get the original's addresses; each record carries the
other heap's top after its call, `other_top`, which the replay follows) and `set_display` (the new back
buffer is the one the original's DirectDraw set-up made: the value its record writes at `0x00E92E18`,
zeroed; the other heap's top then follows that record). The partial ports that need them
(`0x00414BD0`, `0x004D0060`) then run natively instead of being replayed. DirectDraw's COM pointers
(`0x00E39148`, `0x00E39170`..`0x00E3917B`, the lock object's `0x006DE534`..`0x006DE53F`) are the
platform's handles in the port and are excluded from comparisons (`excluded.py`: read only as the
`this` of DirectDraw calls).

**Partial ports and platform requests inside a replay.** A partial port (`state_run --list` partial 1)
or a platform request (partial 2) reached inside a recorded call is recorded as one unit, and the
outermost unported calls it makes are recorded as its nested records (`replay.py`: a record's `nested`
count; the recorder keeps a stack of calls in progress). `state_replay` runs the port natively and
its unported calls consume the nested records (a run that ends with nested records left over is a
mismatch: "made fewer calls"); when it reaches unported code (`state_unported`) the state is restored
and the unit's record is applied, its nested records skipped ("fallback"). Inside a native partial port,
a nested record with no effect on the state (nothing written, no block: a `sprintf` into a stack buffer
for `OutputDebugStringA`, `DirectDrawCreate`'s wrapper `0x0055C0E0`) that the port does not make is
skipped ("skipped ADDR (no effect)"). Inside a platform request (and a platform request compared at the
top), whose original run is presentation, any nested record the port does not make is skipped ("skipped
ADDR (platform)": e.g. the file check and the log buffer `0x004CFF20` of a Smacker video that does not
open); the ones it makes must still come in order.
`harness.compare` records a partial port called at the top whole, so that it falls back to its record
the same way; `harness.py` reports the fallbacks ("replayed, not run natively").

**Frames.** `tools/oracle/framestep.py SAVE [--display WxHxF] [--window ID] [--script FILE | --frames N
--step MS] [--chained] [--png] [--report]` runs the frame `0x004D6B60` in the original and the port,
frame by frame, from the same state and buffers, and compares the state and the back buffer after
each. A script line per frame: the time in ms (`Game.clock`), the cursor, the buttons and keys; the
input becomes window messages delivered to the original's window procedure `0x00414CD0`
(`WM_MOUSEMOVE`, `WM_L/RBUTTONDOWN/UP`, `WM_KEYDOWN/UP`, `WM_CHAR`, raw `msg=`), which writes the
input globals as in the game (docs/presentation.md §5), and `GetCursorPos` / `GetAsyncKeyState`
return the scripted cursor and keys. The window procedure and the frame are top-level calls: while
they have no port they are replayed whole (reported "not ported"); a port registered at their address
runs natively with its unported callees replayed. `tests/differential/oracle_frame_check.py` holds
the frame cases (city view on several saves at each resolution, windows by id; in `make oracle-check`).

The whole game against the original: `tests/differential/oracle_play_check.py` runs a scripted session
of `build/imhotep` (`--hidden --det-clock 0,25 --script --dump --msglog`, from the port's own boot, no
sound, no video) and gives the original the same clock, window messages and cursor before each of its
frames `0x004D6B60`; every frame's state and back buffer are compared (`make present-check`).

## Sound (phase 7)

Without sound (the default) the boot skips WinMain's sound start-up `0x005394F0` and every `MSS32.DLL`
(Miles) import returns 0: the sound flag `0x00F55B88` stays 0 and the game plays nothing.
`Game(sound=True)` (or `IMHOTEP_ORACLE_SOUND=1` for any tool; the harness's state cache is keyed by it)
runs the start-up at its place (after `0x0044EFA0`, dis `0x00414167`) on `tools/oracle/sound.py`
`FakeMiles`, a stand-in that behaves as a working driver: `GetVersionExA` answers Windows XP,
`AIL_waveOutOpen` gives a driver, 5 sample handles are allocated, the sound registry, `music.txt` and the
sound tables are built from the game's files (the start-up opens ~1,300 audio files: `game.py` caches
the game files it reads). The sound options are those of the boot (`Pharaoh.inf`: effects, music, city
sounds and speech on; volumes 100, music 80). `FakeMiles` (the driver's behaviour, not the game's): a
stream opens when its file exists, plays until it is closed or `Game.miles.end_stream(channel)` ends it
(`AIL_stream_status` 4, then 2), keeps the volume last set; samples are over by the next request
(`AIL_sample_status` 2); `AIL_file_read` succeeds for existing files.

Every Miles call is logged in `Game.ail_log` (`emu.py`): name, arguments, result, return address, the
first three stack arguments of the caller's ebp frame (the wrappers `0x0055F32F`..`0x0055F639` keep
their channel there) and the recorded call it is made in (`replay.py`). `sound.requests()` translates
the log into the port's request vocabulary (`core/sim/platform.h`): `stream_play CH LOOPS VOL OPENED
NAME`, `stream_stop CH`, `stream_playing CH -> R`, `stream_volume CH -> V`, `stream_set_volume CH V`,
`sample_play VOL PAN LOOPS NAME` (from `AIL_open_stream` + loop count + volume + `AIL_start_stream`,
`AIL_close_stream`, `AIL_stream_status`, `AIL_stream_volume`, `AIL_set_stream_volume`, and a started
sample's file, volume, pan and loop count). Only the calls the port makes itself count: those made in
ported code and in the recorded platform requests (partial 2), not those inside a replayed function.
`harness.compare` and `framestep.py` pass these lines to `state_replay --sound FILE`, whose platform
prints the port's requests (`sound ...`) and checks them in order: a different request, a missing one or
one too many exits 4; the port's queries (stream opened, playing, volume) get the original's answers.
`tests/differential/oracle_sound_check.py` holds the sound cases.

## Snapshots

Booting the game in the emulator takes about 1 s (15 s with a display) and loading a save about
9 s (20 s with a display), against about 1 s for a comparison. `Game.cached(name, setup, ...)`
snapshots the whole emulated game after the boot and `setup` (every mapped region up to its use,
the CPU context with the x87 registers, the Python-side state; the COM handlers of the display
are rebuilt from their names) into `local/oracle/cache/snap_*.snap` (about 60 MB) and restores it
in about 0.3 s. The key holds the name, the display and sound settings in effect and a hash of
`emu.py`, `game.py` and `sound.py`, so a change to the oracle invalidates every snapshot.
`harness.oracle_state` (loaded saves, with ticks), `lockstep.py`, `framestep.py` and `screen.py`
use it; `Game.booted()` is the boot alone. Checked: after a restore every mapped byte equals the
fresh game's, and stays equal through 20 ticks and drawn frames. A sound boot cannot be
snapshotted (the sound driver object holds emulator pointers): it boots normally.

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
