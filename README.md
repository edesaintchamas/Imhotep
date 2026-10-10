# Imhotep

An open-source reimplementation of the engine of **Pharaoh + Cleopatra** (Impressions Games, 1999-2000),
aiming at faithful simulation and compatibility with original save files.

> **Status: playable.** Everything the game can reach is ported from the original executable
> (3,097 functions) and checked against it: randomised play sessions, over a million frames, are
> equal to the original frame by frame, state and pixels. The map editor is not included. See
> [docs/roadmap.md](docs/roadmap.md) for what is left.

## Requirements

Imhotep does not include any game content. You need a legitimate copy of
**Pharaoh + Cleopatra** (the GOG release, version 2.1, is the reference version).

## Playing

You need a C compiler, `make`, `pkg-config`, SDL2 and FFmpeg (for the videos):

- macOS: `brew install sdl2 ffmpeg pkg-config`
- Debian / Ubuntu: `sudo apt install build-essential pkg-config libsdl2-dev libavformat-dev libavcodec-dev libswresample-dev libswscale-dev libavutil-dev`
- Windows: [MSYS2](https://www.msys2.org/) (UCRT64), then
  `pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-ffmpeg`

```
make game
build/imhotep --game "path/to/Pharaoh + Cleopatra"   # the folder holding Pharaoh.exe; remembered after
```

Imhotep needs an installation of Pharaoh + Cleopatra, the GOG version 2.1: it checks `Pharaoh.exe`
against that exact version and refuses another. It looks for the game folder in this order: `--game DIR`,
the `IMHOTEP_GAME_DIR` variable, the folder found at the last start, then the usual install locations
(GOG on Windows, through its registry entries and `C:\GOG Games`; `~/GOG Games`, `~/Games`, Wine
prefixes and CrossOver bottles on Linux and macOS; a checkout's `local/game/app`, which
`make extract PHARAOH_INSTALLER=path/to/setup_pharaoh.exe` fills). The game folder is only read.
Saves and settings go to `IMHOTEP_PLAYER_DIR` if set, else to the per-user data directory
(macOS `~/Library/Application Support/Imhotep/Imhotep`, Linux `~/.local/share/Imhotep/Imhotep`,
Windows `%APPDATA%\Imhotep\Imhotep`), which also holds `imhotep.cfg` (the remembered game folder).
Options: `--scale N` (window size), `--no-intro`, `--no-sound`, `--load SAVE`; `build/imhotep --help`
lists them all.

### If something goes wrong

Imhotep should behave exactly as the original. If the game reaches a part of the original that
is not ported, it says so on its console and appends a line to `gaps.log` in the player
directory, then goes on without it. Please [open an issue](https://github.com/edesaintchamas/Imhotep/issues)
with `gaps.log`, what you were doing, and a save from just before if you have one (saves are your
own files; never attach files from the game's installation).

### What differs from the original

- The map editor is not included.
- Two messages carry bytes the original never sets, so its own content varies with what was in
  memory: the tutorial's goods message and the monument-finished message. Imhotep fills them
  with fixed values ([docs/unverified.md](docs/unverified.md) lists these cases).
- Saves from versions much older than the GOG release (formats 115–135) are not converted.

## Method

Imhotep is built around verifiable fidelity:

- Every game rule in the simulation core is traced to an address in the original executable,
  or confirmed by a differential test against the original game. Rules not yet confirmed are
  explicitly tagged, and `make provenance` reports how much of the engine is still unconfirmed.
- The simulation core is deterministic and separate from rendering, so it can be tested
  against saves produced by the original game.

See [CONTRIBUTING.md](CONTRIBUTING.md) for the rules.

## Use of AI

This project is developed with the help of Claude Code (Anthropic). AI-written code is held to
the same standard as any contribution: no rule is accepted without a binary address or a
passing differential test.

## Related projects

Imhotep builds on the work of [Julius](https://github.com/bvschaik/julius),
[Augustus](https://github.com/Keriew/augustus) and [Akhenaten](https://github.com/dalerank/Akhenaten).
See [CREDITS.md](CREDITS.md).

## Legal

Imhotep is licensed under the [GNU AGPL-3.0](LICENSE).

Pharaoh and Cleopatra: Queen of the Nile are trademarks of their respective owners. This project
is not affiliated with or endorsed by Activision, Impressions Games, BreakAway Games, Triskell
Interactive or Dotemu.
