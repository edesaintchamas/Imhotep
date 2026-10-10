/* Imhotep - where build/imhotep finds the game files and keeps the player's (roadmap 9.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Nothing here depends on the current directory.
 *
 * The game directory (the Pharaoh + Cleopatra installation, read-only), the first that applies:
 *   1. --game DIR;
 *   2. IMHOTEP_GAME_DIR;
 *   3. the directory remembered from the last start (`game=` in imhotep.cfg, in the per-user data
 *      directory SDL_GetPrefPath("Imhotep", "Imhotep"));
 *   4. the usual places: a checkout's local/game/app next to the executable's directory (developers),
 *      then per system the GOG installs (Windows: the GOG.com registry entries, C:\GOG Games, GOG
 *      Galaxy's Games; Linux and macOS: ~/GOG Games, ~/Games, Wine prefixes and bottles).
 * A directory qualifies when it holds Pharaoh.exe (the name in any case) with the reference SHA-256
 * (GOG 2.1, platform/fingerprint.c). 1 and 2 are taken as given: a wrong one is an error, not passed
 * over. 3 and 4 are passed over when they do not qualify. Another version of Pharaoh.exe is refused:
 * the engine is checked against that exact binary. With none, the player is told what is needed
 * (a message box, unless hidden, and stderr). A qualifying directory found by 1, 3 or 4 is
 * remembered for the next start.
 *
 * The player directory (saves, settings, campaign progress): IMHOTEP_PLAYER_DIR, else the per-user
 * data directory SDL_GetPrefPath("Imhotep", "Imhotep") (macOS ~/Library/Application Support/Imhotep/
 * Imhotep, Linux ~/.local/share/Imhotep/Imhotep, Windows %APPDATA%\Imhotep\Imhotep). */
#ifndef IMHOTEP_PLATFORM_INSTALL_H
#define IMHOTEP_PLATFORM_INSTALL_H

#include <stddef.h>

#ifndef IMHOTEP_VERSION
#define IMHOTEP_VERSION "0.9-dev"
#endif

/* The game directory into out (see above); 0, or -1 when none qualifies (the player was told why).
 * `arg`: --game's value or NULL. `quiet`: no message box (--hidden). `verbose`: each place tried
 * is printed. */
int install_game_dir(const char *arg, int quiet, int verbose, char *out, size_t size);

/* The player directory into out (see above), created if needed; 0, or -1 */
int install_player_dir(char *out, size_t size);

#endif
