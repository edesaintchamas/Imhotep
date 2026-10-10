# Cutting a release

The packages are built by `.github/workflows/release.yml` (roadmap 9.4) with the scripts in
`tools/release/`; nobody builds them by hand.

| Package | Built on | Contents |
|---|---|---|
| `imhotep-X.Y.Z-macos-arm64.zip` | macos-latest | `Imhotep.app`: the game, SDL2, FFmpeg and their dylibs (`bundle_dylibs.py`), the icon (`make_icon.py`, drawn from scratch), README, LICENSE; signed ad hoc only |
| `imhotep-X.Y.Z-windows-x64.zip` | windows-latest, MSYS2 UCRT64 | `imhotep.exe`, the UCRT64 DLLs it needs (found with `ldd`), README.md, LICENSE.txt |
| `imhotep-X.Y.Z-linux-x86_64.tar.gz` | ubuntu-22.04 (glibc 2.35) | `imhotep` (a link to `bin/imhotep`), `lib/` (gathered by linuxdeploy), README.md, LICENSE |
| `Imhotep-X.Y.Z-x86_64.AppImage` | ubuntu-22.04 | the same binary and libraries as an AppImage |
| `SHA256SUMS.txt` | | checksums of the above |

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

The scripts run outside CI too (build with the version first):

```
make game IMHOTEP_VERSION=X.Y.Z
tools/release/package_macos.sh X.Y.Z build/imhotep dist          # macOS
tools/release/package_windows.sh X.Y.Z build/imhotep.exe dist    # MSYS2 UCRT64 shell
LINUXDEPLOY=path/to/linuxdeploy-x86_64.AppImage tools/release/package_linux.sh X.Y.Z build/imhotep dist
tools/release/smoke.sh dist/Imhotep.app/Contents/MacOS/imhotep X.Y.Z
```

## Not done

- No Developer ID signature or notarization on macOS, no Authenticode signature on Windows: players
  see the Gatekeeper and SmartScreen warnings the README explains.
- macOS: arm64 only (Homebrew's libraries on the arm64 runner); the minimum macOS version is the
  highest one among the bundled libraries (`LSMinimumSystemVersion`), that of the runner's Homebrew.
