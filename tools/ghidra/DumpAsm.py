# Ghidra headless script (PyGhidra): dump the disassembly of the functions at the given addresses.
#
# Usage (existing project, no re-analysis):
#   pyghidraRun -H local/ghidra pharaoh -process Pharaoh.exe -noanalysis -readOnly \
#     -scriptPath tools/ghidra -postScript DumpAsm.py local/ghidra-export/asm 0x004767D0 ...
#
# Arguments may also be ranges START-END (e.g. 0x004D1330-0x004D17E0), dumped instruction by
# instruction (disassembled in memory first if needed; -readOnly keeps the project unchanged).
# Output: <outdir>/<address>.asm (one instruction per line). Contains game code: local/ only.
# @category Imhotep

import os

args = getScriptArgs()
out_dir = args[0]
if not os.path.isdir(out_dir):
    os.makedirs(out_dir)

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.program.model.address import AddressSet

for a in args[1:]:
    if "-" in a:   # address range START-END: disassemble if needed, dump every instruction
        lo, hi = [toAddr(int(x, 16)) for x in a.split("-")]
        DisassembleCommand(lo, AddressSet(lo, hi), True).applyTo(currentProgram)
        lines = ["%s  %s" % (i.getAddress(), i) for i in listing.getInstructions(AddressSet(lo, hi), True)]
        path = os.path.join(out_dir, "%s.asm" % a.replace("0x", "").upper())
        with open(path, "w") as f:
            f.write("\n".join(lines) + "\n")
        print("wrote %s (%d instructions)" % (path, len(lines)))
        continue
    addr = toAddr(int(a, 16))
    func = fm.getFunctionContaining(addr)
    if func is None:
        print("no function at %s" % a)
        continue
    lines = []
    for ins in listing.getInstructions(func.getBody(), True):
        lines.append("%s  %s" % (ins.getAddress(), ins))
    path = os.path.join(out_dir, "0x%08X.asm" % func.getEntryPoint().getOffset())
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print("wrote %s (%d instructions)" % (path, len(lines)))
