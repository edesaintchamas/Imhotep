# CLAUDE.md — Imhotep

> Working name. To rename the project: `grep -rl imhotep . | xargs sed -i '' 's/imhotep/NEWNAME/g; s/Imhotep/Newname/g'`

## Goal

Imhotep is an open-source (AGPL-3.0) reimplementation of the engine of **Pharaoh + Cleopatra**
(Impressions Games, 1999-2000). It aims at **faithful simulation**: same rules, same results,
loading and writing original save files. Original assets are loaded from a legitimate
installation of the game; nothing from the game ships with this repository.

Reference version: **GOG Pharaoh + Cleopatra 2.1** (GOG build; `Pharaoh.exe` version resource
2.1.0.0, SHA-256 `b21b7d71…a7eca662`). Its hashes are recorded in
`docs/source-fingerprint.json`. All analysis is performed on this exact binary.

## Sources of truth (in strict order)

1. **The original binary** (`Pharaoh.exe`, GOG 2.1). The only ground truth. Read through the
   Ghidra export, and executed headless by the oracle (`tools/oracle/`, `docs/oracle.md`): its own
   code loads, advances and saves states, and runs single functions for comparison.
2. **Julius / Augustus** (Caesar III). Reliable for engine structure shared with Caesar III
   (walkers, services, routing, save layout). Pharaoh differs on floods, farming, monuments,
   roads, religion and more.
3. **Akhenaten** (Pharaoh). Closest reference, but its engine was redesigned (gameplay
   moved to JS scripts) and parts are approximate. Use it as a map of where to look.

Your own knowledge of the game from training data, wikis and strategy guides is **not a
source**. It can only suggest where to look in the binary.

## Provenance tags (mandatory)

Every game rule in `core/` (formula, constant, threshold, condition, ordering) carries one tag,
in a comment on the line above or at the end of the line:

| Tag | Meaning | Status |
|---|---|---|
| `@bin 0x004A1B20 <short desc>` | Confirmed at this address in the reference binary | confirmed |
| `@diff <test name>` | Confirmed by a differential test against the original (stored data or the oracle) | confirmed |
| `@hint akhenaten:<path>` / `@hint julius:<path>` | Taken from a reference project, not yet confirmed | hint |
| `@unverified <reason>` | Origin unknown; listed in `docs/unverified.md` | unverified |

Code **origin** (licence matter) is a separate tag:

- `@from julius:<path>` / `@from augustus:<path>` / `@from akhenaten:<path>` on any copied or
  translated code. Keep the original copyright notice in the file and update `CREDITS.md`.

A rule copied from a reference project gets **both** `@from` and `@hint`. Copying code never
confirms a rule.

`make provenance` reports the counts. The goal over time: `hint` and `unverified` go down.

## Hard rules

- **Never commit game files**: executables, DLLs, graphics, audio, video, maps, scenarios,
  saves, text resources, or Ghidra exports containing decompiled code. `make check-files`
  enforces it; so does CI.
- **Never implement a game rule from memory.** If you cannot find it in the binary or a
  reference, write `@unverified` and add an entry to `docs/unverified.md`.
- **Never present an assumption as a fact.** In `docs/formats/`, every field is marked
  `confirmed` or `assumed`, with the evidence for confirmed ones.
- **Do not edit `refs/`.** It holds read-only copies of reference projects.
- **Do not generalise across games yet.** Pharaoh first. Shared code with Zeus/Emperor will be
  extracted later, from working code.

## Determinism rules for `core/`

`core/` is the simulation. It must be a pure function of (state, inputs):

- No floating point in simulation logic. Integer arithmetic with the **same widths, overflow
  and rounding as the original** (check signedness and division behaviour in the binary).
- No clock, no I/O, no rendering, no audio, no platform calls. Those live in `platform/`.
- Random numbers only through the core RNG, which must reproduce the original generator
  exactly (`@bin` tagged), including when it is advanced.
- Iteration order over buildings, figures and tiles must match the original.

## Repository layout

```
CLAUDE.md, README.md, CONTRIBUTING.md, CREDITS.md, LICENSE
Makefile
core/                 deterministic simulation, no platform dependency
present/              ported drawing and UI (phase 7): deterministic, draws into the state's back
                      buffer, same rules as core/ (tags, integers, no I/O); depends on core, not the reverse
platform/             SDL window, input, clock, audio, video, file system
docs/formats/         file format specs (confirmed / assumed fields)
docs/provenance/      binary address -> engine function index
docs/unverified.md    open questions, one entry per @unverified tag
docs/roadmap.md       phases and items, in order, with their acceptance tests
docs/source-fingerprint.json   SHA-256 of the reference game files
tools/                fingerprinting, provenance and file checks, Ghidra scripts, loop
tools/oracle/         Pharaoh.exe run headless in a CPU emulator (reference runs, saves, functions)
tools/release/        release packages (macOS app, Windows zip, Linux tar.gz / AppImage), docs/release.md
prompts/              standing prompts for headless runs
tests/differential/   differential test harness (reference saves stay outside the repo)
refs/                 Julius, Augustus, Akhenaten clones (git-ignored, read-only)
local/                game data, Ghidra project and exports, save corpus (git-ignored)
```

