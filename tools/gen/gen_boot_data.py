#!/usr/bin/env python3
# Imhotep - generate core/boot/boot_data.c from the reference Pharaoh.exe.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""Write the start-up's constant data that the binary keeps as code (package boot).

  local/venv/bin/python tools/gen/gen_boot_data.py   # reads $IMHOTEP_GAME_DIR/Pharaoh.exe

The monument objects' constructors (0x0040D280, 0x00416C20, 0x005430E0, 0x005567D0, 0x005480E0,
0x0054FEF0, 0x00401030, 0x004E6400; 72 KB of code) and the chunk descriptor tables (0x004D1350,
0x004D4A60) store constants: part rectangles (0x004E5120 builds one on the stack, 0x004E5140 copies it
into the part), per-part values, loops over the parts, and one CRT rand() % 5 per part. Their code is run
here by a small interpreter of the instructions they use (values are constants plus a multiple of
`this`, the object's address; loops run; calls are given their meaning below) and every store is
recorded. The output is the objects' final contents relative to `this` (the last store to each byte),
the part rectangles, and the bytes that take rand() % 5, in the order of the rand calls (core/boot/
monuments.c applies them). The sound tables' builders (0x00539D60, 0x0053BCF0, 0x0053AF90, 0x0053B130)
are lists of calls (index, file name, kind): the lists are written out in their order (core/boot/sound.c
makes the calls). The 13-dword copy of 0x004E5140 also copies the 9 dwords of the stack
temporary that 0x004E5120 does not set (stack contents, not reproducible): they are not recorded
(the vector constructor's zeros stay), and WinMain's set-up of each object (0x004E52B0) rewrites them.
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'oracle'))
OUT = ROOT / 'core' / 'boot' / 'boot_data.c'

from disasm import image
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_REG, X86_OP_IMM, X86_OP_MEM
BASE, MEM = image()
md = Cs(CS_ARCH_X86, CS_MODE_32)
md.detail = True
M32 = 0xFFFFFFFF

class Stop(Exception):
    pass

class L:
    """c + k * THIS (k: 0 constant, 1 this-relative); STK: stack-relative (k = 's'); tags: rand"""
    __slots__ = ('c', 'k', 'tag')
    def __init__(s, c, k=0, tag=None):
        s.c, s.k, s.tag = (c & M32) if k == 0 and tag is None else c, k, tag
    def const(s):
        if s.k != 0 or s.tag is not None:
            raise Stop('not a constant: %r' % s)
        return s.c & M32
    def __repr__(s):
        if s.tag: return '%s(%s)' % (s.tag, s.c)
        if s.k == 0: return '%#x' % (s.c & M32)
        if s.k == 's': return 'stk%+#x' % s.c
        return 'this%+#x' % s.c if s.k == 1 else '%d*this%+#x' % (s.k, s.c)

def add(a, b, sign=1):
    if a.tag or b.tag:
        raise Stop('arith on %r %r' % (a, b))
    if a.k == 's' or b.k == 's':
        if b.k == 0: return L(s32(a.c + sign * b.c), 's')
        if a.k == 0 and sign == 1: return L(s32(b.c + a.c), 's')
        if a.k == 's' and b.k == 's' and sign == -1: return L(a.c - b.c)
        raise Stop('stack arith %r %r' % (a, b))
    k = a.k + sign * b.k
    c = a.c + sign * b.c
    return L(c & M32 if k == 0 else s32(c), k)

def s32(v):
    v &= M32
    return v - (1 << 32) if v & 0x80000000 else v

REG32 = ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp')
SUB8 = {'al': ('eax', 0), 'ah': ('eax', 8), 'bl': ('ebx', 0), 'bh': ('ebx', 8), 'cl': ('ecx', 0),
        'ch': ('ecx', 8), 'dl': ('edx', 0), 'dh': ('edx', 8)}
