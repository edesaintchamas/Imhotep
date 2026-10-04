# Credits

Imhotep is a new codebase, but it stands on the shoulders of earlier open-source work on
Impressions Games' city builders. All of these projects are licensed under the GNU AGPL-3.0.

| Project | Game | Authors | Used for |
|---|---|---|---|
| [Julius](https://github.com/bvschaik/julius) | Caesar III | Bianca van Schaik and contributors | Engine structure reference |
| [Augustus](https://github.com/Keriew/augustus) | Caesar III | Keriew and contributors | Engine structure reference |
| [Akhenaten](https://github.com/dalerank/Akhenaten) | Pharaoh | dalerank and contributors (based on Ozymandias by Banderi) | Pharaoh mechanics reference |

## Code derived from these projects

Every file containing copied or translated code keeps the original copyright notice and is
tagged `@from <project>:<path>`. List them here:

| Imhotep file | Origin | Notes |
|---|---|---|
| `core/io/pkware.c` | julius:`src/core/zip.c` (`pk_explode`, code tables) | PKWARE DCL explode in C, from the Python port below; confirmed on every compressed map chunk |
| `core/io/pkware_implode.c` | julius:`src/core/zip.c` (`pk_implode` and helpers, tables) | PKWARE DCL implode; output byte-identical to the game's compressor `0x0055BF10` on every compressed chunk of 15 saves (`oracle_save_check`) |
| `tools/extract/pkware.py` | julius:`src/core/zip.c` (`pk_explode`, code tables) | PKWARE DCL explode, translated to Python; checked on all 165 compressed map chunks |
| `tools/extract/extract_graphics.py` | akhenaten:`src/content/imagepak.cpp` (`convert_compressed`, `convert_isometric_footprint`, `FOOTPRINT_X_START_PER_HEIGHT`); julius:`src/core/image.c` (`convert_compressed`, `to_32_bit`) | RLE and isometric footprint decoding translated to Python; layout re-checked against the game files (`docs/formats/graphics.md`) |

## Original games

Pharaoh (1999) by Impressions Games; Cleopatra: Queen of the Nile (2000) by BreakAway Games.
