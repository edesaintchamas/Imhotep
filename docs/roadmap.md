# Roadmap

How Imhotep gets from "reads saves, recomputes desirability" to "simulates like Pharaoh.exe". Each
phase ends with a test that passes; the next phase builds on it. Status marks: `[x]` done, `[~]` in
progress, `[ ]` open.

## Method

The reference is `Pharaoh.exe` (GOG 2.1) itself. It is not run as a program (no Wine). Its code
runs **headless in a CPU emulator** (`tools/oracle/`, Unicorn): the game is booted the way WinMain
does it, minus DirectDraw, sound, video and the message loop; it loads saves with its own loader,
runs its own tick function, and writes saves with its own saver. That gives:

- **reference runs**: any saved state, advanced N ticks / days / months by the original code;
- **function oracle**: any single game function run on any state, its writes compared with the
  port's;
- **a reference save corpus**, written by the game's saver (`tools/oracle/corpus.py`).

Every rule is still ported from the disassembly / decompilation with an `@bin` tag; the oracle only
confirms (`@diff`). Display-only functions replaced by no-ops in the oracle are listed in
`tools/oracle/game.py` (`PRESENTATION`), each checked to write no simulation state.

Scale: the game tick `0x004D7090` reaches about 1,185 game functions (463 KB of code), part of it UI
and messages. Porting goes bottom-up, leaf functions first, each checked against the oracle.

## Phase 1: the oracle

- [x] 1.1 Emulator: Pharaoh.exe mapped at its image base, imports bound to stubs (unknown ones stop
      the run with their name), SEH via a TIB, CRT file / heap functions replaced.
- [x] 1.2 Boot: C++ static constructors, then WinMain's initialisation in order (`Game.boot`).
- [x] 1.3 Load (`0x004D5100`), tick (`0x004D7090`), save (`0x004D4E20`) work on every player save.
      Check: desirability after a reference tick 37 equals the stored grid and the port (Iken,
      sais 6374: 51,984 / 51,984 tiles, three ways).
- [x] 1.4 Reference save corpus: the 7 player cities and campaign record 0 advanced 12 months, saved
      by the game at each month start (`make corpus`, 104 files in `local/saves/generated/`).
      On all of them: desirability exact (5,406,336 tiles), save_check 0 failures.
- [x] 1.5 State diff tool (`tools/oracle/diff.py`): two states compared chunk by chunk, record
      fields named from save.md ("building 412 +0x18 (service counter)").
- [x] 1.6 Loader cross-check (`oracle_load_check`): all 181 saved variables equal to the original
      loader's on 53 campaign records (version upgrades) and 111 player / corpus saves.
- [x] 1.7 Function harness (`tools/oracle/harness.py`): original functions and their ports
      (`core/state/ported.c`, `build/state_run`) run on the same oracle state; every byte of the
      state (data segments and simulation heap) and every return value is compared. `--fill`
      scrambles outputs first. Also: `attribute.py` (which call wrote what), `readset.py` (pages
      the tick reads), `watch.py` (who writes an address).

## Phase 2: game state in core

- [x] 2.1 `core/state/`: the state is a byte-exact mirror of the exe's data segments plus a
      simulation heap at fixed addresses (`docs/formats/state.md`). Desirability runs on it.
