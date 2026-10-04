# Imhotep - PKWARE Data Compression Library "explode" (binary mode), pure Python.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
# @from julius:src/core/zip.c (pk_explode and its tables; AGPL-3.0, Bianca van Schaik and
# contributors). Restructured as a straightforward bit reader; the tables are copied verbatim.
"""Decompress PKWARE DCL "implode" streams, as used by Impressions games for compressed chunks.

Only binary-literal mode is supported (header byte 0 == 0), like Julius.
"""

# @from julius:src/core/zip.c pk_copy_offset_bits / pk_copy_offset_code
OFFSET_BITS = [
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
]
OFFSET_CODE = [
    0x03, 0x0D, 0x05, 0x19, 0x09, 0x11, 0x01, 0x3E, 0x1E, 0x2E, 0x0E, 0x36, 0x16, 0x26, 0x06, 0x3A,
    0x1A, 0x2A, 0x0A, 0x32, 0x12, 0x22, 0x42, 0x02, 0x7C, 0x3C, 0x5C, 0x1C, 0x6C, 0x2C, 0x4C, 0x0C,
    0x74, 0x34, 0x54, 0x14, 0x64, 0x24, 0x44, 0x04, 0x78, 0x38, 0x58, 0x18, 0x68, 0x28, 0x48, 0x08,
    0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10, 0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00,
]
# @from julius:src/core/zip.c pk_copy_length_base_* / pk_copy_length_extra_bits
LENGTH_BITS = [3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7]
LENGTH_BASE = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
               0x08, 0x0A, 0x0E, 0x16, 0x26, 0x46, 0x86, 0x106]
LENGTH_CODE = [0x05, 0x03, 0x01, 0x06, 0x0A, 0x02, 0x0C, 0x14,
               0x04, 0x18, 0x08, 0x30, 0x10, 0x20, 0x40, 0x00]
LENGTH_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]

END_OF_STREAM = 519  # length symbol 0x107 + 2 - 2; Julius' PK_EOF (773) minus 254


class PkError(Exception):
    pass


def _jump_table(bits, codes):
    # @from julius:src/core/zip.c pk_explode_construct_jump_table
    jump = [0] * 256
    for i in range(len(bits) - 1, -1, -1):
        code = codes[i]
        while code < 0x100:
            jump[code] = i
            code += 1 << bits[i]
    return jump


_LENGTH_JUMP = _jump_table(LENGTH_BITS, LENGTH_CODE)
_OFFSET_JUMP = _jump_table(OFFSET_BITS, OFFSET_CODE)


class _Bits:
    """LSB-first bit reader."""

    def __init__(self, data: bytes, pos: int):
        self.data, self.pos, self.acc, self.n = data, pos, 0, 0

    def peek(self, k: int) -> int:
        while self.n < k:
            b = self.data[self.pos] if self.pos < len(self.data) else 0
            self.pos += 1
            self.acc |= b << self.n
            self.n += 8
        return self.acc & ((1 << k) - 1)

    def take(self, k: int) -> int:
        v = self.peek(k)
        self.acc >>= k
        self.n -= k
        return v

    def overrun(self) -> bool:
        return self.pos - self.n // 8 > len(self.data)


def explode(data: bytes, expected_size: int | None = None) -> bytes:
    if len(data) < 4:
        raise PkError("too few input bytes")
    literal_mode, window = data[0], data[1]
    if literal_mode != 0:
        raise PkError("coded-literal mode not supported")
    if not 4 <= window <= 6:
        raise PkError(f"invalid window size {window}")
    bits = _Bits(data, 2)
    out = bytearray()
    while True:
        if bits.overrun():
            raise PkError("input exhausted before end-of-stream marker")
        if bits.take(1):
            # copy: length symbol, then offset
            idx = _LENGTH_JUMP[bits.peek(8)]
            bits.take(LENGTH_BITS[idx])
            length = LENGTH_BASE[idx] + (bits.take(LENGTH_EXTRA[idx]) if LENGTH_EXTRA[idx] else 0) + 2
            if length == END_OF_STREAM:
                break
            oidx = _OFFSET_JUMP[bits.peek(8)]
            bits.take(OFFSET_BITS[oidx])
            low_bits = 2 if length == 2 else window
            offset = ((oidx << low_bits) | bits.take(low_bits)) + 1
            if offset > len(out):
                raise PkError(f"copy offset {offset} before start of output ({len(out)})")
            start = len(out) - offset
            for k in range(length):          # byte by byte: source may overlap destination
                out.append(out[start + k])
        else:
            out.append(bits.take(8))
    if bits.overrun():
        raise PkError("input exhausted")
    if expected_size is not None and len(out) != expected_size:
        raise PkError(f"decompressed {len(out)} bytes, expected {expected_size}")
    return bytes(out)
