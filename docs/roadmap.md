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
Method and conventions: docs/porting.md.

- [x] 4.0 Porting infrastructure:
      - deep replay (`tools/oracle/replay.py`, `build/state_replay`): inside any original call,
        every outermost call to an unported function is recorded with its changes and replayed
        into the port, so a function is checked as soon as it is ported, top-down or bottom-up;
        the harness and the lockstep both use it;
      - `frontier.py` (what a call still replays), `coverage.py` (functions the tick runs, indirect
        calls included);
      - per-module port tables with a hashed index;
      - the Ghidra export completed with the functions reached only through pointer tables
        (`tools/ghidra/CreateTableFunctions.py`: 490 functions, among them 101 of the 102 figure
        action functions of table 0x005DC28C).

- [x] 4.1 Shared infrastructure: building / figure constructors and deletion (`0x0046A400`,
      `0x004B4800`), map helpers, road network and connectivity, terrain updates (`0x00475CC0`,
      `0x004776F0`), the CRT `rand` and x87 arithmetic where the game uses them (integer-exact
      reproduction of 53-bit rounding; no floating point in `core/`).
- [x] 4.2 Daily city passes from the dispatcher: house population and capacity (`0x00458440`),
      industry counts (`0x004677C0`, `0x00468170`), storage (`0x0045AA30`, `0x0045AAA0`),
      labour (`0x00462180`), fire and collapse risk (`0x00468B70`), desirability (done),
      religion / health / entertainment coverage, house evolution (`0x00454190`).
- [x] 4.3 Figures: action dispatch, movement, routing / path finding, each figure type
      (walkers, cart pushers, traders, animals, ...). The largest part.
- [x] 4.4 Production and the Nile: floods (`0x004BE770`), farms, industries, storage yards,
      granaries, bazaars, water.
- [x] 4.5 Month and year: finances (taxes, wages, tribute), trade and prices, ratings, kingdom
      requests and events, invasions, scenario win / loss, autosave arming.
