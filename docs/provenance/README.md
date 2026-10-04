# Provenance index

Maps addresses in the reference executable to Imhotep functions.

| Address | Original role (our name) | Imhotep function | Status |
|---|---|---|---|
| 0x0053DD90 | `sg3_load_family` | — (tools only: `tools/extract/extract_graphics.py`) | read (decompiled) |
| 0x0053DFF0 | `sg3_load_index` | `parse_sg3`, sequential offsets in `main` | read (decompiled, stack frame inferred) |
| 0x0053F660 | `sg3_load_pixels` | — | read (decompiled) |
| 0x004CDF20 | `pixels_555_to_565` | `rgb555` (format only) | read (decompiled) |
| 0x004CDEB0 | `rle_555_to_565` | `decode_rle` (format only) | read (decompiled) |
| 0x004CEB60 | `file_read_at` | — | read (decompiled) |
| 0x00426FF0 | `dd_create_surfaces` | — (platform) | read (decompiled) |
| 0x0053E590 | `image_draw` | mirror handling in `main` (`--render-mirrors`) | read (decompiled) |
| 0x0040FFD0, 0x00410090, 0x0040FED0, 0x0040FF50, 0x004100F0, 0x004101C0 | plain blitters | `decode_plain` (key 0xF81F) | read (decompiled) |
| 0x00427A20, 0x0042C740 | footprint tile, 2×2 footprint | `decode_foot` | read (decompiled) |
| 0x004D4AE0 | `map_load` | `read_map` (container only) | read (decompiled) |
| 0x004D0E10 | `map_read_chunk` (schema migration) | `read_map` | read (decompiled) |
| 0x004D5520, 0x004D4E20 | `map_save` (two variants) | — | skimmed |
| 0x005E17D8 | static map schema table (data) | `CHUNK_NAMES` order | read |
| 0x0055BCF0 | PKWARE explode (game) | `pkware.explode` | not read |
| 0x00414420 | startup graphics slots | `core/graphics/image_groups.c` `image_startup_slots` | read (decompiled) |
| 0x0046F7B0, 0x0046F930, 0x0046F9A0 | map row ranges, on-map test, near-edge test | `core/map/terrain.c` `terrain_init` (rows), `inside`, `near_edge` | read (decompiled) |
| 0x00430530 | view table (projection) | `screen_of` | read (decompiled) |
| 0x0047BF50, 0x0047C140 | place n×n image / sub-images | `core/map/terrain.c` `place`, `place_subimages` | read (decompiled) |
| 0x0047CAE0, 0x0047C880 | block fit tests | `core/map/terrain.c` `fits`, `fits_land` | read (decompiled) |
| 0x004D52A0 | scenario start (zeroes image grid) | `render` | read (decompiled) |
| 0x004DEA50 | terrain image rebuild (sub-pass list) | `core/map/terrain.c` `rebuild` | read (decompiled) |
| 0x00475DC0, 0x00477210 | rocks, dunes | `core/map/terrain.c` `blocks`, `pass_rocks`, `pass_dunes` | read (decompiled) |
| 0x004767D0 | land and grass | `core/map/terrain.c` `land_rect`, `grass_image` | read (decompiled + disassembly) |
| 0x00475CC0, 0x0047A220 | water | `core/map/terrain.c` `pass_water`, `water_image` | read (decompiled) |
| 0x00484BE0, 0x00484EE0, 0x0047DB00, 0x0047DC80 | neighbour pattern matching | `core/map/terrain.c` `match`, `nflags` | read (decompiled) |
| 0x0053DB60 | graphics totals init (image/group ids start at 1) | `ImageTable` | read (decompiled) |
| 0x004BE160, 0x004BD740, 0x004BD610, 0x004BE0F0 | floodplain build, object, manager | `core/map/terrain.c` `floodplain_build` | read (decompiled + disassembly) |
| 0x004777C0, 0x00479E70, 0x00484D70, 0x0047DDD0 | floodplain images | `core/map/terrain.c` `pass_floodplain`, `floodplain_tile`, `match_by_water` | read (decompiled + disassembly) |
| 0x004BE770 | daily flood tick (row progress) | — | skimmed |
| 0x004770F0, 0x004798E0, 0x0047DEE0 | special rocks | `core/map/terrain.c` `pass_special_rocks` | read (decompiled) |
| 0x00475BF0, 0x00479770, 0x00486600, 0x00481490, 0x00481AA0, 0x0046F8D0, 0x00481F20 | trees, rings | `core/map/terrain.c` `pass_trees`, `vegetation_image`, `ring_full` | read (decompiled + disassembly) |
| 0x00476530 | flag 0x20 | `core/map/terrain.c` `pass_flag_0x20` | read (decompiled) |
| 0x0047D870, 0x00482810 | entry/exit signposts | `core/map/terrain.c` `pass_signposts` | read (decompiled) |
| 0x00477500, 0x00479B70 | meadow | `core/map/terrain.c` `pass_meadow` | read (decompiled + disassembly) |
| 0x004776F0, 0x0047AB10 | flag 0x40000 | `core/map/terrain.c` `pass_flag_0x40000` | read (decompiled) |
| 0x00477890, 0x0047A120 | rubble | `core/map/terrain.c` `pass_rubble` | read (decompiled) |
| 0x0047B570, 0x0047B590, 0x0047B670, 0x0047B920, 0x0047E5C0 | roads | `core/map/terrain.c` `pass_roads` | read (decompiled) |
| 0x00475720, 0x00475670 | plaza | `core/map/terrain.c` `pass_plaza`, `plaza_2x2` | read (decompiled) |
| 0x0047B1A0, 0x0047E000, 0x0047E750 | walls | `core/map/terrain.c` `pass_walls` | read (decompiled) |
| 0x004D58B0 | campaign record load (missionN.pak) | `core/io/campaign_pak.c` `campaign_pak_record` | read (decompiled) |
| 0x00475FD0, 0x0047DBC0 | cliff image | `core/map/terrain.c` `cliff_index` | read (decompiled) |
| 0x004D41E0, 0x004BD360 | saved-game load, pre-start hook | — | skimmed |
| 0x004D41E0 | saved-game loader | — (documented in docs/formats/save.md) | read (decompiled) |
| 0x005E09A0, 0x004D1350 | save schema: static records 0–146, run-time records 147–180 | — | read (data + disassembly) |
| 0x004D4A60 | map schema run-time records 21–23 | — | read (disassembly) |
| 0x004D17E0 … 0x004D4180 (32 functions, see save.md §4) | save version upgrades | `upgrades` (core/io/save_load.c); relayouts as generated tables (core/io/save_relayout.c) | ported @bin |
| 0x004D41E0 (steps 4, 6, 7), 0x004D4720, 0x004D32F0, 0x0041FCA0, clean-ups | route rebuild, figure names, monument work-step resets, restores | `save_file_load` (core/io/save_load.c), tables core/io/save_tables.c | ported @bin; save_check (route lengths, endpoints, figure groups); @diff oracle_load_check (all 181 variables equal to the original loader's, 60 saves) |
| 0x004DE820 | scenario setup: treasury, entry/exit points into city data | — (save.md §5) | read (decompiled) |
| 0x004CDE40, 0x005D1960 | a·b/100; difficulty funds percentages | — | read |
| 0x004CCBB0, 0x004CCB30, 0x004D62F0, 0x004D6250 | text lookup and drawing (group, index) | `tools/extract/read_text.py` | read (decompiled) |
| 0x004FAB20 | info panels: building names (group 41), figure names (group 64) | — | read (decompiled, partly) |
| 0x0047BAE0, 0x0047E870, 0x0047EB60 | canal sub-pass, canal neighbour flags | `pass_canals`, `canal_flags`, `canal_link` (core/map/terrain.c) | ported @bin (no test data) |
| 0x0047B920 | paved-road test (desirability) | `road_paved` | ported @bin |
| 0x0055BCF0, 0x0055C000, 0x0055CBD0 | compressed chunk reader, DCL explode wrapper, explode | `map_file_read` (stored size 0 / 0x80000000 / limit), `pkware_explode` | read (decompiled) |
| 0x004D7090 | RNG step (two LFSRs, pool of 100) | `random_next` (core/random/random.c) | ported @bin |
| 0x00562348, 0x0056233E | CRT rand / srand (reseeded with timeGetTime by 0x004EAEC0, 0x004EB000) | — (save.md §5) | read |
| 0x005640EC | CRT startup `_controlfp(_PC_53, _MCW_PC)` | precision note in core/map/terrain.c | read |
| 0x004CEC40 | text + message file loader | `read_text.py --messages` | read (decompiled) |
| 0x00520000 | build menus (text group 28 by building type) | — | read |
| 0x004BD360 | floodplain settings reset before a campaign start | — (save.md §2) | read |
| 0x004D0E10, 0x005E09A0, 0x004D1350 | per-chunk schema-migrating reader; save schema (static, run time) | `save_file_read`, `migrate` (core/io/save_file.c), `save_schema` (generated) | ported @bin; checked by save_check |
| 0x0046A0C0, 0x0046A140, 0x00481830, 0x004818A0, 0x00481AA0, 0x0046F8D0 | daily desirability update, spread, ring add, edge test, 1x1 rings | `desirability_update` (core/city/desirability.c), ring tables (core/city/ring_tables.c, generated) | ported @bin, @diff desirability_check |
| 0x00540C90, 0x004CBDD0, 0x004CBE30, 0x004CC0E0, 0x004CC070 | building / house model loader and its text helpers | `building_model_parse` (core/model/building_model.c) | ported @bin |
| 0x0046A400 | building constructor | — (save.md §5) | read (decompiled) |
| 0x004B4800 | figure constructor | — (save.md §5) | read (decompiled) |
| 0x0053DD90 (reloads), 0x004BC720, 0x0053D970, 0x004F4520 | sg3 slot reloads at scenario start / load | — (graphics.md §11) | read (decompiled) |
| 0x004D3DA0, 0x004D18F0, 0x004D1AD0, 0x004D2FA0, 0x004D3200 | version < 146 clean-up; version-124 relayouts | — (save.md §4) | read (asm checked) |
| 0x00420D10, 0x00421010, 0x004206A0, 0x004209E0, 0x004212F0, 0x0041FCA0 | Cleopatra monument clean-ups (CRT rand) | — (save.md §4) | read (asm checked) |
| 0x00413ED0 | WinMain: initialisation order and `this` pointers | `Game.boot` (tools/oracle/game.py) | read (disassembly) |
| 0x00576000 … 0x0057607C, 0x005646C0 | CRT C++ static constructors, `_cinit` | `Game.boot` | read |
| 0x004D5100, 0x004D52A0 | load a saved game (file → loader → scenario start, post-load rebuilds) | `Game.load_save` | read (decompiled) |
| 0x004D4E20 | save a game (also the autosave) | `Game.save` | read (decompiled) |
| 0x004D7090, 0x00451C50, 0x00452370 | game tick, 51-step day dispatcher, day / month / year cascade | `tools/oracle/corpus.py` (run as is) | read (decompiled) |
| 0x00568EAB, 0x005660ED, 0x005664D5, 0x005647EB, 0x0056B1B6, 0x0056B2DD, 0x00561B7A, 0x00561BDC, 0x00563129, 0x005645A0, 0x00561953, 0x00604950 | CRT `_open`, `_read`, `_write`, `_close`, `_filelength`, `_chdir`, `malloc`, `free`, `calloc`, `realloc`, `atexit`; `rand` state | replaced in tools/oracle/game.py | read |
| 0x004D5100 (port), 0x0046FA40, 0x0046FC30, 0x0046FC80, 0x00475B80, 0x00484EE0, 0x004BD330 | load a saved game into the state; pre-load clear (grids, random grid with the RNG, image grid) | `game_load_save` (core/state/load_game.c), `pre_load_clear`, `floodplain_settings_reset` (core/state/pre_load.c) | ported @bin; harness; oracle_state_check |
| 0x004D52A0 (part), 0x0046F7B0, 0x00540ED0, 0x00541160, 0x004E7E20 | scenario start: map globals, row ranges, figure and tax-sentiment models | `scenario_start` (core/state/scenario_start.c), `model_load_figures`, `model_load_tax_sentiment` (core/model/model_text.c) | ported @bin; oracle_state_check |
| 0x004BE0F0, 0x004BD6A0, 0x004BD740, 0x004BD610, 0x004BE160 | floodplain manager: reset, list release, object + sorted node, build | core/map/floodplain.c, `terrain_floodplain_build` | ported @bin; harness byte-exact incl. heap (15 states) |
| 0x004DEA50 and its 16 sub-passes; 0x0047B670, 0x0047E5C0, 0x0051C5F0, 0x0047E870, 0x0047C880, 0x0047C140, 0x00482650, 0x004F3D70, 0x004F3F70, 0x004F60D0 | terrain image rebuild on the state: paved roads, gatehouses, water lifts, figures in block fits, monument tiles, pyramid tile states | core/map/terrain.c | ported @bin; harness byte-exact (15 states); terrain_check unchanged |
| 0x00517A20, 0x005178B0, 0x00517690, 0x005178E0, 0x00517660, 0x005173A0, 0x00517370, 0x00516F90 | routing grids | core/map/routing.c | ported @bin; harness byte-exact |
| 0x004439C0, 0x00443950, 0x00443B90, 0x00443D90, 0x00559480, 0x00447790, 0x004C5240, 0x004C5190, 0x005596D0 | goods the city can obtain; available goods lists | core/city/goods.c | ported @bin; harness (all goods, 4 states) |
| 0x0046B100, 0x0046B320, 0x0046EFD0, 0x0046F030, 0x0046A280, 0x0046A320 | per-building records rebuilt after a load | core/state/post_load.c | ported @bin; harness |
| 0x004F6700, 0x004F6760, 0x004F6810, 0x004F6A10, 0x004D0A40 | growable array; shipwright queues restored / packed | core/state/vec.c, save_load.c, save_game.c | ported @bin; oracle_state_check, oracle_save_check |
| 0x004D4E20, 0x0055BC50, 0x0055BF10, 0x004771C0 | save a game: preparation, chunk writer, compressor | `game_save` (core/state/save_game.c), `pkware_implode` (core/io/pkware_implode.c) | ported @bin; @diff oracle_save_check (15 / 15 files identical) |
| 0x004D7090, 0x00451C50, 0x00452370 | game tick, day dispatcher, day / month / year cascade | `game_tick`, `day_dispatch`, `day_cascade` (core/sim/tick.c, docs/formats/tick.md) | ported @bin; @diff lockstep (call sequence and inline writes equal to the original's, every call replayed or native) |
| 0x004CDC70, 0x00452150, 0x004E7E40, 0x0051F780 | animation counters; small tick functions; the empty function | core/sim/small.c, `run_nothing` | ported @bin; lockstep native |