- [x] 2.2 Load into the state (`game_load_save`, port of `0x004D5100`): pre-load clear, loader
      (shipwright queues, route buffers in the heap arena), models, terrain image rebuild and
      floodplain build on the state with the real heap objects, routing grids, goods lists,
      post-load links. Check: `oracle_state_check` (the whole state after our load against the
      original's, from the same booted state). The scenario-start steps still marked `not_ported`
      are tick functions and move to phase 4: road access `0x00457AF0`, building images
      `0x00485F70`, `0x00468860`, industry counts `0x004677C0`, `0x00468170`, storage summary
      `0x0045E040`, the type-0x88 route search (`0x0043F060`, `0x0051DE70`, `0x0051E720`), and
      small ones; display steps (`0x00430530`, `0x004BC720`, `0x004F4520`, `0x0053D970`,
      `0x004E59A0`, sounds) stay out (docs/formats/state.md).
- [x] 2.3 Save writer (`game_save`, core/state/save_game.c, port of `0x004D4E20`; PKWARE implode
      core/io/pkware_implode.c). Check: `oracle_save_check`, 15 / 15 files byte-identical to the
      original saver's from the same state.

## Phase 3: the tick skeleton

- [x] 3.1 `core/sim/tick.c`: the tick `0x004D7090`, the day dispatcher `0x00451C50`, the cascade
      `0x00452370` (docs/formats/tick.md). Every callee goes through `sim_call`: the port when
      there is one, else the environment, else recorded as unported.
- [x] 3.2 Lockstep (`tools/oracle/lockstep.py`, `build/state_tick`): the original records what
      each skeleton call changed; our skeleton makes the same calls in the same order (checked),
      runs ported callees natively (falling back to the replay when a port reaches unported
      code) and replays the others; the states after each tick are compared. Results: Actium two
      months chained (1,632 ticks) and Iken 300 ticks across a month end: every tick equal.
      `--report` lists the callees by bytes written: the order of phase 4. In 300 ticks of Iken:
      `0x00489B70` (every tick, 1.7 MB), road access `0x00457AF0`, `0x00457A40`, `0x004B7790`,
      `0x00462180`, `0x00459670`, `0x00454E20`, `0x00468B70`, `0x004E1640`, ...

## Phase 4: simulation subsystems

Each item: decompile, port with `@bin`, function-harness test on corpus states, then lockstep.
Order follows dependencies (shared helpers before their users).

- [ ] 4.1 Shared infrastructure: building / figure constructors and deletion (`0x0046A400`,
      `0x004B4800`), map helpers, road network and connectivity, terrain updates (`0x00475CC0`,
      `0x004776F0`), the CRT `rand` and x87 arithmetic where the game uses them (integer-exact
      reproduction of 53-bit rounding; no floating point in `core/`).
- [ ] 4.2 Daily city passes from the dispatcher: house population and capacity (`0x00458440`),
      industry counts (`0x004677C0`, `0x00468170`), storage (`0x0045AA30`, `0x0045AAA0`),
      labour (`0x00462180`), fire and collapse risk (`0x00468B70`), desirability (done),
      religion / health / entertainment coverage, house evolution (`0x00454190`).
- [ ] 4.3 Figures: action dispatch, movement, routing / path finding, each figure type
      (walkers, cart pushers, traders, animals, ...). The largest part.
- [ ] 4.4 Production and the Nile: floods (`0x004BE770`), farms, industries, storage yards,
      granaries, bazaars, water.
- [ ] 4.5 Month and year: finances (taxes, wages, tribute), trade and prices, ratings, kingdom
      requests and events, invasions, scenario win / loss, autosave arming.
- [ ] 4.6 Monuments (pyramids, mastabas, Cleopatra monuments) and their work gangs.
- [ ] 4.7 Military: companies, formations, battles, enemies.
- [ ] 4.8 Done when: lockstep runs 12 months on the whole corpus with no difference.

## Phase 5: player commands

- [ ] 5.1 Construction and clearing (every building type, roads, walls, canals, plazas, gardens),
      with their checks and costs.
- [ ] 5.2 Settings and orders: taxes, wages, trade routes, storage orders, military orders,
      gifts and requests.
- [ ] 5.3 Check: each command applied by the original's own command functions in the oracle and by
      the port, on corpus states, then lockstep.

## Phase 6: new games

- [ ] 6.1 Scenario start from `.map` files and campaign missions (scenario setup `0x004DE820`,
      terrain rebuild (done), initial buildings and figures). Check: state after start equals the
      oracle's for every campaign mission and bundled map.

## Phase 7: platform (playable)

- [ ] 7.1 SDL renderer: city view (sprite readers done), terrain and buildings, figures, overlays.
- [ ] 7.2 UI: menus, sidebar, info panels, advisors, messages (text groups done).
- [ ] 7.3 Input, sound and music; videos where a free decoder exists.
- [ ] 7.4 Check: screenshots against the original's frames for the same state (renderer run in the
      oracle on the off-screen buffer, if feasible).
