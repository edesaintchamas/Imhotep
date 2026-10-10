#!/bin/bash
# Imhotep - the macOS package: Imhotep.app with SDL2 and FFmpeg inside, zipped (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/package_macos.sh VERSION EXE OUTDIR
#
# EXE: the game built with IMHOTEP_VERSION=VERSION (make game). Writes OUTDIR/Imhotep.app and
# OUTDIR/imhotep-VERSION-macos-ARCH.zip (ditto). The app is signed ad hoc only (no Developer ID):
# Gatekeeper asks the player to open it with right-click > Open the first time.
set -euo pipefail
[ $# = 3 ] || { echo "usage: $0 VERSION EXE OUTDIR"; exit 1; }
version=$1 exe=$2 out=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
arch=$(uname -m)
app=$out/Imhotep.app
rm -rf "$app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Frameworks" "$app/Contents/Resources"
cp "$exe" "$app/Contents/MacOS/imhotep"
cp "$root/README.md" "$root/LICENSE" "$app/Contents/Resources/"

# the icon (drawn by tools/release/make_icon.py, no game content)
iconset=$(mktemp -d)/Imhotep.iconset
mkdir -p "$iconset"
python3 "$root/tools/release/make_icon.py" 1024 "$iconset/icon_512x512@2x.png"
for s in 16 32 128 256 512; do
    sips -z $s $s "$iconset/icon_512x512@2x.png" --out "$iconset/icon_${s}x${s}.png" > /dev/null
    sips -z $((s * 2)) $((s * 2)) "$iconset/icon_512x512@2x.png" --out "$iconset/icon_${s}x${s}@2x.png" > /dev/null
done
iconutil -c icns "$iconset" -o "$app/Contents/Resources/Imhotep.icns"

minos=$(python3 "$root/tools/release/bundle_dylibs.py" "$app/Contents/MacOS/imhotep" "$app/Contents/Frameworks")

cat > "$app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Imhotep</string>
    <key>CFBundleDisplayName</key>
    <string>Imhotep</string>
    <key>CFBundleIdentifier</key>
    <string>io.github.edesaintchamas.imhotep</string>
    <key>CFBundleExecutable</key>
    <string>imhotep</string>
    <key>CFBundleIconFile</key>
    <string>Imhotep</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>$version</string>
    <key>CFBundleVersion</key>
    <string>$version</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>$minos</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.strategy-games</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>Imhotep contributors, GNU AGPL-3.0. Needs Pharaoh + Cleopatra (GOG 2.1), not included.</string>
</dict>
</plist>
EOF
plutil -lint "$app/Contents/Info.plist" > /dev/null
# the bundle as a whole (ad hoc: no identity; Gatekeeper still treats it as from an unidentified developer)
codesign --force --sign - "$app"
python3 "$root/tools/release/check_package.py" "$app"
zip=$out/imhotep-$version-macos-$arch.zip
rm -f "$zip"
ditto -c -k --keepParent "$app" "$zip"
echo "$zip (macOS $minos or later, $arch)"
