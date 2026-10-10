#!/bin/bash
# Imhotep - the Linux packages: a tar.gz and an AppImage (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   LINUXDEPLOY=path/to/linuxdeploy-x86_64.AppImage tools/release/package_linux.sh VERSION EXE OUTDIR
#
# EXE: the game built with IMHOTEP_VERSION=VERSION, SDL2 and FFmpeg linked statically
# (tools/release/build_deps.sh). It must need nothing but glibc: SDL2 loads X11 / Wayland and the sound
# server's libraries at run time from the system, as every desktop has them. Writes
#   OUTDIR/imhotep-VERSION-linux-ARCH.tar.gz: imhotep-VERSION-linux-ARCH/{imhotep, README.md, LICENSE,
#       licenses/, imhotep.desktop, imhotep.png}
#   OUTDIR/Imhotep-VERSION-ARCH.AppImage (linuxdeploy; LINUXDEPLOY unset: no AppImage)
# LICENSES: the directory with SDL2's and FFmpeg's licences (default build-deps/share/licenses).
set -euo pipefail
[ $# = 3 ] || { echo "usage: $0 VERSION EXE OUTDIR"; exit 1; }
version=$1 exe=$2 out=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
licenses=${LICENSES:-$root/build-deps/share/licenses}
arch=$(uname -m)
mkdir -p "$out"
out=$(cd "$out" && pwd)
work=$(mktemp -d)

# only glibc's libraries
extra=$(ldd "$exe" | awk '{ print $1 }' | grep -Ev '^(linux-vdso|/lib.*/ld-linux|ld-linux|libc|libm|libdl|libpthread|librt)[.-]' || true)
if [ -n "$extra" ]; then
    echo "$exe needs libraries other than glibc's:"
    echo "$extra"
    exit 1
fi
echo "glibc $(objdump -T "$exe" | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1) or later"

name=imhotep-$version-linux-$arch
stage=$work/$name
mkdir -p "$stage"
cp "$exe" "$stage/imhotep"
strip "$stage/imhotep"
cp "$root/README.md" "$root/LICENSE" "$stage/"
cp -R "$licenses" "$stage/licenses"
python3 "$root/tools/release/make_icon.py" 256 "$stage/imhotep.png"
cat > "$stage/imhotep.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Imhotep
Comment=The engine of Pharaoh + Cleopatra (needs the game, GOG 2.1)
Exec=imhotep
Icon=imhotep
Categories=Game;StrategyGame;
Terminal=false
EOF
python3 "$root/tools/release/check_package.py" "$stage"
tar -C "$work" -czf "$out/$name.tar.gz" "$name"
echo "$out/$name.tar.gz"

if [ -n "${LINUXDEPLOY:-}" ]; then
    appdir=$work/AppDir
    mkdir -p "$appdir/usr/share/doc/imhotep"
    cp -R "$stage/README.md" "$stage/LICENSE" "$stage/licenses" "$appdir/usr/share/doc/imhotep/"
    export APPIMAGE_EXTRACT_AND_RUN=1 LINUXDEPLOY_OUTPUT_VERSION=$version
    (cd "$work" && "$LINUXDEPLOY" --appdir "$appdir" --executable "$stage/imhotep" \
        --desktop-file "$stage/imhotep.desktop" --icon-file "$stage/imhotep.png" --output appimage)
    python3 "$root/tools/release/check_package.py" "$appdir"
    mv "$work"/Imhotep-*.AppImage "$out/Imhotep-$version-$arch.AppImage"
    echo "$out/Imhotep-$version-$arch.AppImage"
fi
rm -rf "$work"