SUB16 = {'ax': 'eax', 'bx': 'ebx', 'cx': 'ecx', 'dx': 'edx', 'si': 'esi', 'di': 'edi', 'bp': 'ebp'}

class Interp:
    def __init__(s, calls, this_reg='ecx', regs=None):
        s.calls = calls
        s.regs = {r: L(0, 0, 'undef') for r in REG32}
        s.regs['esp'] = L(0, 's')
        s.regs[this_reg] = L(0, 1)
        if regs: s.regs.update(regs)
        s.stack = {}
        s.effects = []
        s.flags = None
        s.nrand = 0
        s.steps = 0

    def reg(s, n):
        if n in REG32: return s.regs[n]
        if n in SUB8:
            r, sh = SUB8[n]
            v = s.regs[r]
            if v.tag == 'randmod' and sh == 0: return v
            return L((v.const() >> sh) & 0xFF)
        if n in SUB16: return L(s.regs[SUB16[n]].const() & 0xFFFF)
        raise Stop('reg %s' % n)

    def setreg(s, n, v):
        if n in REG32: s.regs[n] = v; return
        raise Stop('partial register %s' % n)

    def ea(s, ins, op):
        m = op.mem
        if m.segment: return None
        v = L(m.disp)
        if m.base: v = add(s.regs[ins.reg_name(m.base)], v)
        if m.index:
            iv = s.regs[ins.reg_name(m.index)]
            if m.scale == 1: v = add(v, iv)
            else: v = add(v, L(iv.const() * m.scale))
        return v

    def load(s, ins, op):
        if op.type == X86_OP_IMM: return L(op.imm)
        if op.type == X86_OP_REG: return s.reg(ins.reg_name(op.reg))
        a = s.ea(ins, op)
        if a is None: return L(0, 0, 'fs')
        if a.k == 's':
            if a.c in s.stack and s.stack[a.c][0] == op.size: return s.stack[a.c][1]
            return L(0, 0, 'undef')
        if a.k == 0:
            return L(0, 0, 'mem%08X' % a.c)  # a global read: not tracked
        raise Stop('read of the object at %r' % a)

    def store(s, ins, op, v):
        a = s.ea(ins, op)
        if a is None: return
        if a.k == 's':
            for x in range(op.size): s.stack.pop(a.c + x, None)
            s.stack[a.c] = (op.size, v)
            return
        if v.tag not in (None, 'randmod'):
            raise Stop('store of %r at %08X' % (v, ins.address))
        s.effects.append(('w', op.size, a, v, ins.address))

    def push(s, v):
        s.regs['esp'] = add(s.regs['esp'], L(4), -1)
        s.stack[s.regs['esp'].c] = (4, v)

    def pop(s):
        v = s.stack.get(s.regs['esp'].c, (4, L(0, 0, 'undef')))[1]
        s.regs['esp'] = add(s.regs['esp'], L(4))
        return v

    def cond(s, cc):
        kind, a, b = s.flags
        if kind == 'sub':
            x, y = a & M32, b & M32
            r = (x - y) & M32
            sa, sb = s32(x), s32(y)
            res = {'e': x == y, 'ne': x != y, 'l': sa < sb, 'le': sa <= sb, 'g': sa > sb, 'ge': sa >= sb,
                   'b': x < y, 'be': x <= y, 'a': x > y, 'ae': x >= y, 's': bool(r & 0x80000000),
                   'ns': not (r & 0x80000000)}
        else:  # a result: ZF, SF (and OF = 0 for test / and)
            r = a & M32
            res = {'e': r == 0, 'ne': r != 0, 's': bool(r & 0x80000000), 'ns': not r & 0x80000000,
                   'l': bool(r & 0x80000000), 'ge': not r & 0x80000000, 'le': r == 0 or bool(r & 0x80000000),
                   'g': r != 0 and not r & 0x80000000}
        return res[cc]

    def run(s, start, limit=2_000_000):
        addr = start
        while True:
            s.steps += 1
            if s.steps > limit: raise Stop('too many steps')
            ins = next(md.disasm(MEM[addr - BASE:addr - BASE + 16], addr))
            m, ops = ins.mnemonic, ins.operands
            nxt = addr + ins.size
            if m == 'push':
                s.push(s.load(ins, ops[0]))
            elif m == 'pop':
                s.setreg(ins.reg_name(ops[0].reg), s.pop())
            elif m in ('mov', 'movzx', 'movsx'):
                v = s.load(ins, ops[1])
                if ops[0].type == X86_OP_REG:
                    n = ins.reg_name(ops[0].reg)
                    if n not in REG32: raise Stop('mov to %s at %08X' % (n, addr))
                    if m != 'mov' and v.tag is None:
                        c = v.const()
                        if ops[1].size == 1: c = c if m == 'movzx' else s32(c << 24) >> 24
                        if ops[1].size == 2: c = c if m == 'movzx' else s32(c << 16) >> 16
                        v = L(c)
                    s.setreg(n, v)
                else:
                    if ops[0].size < 4 and v.tag is None and v.k == 0: v = L(v.c & ((1 << (8 * ops[0].size)) - 1))
                    s.store(ins, ops[0], v)
            elif m == 'lea':
                s.setreg(ins.reg_name(ops[0].reg), s.ea(ins, ops[1]))
            elif m in ('add', 'sub', 'and', 'or', 'xor', 'cmp', 'test'):
                a, b = s.load(ins, ops[0]), s.load(ins, ops[1])
                same = ops[1].type == X86_OP_REG and ops[0].type == X86_OP_REG and ops[0].reg == ops[1].reg
                if m == 'xor' and same: r = L(0)
                elif m == 'or' and ops[1].type == X86_OP_IMM and (ops[1].imm & M32) == M32: r = L(M32)
                elif m == 'and' and ops[1].type == X86_OP_IMM and ops[1].imm == 0: r = L(0)
                elif m in ('add', 'sub', 'cmp'): r = add(a, b, 1 if m == 'add' else -1)
                else:
                    x, y = a.const(), b.const()
                    r = L({'and': x & y, 'or': x | y, 'xor': x ^ y, 'test': x & y}[m])
                if m in ('cmp', 'sub') and a.tag is None and b.tag is None and r.k == 0:
                    try: s.flags = ('sub', a.const(), b.const())
                    except Stop: s.flags = ('res', r.c, 0)
                elif r.k == 0 and r.tag is None:
                    s.flags = ('res', r.c, 0)
                if m not in ('cmp', 'test'):
                    if ops[0].type == X86_OP_REG: s.setreg(ins.reg_name(ops[0].reg), r)
                    else: s.store(ins, ops[0], r)
            elif m in ('inc', 'dec', 'neg', 'not'):
                v = s.load(ins, ops[0])
                if m == 'inc': r = add(v, L(1))
                elif m == 'dec': r = add(v, L(1), -1)
                elif m == 'neg': r = L(-v.const())
                else: r = L(~v.const())
                if r.k == 0: s.flags = ('res', r.c, 0)
                if ops[0].type == X86_OP_REG: s.setreg(ins.reg_name(ops[0].reg), r)
                else: s.store(ins, ops[0], r)
            elif m in ('shl', 'sar', 'shr', 'imul'):
                if m == 'imul' and len(ops) == 3:
                    r = L(s.load(ins, ops[1]).const() * ops[2].imm)
                else:
                    a, b = s.load(ins, ops[0]).const(), s.load(ins, ops[1]).const() if len(ops) > 1 else 1
                    r = L({'shl': a << b, 'sar': s32(a) >> b, 'shr': a >> b, 'imul': s32(a) * s32(b)}[m])
                s.setreg(ins.reg_name(ops[0].reg), r)
            elif m == 'cdq':
                v = s.regs['eax']
                s.regs['edx'] = L(0, 0, 'randsign') if v.tag == 'rand' else L(M32 if v.const() & 0x80000000 else 0)
            elif m == 'idiv':
                d = s.load(ins, ops[0]).const()
                v = s.regs['eax']
                if v.tag == 'rand':
                    s.regs['edx'] = L(v.c, 0, 'randmod')
                    s.regs['eax'] = L(v.c, 0, 'randdiv')
                    s.effects.append(('randmod', v.c, d, addr))
                else:
                    raise Stop('idiv at %08X' % addr)
            elif m == 'call':
                t = ops[0].imm if ops[0].type == X86_OP_IMM else None
                sem = s.calls.get(t)
                if sem is None: raise Stop('call %s at %08X' % (hex(t) if t else ins.op_str, addr))
                esp = s.regs['esp'].c
                args = [s.stack.get(esp + 4 * k, (4, L(0, 0, 'undef')))[1] for k in range(6)]
                pop, ret = sem(s, args, addr)
                s.regs['esp'] = add(s.regs['esp'], L(4 * pop))
                s.regs['eax'] = ret if ret is not None else L(0, 0, 'undef')
                s.regs['ecx'] = L(0, 0, 'undef')
                s.regs['edx'] = L(0, 0, 'undef')
            elif m == 'ret':
                s.effects.append(('ret', addr))
                return
            elif m == 'rep stosd':
                n, d, v = s.regs['ecx'].const(), s.regs['edi'], s.regs['eax']
                for k in range(n):
                    a = add(d, L(4 * k))
                    if a.k == 's': s.stack[a.c] = (4, v)
                    else: s.effects.append(('w', 4, a, v, addr))
                s.regs['edi'] = add(d, L(4 * n))
                s.regs['ecx'] = L(0)
            elif m == 'jmp':
                nxt = ops[0].imm
            elif m.startswith('j'):
                if s.cond(m[1:]): nxt = ops[0].imm
            elif m.startswith('set'):
                s.setreg(ins.reg_name(ops[0].reg), L(1 if s.cond(m[3:]) else 0))
            elif m == 'nop':
                pass
            else:
                raise Stop('%s %s at %08X' % (m, ins.op_str, addr))
            addr = nxt

