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