- [x] 4.6 Monuments (pyramids, mastabas, Cleopatra monuments) and their work gangs.
- [x] 4.7 Military: companies, formations, battles, enemies.
- [x] 4.8 Done when: lockstep runs 12 months on the whole corpus with no difference.

      Result (2026-10-05): 1,240 functions ported (docs/provenance/ported.md). `lockstep.py
      --ticks 9792 --chained --report` on the 8 corpus cities (the 7 downloaded saves and
      pak00 m00): 9,792 ticks each, 0 with differences, the ports write 100 % of the changed
      bytes. Still recorded as units, by design: the platform requests (video 0x00538E30,
      sounds 0x0053B4A0 / 0x0053B090; core/sim/platform.h) and two partial ports whose
      unported paths the game never takes (0x0051CAE0 outside the angles the game passes,
      0x00496640's endless-loop path). `make test` passes; provenance 100 % confirmed.

      Scope (found in phase 7, docs/presentation.md): this lockstep runs ticks only, as the
      original does with no frame drawn. In play each frame also writes state the simulation reads
      (the CRT rand seed, building and water animation frames in the grids, goods flags, autosave):
      phase 7.1 moves those frame-side writes into core and checks frame by frame.

      Carried over (not reached by the tick on the corpus, or file input):
      - 0x0053DFF0, the `.sg3` reader under the graphics loader: done in phase 5 (core/graphics/sg3.c;
        the file bytes come from the platform, the `.555` pixel load is a platform request).
      - 0x00470990 (building placement): done in phase 5.
      - 0x00422A90, 0x004BD740 (floodplain objects at load): load path (0x0053D850 done in phase 5).

## Phase 5: player commands

Commands are ported at the UI-handler boundary (docs/porting.md, docs/commands.md): the original's
own handler functions, whose inputs are globals of the state that the platform sets.

- [x] 5.0 Command infrastructure:
      - docs/commands.md: the command layer of the original (input dispatch, button tables,
        confirmation popup, every command with its handler, inputs and effects);
      - `lockstep.py --commands`: commands applied between ticks, in both games, states compared
        after each command and each following tick; `tests/differential/oracle_command_check.py`;
      - platform requests for what only drives the screen or reads files (core/sim/platform.h):
        modal message box, file reads, pixel loads, video close.
- [x] 5.1 Construction and clearing: choosing a building (menus, availability), mouse down / drag
      / up for every building type and drag type, the place step `0x004728A0` with its checks,
      costs and notices, the map add `0x00470990`, clearing with its confirmations, undo, the
      hover rules the place step reads (ferry reach and validity, bridge measure:
      core/command/hover.c, `oracle_hover_check.py`), the graphics slot loads monuments trigger
      (`0x0053DFF0`).
- [x] 5.2 Settings and orders: taxes, wages, salary, donations, gifts, labour priorities,
      festivals, trade routes and goods, storage orders, kingdom requests, grave goods, messages
      and event replies, military and ship orders, troop dispatch, advisors, difficulty (model
      reloads), options, map rotation, cheats.
- [x] 5.3 Check: each command applied by the original's own handlers in the oracle and by the
      port, on corpus states, then lockstep.

      Result (2026-10-06): 1,620 functions ported in all (docs/provenance/ported.md).
      `oracle_command_check.py`: 203 / 203 command sequences equal to the original, after each
      command and each following tick (chained), on the downloaded saves and pak00 m00;
      `oracle_hover_check.py`: 175 / 175 hovered tiles; `oracle_function_check.py`: 423 / 423.
      Nothing under the commands is replayed but platform requests. The 12-month lockstep still
      runs on the 8 corpus cities with 0 differences; `make test` passes; provenance 100 %.

      Carried over:
      - the ferry landing validity `0x00BCA314` is a core function (`ferry_hover`, checked
        against the original's draw), but the command tests set it as an input: the platform
        will call it for the hovered tile (phase 7); the link of a second ferry landing
        (`0x00486A30` returning 1) is ported from the disassembly but no corpus geometry reaches it;
      - the map editor's commands (`0x00478440`, `0x004795E0`, editor flag `0x00605A3C`): out of
        scope;
      - cheats 4–6 (mock attacks, frame counter) cannot be typed in the shipped game (gate byte
        `0x00578C68`, docs/commands.md §7); cheat 5 reads stack leftovers and stays unported;
      - the window id and video request around a playing video (`0x00539170`, `0x00539220`):
        the platform's video handling (phase 7);
      - the main menu `0x004CB6A0` (reached from "stop governing"): phase 6.

## Phase 6: new games

- [x] 6.1 Scenario start from `.map` files and campaign missions (docs/newgame.md): every new city
      starts in `0x005405B0`. Ported: the start and its start-only steps, the reset `0x00540A10`,
      the custom-scenario setup `0x004DE820` and map load `0x004DE600` / `0x004D5820` /
      `0x004D4AE0`, the initial figures and graphics slots, the campaign record load
      `0x004D58B0` and saved-game loads `0x004D5100` / `0x004D51B0` as registered ports, the
      scenario start `0x004D52A0` and its steps, and the campaign flow (mission list, briefing,
      next mission and carry-over, replay, families and the progress file, main menu, period
      choice, campaign.txt). Game files come from the platform (file reads, writes, listings).
      Check: state after start equals the oracle's for every campaign mission and bundled map.

      Result (2026-10-06): 1,861 functions ported in all (docs/provenance/ported.md).
      `oracle_newgame_check.py`: 160 / 160 starts equal to the original — the 33 bundled maps,
      the 53 mission1.pak records, difficulties 0 and 4, the mission-list path, carry-over, starts
      from a running city, and the campaign flow cases — each followed by a day of identical ticks
      (`--ticks 51`); across every start only platform requests are replayed.
      `oracle_state_check.py`: 31 / 31 saved games load equal. The 12-month lockstep on the 8
      corpus cities still has 0 differences; `make test` passes (now failing on any failed check:
      `pipefail`).

      Oracle changes: CRT `_lseek`, `fopen` / `fgets` / `fread` / `fseek` / `fwrite` / `fclose`,
      `FindFirstFileA` / `FindNextFileA`, `QueryPerformanceFrequency` (0), the Bink object at boot;
      the boot now reads campaign.txt and Pharaoh.inf as the real game does; the deep recorder
      records writes made by the Python file hooks; `Pharaoh_MM.eng` is mapped like the text file.

      Carried over:
      - played videos (Bink `0x00413580`, Smacker): the display-mode switch and window handling
        around a playing video are the platform's (phase 7); no video plays in the oracle;
      - uninitialised stack bytes reaching the campaign table and old-size progress files
        (docs/unverified.md UNINIT-41AE40, UNINIT-41B680);
      - file-system edge cases: the CD-drive retries, `CreateDirectoryA`, 8.3 short-name matches;
      - the map editor's paths (`0x00605A3C`): out of scope.

## Phase 7: platform (playable)

Survey: docs/presentation.md. The original's own drawing runs headless in the oracle into its
back buffer (pointer 0x00E92E18, pitch 0x00E38D94): the renderer and the UI are ported from the
binary into `present/` (deterministic, drawing into a pixel buffer) and checked pixel by pixel;
`platform/` holds the SDL window, events, clock, audio and video.

- [x] 7.0 Oracle and checks: display boot (fake DirectDraw), back buffer in the comparisons, pixel
      data mapped on the port's side, frame lockstep with a scripted clock and input
      (`tools/oracle/framestep.py`, `harness.py --display`, docs/oracle.md: Display, Snapshots).
- [x] 7.1 Frame skeleton in core (`core/frame`): pacing, tick loop, autosave, per-frame rand draws,
      animation timers, the frame-side state writes, the window procedure; 12 months of frames
      equal to the original's.
- [x] 7.2 Drawing primitives (blitters, fills) and text (sprite fonts), `present/draw`:
      `oracle_draw_check.py` 1,968 / 1,968 cases equal (several displays and saves).
- [x] 7.3 City view (`present/city`): terrain, buildings, figures, overlays, construction ghosts,
      animations, camera (`oracle_cityview_check.py`).
- [x] 7.4 Sidebar, top bar, minimap, tooltips (`present/sidebar`).
- [x] 7.5 Windows (`present/ui`): the window machinery and dispatchers, menus, advisors, info panels,
      messages and the empire map, dialogs, the front end (`oracle_info_check.py`,
      `oracle_msgempire_check.py` 31 / 31, `oracle_frontend_check.py`).
- [x] 7.6 Input: the window procedure `0x00414CD0` ported (`core/frame/wndproc.c`): keys, mouse,
      scrolling, rotation, hotkeys, resolution keys F6–F9, window messages; the platform answers
      cursor, key and message requests.
- [x] 7.7 Sound and music logic (requests checked against the original's driver calls,
      `oracle_sound_check.py` 7 / 7); video: Bink through FFmpeg (`platform/media.c`), with the
      original's 640 x 480 switch around a video (`oracle_video_check.py` 24 / 24).
- [x] 7.8 SDL platform (`platform/`, `make game` → `build/imhotep`): window, events, clock, audio,
      video, files; the original's start-up ported (`core/boot`, `oracle_boot_check.py` 7 / 7), so
      playing needs only the game files.
- [x] 7.9 Check: every frame of a scripted session equal to the original's, state and pixels.

      Result (2026-10-08): 2,848 functions ported in all (987 in this phase; 794 in `present/`,
      ~40,000 lines). Scripted SDL sessions (`build/imhotep --hidden --det-clock --script --dump
      --msglog`, then the same clock, window messages and cursor given to the original's frame
      `0x004D6B60` and window procedure in the oracle) are equal frame by frame, state and pixels:
      - from the port's own boot (`core/boot`, as `make game` plays), `oracle_play_check.py` (in
        `make present-check`, session `tests/differential/play/session.txt`): 1,042 / 1,042 frames (a
        new family, a mission start, building, scrolling, an info panel, the overlays menu); the only
        bytes set aside are the boot check's known ones (the import table, the stack bytes the
        original leaves in the monument objects and the campaign steps) and platform values (the
        window, DirectDraw and cursor handles, and pointers into memory the two sides allocate at
        different addresses);
      - from the original's booted state: 1,190 frames (also File > Save then Load, F7 to 640 x
        480), and a 565 session (F8 with its confirmation, F9).
      Both sides run without video: in the oracle `BinkOpen` fails (no video device) and the
      original skips the video, while the port plays it (the player's state effects, the 640 x 480
      switch and back included, are checked with a video that opens: `oracle_video_check.py` 24 / 24).
      `make test` passes: `oracle_frame_check.py` 190 / 190 cases (every package's frame cases),
      `oracle_draw_check.py` 1,968 / 1,968, `oracle_boot_check.py` 7 / 7, and phases 1–6's checks
      (functions 1,029 / 1,029, commands 203 / 203, new games 160 / 160, saves loaded 31 / 31,
      hover 175 / 175). The 12-month lockstep on the 8 corpus cities: 9,792
      ticks each, 0 with differences.

      Closed after the first result (2026-10-09):
      - the windowed mode (F6, kept in Pharaoh.inf): the boot's windowed set-up (0x00426B40,
        0x00426FF0: the kept mode, the window's place, the clipper) is ported; before, a game left
        windowed did not start again. `oracle_boot_check.py` case `1024x768x555+windowed` equal,
        `oracle_play_check.py --windowed` 1,042 / 1,042 frames equal;
      - the Smacker videos (logo, intro, credits, message videos): the GOG install has no `.smk`
        files, so the original's opens fail as the port's do (docs/presentation.md 7): nothing to play.

      Carried over:
      - a video's frames: the oracle cannot decode Bink, so the frames FFmpeg shows are not compared
        (the player's state effects are);
      - the CD-drive and 8.3 file-system edge cases (6.1);
      - 0x004179A0 loops forever in the original when no advisor is available from index 0; the
        port reproduces it;
      - the map editor (out of scope).

## Phase 8: completeness

The scripted sessions of 7.9 reach no unported function, but they cover a small part of the game: a
call to a function without a port answers 0 and does nothing, silently. By direct calls alone, 290
functions reachable from WinMain `0x00413ED0` have no port (about 75 KB of code, 2026-10-09); calls
through function tables (windows, buttons, building and figure actions) add more. This phase ports
everything the game can reach and looks for what scripted sessions miss.

- [x] 8.0 Gap report: the game reports every unported function it reaches (address, once, on stderr
      and in a log file of the player directory), so a gap met in play is seen, not silent.

      Done (2026-10-09, platform/main.c): calls to functions without a port and `state_unported` stops
      inside ports (which were silent before: nothing read them) are printed once each and appended
      to `$IMHOTEP_PLAYER_DIR/gaps.log` with the window and frame; the game goes on (the call answers
      0).
- [ ] 8.1 Frontier: every function reachable from WinMain through direct calls, function addresses in
      code (callbacks, `GAME_CALL` targets) and the function tables in the data sections, minus the
      ported ones (`tools/oracle/reach.py`, written to docs/provenance/frontier.md, each entry with its
      callers and a group). Out of scope, listed as such: the map editor (`0x00605A3C`), and code the
      binary cannot reach.

      First count (2026-10-09): 3,565 functions reachable (2,801 of the 2,848 ports among them), 737
      without a port (119 KB of code), 27 more only through the map editor. Each is being classified
      (`tools/oracle/frontier_notes.txt`: gap, platform, crt, unused, scope) by area: runtime (343:
      the C runtime, the start-up), core (176), ui (218, mostly through the window tables).

      Classified (2026-10-10): of the 489 still without a port, 173 are C runtime internals the port
      does natively, 49 platform work (Win32, DirectDraw, Miles, Bink, CD, message loop), 181 map
      editor code (reached only with the editor flag `0x00605A3C` set), 84 never called in the shipped
      game (table entries no index selects, objects nothing sets, flags only ever written 0), each
      with its evidence in `tools/oracle/frontier_notes.txt`; 2 gaps left (below).
- [ ] 8.2 Port the frontier by package, each function checked against the original (harness,
      framestep, frame cases); with it the `state_unported` stops left inside ported functions
      (paths the sessions never took).

      Progress (2026-10-10): 249 functions ported, 3,097 in all. Among them: the message keyword
      handlers (`[amount]`, `[city_name]`... came out empty), the File / Options / Help menu items,
      the monument clears and the tombs' stone laying, figure and building checks, frogs, the
      screen-shake fade, the empire map, overlays, message and trade buttons, advisor pages and
      windows 0x2A, 0x2D, 0x2E, 0x4C, 0x52, 0x56, the monument info queries; the tutorial goods
      message (UNINIT-417B40). Checks: harness on the corpus, 22 new frame cases (212 / 212), lockstep
      (0 differences). Left: the converters of old saves and maps 0x004D0C60 (events, versions
      115–129) and 0x004D0D40 (messages, 115–135), whose output holds uninitialised stack bytes,
      reached only by files older than any bundled map or corpus save.
- [x] 8.3 Randomised sessions: random input (clicks on buttons and the city, keys, scrolling, menus,
      saves and loads) from new games and corpus saves, run by `build/imhotep` and by the original in
      the oracle with the same clock and messages, compared frame by frame (`oracle_play_check.py`);
      every difference and every unported function reached is a case to fix.

      Result (2026-10-10, `tools/oracle/fuzz.py`, docs/oracle.md "Randomised sessions"): 94 seeded
      sessions, 1,040,743 frames compared with the original (7.2 hours of game time at 25 ms a
      frame), from new families, chosen missions, custom maps and corpus saves loaded through the
      menus, at 1024 x 768, 800 x 600 and 640 x 480, sessions of 3,000 to 43,000 frames (four of about
      three game years). 93 equal on every frame, state and pixels; seed 202 equal for 28,669 frames,
      then the oracle's limit (below). No crash. Found and fixed: a resolution key while windowed sent
      the game back to fullscreen (the platform reported a display change for every new back buffer);
      the advisors' per-frame part 0x00521060 unported (ported in 8.2). The play check's oracle
      follows the display mode (GetSystemMetrics after SetDisplayMode), boots on a 1024 x 768 screen as
      the game does, and shares the COM vtables (oracle_play_check.py); the tutorial message's
      uninitialised bytes (UNINIT-417B40) are copied from the port when posted during a session.
- [x] 8.4 Done when: the frontier holds only out-of-scope code, and the randomised sessions (several
      hours of frames across the corpus) show no difference and reach no unported function.

      Result (2026-10-10): 3,097 functions ported (249 in this phase); the frontier holds only C runtime
      internals, platform work, map editor code and functions the shipped game never calls, each
      with its evidence, and the two converters below; 94 randomised sessions (1.04 million frames)
      equal to the original. `make test` passes (now enforced): frame cases 212 / 212, drawing
      1,968 / 1,968, boot 8 / 8 (windowed included), and phases 1–7's checks.

      Found on the way: `make test` did not fail on a failed check since phase 6: its `pipefail` was set
      through `.SHELLFLAGS`, which GNU Make 3.81 (macOS's) ignores. It is now in `SHELL`. The results
      recorded for phases 6 and 7 were read from each check's own summary line.

      Carried over:
      - the old-format converters 0x004D0C60 and 0x004D0D40 (8.2): their output holds the original's
        uninitialised stack; only files older than any bundled map or corpus save reach them;
      - the oracle: its emulated heap runs out when a game changes its display mode every frame
        (window 0x3E reverting to itself, seed 202), and scripted keys set no GetAsyncKeyState state;
        the display-mode fixes of the play check are not in tools/oracle/game.py (its code is part of
        every snapshot's key);
      - the fuzzer knows the layout of the sidebar, build menus and campaign map; advisor, empire and
        trade windows get random clicks only;
      - the map editor (out of scope: 181 functions of the frontier).
