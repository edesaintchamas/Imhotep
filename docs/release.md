# Cutting a release

The packages are built by `.github/workflows/release.yml` (roadmap 9.4) with the scripts in
`tools/release/`; nobody builds them by hand.

| Package | Built on | Contents |
|---|---|---|
| `imhotep-X.Y.Z-macos-arm64.zip` (3 MB) | macos-latest, for macOS 11 and later | `Imhotep.app`: the game, the icon (`make_icon.py`, drawn from scratch), README, LICENSE, the libraries' licences; signed ad hoc only |
| `imhotep-X.Y.Z-windows-x64.zip` (3.5 MB) | windows-latest, MSYS2 UCRT64 | `imhotep.exe`, `SDL2.dll` and `libwinpthread-1.dll` (found with `ldd`), README.md, LICENSE.txt, licences |
| `imhotep-X.Y.Z-linux-x86_64.tar.gz` (3 MB) | ubuntu-22.04 (needs glibc 2.35) | `imhotep`, README.md, LICENSE, licences, a `.desktop` file and the icon |
| `Imhotep-X.Y.Z-x86_64.AppImage` (3.5 MB) | ubuntu-22.04 | the same binary as an AppImage (linuxdeploy) |
| `SHA256SUMS.txt` | | checksums of the above |

FFmpeg is not taken from the system: `tools/release/build_deps.sh` builds FFmpeg (pinned release,
SHA-256 checked; LGPL, only the MP3, WAV and Bink demuxers and decoders the game uses, no external
libraries) and, on macOS and Linux, SDL2 from source, and the game links them statically. On Linux
the executable then needs only glibc (SDL2 loads X11 / Wayland and the sound server's libraries at
run time); on macOS only the system's frameworks, from macOS 11 (`MACOSX_DEPLOYMENT_TARGET`); on
Windows SDL2 comes from MSYS2 as a DLL. The builds are cached (keyed on `build_deps.sh`); to update
FFmpeg or SDL2, change the version and SHA-256 there. The minimal FFmpeg decodes all 837 MP3, WAV
and Bink files of the GOG installation exactly as a full FFmpeg does (same PCM and frames; checked
when this was set up).

Every package goes through `tools/release/check_package.py` (no file with the name or the content of
a file of the game, no game content extension) and, on a fresh runner with nothing installed,
`tools/release/smoke.sh` (`imhotep --version` prints the version; a start without the game stops
with a clean error, status 1), which fails if a library is missing. The Linux packages are tested in
Ubuntu 22.04, Ubuntu 24.04, Debian 12 and Fedora containers.

## Steps

1. On `main`, with CI green: choose the version `X.Y.Z` and tag the commit.

   ```
   git tag -a vX.Y.Z -m "Imhotep X.Y.Z"
   git push origin vX.Y.Z
   ```

   The tag without its `v` becomes the version (`IMHOTEP_VERSION`, printed by `imhotep --version`,
   and the macOS bundle's version).
2. Wait for the Release workflow (Actions tab). When all its jobs pass, it creates a **draft**
   release named "Imhotep X.Y.Z" with the packages attached and notes generated from the commits.
   If a job fails, nothing is released: fix it on `main`, delete the tag
   (`git push origin :vX.Y.Z; git tag -d vX.Y.Z`) and tag again.
3. Check the draft: the five files are attached; download at least the one for your system, unpack
   it and play it against a Pharaoh installation (roadmap 9.6: a fresh machine with only Pharaoh).
   Edit the notes (what changed, known issues).
4. Publish the draft.

To try the workflow without releasing: Actions > Release > Run workflow (on any branch; the file
must be on the default branch for the button to appear). Such runs upload the packages as workflow
artifacts, versioned `dev-<commit>`, and create no release.

## Locally

The scripts run outside CI too (macOS shown; Windows builds only `ffmpeg` and uses MSYS2's SDL2
with `PKG_CONFIG_PATH`, Linux needs the X11 / Wayland / ALSA / PulseAudio headers the workflow
installs):

```
tools/release/build_deps.sh build-deps ffmpeg sdl2
PKG_CONFIG_LIBDIR=$PWD/build-deps/lib/pkgconfig MACOSX_DEPLOYMENT_TARGET=11.0 \
    make game PKG_CONFIG="pkg-config --static" IMHOTEP_VERSION=X.Y.Z
tools/release/package_macos.sh X.Y.Z build/imhotep dist          # macOS
tools/release/package_windows.sh X.Y.Z build/imhotep.exe dist    # MSYS2 UCRT64 shell
LINUXDEPLOY=path/to/linuxdeploy-x86_64.AppImage tools/release/package_linux.sh X.Y.Z build/imhotep dist
tools/release/smoke.sh dist/Imhotep.app/Contents/MacOS/imhotep X.Y.Z
```

## Not done

- No Developer ID signature or notarization on macOS, no Authenticode signature on Windows: players
  see the Gatekeeper and SmartScreen warnings the README explains.
- macOS: arm64 only. An Intel or universal build would need the dependencies built for x86_64 too
  (`build_deps.sh` with `-arch x86_64`, then `lipo`).
