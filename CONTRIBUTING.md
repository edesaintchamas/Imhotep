# Contributing to Imhotep

Thank you for helping. Imhotep has one distinctive rule: **every game rule must be traceable
to the original game.** Please read this page before opening a pull request.

## The reference version

All analysis targets **Pharaoh + Cleopatra, GOG release, patched 1.3**. Check your files with
`make verify`. If your hashes differ, say so in your PR.

## Provenance tags

Any game rule in `core/` carries one of:

- `@bin 0x<address> <desc>`: confirmed in the reference executable
- `@diff <test>`: confirmed by a differential test against the original
- `@hint <project>:<path>`: taken from Julius, Augustus or Akhenaten, not yet confirmed
- `@unverified <reason>`: unknown origin, with an entry in `docs/unverified.md`

Copied or translated code also carries `@from <project>:<path>`, keeps its original copyright
notice, and is listed in `CREDITS.md`.

Wikis, guides and memories of how the game plays are welcome as hints, never as proof.

## Never commit game content

No executables, graphics, audio, video, maps, scenarios, saves, text resources, or decompiled
code. CI rejects them (`make check-files`).

## Pull request checklist

- [ ] `make test` passes
- [ ] New rules are tagged; `make provenance` output pasted in the PR if counts changed
- [ ] New `@unverified` tags have an entry in `docs/unverified.md`
- [ ] Copied code: original notice kept, `@from` tag, `CREDITS.md` updated
- [ ] AI assistance disclosed in the PR description (tool used, what it wrote)

## AI-assisted contributions

Welcome, under the same standard as any other code. Disclose the tool. You remain responsible
for every line, including its provenance tags.
