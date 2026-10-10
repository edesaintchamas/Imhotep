#!/bin/bash
# Imhotep - the libraries a release links statically: a minimal FFmpeg, and SDL2 (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/build_deps.sh PREFIX ffmpeg [sdl2]
#
# Downloads the pinned source releases (checked against their SHA-256) into $DEPS_SRC (default
# PREFIX/src) and installs static libraries and pkg-config files into PREFIX. Then
#   PKG_CONFIG_PATH=PREFIX/lib/pkgconfig make game PKG_CONFIG="pkg-config --static"
# links them into the executable, which needs no FFmpeg (nor SDL2, with sdl2) at run time.
#
# FFmpeg: LGPL only (no --enable-gpl), and only what the game opens (platform/media.c): the file
# protocol; MP3, WAV and Bink files; their decoders (MP3, PCM, Bink video and audio, as found in the
# GOG installation: mp3, pcm_s16le, binkvideo (decoder "bink"), binkaudio_rdft; a few neighbours); no
# assembly (nothing to install; C is plenty for 640x480 videos), no external libraries.
# SDL2: its backends (X11, Wayland, ALSA, PulseAudio, PipeWire on Linux) are loaded at run time, as
# SDL does by default, so the executable depends on none of them; their headers are needed to build.
# On macOS MACOSX_DEPLOYMENT_TARGET (default 11.0) applies to both, and to the game built after.
# The licences and the FFmpeg configuration go to PREFIX/share/licenses, for the packages.
set -euo pipefail
[ $# -ge 2 ] || { echo "usage: $0 PREFIX ffmpeg [sdl2]"; exit 1; }
mkdir -p "$1"
prefix=$(cd "$1" && pwd)
shift
src=${DEPS_SRC:-$prefix/src}
mkdir -p "$src" "$prefix/share/licenses"
jobs=${JOBS:-4}
if [ "$(uname)" = Darwin ]; then
    export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-11.0}
fi

FFMPEG_VERSION=8.1.3
FFMPEG_SHA256=7138d28c96d9d3e3af4ee3d8cad72741f8ffb40da90c1112235dea3ecd3178a3
SDL2_VERSION=2.32.10
SDL2_SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165

sha256() { if command -v sha256sum > /dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

fetch() { # URL SHA256 -> the checked file in $src
    local file=$src/$(basename "$1")
    if [ ! -f "$file" ]; then
        curl -fsSL --retry 3 -o "$file.part" "$1"
        mv "$file.part" "$file"
    fi
    local got
    got=$(sha256 "$file")
    [ "$got" = "$2" ] || { echo "$file: SHA-256 $got, expected $2"; rm -f "$file"; exit 1; }
    echo "$file"
}

ffmpeg_flags=(
    --enable-static --disable-shared --disable-programs --disable-doc --disable-debug
    --disable-autodetect --disable-everything --disable-network --disable-x86asm
    --disable-avdevice --disable-avfilter
    --enable-avformat --enable-avcodec --enable-swresample --enable-swscale
    --enable-protocol=file
    --enable-demuxer=mp3,wav,bink
    --enable-parser=mpegaudio
    --enable-decoder=mp3,mp3float,pcm_s16le,pcm_u8,adpcm_ms,adpcm_ima_wav,bink,binkaudio_rdft,binkaudio_dct
)

build_ffmpeg() {
    local tarball dir
    tarball=$(fetch "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz" "$FFMPEG_SHA256")
    dir=$(mktemp -d)
    tar -C "$dir" -xJf "$tarball"
    (cd "$dir/ffmpeg-$FFMPEG_VERSION" &&
        ./configure --prefix="$prefix" --pkg-config-flags=--static "${ffmpeg_flags[@]}" &&
        make -j"$jobs" && make install)
    mkdir -p "$prefix/share/licenses/FFmpeg"
    cp "$dir/ffmpeg-$FFMPEG_VERSION/COPYING.LGPLv2.1" "$prefix/share/licenses/FFmpeg/"
    cat > "$prefix/share/licenses/FFmpeg/README.txt" <<EOF
Imhotep links FFmpeg $FFMPEG_VERSION (https://ffmpeg.org), licensed under the GNU LGPL 2.1 or later
(COPYING.LGPLv2.1). Source: https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz
(SHA-256 $FFMPEG_SHA256), built by tools/release/build_deps.sh in Imhotep's repository
(https://github.com/edesaintchamas/Imhotep), configured with:
  ${ffmpeg_flags[*]}
EOF
    rm -rf "$dir"
}

build_sdl2() {
    local tarball dir
    tarball=$(fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDL2_VERSION/SDL2-$SDL2_VERSION.tar.gz" "$SDL2_SHA256")
    dir=$(mktemp -d)
    tar -C "$dir" -xzf "$tarball"
    (cd "$dir/SDL2-$SDL2_VERSION" &&
        ./configure --prefix="$prefix" --disable-shared --enable-static --with-pic &&
        make -j"$jobs" && make install)
    mkdir -p "$prefix/share/licenses/SDL2"
    cp "$dir/SDL2-$SDL2_VERSION/LICENSE.txt" "$prefix/share/licenses/SDL2/"
    rm -rf "$dir"
}

for dep in "$@"; do
    case $dep in
        ffmpeg) build_ffmpeg ;;
        sdl2) build_sdl2 ;;
        *) echo "unknown: $dep"; exit 1 ;;
    esac
done