# call semantics for the constructors
def c_rect(s, args, at):  # 0x004E5120 thiscall (a, b, c, d) on a stack temporary: its first 4 dwords
    t = s.regs['ecx']
    for k in range(4): s.stack[t.c + 4 * k] = (4, args[k])
    return 4, t
def c_copy(s, args, at):  # 0x004E5140 thiscall (src): 13 dwords copied
    dst, src = s.regs['ecx'], args[0]
    vals = [s.stack.get(src.c + 4 * k, (4, L(0, 0, 'undef')))[1] for k in range(4)]
    s.effects.append(('rect', dst, vals, at))
    return 1, dst
def c_nop(s, args, at): return 0, None
def c_vec(s, args, at):  # 0x00561441 (ptr, size, count, ctor, dtor), stdcall
    s.effects.append(('vec', args[0], args[1].const(), args[2].const(), args[3].const(), at))
    return 5, None
def c_this(name):
    def f(s, args, at):
        s.effects.append(('call', name, s.regs['ecx'], at))
        return 0, s.regs['ecx']
    return f
def c_rand(s, args, at):
    s.effects.append(('rand', s.nrand, at))
    s.nrand += 1
    return 0, L(s.nrand - 1, 0, 'rand')
MON_CALLS = {0x004E5120: c_rect, 0x004E5140: c_copy, 0x0051F780: c_nop, 0x00561441: c_vec,
             0x00545F80: c_this('0x00545F80'), 0x004E5210: c_this('0x004E5210'), 0x00562348: c_rand}

