#!/usr/bin/env python3
# Imhotep - generate core/io/save_relayout.c from the disassembly of Pharaoh.exe's relayout code.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""Derive the version-124 record relayouts of the saved-game loader as byte-move tables.

  python3 tools/gen/gen_relayouts.py     # needs local/ghidra-export/asm/0x004D18F0.asm etc.
                                         # (tools/ghidra/DumpAsm.py, see Makefile `analyze`)

The four relayout functions (docs/formats/save.md §4) build each record in a stack buffer with
MOV / REP MOVSD / REP STOSD and copy it back. Translating 1,100 such instructions by hand is
error-prone, so this script runs them in a small symbolic x86 interpreter: every byte of the
record starts as "old byte k", every store moves provenance, and the record bytes after the
copy-back give the table "new byte i = old byte src(i), or 0". Branches on the building type
(0x004D2FA0) are taken with a concrete type per case. Only the instructions these functions use
are implemented; anything else stops the script.
"""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASM = ROOT / "local" / "ghidra-export" / "asm"
OUT = ROOT / "core" / "io" / "save_relayout.c"

REG32 = ["EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"]
SUB = {"AX": ("EAX", 0, 2), "AL": ("EAX", 0, 1), "AH": ("EAX", 1, 1), "CX": ("ECX", 0, 2),
       "CL": ("ECX", 0, 1), "CH": ("ECX", 1, 1), "DX": ("EDX", 0, 2), "DL": ("EDX", 0, 1),
       "BX": ("EBX", 0, 2), "BL": ("EBX", 0, 1), "SI": ("ESI", 0, 2), "DI": ("EDI", 0, 2)}
SIZE = {"byte": 1, "word": 2, "dword": 4}
STACK_TOP = 0x40000000


def const(v, n=4):
    return [("c", (v >> (8 * i)) & 0xFF) for i in range(n)]


def concrete(bs):
    """Numeric value of a byte list: constants, or record bytes given a known value (branch input)."""
    vals = []
    for b in bs:
        if b[0] == "c":
            vals.append(b[1])
        elif b[0] == "old" and len(b) == 3:
            vals.append(b[2])
        else:
            raise ValueError(f"symbolic value used as a number: {bs}")
    return sum(v << (8 * i) for i, v in enumerate(vals))


class Machine:
    def __init__(self, record_base, record_size, known):
        self.reg = {r: const(0) for r in REG32}
        self.reg["ESP"] = const(STACK_TOP)
        self.mem = {}
        self.base, self.size = record_base, record_size
        self.known = known                 # record offset -> concrete byte (branch inputs)
        self.flags = None

    def read(self, addr, n):
        out = []
        for a in range(addr, addr + n):
            if a in self.mem:
                out.append(self.mem[a])
            elif self.base <= a < self.base + self.size:
                k = a - self.base
                out.append(("old", k, self.known[k]) if k in self.known else ("old", k))
            else:
                out.append(("garbage", a))
        return out

    def write(self, addr, bs):
        for i, b in enumerate(bs):
            self.mem[addr + i] = b

    def getreg(self, name):
        if name in REG32:
            return list(self.reg[name])
        r, off, n = SUB[name]
        return self.reg[r][off:off + n]

    def setreg(self, name, bs):
        if name in REG32:
            self.reg[name] = list(bs)
        else:
            r, off, n = SUB[name]
            self.reg[r][off:off + n] = list(bs)

    def addr(self, text):
        m = re.fullmatch(r"\[(\w+)(?: \+ (-?0x[0-9a-f]+))?\]", text)
        if m:
            return concrete(self.getreg(m.group(1))) + (int(m.group(2), 16) if m.group(2) else 0) & 0xFFFFFFFF
        m = re.fullmatch(r"\[(0x[0-9a-f]+)\]", text)
        if m:
            return int(m.group(1), 16)
        raise ValueError(text)

    def operand(self, text, n=4):
        m = re.fullmatch(r"(byte|word|dword) ptr (\[.*\])", text)
        if m:
            return self.read(self.addr(m.group(2)), SIZE[m.group(1)])
        if text in REG32 or text in SUB:
            return self.getreg(text)
        return const(int(text, 16) & 0xFFFFFFFF, n)

    def run(self, lines, start, stop_at):
        pc = {int(l.split()[0], 16): i for i, l in enumerate(lines)}
        i = pc[start]
        while True:
            addr_s, ins = lines[i].split(None, 1)
            a = int(addr_s, 16)
            if a in stop_at:
                return
            op, _, args = ins.partition(" ")
            args = [x.strip() for x in re.split(r",(?![^\[]*\])", args)] if args else []
            nxt = i + 1
            if op == "MOV":
                dst, src = args
                if "ptr" in dst:
                    m = re.fullmatch(r"(byte|word|dword) ptr (\[.*\])", dst)
                    n = SIZE[m.group(1)]
                    self.write(self.addr(m.group(2)), self.operand(src, n)[:n])
                elif src.startswith("["):              # MOV EAX,[0x...]
                    self.setreg(dst, self.read(self.addr(src), 4))
                else:
                    n = 4 if dst in REG32 else SUB[dst][2]
                    self.setreg(dst, self.operand(src, n)[:n])
            elif op == "MOVSX":
                dst, src = args
                v = self.operand(src)
                val = concrete(v)
                bits = 8 * len(v)
                if val & (1 << (bits - 1)):
                    val -= 1 << bits
                self.setreg(dst, const(val & 0xFFFFFFFF))
            elif op == "LEA":
                self.setreg(args[0], const(self.addr(args[1])))
            elif op == "XOR" and args[0] == args[1]:
                n = 4 if args[0] in REG32 else SUB[args[0]][2]
                self.setreg(args[0], const(0, n))
            elif op in ("ADD", "SUB"):
                v = concrete(self.getreg(args[0]))
                w = int(args[1], 16)
                self.setreg(args[0], const((v + w if op == "ADD" else v - w) & 0xFFFFFFFF))
            elif op == "IMUL":
                self.setreg(args[0], const(concrete(self.getreg(args[1])) * int(args[2], 16) & 0xFFFFFFFF))
            elif op == "INC":
                self.setreg(args[0], const(concrete(self.getreg(args[0])) + 1 & 0xFFFFFFFF))
            elif op == "PUSH":
                sp = concrete(self.reg["ESP"]) - 4
                self.reg["ESP"] = const(sp)
                self.write(sp, self.getreg(args[0]))
            elif op == "POP":
                sp = concrete(self.reg["ESP"])
                self.setreg(args[0], self.read(sp, 4))
                self.reg["ESP"] = const(sp + 4)
            elif op == "CALL" and args[0] == "0x00561f30":   # __chkstk: ESP -= EAX
                self.reg["ESP"] = const(concrete(self.reg["ESP"]) - concrete(self.reg["EAX"]))
            elif op in ("MOVSD.REP", "STOSD.REP", "MOVSW", "STOSW"):
                n = 4 if "D" in op[4] else 2
                count = concrete(self.reg["ECX"]) if op.endswith(".REP") else 1
                for _ in range(count):
                    di = concrete(self.reg["EDI"])
                    if op.startswith("MOVS"):
                        si = concrete(self.reg["ESI"])
                        self.write(di, self.read(si, n))
                        self.reg["ESI"] = const(si + n)
                    else:
                        self.write(di, self.getreg("EAX")[:n])
                    self.reg["EDI"] = const(di + n)
                if op.endswith(".REP"):
                    self.reg["ECX"] = const(0)
            elif op == "CMP":
                x = concrete(self.operand(args[0]))
                n = 4 if args[0] in REG32 else (SUB[args[0]][2] if args[0] in SUB else 4)
                y = int(args[1], 16) & ((1 << (8 * n)) - 1)
                sx = x - (1 << (8 * n)) if x & (1 << (8 * n - 1)) else x
                sy = y - (1 << (8 * n)) if y & (1 << (8 * n - 1)) else y
                self.flags = (sx, sy, x == y)
            elif op in ("JL", "JG", "JZ", "JMP"):
                sx, sy, eq = self.flags if op != "JMP" else (0, 0, True)
                take = op == "JMP" or (op == "JL" and sx < sy) or (op == "JG" and sx > sy) or (op == "JZ" and eq)
                if take:
                    nxt = pc[int(args[0], 16)]
            else:
                raise ValueError(f"unsupported instruction at {addr_s}: {ins}")
            i = nxt


def record_program(func, record_base, size, start, stop, known=None):
    lines = [l for l in (ASM / f"0x{func:08X}.asm").read_text().splitlines() if l.strip()]
    m = Machine(record_base, size, known or {})
    m.run(lines, start, stop)
    prog = []
    for k in range(size):
        b = m.mem.get(record_base + k, ("old", k))
        if b[0] == "old":
            prog.append(b[1])
        elif b == ("c", 0):
            prog.append(-1)
        else:
            raise ValueError(f"0x{func:08X}: record byte {k:#x} = {b}")
    return prog


def runs(prog):
    out, k = [], 0
    while k < len(prog):
        j = k
        if prog[k] == -1:
            while j < len(prog) and prog[j] == -1:
                j += 1
            out.append((k, -1, j - k))
        else:
            while j < len(prog) and prog[j] == prog[k] + (j - k):
                j += 1
            out.append((k, prog[k], j - k))
        k = j
    return out


def main() -> None:
    progs = {
        # (function, record base, record size, first instruction, stop before, known bytes)
        "relayout_chunk50": record_program(0x004D18F0, 0x0077DE00, 0x6A, 0x004D18F0, {0x004D19CF}),
        "relayout_chunk113": record_program(0x004D3200, 0x007900B8, 0xC4, 0x004D3200, {0x004D328E}),
        "relayout_city": record_program(0x004D1AD0, 0x00EA1870, 0x49D8, 0x004D1AD0, {0x004D2F8E}),
    }
    for name, t in (("house", 10), ("pyramid_sphinx", 0xB7), ("other", 1)):
        progs[f"relayout_building_{name}"] = record_program(
            0x004D2FA0, 0x007999D8, 0x108, 0x004D2FA0, {0x004D31E1}, {0x10: t & 0xFF, 0x11: t >> 8})
    lines = [
        "/* Generated by tools/gen/gen_relayouts.py from the disassembly of the reference Pharaoh.exe",
        " * (GOG 2.1). Do not edit. Licensed under the GNU AGPL-3.0. */",
        '#include "io/save_relayout.h"',
        "",
    ]
    funcs = {"relayout_chunk50": "0x004D18F0", "relayout_chunk113": "0x004D3200",
             "relayout_city": "0x004D1AD0"}
    for name, prog in progs.items():
        rr = runs(prog)
        src = funcs.get(name, "0x004D2FA0")
        lines.append(f"/* @bin {src} new record bytes [dst, dst+len) = old [src, src+len), or 0 if src < 0 */")
        lines.append(f"const save_relayout_run save_{name}[] = {{")
        for d, s, n in rr:
            lines.append(f"    {{0x{d:04X}, {s if s < 0 else f'0x{s:04X}'}, {n}}},")
        lines.append("};")
        lines.append(f"const int save_{name}_count = {len(rr)};")
        lines.append("")
    OUT.write_text("\n".join(lines))
    print(f"wrote {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
