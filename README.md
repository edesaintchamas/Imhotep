# Imhotep

An open-source reimplementation of the engine of **Pharaoh + Cleopatra** (Impressions Games, 1999-2000),
aiming at faithful simulation and compatibility with original save files.

> **Status: early development.** Nothing is playable yet.

## Requirements

Imhotep does not include any game content. You need a legitimate copy of
**Pharaoh + Cleopatra** (the GOG release, version 2.1, is the reference version).

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