## Commands

```
make verify        # check game files against docs/source-fingerprint.json
make extract       # extract the GOG installer into local/game; game files land in local/game/app
make refs          # clone reference projects into refs/
make analyze       # Ghidra headless: export functions to local/ghidra-export/
make provenance    # provenance report for core/
make core          # build core/ (C11) into build/libimhotep_core.a
make differential  # differential tests (terrain, save, desirability), needs the game files
make oracle-env    # local/venv with Unicorn + Capstone, for tools/oracle
make corpus        # reference save corpus: player saves advanced 12 months by the original
make check-files   # fail if game files are tracked by git
make build-all     # what CI builds on Linux (gcc, clang), macOS and Windows (MSYS2): core, present, tools, game
make test          # all checks + unit tests + differential tests (when available)
make game          # the game, build/imhotep (SDL2 + FFmpeg; needs only the game files)
make present-check # the presentation port's full per-package matrices against the original (long)
```

Environment: `IMHOTEP_GAME_DIR` (default `local/game/app`), `GHIDRA_HOME` and `JAVA_HOME` (default: Homebrew `ghidra` / `openjdk@21`), `PHARAOH_INSTALLER`.

## How to work on a task

1. Read the relevant spec in `docs/` and the exported functions in `local/ghidra-export/`.
2. Look at `refs/` only to locate the mechanic, then find it in the binary.
3. Implement in `core/` with provenance tags. Small, testable units.
4. Write or update the test. For simulation rules, prefer a differential test; the oracle can run
   the original function on corpus states for comparison.
5. Update `docs/provenance/` (address -> function) and remove resolved `docs/unverified.md` entries.
6. Run `make test`. Do not commit; the human or the loop script commits.

When unsure, stop and write down the question in `docs/unverified.md` rather than guessing.

## Current status

- [x] Fingerprint of the reference installation recorded
- [x] Graphics format readers (sprites) + PNG extractor, visually validated (tools/extract)
- [x] Map reader (core/io/map_file.c)
- [x] Terrain image rebuild at scenario start (core/map/terrain.c), differential test against the
      image grids stored by the original game (`make differential`)
- [x] Save reader (core/io/save_file.c: original saves -> version-160 layout; core/io/save_load.c:
      the loader's later steps incl. version upgrades; `build/save_dump`, `build/save_check`)
- [x] Oracle: Pharaoh.exe boots, loads, ticks and saves headless in a CPU emulator (tools/oracle/)
- [x] Reference save corpus generated: player saves advanced 12 months by the original, saved by
      its own saver (`make corpus`, local/saves/generated/, 104 files)
- [x] First simulation differential test passing (desirability grid, `build/desirability_check`)

- [x] Game state in core (core/state: mirror of the exe's data segments + heap arena), load and
      save on it; save files byte-identical to the original's (`make oracle-check`)

- [x] Tick skeleton (core/sim/tick.c) running in lockstep with the original: unported callees
      replayed from the original, every tick equal (`tools/oracle/lockstep.py`)

- [x] Simulation ported (phase 4): 1,240 functions of the tick, 12 months in lockstep with the
      original on the whole corpus with no difference (docs/roadmap.md 4.8)

- [x] Player commands (phase 5): the original's UI handlers ported (construction, clearing, undo,
      settings and orders, military), 203 command sequences equal to the original in lockstep
      (docs/commands.md, docs/roadmap.md 5.3)

- [x] New games (phase 6): campaign missions, bundled maps and the campaign flow start equal to the
      original (160 / 160 starts, docs/newgame.md, docs/roadmap.md 6.1)

- [x] Platform, playable (phase 7): the frame loop, drawing, city view, sidebar, windows, input,
      sound and the start-up ported (`core/frame`, `core/boot`, `present/`), the SDL platform in
      `platform/` (`make game`); a 1,190-frame scripted session equal to the original, state and
      pixels (docs/presentation.md, docs/roadmap.md 7.9)

- [x] Completeness (phase 8): every function the game can reach is ported or classified with its
      evidence (`tools/oracle/reach.py`, docs/provenance/frontier.md: C runtime, platform, map editor,
      never called); the game reports any gap it meets (gaps.log); 94 randomised sessions, 1.04 million
      frames, equal to the original (`tools/oracle/fuzz.py`, docs/roadmap.md 8.3)

Next steps: `docs/roadmap.md` (carried-over items under 4.8, 5.3, 6.1, 7.9 and 8.4).
