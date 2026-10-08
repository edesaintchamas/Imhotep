# The game state (`core/state/`)

Reference: GOG Pharaoh + Cleopatra 2.1. The original keeps its simulation in global variables and
a few heap records. Imhotep keeps the same bytes at the same addresses, so a ported function reads
and writes what the original does, and the oracle (`docs/oracle.md`) compares the two states byte
for byte.

## Layout (confirmed: PE section table of Pharaoh.exe)

| Part | Addresses | Contents |
|---|---|---|
| data segments | 0x0056F000 – 0x01267AA8 | `.rdata` (0x0056F000, 0x61A6 bytes) and `.data` (0x00576000, 0xCF1AA8 bytes, of which 0x90000 initialised) |
| simulation heap | 0x60000000 – (used) | records the original allocates with `new` / `malloc` that the simulation reads |

- The initial contents of the data segments are the exe's initialised sections, read from the
  user's own `Pharaoh.exe` by `platform/exe_image.c`. Nothing from the game is in the repository.
- Saved chunks load at their variables' addresses (`save_schema[].address`).
- Access: `S8/U8/S16/U16/S32/U32(st, address)` for constant addresses in the data segments,
  `P8/P16/P32` for addresses taken from the game's own pointers (data or heap). Little-endian,
  unaligned.
- **Heap.** The oracle sends an allocation to the simulation heap when a function of
  `SIM_ALLOCATORS` (`tools/oracle/game.py`) is on the call stack; every other allocation
  (graphics, UI) goes to a separate heap that is not part of the state. Both the oracle and core
  (`state_alloc`) allocate by bumping a pointer in 16-byte steps, never reuse memory, and
  zero-fill freed blocks (`state_dealloc`). So pointers stored by the game have the same values in
  the port and the oracle, and dead memory compares equal. Pointer values are not otherwise
  observable by the game.
  - Simulation allocators found so far: `0x004BD740` (floodplain objects), `0x004D4720` (empire
    route paths), `0x004D0A40` (shipwright queue arrays, `0x004F6700`), `0x00451A70` (static
    constructor: arrays of 20 for queues 0-4, which a load keeps when the saved capacity fits).
- **State files** (tools): the data segments, u32 heap size, the heap, u32 block count, then
  (offset, size) per heap block.

## Simulation state and display state

The tick reads about 1,010 of the 3,321 pages of the data segments, measured over 13 game months
on three cities (`tools/oracle/readset.py`: pages made unreadable, the first read of each one
recorded). A difference on a page the tick never reads is display state. Comparisons treat as
failures only differences on read pages (page granularity, so a few display bytes that share a
page with simulation variables also count).

Explicit exclusions (`tools/oracle/excluded.py`), never read by the simulation:

| State | Why |
|---|---|
| 0x01265188 – 0x01265197 | compressed-chunk reader workspace (`0x0055BCF0`, `0x0055C000`): buffer pointer, length, checksum |
| empire-route point buffers (heap, chunk 143 +0x04, capacity +0x138 × 6 bytes) | computed with x87 arithmetic for the empire map; no read in 13 months |

## Loading a saved game (`game_load_save`, port of `0x004D5100`)

| Step | Original | Port | Checked |
|---|---|---|---|
| floodplain settings | `0x004BD330` (this = 0x00D38360) | `floodplain_settings_reset` | state check |
| pre-load clear | `0x0046FA40`: grids, floodplain manager (`0x004BE0F0`, list release `0x004BD6A0`), table flags (`0x00484EE0`), outside tiles (`0x0046FC30`), random grid with 51,984 RNG steps (`0x0046FC80`), image grid (`0x00475B80`) | `pre_load_clear` | harness, byte-exact incl. heap (Iken, Alexandria) |
| loader | `0x004D41E0` | `save_file_load_state` | `oracle_load_check`; state check (shipwright queues `0x004D0A40`, route buffers) |
| scenario start | `0x004D52A0` | `scenario_start`: see below | state check |
| post-load links | `0x0046B320` (storage, `0x0046B100`), `0x0046F030` (type 0x88, `0x0046EFD0`), `0x0046A320` (type 0x4A, `0x0046A280`) | `load_rebuild_*` | harness |
| player name | `strcpy(0x00EA184C, 0x00E38E74)` | in `game_load_save` | state check |

Scenario start (`0x004D52A0`), ported so far: map globals and row ranges (`0x0046F7B0`), the
three model files (`0x00540C90`, `0x00540ED0`, `0x00541160`), the map-point check, `0x004E7E20`,
the view orientation clamp, the five routing grids (`0x00517A20`, `0x005178B0`, `0x00517370`,
`0x00517660`, `0x005178E0`), the inline flags, the goods lists (`0x005596D0`). The others are
marked `not_ported` in `core/state/scenario_start.c`.

