"""Imhotep - parts of the game state the port does not reproduce, and why.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

Each rule names state the simulation never reads; differences there are not failures.
docs/formats/state.md lists the same rules with their evidence.
"""
import struct

STATE_BASE = 0x0056F000

# (first address, length, reason)
RANGES = [
    (0x01265188, 0x10, 'compressed-chunk reader workspace (0x0055BCF0, 0x0055C000): buffer pointer, '
                       'length, checksum of the last chunk read; file I/O only'),
    # scratch: written before every read within a tick, load included (tools/oracle/scratch.py,
    # Itjtawy, 816 ticks); their value between ticks cannot influence the simulation
    (0x00BC9740, 0xC, 'scratch: pattern matcher results (variant, extra, offset)'),
    (0x00BC9760, 0xC, 'scratch: terrain pass cursor (y, x, grid offset)'),
    (0x00BC97C0, 0x20, 'scratch: neighbour flags'),
    # presentation (core/sim/platform.h): only the video module (0x00538E30 .. 0x00539220) and the
    # screen present 0x004D0280 read the video globals; the sound request block is read only by the
    # sound driver 0x0055F0E8; the path and log buffers by the file and log functions (0x004CE100,
    # 0x004CE330 .. 0x004CE480, 0x004CFF20). Static references: local/ghidra-export.
    (0x00F53FF4, 0x68, 'video player state (0x00538E30)'),
    (0x00F54114, 0x4, 'video player handle (0x00538E30)'),
    (0x00E92DA8, 0x14, 'sound request block (0x0053B4A0, 0x0053B090)'),
    (0x00E36BF8, 0x28, 'file path buffer (0x004CE100)'),
    (0x00E38AEC, 0x4, 'file path state (0x004CE100 and the file functions)'),
    (0x00E39108, 0x20, 'log buffer (0x004CFF20)'),
    (0x00E92BF8 + 0x1D8, 0xC, 'sound stream handles (0x0055F449; read by the sound functions 0x0055Fxxx)'),
    (0x00E36C90, 0x100, 'log line buffer (0x004CE6E0)'),
]

ROUTES = 0x00776FB0      # chunk 143: 50 records of 0x144 bytes
ROUTE_SIZE = 0x144


def route_point_blocks(state):
    """[(address, length)] of the empire-route point buffers (+0x04 pointer, +0x138 capacity in
    points of 6 bytes). Their points are computed with x87 arithmetic for the empire map; the
    tick never reads them (tools/oracle/readset.py: no read of these heap pages in 13 months)."""
    out = []
    for k in range(50):
        o = ROUTES + ROUTE_SIZE * k - STATE_BASE
        ptr, = struct.unpack_from('<I', state, o + 4)
        cap, = struct.unpack_from('<i', state, o + 0x138)
        if ptr and cap > 0:
            out.append((ptr, cap * 6))
    return out


def excluder(state):
    """A predicate addr -> reason or None, for the state file `state` (the original's). The
    harness applies it too (function results compared with scratch excluded)."""
    spans = [(a, a + n, r) for a, n, r in RANGES]
    spans += [(a, a + n, 'empire-route points (display)') for a, n in route_point_blocks(state)]

    def reason(addr):
        for lo, hi, r in spans:
            if lo <= addr < hi:
                return r
        return None
    return reason
