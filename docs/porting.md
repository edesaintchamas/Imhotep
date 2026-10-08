# Porting a game function

How functions of `Pharaoh.exe` (GOG 2.1) become C in `core/`, and how each port is checked. Read
CLAUDE.md first (sources of truth, provenance tags, determinism rules).

## 1. What to port, in what order

- `tools/oracle/coverage.py` runs the original's ticks on the corpus and records every function it
  enters with its callers (indirect calls included): `local/oracle/coverage.json`. Only functions
  that run matter; others are ported when a test reaches them.
- `tools/oracle/lockstep.py SAVE --report` lists the original functions that the current ports
  still replay, by bytes written: the to-do list.
- A port may call functions that are not ported yet (§4), so the order is free: top-down (a
  dispatcher first, its callees replayed) or bottom-up (helpers first). Small shared helpers first
  saves work: every later port calls them directly.

Player commands (phase 5) are ported at the UI-handler boundary: the handler functions
themselves (`0x00405200` choose a building, `0x00405C20` / `0x00405D90` / `0x00405E90` mouse down,
drag, up, the button callbacks; docs/commands.md §4). Their inputs are globals of the state that
the platform will set (hovered tile, menu item, callback parameters, confirmation answer). Values
the original computes while drawing the hovered tile and the place step reads (ferry reach and
validity, bridge measure) are core "hover" functions (core/command/hover.c) that the platform calls
for the hovered tile, as the original's draw does. Effects that only drive the screen, sound or
files are platform requests (core/sim/platform.h: videos, sounds, message boxes, file reads, pixel
loads).

## 2. Sources for one function

- `local/ghidra-export/functions/0x........json`: Ghidra's decompiled C, callers, callees. A
  guide only: Ghidra loses signedness, merges variables, mistypes globals, drops x87 details.
- `local/venv/bin/python tools/oracle/disasm.py ADDR [COUNT]`: the instructions. **The
  disassembly decides** every width, signedness (movsx / movzx, jl / jb, sar / shr, idiv / div),
  evaluation order and re-read of globals.
- `tools/oracle/where.py ADDR`: which function an address belongs to.
- Field meanings: docs/formats/save.md §5 (buildings, figures, city data), state.md, tick.md.
  `refs/` (Julius, Akhenaten) only to guess what a function is for; never copy a rule from there
  without the binary (`@hint` otherwise).

## 3. Writing the port

- **Place.** One module per subsystem (`core/figure/`, `core/building/`, `core/city/`, ...). A
  module lists its ports in its own table (core/state/ported.h):
  ```c
  static int32_t run_figure_delete(game_state *st, const int32_t *a) { figure_delete(st, a[0]); return 0; }
  const ported_function figure_ports[] = {
      {0x004B4980u, "figure_delete", 1, 0, 0, run_figure_delete, 0},
  };
  const int figure_ports_count = sizeof(figure_ports) / sizeof(figure_ports[0]);
  ```
  and the table is added once to `ported_modules` in core/state/ported.c. Fields: address, name,
  stack argument count, significant bits of the return value (0, 8, 16, 32), `this` for thiscall
  functions called on a fixed object (else 0), wrapper, partial flag.
- **Every ported function is registered**, including helpers that are only called by other ports
  (the oracle skips registered functions when recording, §4). A helper ported inside its caller
  with no standalone entry point is registered with `run = NULL`.
- **Names.** A descriptive name when the binary shows what the function does (strings, the
  fields it touches, its callers); otherwise `<module>_<address>` (`figure_0x00489CA0`). The
  `@bin ADDR` tag on the function always gives the address.
- **State access.** The game state mirrors the exe's data segments (docs/formats/state.md):
  `S8/U8/S16/U16/S32/U32(st, address)` for globals and fixed arrays, `P8/P16/P32(st, address)` for
  addresses read from the game's own pointers (heap included). Record bases and sizes:
  core/state/records.h (`FIGURE_ADDR(id) + 0x0A`). Write every global the original writes,
  including loop counters the original keeps in globals (the current figure 0x00D056F8, ...):
  they are state, other functions read them.
- **Integers.** Same widths and signedness as the instructions: `(int16_t)` for movsx of a word,
  `uint8_t` for movzx of a byte, signed vs unsigned compares as the jump says. C division
  truncates like idiv; `>>` on a signed int is arithmetic (sar) with our compilers. Overflow wraps
  as in the original: compute in `uint32_t` where it can overflow, then cast.
