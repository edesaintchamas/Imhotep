# Unverified rules and open questions

One entry per `@unverified` tag in `core/`. Remove the entry when the rule is confirmed.

| ID | Location | Question | Leads |
|---|---|---|---|
| _(none yet)_ | | | |

## Open questions on the method (not tied to a tag)

| ID | Location | Question | Leads |
|---|---|---|---|
| X87-PC | core/city/month_mood.c (0x004E81D0, 0x004E7F00) | x87 results reproduced with integers (`score * 0.01f * pop` truncated; `fsqrt` truncated) are exact at 53-bit precision, which the game sets (`0x005640EC`) and the oracle sets at boot. Does Unicorn's x87 honour the precision-control bits, so that the oracle's results are the real game's? | compare a 24-bit-sensitive computation in the emulator with a native run |
| LOOP-4003 | 0x004E8DB0, 0x004BC7A0, 0x004E8E80, 0x004E81D0, 0x004E8840, 0x00451B10 | loops over buildings end at an address past the 4000 records (ids up to 4003), reading the next variables as buildings; ported literally | none needed unless those bytes can hold building-like values |
| UNINIT-462180 | core/building/building_update.c (0x00462180) | two locals (`local_30`, `local_20` in Ghidra's naming) are read before being set in the original (stack leftovers); the port uses 0. No corpus state shows a difference | find the paths that read them before setting; the values depend on the previous calls' stack frames |
| OOB-READS | 0x0044BB40, 0x0045B880 | out-of-range indexes read absolute address 2 / past a 36-entry stack array in the original; the port stops (`state_unported`) there | not reached on the corpus |
| BOAT-FLEE | core/figure/actions_d4.c (0x004ADD90, 0x004AE490) | when no line search finds water, the original starts the nearest-free-water search (0x004A4850) from its own argument slots, i.e. a stack address: not reproducible; the port starts from (0, id) / (id, 0) | not reached on the corpus |
| FDIV-FLAGS | core/sim/x87_angle.c (0x0051CAE0) | the original reads the FPU status (`fnstsw`) after an `fdiv` that follows the `fcomp` it means to test; Intel leaves C0/C2/C3 undefined after `fdiv`. The port assumes they keep the compare's result (as the emulator does, and as the code intends) | a real x87 run of the function with dx <= 0 |
| ORACLE-FPATAN | tools/oracle (Unicorn) | the emulator's `fpatan` uses the host's double `atan2` (not correctly rounded, low bits zero), unlike a real x87's 64-bit result; on 0x0051CAE0's whole domain both give the same final value (checked exhaustively), but a future port of another transcendental must check this again | |
