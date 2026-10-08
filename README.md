# Imhotep

An open-source reimplementation of the engine of **Pharaoh + Cleopatra** (Impressions Games, 1999-2000),
aiming at faithful simulation and compatibility with original save files.

> **Status: playable.** The simulation, the player commands, new games, the drawing and the
> interface are ported from the original executable and checked against it; a scripted session
> of play is equal to the original frame by frame, state and pixels. See
> [docs/roadmap.md](docs/roadmap.md) for what is left.

## Requirements

Imhotep does not include any game content. You need a legitimate copy of
**Pharaoh + Cleopatra** (the GOG release, version 2.1, is the reference version).

## Playing

You need a C compiler, SDL2 and FFmpeg (for the videos), e.g. `brew install sdl2 ffmpeg`.

```
make extract PHARAOH_INSTALLER=path/to/setup_pharaoh.exe   # or point IMHOTEP_GAME_DIR at an installation
make game
build/imhotep
```

The game files are read from `IMHOTEP_GAME_DIR` (default `local/game/app`) and checked against
the reference version; saves and settings go to `IMHOTEP_PLAYER_DIR` (default `local/player`).
Options: `--scale N` (window size), `--no-intro`, `--no-sound`, `--load SAVE`; `platform/main.c` lists them all.

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
