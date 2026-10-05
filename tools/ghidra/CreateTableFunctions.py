# Ghidra headless script (PyGhidra): create the functions that are reached only through pointer
# tables in data (figure action table 0x005DC28C, ...), which auto-analysis leaves undefined.
#
# Also every target of a direct call (E8 rel32) in .text that lies outside any function, at a
# 16-byte boundary: code that auto-analysis never disassembled.
# Every aligned dword of the initialised data sections whose value points into .text, outside any
# existing function, at a 16-byte boundary (the compiler's function alignment), becomes a function
# entry. Repeated until no new function appears (new code may hold more pointers). Pointers inside
# functions (switch jump tables) are left alone.
#
# Usage (see Makefile `analyze`): -preScript/-postScript CreateTableFunctions.py, before
# ExportFunctions.py.
# @category Imhotep

from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.util.task import ConsoleTaskMonitor

program = currentProgram
monitor = ConsoleTaskMonitor()
memory = program.getMemory()
fm = program.getFunctionManager()
listing = program.getListing()
space = program.getAddressFactory().getDefaultAddressSpace()

text = memory.getBlock(".text")
lo, hi = text.getStart().getOffset(), text.getEnd().getOffset()
data_blocks = [b for b in memory.getBlocks() if b.isInitialized() and not b.isExecute()]

total = 0
for rnd in range(10):
    created = 0
    for block in data_blocks:
        start, end = block.getStart().getOffset(), block.getEnd().getOffset()
        a = (start + 3) & ~3
        while a + 4 <= end + 1:
            try:
                v = memory.getInt(space.getAddress(a)) & 0xFFFFFFFF
            except Exception:
                a += 4
                continue
            if lo <= v <= hi and v % 16 == 0:
                target = space.getAddress(v)
                if fm.getFunctionContaining(target) is None:
                    if listing.getInstructionAt(target) is None:
                        DisassembleCommand(target, None, True).applyTo(program, monitor)
                    if CreateFunctionCmd(target).applyTo(program, monitor):
                        created += 1
            a += 4
    tb = text.getStart()
    n = text.getEnd().getOffset() - lo + 1
    buf = bytearray(n)
    import jpype
    jbuf = jpype.JArray(jpype.JByte)(n)
    memory.getBytes(tb, jbuf)
    buf = bytes((b & 0xFF) for b in jbuf)
    for o in range(n - 5):
        if buf[o] != 0xE8:
            continue
        rel = int.from_bytes(buf[o + 1:o + 5], 'little', signed=True)
        v = (lo + o + 5 + rel) & 0xFFFFFFFF
        if lo <= v <= hi and v % 16 == 0:
            target = space.getAddress(v)
            if fm.getFunctionContaining(target) is None:
                if listing.getInstructionAt(target) is None:
                    DisassembleCommand(target, None, True).applyTo(program, monitor)
                if CreateFunctionCmd(target).applyTo(program, monitor):
                    created += 1
    total += created
    print("CreateTableFunctions: round %d, %d functions created" % (rnd, created))
    if created == 0:
        break
print("CreateTableFunctions: %d functions created in all" % total)