On Actium (no floodplain, no type-0x88 buildings) the simulation bytes that still differ after a
load come from: the terrain image rebuild `0x004DEA50` (in `core/map/terrain.c` on its own
structures; to be moved onto the state), `0x00457AF0`, `0x00485F70` (building images and grid),
and a dozen small steps (`0x00468860`, `0x00404350`, `0x00443330`, `0x004677C0`, ...).

## Game data outside the segments, and presentation

- **Text.** The original loads `Pharaoh_Text.eng` at start-up (`0x004CEC40`): its 0x1C-byte header
  (0x00E36B78) and its 8,000-byte group index (0x00E34C38) are in the data segments; the strings
  (the file from byte 0x1F5C) are in a heap block whose address is at 0x00E36C70. Some simulation
  functions read strings (`0x00469D50` reads the mission's flag string, text group 0x133, through
  `0x004CC110`). Core reads them through `state_map_external`: the platform (and the test tools,
  `tools/oracle/replay.py` `external_maps`) maps the strings read-only at the address the state's
  pointer holds, so pointers the game stores into the text (the cursor 0x00E20768) compare equal.
  The same loader reads `Pharaoh_MM.eng`, the messages: its 0x13898-byte header and index at
  0x00E209A0 in the segments, the rest (from byte 0x13898) in a heap block at 0x00E36C74, mapped
  the same way. The message posts read video names from it (`0x004E0CF0` checks that the
  message's video file exists, `0x004CE3F0`).
- **Image files** (`.sg3` indexes). The slot loader `0x0053DFF0` reads `data\<name>.sg3` whole into its
  own stack frame (`0x004CEB60`) and writes simulation state from it (group records, entries, names:
  docs/formats/graphics.md §11). Core asks the platform for the file by the path the game builds
  (`platform_file_read`, `core/sim/platform.h`: path, offset, buffer; returns the file's size or -1),
  so the real game reads it from its own directory and core holds no file data. The test tools pass
  the game directory (`state_replay --map files=DIR`, `external_maps`), the same files the oracle's
  file hooks read (`tools/oracle/game.py`). `0x004CEB60` is registered with that request for callers
  whose buffer is in the state; callers with a stack buffer call `platform_file_read` themselves.
  The `.555` pixels (`0x0053F660`), the group list nodes and the 565 conversions live in the graphics
  heap, outside the state; only the buffer's address (slot +0xCC24) and the pointers derived from it
  (entries +0, 0x00D3B5F4) are state, read by drawing only.
- **Video and sound** (`core/sim/platform.h`). The tick calls the video player `0x00538E30` and the
  sound functions `0x0053B4A0`, `0x0053B090`. They write only presentation state: the video
  player's globals (0x00F53FF4 – 0x00F5405B, 0x00F54114), the sound request block (0x00E92DA8,
  20 bytes), file path and log buffers (0x00E36BF8, 0x00E38AEC, 0x00E39108). Static references
  show no other readers than the video module, the sound driver and the file and log functions.
  Core turns the calls into requests to an optional platform layer and writes none of these; the
  comparisons exclude them (`tools/oracle/excluded.py`). The video returns 0 (not played) without a
  platform, as the original does without a video device (under the oracle it then retries every
  tick).
- **Drawing buffers and pixel data** (phase 7, docs/presentation.md §2; `tools/oracle` with a
  display, docs/oracle.md "Display"). The drawing writes 16-bit pixels through the back buffer's
  address `u32 0x00E92E18` (rows of `i32 0x00E38D94` bytes, `0x00E38DB0` × `0x00E38DAC` pixels, 565
  when `0x00E92DFC` = 2, else 555) and reads image pixels through pointers the state holds (slot
  `+0xCC24` from `0x0053F660`, the image entries `+0` derived from it, `0x00D3B5F4`) and through the
  spare screen `u32 0x00E92E1C` (100,000 bytes, where `0x004CE480` reads external images). None of
  these buffers is part of the state; their addresses are. Scheme for the comparisons
  (`build/state_replay`):
  - the back buffer is given with `--screen W,H,PITCH,FORMAT,IN,OUT` and the spare screen with
    `--rw ADDR,IN,OUT`: the original's bytes before the call, mapped **writable** at the original's
    address with `state_map_external` (so a port writes `P16(st, U32(st, 0x00E92E18) + y·pitch + 2x)`
    exactly as the original does), written back to OUT after the call; the back buffer is compared
    with the original's (the spare screen is a work buffer: the ports of the file readers use their
    own buffers, so it is not compared); the unported calls' writes into them are recorded (`tools/oracle/replay.py`, page
    protection) and replayed like state writes;
  - all other pixel data: the oracle's whole other heap (0x68000000 up to its allocation top: the
    `.555` buffers of every graphics slot, the 565 conversions, the fonts, the group list nodes) after
    the original's run, mapped **read-only** at the same addresses (`--map 68000000:FILE`). Image
    buffers do not change once loaded, so their contents after the call are what the port's reads see
    (buffers loaded during the call are loaded by recorded calls before the port reads them). Mutable
    data in that heap other than the two buffers above (UI objects) must not be read by ports through
    this map; a port that needs one gets it added as a `--rw` buffer.
  - The mappings are tried in order (`state_ptr`): the writable buffers first (they lie inside the
    other heap). `game_state.external` holds 16 mappings.
  In the game, the platform owns these buffers: it allocates the back buffer and stores its address
  in `0x00E92E18` (as the original's `0x00427480` / Lock does), and loads the pixel data
  (`platform_pixels_load`).

## The booted state (`game_boot`, core/boot: WinMain's start-up)

A state made from the exe's sections (`platform/exe_image.c`) becomes the booted game with
`game_boot(st, host, display, sound)` (core/boot/boot.h): the C++ static constructors (the CRT's table
`0x00576000` .. `0x0057607C`, read from the state as `_initterm` reads it), then WinMain's (`0x00413ED0`)
steps in its order, the same steps as `tools/oracle/game.py` `Game.boot_steps` (`boot_step(k)` runs one):
the settings `0x004147F0` (Pharaoh.ini, Pharaoh.inf), the text files `0x004CEC40`, the CD letter, the
default names `0x004149D0`, the display set-up `0x004CF960` (display boots), the graphics slots and
loading screen `0x00414420`, the model files, campaign.txt, trade_recommends.txt `0x00541310`, the
reason phrases `0x0044EFA0`, the sound start-up `0x005394F0` (sound boots), the view reset, the
monument objects' layout, `0x00541D50`, `0x004DEE10`, `0x004CF8E0`, the Bink object.

- **Files** come from the platform (`platform_file_read`): an `fopen` that only tests a file is a size
  query. The CRT's C initialisers are not run: their effects are the CRT's own data (stdio tables, the
  x87 precision 53 bits, which core's integer arithmetic reproduces where the game uses x87).
- **Memory outside the state** (the text files' strings, the spare screen `0x00E92E1C`, the event
  phrases, the sound registry and voices, the DirectDraw surfaces, the Bink object, the pixel data)
  comes from the boot's host (`boot_host.alloc` with the original's call site, `surface`; pixel loads
  from `platform_pixels_load`): core reads and writes it through `state_ptr`, the host maps it.
  `boot_arena` is such a host: one region at `0x68000000` bumped in 16-byte steps, like the oracle's
  other heap. The text files are read into these blocks by core (`Pharaoh_Text.eng` from byte 0x1F5C,
  `Pharaoh_MM.eng` from 0x13898): a game's platform maps nothing else for them.
- **Handles** of the window system, DirectDraw and Miles (window, cursors, DirectDraw objects, the
  sound driver and its sample handles) and the current directory and GDI text extents are the host's
  answers (`boot_host.handle`, `cwd`, `text_extent`); the state keeps them as the original does.
- **Constants kept as code.** The eight monument objects' constructors (72 KB of stores, one CRT
  `rand() % 5` per part) and the sound tables' builders are run by `tools/gen/gen_boot_data.py` (an
  interpreter of their instructions) into `core/boot/boot_data.c`: the objects' final bytes, the part
  rectangles, the rand bytes in call order, the builders' call lists.

