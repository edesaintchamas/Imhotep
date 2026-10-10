#!/usr/bin/env python3
# Imhotep - the application icon, drawn from scratch (no game content): a pyramid at dusk.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/make_icon.py SIZE OUT.png
#
# Plain Python (zlib, no imaging library); shapes are anti-aliased from their signed distance.
import math, struct, sys, zlib


def clamp(x):
    return 0.0 if x < 0 else 1.0 if x > 1 else x


def half_plane(px, py, ax, ay, bx, by):
    """signed distance (pixels) of p to line a->b, positive on the left"""
    dx, dy = bx - ax, by - ay
    return ((px - ax) * dy - (py - ay) * dx) / math.hypot(dx, dy)


def triangle(px, py, a, b, c):
    """coverage of a triangle given in either winding"""
    sign = 1 if half_plane(*c, *a, *b) > 0 else -1
    inside = min(half_plane(px, py, *a, *b), half_plane(px, py, *b, *c), half_plane(px, py, *c, *a), key=lambda d: sign * d)
    return clamp(0.5 + sign * inside)


def mix(c0, c1, t):
    return tuple(c0[i] + (c1[i] - c0[i]) * t for i in range(3))


def render(n):
    s = n / 1024.0
    radius = 180 * s
    sky_top, sky_low = (32, 46, 92), (226, 120, 72)
    sun = (252, 214, 120)
    sand = (196, 142, 74)
    lit, shade = (240, 200, 110), (170, 112, 52)
    apex, left, right, mid = (512 * s, 250 * s), (150 * s, 760 * s), (874 * s, 760 * s), (600 * s, 760 * s)
    ground = 740 * s
    rows = []
    for y in range(n):
        py = y + 0.5
        row = bytearray([0])
        sky = mix(sky_top, sky_low, clamp(py / (ground)))
        for x in range(n):
            px = x + 0.5
            # rounded square mask
            qx = max(abs(px - n / 2) - (n / 2 - radius), 0.0)
            qy = max(abs(py - n / 2) - (n / 2 - radius), 0.0)
            alpha = clamp(0.5 + radius - math.hypot(qx, qy))
            col = sky
            d_sun = math.hypot(px - 760 * s, py - 330 * s)
            col = mix(col, sun, clamp(0.5 + 110 * s - d_sun))
            col = mix(col, sand, clamp(0.5 + py - ground))
            col = mix(col, lit, triangle(px, py, apex, mid, left))
            col = mix(col, shade, triangle(px, py, apex, right, mid))
            row += bytes((int(col[0] + 0.5), int(col[1] + 0.5), int(col[2] + 0.5), int(alpha * 255 + 0.5)))
        rows.append(bytes(row))
    return b"".join(rows)


def png(n, data):
    def chunk(tag, body):
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", n, n, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(data, 9)) + chunk(b"IEND", b""))


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: make_icon.py SIZE OUT.png")
    n = int(sys.argv[1])
    with open(sys.argv[2], "wb") as f:
        f.write(png(n, render(n)))


if __name__ == "__main__":
    main()
