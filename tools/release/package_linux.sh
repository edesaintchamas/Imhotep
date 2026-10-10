#!/bin/bash
# Imhotep - the Linux packages: a tar.gz and an AppImage, with SDL2 and FFmpeg inside (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   LINUXDEPLOY=path/to/linuxdeploy-x86_64.AppImage tools/release/package_linux.sh VERSION EXE OUTDIR
#
# EXE: the game built with IMHOTEP_VERSION=VERSION (make game). linuxdeploy gathers the shared
# libraries EXE needs, minus those every desktop system provides (glibc, the GPU drivers, X11 /
# Wayland, ALSA / PulseAudio: AppImage's exclude list), and sets the executable's RUNPATH to
# $ORIGIN/../lib. Writes
#   OUTDIR/imhotep-VERSION-linux-ARCH.tar.gz: imhotep-VERSION-linux-ARCH/{imhotep, bin/, lib/, README.md, LICENSE}
#   OUTDIR/Imhotep-VERSION-ARCH.AppImage
set -euo pipefail
[ $# = 3 ] || { echo "usage: $0 VERSION EXE OUTDIR"; exit 1; }
version=$1 exe=$2 out=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
: "${LINUXDEPLOY:?set LINUXDEPLOY to the linuxdeploy AppImage}"
arch=$(uname -m)
mkdir -p "$out"
out=$(cd "$out" && pwd)
work=$(mktemp -d)
appdir=$work/AppDir

python3 "$root/tools/release/make_icon.py" 256 "$work/imhotep.png"
cat > "$work/imhotep.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Imhotep
Comment=The engine of Pharaoh + Cleopatra (needs the game, GOG 2.1)
Exec=imhotep
Icon=imhotep
Categories=Game;StrategyGame;
Terminal=false
EOF

export APPIMAGE_EXTRACT_AND_RUN=1 LINUXDEPLOY_OUTPUT_VERSION=$version
(cd "$work" && "$LINUXDEPLOY" --appdir "$appdir" --executable "$root/$exe" \
    --desktop-file "$work/imhotep.desktop" --icon-file "$work/imhotep.png" --output appimage)
mv "$work"/Imhotep-*.AppImage "$out/Imhotep-$version-$arch.AppImage"

# the archive: the same binary and libraries, without the AppImage runtime
name=imhotep-$version-linux-$arch
stage=$work/$name
mkdir -p "$stage"
cp -a "$appdir/usr/bin" "$appdir/usr/lib" "$stage/"
ln -s bin/imhotep "$stage/imhotep"
cp "$root/README.md" "$root/LICENSE" "$stage/"
python3 "$root/tools/release/check_package.py" "$stage"
python3 "$root/tools/release/check_package.py" "$appdir"
tar -C "$work" -czf "$out/$name.tar.gz" "$name"
echo "$out/$name.tar.gz"
echo "$out/Imhotep-$version-$arch.AppImage"
rm -rf "$work"
