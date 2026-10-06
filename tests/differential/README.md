# Differential tests

`make differential` builds and runs every test below. Tests need the reference game files
(`$IMHOTEP_GAME_DIR`); without them they are skipped.

## terrain_check (core/map/terrain.c)

The map editor stores, in every `.map`, the image grid it computed with the code that
`core/map/terrain.c` reimplements (the terrain image rebuild of scenario start). `terrain_check`
rebuilds each scenario map and compares the result with the stored grid, tile for tile, per terrain
type. It passes if every type matches at ≥ 99.9 %. Current result: 29 maps, 589,509 / 589,524
tiles; the 15 differing tiles are explained in `docs/formats/map.md` §6.

    build/terrain_check                  # all comparable maps
    build/terrain_check Cataract         # one map, listing differing tiles
    build/terrain_check --dump DIR       # write the rebuilt grids (all 33 maps) for inspection

    build/terrain_check --campaign       # records of mission1.pak (version >= 149)
    build/terrain_check --campaign 38    # one record, listing differing tiles

`--campaign` checks the scenario records of `mission1.pak` (saved states rebuilt by the same
scenario start). It gates on cliffs and rocks only: 33,464 cliff and 14,729 rock tiles match
(cliff ids shifted by +152, see `docs/formats/campaign.md` §4). `make differential` runs both modes.

The rule tag `@diff terrain_check` in `core/` refers to this test.

## desirability_check (core/city/desirability.c)

The game rebuilds the whole desirability grid (save chunk 16) at tick 37 of every day, from the
buildings, terrain, bitfields and the building model of the save's difficulty
(`docs/formats/desirability.md`). `desirability_check` recomputes it from each save's stored state
and compares all 51,984 tiles.

When a grid differs, the check looks for the largest building-id prefix that reproduces it
exactly. That means buildings placed after the last daily update, which is legitimate.

Current result:
- the 7 player saves are exact (363,888 tiles, difficulties 0/2/4);
- 49 of 53 campaign records are exact, and the other 4 are fully explained by buildings placed
  after the last update;
- tile total: 99.965 % equal.

    build/desirability_check                      # mission1.pak records
    build/desirability_check -v FILE.sav ...      # player saves, listing differing tiles

The rule tag `@diff desirability_check` in `core/` refers to this test.

## save_check (core/io/save_file.c)

Reads every saved game of `mission1.pak` (53 records, versions 147–160) with the save reader and
checks:
- every chunk equals the file's chunk, or the migration of a grown record (the file's element then
  zeros; versions 147–150 grow chunks 137 and 142);
- the cross-references the game maintains: building grid offsets and the building grid,
  figure grid offsets and the figure chains, and the city entry/exit offsets.

It then runs the loader's later steps (`core/io/save_load.c`) on every record. On the
version-160 records it compares what the load recomputes with what the game stored:
- route lengths: 850 / 850 equal;
- route endpoints: 850 / 850 equal;
- figure name groups: 308 / 308 equal.

89 more figures keep the constructor's 0, because the records were saved without a load in
between.

Current result: 53 saved games, 601 buildings, 1,004 figures, 12,691 checks, 0 failures.

    build/save_check                     # mission1.pak records
    build/save_check FILE.sav ...        # player saves
    build/save_dump --pak 20             # state as JSON (tools/extract/save_dump.c)

### Downloaded saves (`local/saves/downloaded/`, manifest `local/saves/manifest.json`)

Seven player saves from Pharaoh Heaven (6 completed-city downloads, 2026-10-04):

- **Version:** all are version 160, so they add no upgrade coverage.
- **Size:** 139–1,704 buildings in use and 150–807 figures each.

`build/save_check local/saves/downloaded/*/*.sav` gives:
- 0 failures in 21,249 checks;
- route lengths and endpoints: 350 / 350;
- figure name groups: 3,646 / 3,646;
- 16 live figures are off the figure grid (walkers and caravans in action states 6 and 8),
  which led to the corrected chain check.

These are single snapshots after human play. They are not differential pairs, but they can serve
as start states for reference runs. The two Itjtawy saves are the same city 5 years apart, with
unknown play in between, so they are not a pair either.

## oracle_load_check (core/io/save_file.c, core/io/save_load.c)

Compares our loader with the original's own, executed by the oracle (`docs/oracle.md`). For each
save, the reference Pharaoh.exe runs its chunk reader and loader `0x004D41E0` in the emulator.
`build/save_dump` runs `save_file_read` + `save_file_load` from the same CRT `rand()` state. All
181 saved variables must be equal byte for byte. The only exception is the route records'
path-buffer pointers, which are heap addresses.