- **No floating point** in core/. Where the original uses x87, reproduce its exact result with
  integers (53-bit precision, round-to-nearest-even) and document the derivation; stop and record
  the question if that is not possible.
- **Random numbers.** The game RNG (core/random/random.h) and the CRT `rand` (core/random/crt_rand.h,
  `0x00562348`). Wall clock reads (`timeGetTime`, `GetTickCount`) are inputs, not core logic: the
  oracle returns 0 for them; ports read 0 too, with a comment, until the platform layer passes a
  clock.
- **Tags.** `@bin ADDR what` on each function and on each rule that is not obvious from the
  function's own tag (constants, tables, conditions). Copy no code from refs/ without `@from`.

## 4. Calls

- A port calls another **ported** function directly in C.
- A call to a function **not ported yet**, or through a pointer read from the state (action and
  handler tables), goes through `GAME_CALL(st, address, args...)` / `GAME_CALLT(st, this, address,
  args...)` (core/sim/call.h): it runs the port if there is one, else the test environment
  replays what the original did. Never skip such a call and never call `state_unported` for it.
- **thiscall.** Ghidra hides `this`: look in the disassembly for `mov ecx, imm32` right before the
  call (e.g. `mov ecx, 0xbca350; call 0x488050`) and a `ret N` in the callee; such calls are
  `GAME_CALLT(st, 0x00BCA350u, 0x00488050u, id)`, and the port's table entry gives `this`.
- **Calls whose result is unused still happen** in the port, in the same order, as long as the
  callee is not ported (the replay checks the sequence of unported calls).
- A callee that writes into its caller's stack (an output buffer) cannot be replayed: port it
  first (the replay tool reports it: `stack 0x...`).
- Partial ports (correct for some arguments only) set the partial flag and call `state_unported`
  outside their range; their callers reach them only through `GAME_CALL`.

## 5. Checking

- **One function:** `local/venv/bin/python tools/oracle/harness.py ADDR[:ARGS] [--ticks N] [SAVE...]`
  runs the original and the port on the same loaded states (downloaded saves and corpus m06 by
  default) and compares every byte and the return value. The original's calls to unported
  functions are recorded and replayed into the port (tools/oracle/replay.py, build/state_replay):
  a port can be checked before its callees are ported. `--ticks N` advances the game first, to
  reach other situations; `--fill` overwrites an output region first.
- **In the tick:** `local/venv/bin/python tools/oracle/lockstep.py SAVE --ticks N [--chained]`:
  every tick of the original against ours, with every unported function replayed. `--report`
  shows what is still replayed.
- **Regression set:** `tests/differential/oracle_function_check.py` (add the new function's
  test there) and `make test`.
- A difference is never "close enough": find the instruction that explains it.

## 6. Working in parallel

Several people (or agents) port at once in the same tree:
- **Claims.** Before porting a function, append `ADDR owner` to `local/oracle/claims.txt`
  (`echo "0x00459240 labour" >> local/oracle/claims.txt`) after checking that nobody claimed it and
  that `build/state_run --list` does not list it. Claimed by someone else: call it through
  `GAME_CALL` and move on (it runs natively once its port lands).
- **Own files.** Each package writes its own module files and adds its table to
  core/state/ported.c (one line in each of the two lists). Never edit another package's files; a
  compile error in someone else's file is theirs: wait and retry.
- **Own build.** `make BUILD=build-<package> core build-<package>/state_run build-<package>/state_replay`
  and `IMHOTEP_BUILD=build-<package>` for the Python tools.

Waiting for background jobs: never a hand-written `until grep ...; do sleep; done` loop (it runs
forever when the job fails or prints something unexpected). Run checks in the foreground with a
timeout, or wait with `tools/agent/wait_for.sh FILE PATTERN [MAX_SECONDS] [IDLE_SECONDS]`, which
always stops (pattern, error line, no output growth, time limit); stop every background job and
wait before reporting.

## 7. Documenting

- docs/provenance/README.md: one row per ported function (address, port, test).
- New field meanings: docs/formats/save.md §5 with their evidence.
- Open questions: docs/unverified.md.
