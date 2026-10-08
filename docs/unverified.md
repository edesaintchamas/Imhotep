# Unverified rules and open questions

One entry per `@unverified` tag in `core/`. Remove the entry when the rule is confirmed.

| ID | Location | Question | Leads |
|---|---|---|---|
| CURSOR-WINDOWED | core/frame/input.c (GetCursorPos in 0x004D6B60, 0x00414CD0) | without a platform cursor, the cursor is taken as the last mouse message's point (0x00E38E0A / 0x00E38E08). In the fullscreen game GetCursorPos returns the same point; in a windowed game it is in screen coordinates. The SDL platform gives the cursor (platform_cursor_pos) | how the original behaves windowed (it offers windowed mode: 0x00E38E5C) |

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
| UNINIT-40A450 | core/command/military.c (0x0040A450) | dispatching troops: when a request row has no event, `0x0044D520` leaves its output slot unset; for the first row that slot was never written (stack leftover). The port uses 0; the command tests stay equal | find what the caller's frame held there in play (the dispatcher's previous calls) |
| UNINIT-406300 | core/command/orders_storage.c (0x00406300) | a storage order on a row past the bazaar's list reads an unset stack slot; the port stops (`state_unported`) and changes nothing | the UI offers no such row |
| UNINIT-4405C0 | core/command/place_ent.c (inline 0x004405C0) | when the venue orientation is not 0/1 or the view not 0/2/4/6, the original reads unset outputs; the port stops (`state_unported`) | not reachable from the menus |
| UNINIT-41AE40 | core/newgame/flow.c (0x0041AE40) | the campaign.txt parser leaves bytes +2..+6, +0x2B, +0x37 and +0x3D..+0x3F of each campaign step unset; they come from its stack frame and reach the progress file (0x0041B8A0). The port writes 0; the tests clear the emulator's stack first | what the boot's previous calls leave in that frame |
| UNINIT-41B680 | core/newgame/flow2.c (0x0041B680) | reading a progress file of the older size (0x4A6C bytes), the original copies 24 bytes of its own stack frame into the table (+0x4A6C..+0x4A84); the port writes 0. The tests clear the stack first | what the callers' frames leave there; only old progress files reach it |
| UNINIT-417B40 | present/ui/cityframe.c (0x00417B40, tutorial message 0x83) | the 30-byte message block the original passes is a buffer of its own frame, 24 bytes of it never written: the port stops there (`state_unported`) and the rest of the function is ported | what the frame holds there in play; only the tutorial mission reaches it |
| CRT-REMOVE-ERRNO | present/ui/frontend/files.c (0x00562BB5 remove) | a failed platform delete is mapped to Win32 error 2 (file not found): the platform does not give the error code | add an error code to platform_file_delete if a caller tells errors apart |