def run_ctor(addr, calls=MON_CALLS, this_reg='ecx'):
    it = Interp(calls, this_reg)
    it.run(addr)
    return it.effects


# the sound tables' builders (0x005394F0): the calls they make, (index, file name, kind)
SOUND_LISTS = [(0x00539D60, 0x00539CF0, 3), (0x0053BCF0, 0x0053BC80, 3), (0x0053AF90, 0x0053AF70, 2),
               (0x0053B130, 0x0053B100, 3)]

MONUMENTS = [0x0040D280, 0x00416C20, 0x005430E0, 0x005567D0, 0x005480E0, 0x0054FEF0, 0x00401030, 0x004E6400]
TABLES = [0x004D1350, 0x004D4A60]


def monument(addr):
    eff = run_ctor(addr)
    img = {}  # offset -> byte or ('rand', k)
    base = None
    nrand = 0
    vec = None
    rects = []
    for e in eff:
        if e[0] == 'call':
            assert base is None and e[2].k == 1 and e[2].c == 0, e
            base = e[1]
        elif e[0] == 'vec':
            ptr, size, count, ctor = e[1], e[2], e[3], e[4]
            assert ctor == 0x004E50F0 and size == 0x34 and ptr.k == 1 and vec is None, e
            vec = (ptr.c, count)
            for o in range(ptr.c, ptr.c + size * count):
                img[o] = 0
        elif e[0] == 'rect':
            d = e[1]
            assert d.k == 1 and vec is not None and d.c == vec[0] + 0x34 * len(rects), e
            rects.append(tuple(s32(v.const()) for v in e[2]))
            for k, v in enumerate(e[2]):
                for b in range(4):
                    img[d.c + 4 * k + b] = (v.const() >> (8 * b)) & 0xFF
        elif e[0] == 'w':
            _, size, a, v, at = e
            assert a.k == 1, e
            if v.tag == 'randmod':
                assert size == 1
                img[a.c] = ('rand', v.c)
            else:
                c = v.const()
                for b in range(size):
                    img[a.c + b] = (c >> (8 * b)) & 0xFF
        elif e[0] == 'rand':
            nrand += 1
        elif e[0] == 'randmod':
            assert e[2] == 5, e
    assert len(rects) == vec[1], (hex(addr), len(rects), vec)
    # the baseline the C code makes before the runs: the parts zeroed, the rectangles
    baseline = {}
    for o in range(vec[0], vec[0] + 0x34 * vec[1]):
        baseline[o] = 0
    for k, r in enumerate(rects):
        for j, v in enumerate(r):
            for b in range(4):
                baseline[vec[0] + 0x34 * k + 4 * j + b] = (v >> (8 * b)) & 0xFF
    rand = sorted((v[1], o) for o, v in img.items() if isinstance(v, tuple))
    assert [k for k, _ in rand] == list(range(len(rand))), 'a rand byte overwritten'
    runs = []
    offs = sorted(o for o, v in img.items() if not isinstance(v, tuple) and baseline.get(o) != v)
    i = 0
    while i < len(offs):
        o = offs[i]
        if o % 4 == 0 and all(o + b in img and not isinstance(img[o + b], tuple) for b in range(4)):
            val = sum(img[o + b] << (8 * b) for b in range(4))
            n = 1
            while all((o + 4 * n + b) in img and img[o + 4 * n + b] == ((val >> (8 * b)) & 0xFF) for b in range(4)):
                n += 1
            runs.append((o, n, 4, val))
            nxt = o + 4 * n
        else:
            runs.append((o, 1, 1, img[o]))
            nxt = o + 1
        while i < len(offs) and offs[i] < nxt:
            i += 1
    return {'base': base, 'vec': vec, 'rects': rects, 'runs': runs, 'rand': [o for _, o in rand], 'nrand': nrand}


