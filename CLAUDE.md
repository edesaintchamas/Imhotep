# CLAUDE.md — Imhotep

> Working name. To rename the project: `grep -rl imhotep . | xargs sed -i '' 's/imhotep/NEWNAME/g; s/Imhotep/Newname/g'`

## Goal

Imhotep is an open-source (AGPL-3.0) reimplementation of the engine of **Pharaoh + Cleopatra**
(Impressions Games, 1999-2000). It aims at **faithful simulation**: same rules, same results,
loading and writing original save files. Original assets are loaded from a legitimate
installation of the game; nothing from the game ships with this repository.

Reference version: **GOG Pharaoh + Cleopatra, patched 1.3**. Its hashes are recorded in
`docs/source-fingerprint.json`. All analysis is performed on this exact binary.

## Sources of truth (in strict order)

1. **The original binary** (`Pharaoh.exe`, GOG 1.3). The only ground truth.
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
| `@diff <test name>` | Confirmed by a differential test against the original | confirmed |
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
platform/             SDL rendering, input, audio, file system
docs/formats/         file format specs (confirmed / assumed fields)
docs/provenance/      binary address -> engine function index
docs/unverified.md    open questions, one entry per @unverified tag
docs/source-fingerprint.json   SHA-256 of the reference game files
tools/                fingerprinting, provenance and file checks, Ghidra scripts, loop
prompts/              standing prompts for headless runs
tests/differential/   differential test harness (reference saves stay outside the repo)
refs/                 Julius, Augustus, Akhenaten clones (git-ignored, read-only)
local/                game data, Ghidra project and exports, save corpus (git-ignored)
```

## Commands

```
make verify        # check game files against docs/source-fingerprint.json
make extract       # extract the GOG installer into local/game (innoextract)
make refs          # clone reference projects into refs/
make analyze       # Ghidra headless: export functions to local/ghidra-export/
make provenance    # provenance report for core/
make check-files   # fail if game files are tracked by git
make test          # all checks + unit tests + differential tests (when available)
```

Environment: `IMHOTEP_GAME_DIR` (default `local/game`), `GHIDRA_HOME`, `PHARAOH_INSTALLER`.

## How to work on a task

1. Read the relevant spec in `docs/` and the exported functions in `local/ghidra-export/`.
2. Look at `refs/` only to locate the mechanic, then find it in the binary.
3. Implement in `core/` with provenance tags. Small, testable units.
4. Write or update the test. For simulation rules, prefer a differential test.
5. Update `docs/provenance/` (address -> function) and remove resolved `docs/unverified.md` entries.
6. Run `make test`. Do not commit; the human or the loop script commits.

When unsure, stop and write down the question in `docs/unverified.md` rather than guessing.

## Current status

- [ ] Fingerprint of the reference installation recorded
- [ ] Graphics format readers (sprites) + PNG extractor, visually validated
- [ ] Map reader
- [ ] Save reader (load original saves, dump state)
- [ ] Reference save corpus generated (campaign missions, monthly saves)
- [ ] First differential test passing