Current result:
- 53 / 53 records of `mission1.pak`, which exercise the version upgrades;
- 111 / 111 player and corpus saves (7 downloaded, 104 generated).

    make oracle-check                                                   # both sets
    local/venv/bin/python tests/differential/oracle_load_check.py --pak # mission1.pak records
    local/venv/bin/python tests/differential/oracle_load_check.py FILE.sav ...

The rule tag `@diff oracle_load_check` refers to this test.

## oracle_function_check (every ported function)

Each ported function (core/state/ported.c) runs with the original on the same state: player
saves loaded by the original, outputs scrambled first where useful (`--fill`). The whole state
(data segments, simulation heap) and every return value must be equal (`tools/oracle/harness.py`).
Original functions that are not ported yet and are called inside are recorded in the original's
run and replayed into the port (`tools/oracle/replay.py`), so a port is checked on its own.
Scratch variables written before every read within a tick are ignored (tools/oracle/excluded.py).
Covered: desirability, map row ranges, routing grids and fillers, goods predicates over every
good, goods lists, post-load links, pre-load clear. The terrain rebuild `0x004DEA50` (and its 16
sub-passes) and the floodplain build `0x004BE160` are checked the same way with
`harness.py 004DEA50`, `harness.py 004BE160:0,0,300,300`: 15 / 15 states equal.

## oracle_state_check (core/state/load_game.c)

The whole state after our load (`game_load_save`) against the original's, both starting from the
original's booted state. Each difference is listed under the step of `0x004D5100` / scenario start
that last wrote it in the original; only bytes on pages the tick reads count as failures
(docs/formats/state.md). The remaining steps are tick functions listed in docs/roadmap.md (2.2).

## oracle_save_check (core/state/save_game.c)

From the same state (a save loaded by the original), the original's saver `0x004D4E20` and our
`game_save` write a file each; they must be equal byte for byte, PKWARE compression included.
Current result: 15 / 15 (7 player saves, 8 corpus saves).

## lockstep (core/sim/tick.c)

`tools/oracle/lockstep.py SAVE --ticks N [--chained] [--report]` runs our tick against the
original's, tick by tick. The original records every outermost call to an unported function, at
any depth, with what it changed; our tick runs the ports natively and must make the same unported
calls in the same order, which are replayed; the states after each tick must be equal
(docs/formats/tick.md). `--chained`
continues from our state, so a difference would accumulate; `--report` lists the replayed
functions with the bytes they wrote (the porting to-do list of phase 4). `make oracle-check` runs one chained day on two
cities.

Result (phase 4 done): `--ticks 9792 --chained --report` on the 8 corpus cities, 0 ticks with
differences on each; only platform requests and two partial ports are still recorded
(docs/roadmap.md 4.8).

## oracle_command_check (core/command/, player commands)

`tests/differential/oracle_command_check.py [NAME ...] [-j JOBS]` applies scripted player
commands (docs/commands.md) between the ticks of a chained lockstep run (`lockstep.py
--commands`): each command's UI handler runs in the original and in the port from the same state,
the states must be equal after it and after each following tick. Helpers write the UI sequences
(`build()`: choose a building, mouse down, drag, mouse up). The entries cover every building type,
drags, clearing with its confirmations, undo, every setting and order of the advisors and
building windows, kingdom requests, military and ship orders, cheats, map rotation and difficulty
changes, with refusals, on the downloaded saves and pak00 m00.

## oracle_hover_check (core/command/hover.c)

The hover functions (ferry reach, bridge measure, ferry landing validity) against what the
original's city-view drawing computes for the hovered tile, on shore tiles of several saves.

## Reference save corpus (`local/saves/generated/`)

`make corpus` advances each downloaded save 12 months with the original's tick function in the
emulator. The game's own saver writes each month start as `<city>/mNN.sav`; m00 is the state as
loaded. The files are recorded in `local/saves/manifest.json` ("generated"). They are differential
pairs: a port that loads mNN and runs one month must produce mNN+1 (`docs/roadmap.md`, phase 3).
`tools/oracle/diff.py A B` lists the differences between two states field by field.

On the 104 generated saves: desirability_check is exact on all of them (5,406,336 tiles), and
save_check reports 0 failures in 278,130 checks.

Reference saves are **not** in the repository. Each contributor generates them locally.
