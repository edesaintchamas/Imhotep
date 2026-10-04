"""Imhotep - disassemble Pharaoh.exe: python tools/oracle/disasm.py START [COUNT] (needs capstone in local/venv)."""
import os
import struct
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME_DIR = os.environ.get('IMHOTEP_GAME_DIR', os.path.join(ROOT, 'local', 'game', 'app'))


def image():
    data = open(os.path.join(GAME_DIR, 'Pharaoh.exe'), 'rb').read()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    nsec = struct.unpack_from('<H', data, pe + 6)[0]
    opt = struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
    mem = bytearray(struct.unpack_from('<I', data, pe + 24 + 56)[0])
    sec = pe + 24 + opt
    for i in range(nsec):
        _, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', data, sec + 40 * i)
        n = min(rsize, vsize or rsize)
        mem[va:va + n] = data[raw:raw + n]
    return base, bytes(mem)


def disasm(addr, count=40):
    base, mem = image()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    out = []
    for ins in md.disasm(mem[addr - base:addr - base + 16 * count], addr):
        out.append('%08X  %-8s %s' % (ins.address, ins.mnemonic, ins.op_str))
        if len(out) >= count:
            break
    return out


if __name__ == '__main__':
    print('\n'.join(disasm(int(sys.argv[1], 16), int(sys.argv[2]) if len(sys.argv) > 2 else 40)))
