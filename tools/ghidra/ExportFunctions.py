# Ghidra headless script (PyGhidra, Python 3; Ghidra 12 no longer ships Jython): export every
# function of the current program as one JSON file: address, name, signature, callers, callees,
# decompiled C.
#
# Usage (see Makefile `analyze`):
#   pyghidraRun -H <projdir> <projname> -import Pharaoh.exe \
#     -postScript ExportFunctions.py <outdir> -scriptPath tools/ghidra
#
# Output goes to local/ghidra-export/ and must never be committed (it contains decompiled code).
# @category Imhotep

import json
import os
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

args = getScriptArgs()
out_dir = args[0] if args else "local/ghidra-export"
func_dir = os.path.join(out_dir, "functions")
if not os.path.isdir(func_dir):
    os.makedirs(func_dir)

program = currentProgram
monitor = ConsoleTaskMonitor()
decomp = DecompInterface()
decomp.openProgram(program)

index = []
fm = program.getFunctionManager()
for func in fm.getFunctions(True):
    if monitor.isCancelled():
        break
    addr = "0x%08X" % func.getEntryPoint().getOffset()
    res = decomp.decompileFunction(func, 60, monitor)
    c_code = res.getDecompiledFunction().getC() if res and res.decompileCompleted() else None
    entry = {
        "address": addr,
        "name": func.getName(),
        "signature": func.getPrototypeString(False, False),
        "size": func.getBody().getNumAddresses(),
        "callers": sorted(["0x%08X" % f.getEntryPoint().getOffset() for f in func.getCallingFunctions(monitor)]),
        "callees": sorted(["0x%08X" % f.getEntryPoint().getOffset() for f in func.getCalledFunctions(monitor)]),
        "decompiled": c_code,
    }
    with open(os.path.join(func_dir, addr + ".json"), "w") as f:
        json.dump(entry, f, indent=1)
    index.append({k: entry[k] for k in ("address", "name", "size")})

with open(os.path.join(out_dir, "index.json"), "w") as f:
    json.dump({"program": program.getName(), "functions": index}, f, indent=1)

print("Exported %d functions to %s" % (len(index), out_dir))
