"""Imhotep - name the exported function that contains an address: python3 tools/oracle/where.py ADDR..."""
import bisect
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INDEX = json.load(open(os.path.join(ROOT, 'local', 'ghidra-export', 'index.json')))['functions']
STARTS = sorted((int(f['address'], 16), f['size'], f['name']) for f in INDEX)


def where(addr):
    i = bisect.bisect_right(STARTS, (addr, 1 << 40)) - 1
    if i >= 0 and STARTS[i][0] <= addr < STARTS[i][0] + STARTS[i][1]:
        return '0x%08X %s +0x%X' % (STARTS[i][0], STARTS[i][2], addr - STARTS[i][0])
    return None


def function_start(addr):
    """Start address of the exported function containing addr, or None."""
    i = bisect.bisect_right(STARTS, (addr, 1 << 40)) - 1
    if i >= 0 and STARTS[i][0] <= addr < STARTS[i][0] + STARTS[i][1]:
        return STARTS[i][0]
    return None


if __name__ == '__main__':
    for a in sys.argv[1:]:
        print(a, where(int(a, 16)))