def table_stores(addr):
    out = []
    for e in run_ctor(addr):
        if e[0] == 'w':
            _, size, a, v, at = e
            assert a.k == 0
            out.append((size, a.c, v.const()))
    return out


def sound_calls(addr, callee, nargs):
    """the arguments of the builder's calls to callee, in order"""
    out = []

    def rec(it, args, at):
        out.append(tuple(a.const() for a in args[:nargs]))
        return 0, None
    it = Interp({callee: rec})
    it.run(addr)
    return out


def main():
    lines = ['/* Generated by tools/gen/gen_boot_data.py from the reference Pharaoh.exe (GOG 2.1,',
             ' * docs/source-fingerprint.json): constant data the start-up keeps as code. Do not',
             ' * edit. Licensed under the GNU AGPL-3.0. */',
             '#include "boot/monuments.h"', '']
    for addr in TABLES:
        lines.append('/* @bin 0x%08X */' % addr)
        lines.append('void boot_ctor_%x(game_state *st)' % addr)
        lines.append('{')
        for size, a, v in table_stores(addr):
            lines.append('    %s(st, 0x%08Xu) = 0x%Xu;' % ({4: 'U32', 2: 'U16', 1: 'U8'}[size], a, v))
        lines.append('}')
        lines.append('')
    defs = []
    for addr in MONUMENTS:
        m = monument(addr)
        n = '%x' % addr
        lines.append('/* @bin 0x%08X the part rectangles (x, y, w, h), parts from +0x%X */' % (addr, m['vec'][0]))
        lines.append('static const int16_t rects_%s[%d][4] = {' % (n, len(m['rects'])))
        for r in m['rects']:
            assert all(-32768 <= v < 32768 for v in r)
        row = []
        for r in m['rects']:
            row.append('{%d, %d, %d, %d}' % r)
        for k in range(0, len(row), 8):
            lines.append('    ' + ', '.join(row[k:k + 8]) + ',')
        lines.append('};')
        lines.append('/* @bin 0x%08X the stores (offset, count, size, value) */' % addr)
        lines.append('static const monument_run runs_%s[%d] = {' % (n, len(m['runs'])))
        row = ['{0x%X, %d, %d, 0x%X}' % r for r in m['runs']]
        for k in range(0, len(row), 4):
            lines.append('    ' + ', '.join(row[k:k + 4]) + ',')
        lines.append('};')
        if m['rand']:
            lines.append('/* @bin 0x%08X the bytes of rand() %% 5, in the order of the calls */' % addr)
            lines.append('static const uint16_t rand_%s[%d] = {' % (n, len(m['rand'])))
            row = ['0x%X' % o for o in m['rand']]
            for k in range(0, len(row), 12):
                lines.append('    ' + ', '.join(row[k:k + 12]) + ',')
            lines.append('};')
        lines.append('')
        defs.append('    {0x%08Xu, %su, 0x%Xu, %d, rects_%s, runs_%s, %d, %s, %d},' % (
            addr, '0x' + m['base'][2:].upper() if m['base'] else '0', m['vec'][0], m['vec'][1], n, n,
            len(m['runs']), 'rand_%s' % n if m['rand'] else 'NULL', m['nrand']))
    for addr, callee, nargs in SOUND_LISTS:
        calls = sound_calls(addr, callee, nargs)
        lines.append('/* @bin 0x%08X its calls of 0x%08X (index, the file name\'s address%s), in order */' % (
            addr, callee, ', kind' if nargs == 3 else ''))
        lines.append('const sound_entry sound_list_%x[%d] = {' % (addr, len(calls)))
        row = ['{0x%X, 0x%08Xu, %d}' % (c[0], c[1], c[2] if nargs == 3 else 0) for c in calls]
        for k in range(0, len(row), 4):
            lines.append('    ' + ', '.join(row[k:k + 4]) + ',')
        lines.append('};')
        lines.append('const int sound_list_%x_count = %d;' % (addr, len(calls)))
        lines.append('')
    lines.append('const monument_def monument_defs[%d] = {' % len(MONUMENTS))
    lines += defs
    lines.append('};')
    OUT.write_text('\n'.join(lines) + '\n')
    print('wrote %s' % OUT)


if __name__ == '__main__':
    main()