Checked by `tests/differential/oracle_boot_check.py` (the port's boot against `Game.boot`, no display,
each display mode, with and without sound; the allocations outside the state get the original's
addresses by call site; downstream: a save loaded, two new games, ticks and frames started from the
port's booted state). Bytes that differ for a known reason (the check lists them):

| Bytes | Why |
|---|---|
| import address table `0x0056F000` (PE directory 12) | the Windows loader writes the DLLs' entry points (the oracle its stubs); core calls no DLL |
| the base part of each monument object, +0x1C .. +0x3F | `0x004E5140` copies 9 dwords of `0x004E5210`'s stack temporary that nothing initialised (stack contents) |
| campaign steps +2..+6, +0x2B, +0x37, +0x3D..+0x3F | `0x0041AE40` copies a stack record that `0x0041AC00` leaves partly uninitialised |
| `0x00E38DD6` | WinMain sets it to 1 after `0x004CF8E0` (dis `0x0041424B`); `game.py` does not |
| `0x00784F28` and the phrases' pointer array | EVENTMSG.TXT: the oracle's `fopen` FILE block has `_fileno` 0, so `_filelength` fails and `0x004CBA60` reads nothing; the port reads the file as the game does |
| `tools/oracle/excluded.py` ranges | the CRT strtok position, the file path buffer |
