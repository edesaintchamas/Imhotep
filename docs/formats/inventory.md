# Inventory of the reference installation

Reference: GOG Pharaoh + Cleopatra 2.1, `Pharaoh.exe` version resource 2.1.0.0
(hashes in `docs/source-fingerprint.json`, 1335 files). Game directory: `local/game/app`
(`$IMHOTEP_GAME_DIR`). The installer's other output, `local/game/tmp/`, holds only GOG
installer artwork and is not part of the game.

Generated 2026-10-04 by walking `$IMHOTEP_GAME_DIR` recursively. Extensions are compared
case-insensitively (the files mix `.wav`/`.WAV`, `.bmp`/`.BMP` names, etc.).

## By extension

| Ext | Count | Total size | Location |
|---|---:|---:|---|
| `.mp3` | 193 | 181.52 MB | `AUDIO/Music`, `AUDIO/Ambient`, `AUDIO/Voice/Mission` |
| `.555` | 220 | 177.21 MB | `Data/` |
| `.bik` | 7 | 146.72 MB | `BINKS/High/` |
| `.sg3` | 175 | 124.90 MB | `Data/` |
| `.wav` | 637 | 102.60 MB | `AUDIO/Wavs`, `AUDIO/Voice/Walker` |
| `.pdf` | 4 | 28.85 MB | root (manuals, reference card, editor guide) |
| `.map` | 33 | 25.03 MB | `Maps/` |
| `.pak` | 1 | 17.19 MB | root (`mission1.pak`) |
| `.exe` | 1 | 2.14 MB | root (`Pharaoh.exe`) |
| `.emp` | 1 | 1.91 MB | root (`Pharaoh2.emp`) |
| `.txt` | 38 | 1.40 MB | root, `Maps/` |
| `.dll` | 6 | 1.27 MB | root |
| `.eng` | 2 | 0.86 MB | root (`Pharaoh_Text.eng`, `Pharaoh_MM.eng`) |
| `.zip` | 1 | 0.33 MB | root (`webcache.zip`) |
| `.m3d` | 5 | 0.25 MB | root |
| `.ico` | 3 | 0.14 MB | root |
| `.asi` | 1 | 0.13 MB | root (`MP3DEC.ASI`) |
| `.hashdb` | 1 | 0.04 MB | root (`goggame-1207659046.hashdb`) |
| `.inf` | 3 | < 0.01 MB | root (`Pharaoh.inf`, `Pharaohmap.inf`, `Language.inf`) |
| `.ini` | 1 | < 0.01 MB | root (`Pharaoh.ini`) |
| `.info` | 1 | < 0.01 MB | root (`goggame-1207659046.info`) |
| `.tsk` | 1 | < 0.01 MB | root (`mssb16.tsk`) |

Empty directories: `__support/app`, `__support/save/Save`. The installation contains **no save
files**.

## Families

The status column says whether the grouping was checked against file contents or is a guess
from names and locations.

| Family | Files | Status | Evidence / notes |
|---|---|---|---|
| Graphics index | `Data/*.sg3` (175) | **confirmed** | Parsed: see `graphics.md`. Every `.sg3` has a same-named `.555`. |
| Graphics pixel data | `Data/*.555` (220) | **confirmed** | 175 are paired with a `.sg3`, and the index offsets land exactly on their end of file. Of the 45 unpaired ones, 39 are referenced as "external" bitmaps by `Pharaoh_Unloaded.sg3` / `Expansion.sg3` (see `graphics.md`). 6 unpaired files are not referenced by any `.sg3`: `ckit`, `picture0`, `picture5`–`picture8`. |
| Maps / scenarios | `Maps/*.map` (33) | **confirmed** | Self-describing chunk container, see `map.md`. All 33 parse to end of file. |
| Scenario descriptions | `Maps/*.txt` | assumed | Plain text, a quoted title then a briefing, named after a `.map`. |
| Campaign missions | `mission1.pak` | **confirmed** | 60-slot offset table, 53 saved-game records; see `campaign.md`. |
| Empire | `Pharaoh2.emp` | assumed | Name only. Akhenaten's `empire/empire_map.cpp` refers to `.emp`. Not parsed. |
| Text | `Pharaoh_Text.eng` (**confirmed**, `text.md`), `Pharaoh_MM.eng` (assumed) | mixed | Magic strings `Pharaoh textfile`, `Pharaoh MM file.` (MM = message manager?). `Pharaoh_Text.txt` / `Pharaoh_MM.txt` look like plain-text sources of the same. |
| Model / tuning tables | `*_Model_*.txt`, `Figure_model*.txt`, `Tax_Sentiment_Model_*.txt`, `campaign.txt`, `eventmsg.txt`, `auto reason phrases.txt` | assumed | Plain text, one per difficulty level for the models. These very likely hold **game rules** and will need checking against how the binary reads them. |
| Saves | none | — | No `.sav` in the installation; layout from the game's schema in `save.md`. A reference corpus has to be produced by running the game. |
| Audio | `AUDIO/**/*.wav`, `*.mp3` | assumed | By extension and directory (Music, Ambient, Voice/Mission, Voice/Walker, Wavs). |
| Video | `BINKS/High/*.bik` | assumed | Bink video by extension. Only a `High` quality set. |
| Executable | `Pharaoh.exe` | confirmed | PE32, version resource 2.1.0.0. |
| Third-party runtime | `BINKW32.DLL`, `SMACKW32.DLL`, `mss16.dll`, `*.M3D`, `MP3DEC.ASI`, `mssb16.tsk` | assumed | Bink/Smacker video and Miles Sound System, by name. Not ours to reimplement bit for bit. |
| Configuration | `Pharaoh.inf`, `Pharaohmap.inf`, `Language.inf`, `Pharaoh.ini` | assumed | INI-style text (`[Ident]`, `Title=Pharaoh`, ...). |
| GOG wrapper | `goggame-*`, `GameuxInstallHelper.dll`, `webcache.zip`, `*.ico`, `__support/` | assumed | GOG Galaxy / launcher files, by name. Not game data. |
| Documentation | `*.pdf`, `Readme.txt`, `Mission Editor Guide.txt` | assumed | Manuals. `Readme.txt` says "Version 2.1". |

## Open questions

- What are the 6 `.555` files that no `.sg3` references (`ckit`, `picture0`, `picture5`–`picture8`)?
  They may be loaded directly by the executable, or unused.
- ~~`.map` container~~: answered in `map.md` (versioned, self-describing chunk list).
