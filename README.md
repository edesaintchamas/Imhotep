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

## Download

Ready-made builds are on the [Releases page](https://github.com/edesaintchamas/Imhotep/releases).
None of them contains anything from the game: you still need your own installation (see below).
Release 1.0.0 is for macOS; the Windows and Linux builds will follow once they are play-tested
(until then, build them from source: see Playing).

- **macOS** (Apple silicon, macOS 11 or later): `imhotep-VERSION-macos-arm64.zip` holds
  `Imhotep.app`; move it to Applications. The app is not signed with an Apple Developer ID, so the
  first time Gatekeeper refuses to open it: right-click (or Control-click) `Imhotep.app` > Open, then
  Open in the dialog (or System Settings > Privacy & Security > Open Anyway). Later starts open it
  normally.
- **Windows** (64-bit): unzip `imhotep-VERSION-windows-x64.zip` anywhere and run `imhotep.exe`
  (`SDL2.dll` must stay next to it). The executable is not signed: if SmartScreen says "Windows
  protected your PC", click More info > Run anyway.
- **Linux** (x86-64, glibc 2.35 or later: Ubuntu 22.04, Debian 12, Fedora 36 and later): either
  `Imhotep-VERSION-x86_64.AppImage` (`chmod +x` it and run it; it needs FUSE, else run it with
  `--appimage-extract-and-run`), or `imhotep-VERSION-linux-x86_64.tar.gz` (unpack it and run
  `./imhotep`). SDL2 and FFmpeg are built in; the system provides the graphics drivers, X11 or
  Wayland and the sound server (ALSA, PulseAudio or PipeWire), as on any desktop.

Imhotep finds a GOG installation in the usual places by itself. Otherwise start it once with the
game folder, the one holding `Pharaoh.exe` (it remembers it afterwards):
`imhotep --game "FOLDER"`; on macOS, from Terminal,
`/Applications/Imhotep.app/Contents/MacOS/imhotep --game "FOLDER"`.

## Playing

To build from source, you need a C compiler, `make`, `pkg-config`, SDL2 and FFmpeg (for the videos):

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
