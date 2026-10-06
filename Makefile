# Imhotep — development commands. See CLAUDE.md.

IMHOTEP_EXTRACT_DIR ?= local/game
IMHOTEP_GAME_DIR ?= $(IMHOTEP_EXTRACT_DIR)/app
# Ghidra and its JDK: default to Homebrew (`brew install ghidra`, which pulls in openjdk@21,
# keg-only so not on PATH). Override either from the environment or the command line.
BREW_GHIDRA      := $(shell brew --prefix ghidra 2>/dev/null)
BREW_JDK         := $(shell brew --prefix openjdk@21 2>/dev/null)
GHIDRA_HOME      ?= $(if $(BREW_GHIDRA),$(BREW_GHIDRA)/libexec,$(HOME)/ghidra)
JAVA_HOME        ?= $(if $(BREW_JDK),$(BREW_JDK)/libexec/openjdk.jdk/Contents/Home)
PHARAOH_INSTALLER ?=
GAME_EXE         ?= $(IMHOTEP_GAME_DIR)/Pharaoh.exe
GHIDRA_PROJ_DIR  := local/ghidra
GHIDRA_EXPORT    := local/ghidra-export
export IMHOTEP_GAME_DIR

.PHONY: help verify fingerprint extract refs analyze provenance check-files core tools unit differential oracle-env oracle-check corpus test

help:
	@grep -E '^[a-z-]+:.*##' $(MAKEFILE_LIST) | sed 's/:.*##/ —/'

verify: ## Check game files against docs/source-fingerprint.json
	python3 tools/fingerprint.py

fingerprint: ## Record the reference fingerprint (once, on the reference installation)
	python3 tools/fingerprint.py --init

extract: ## Extract the GOG installer into local/game (game in local/game/app; needs: brew install innoextract)
	@test -n "$(PHARAOH_INSTALLER)" || (echo "Set PHARAOH_INSTALLER=/path/to/setup_pharaoh_*.exe" && exit 1)
	mkdir -p $(IMHOTEP_EXTRACT_DIR)
	innoextract --output-dir $(IMHOTEP_EXTRACT_DIR) "$(PHARAOH_INSTALLER)"

refs: ## Clone reference projects into refs/ (read-only, git-ignored)
	mkdir -p refs
	test -d refs/julius    || git clone --depth 1 https://github.com/bvschaik/julius refs/julius
	test -d refs/augustus  || git clone --depth 1 https://github.com/Keriew/augustus refs/augustus
	test -d refs/akhenaten || git clone --depth 1 https://github.com/dalerank/Akhenaten refs/akhenaten

analyze: verify ## Ghidra headless analysis and export to local/ghidra-export (Ghidra >= 11.3, PyGhidra)
	@test -f "$(GAME_EXE)" || (echo "Executable not found: $(GAME_EXE) (set GAME_EXE)" && exit 1)
	@test -x "$(JAVA_HOME)/bin/java" || (echo "JDK not found: JAVA_HOME=$(JAVA_HOME) (brew install ghidra, or set JAVA_HOME)" && exit 1)
	mkdir -p $(GHIDRA_PROJ_DIR) $(GHIDRA_EXPORT)
	JAVA_HOME="$(JAVA_HOME)" "$(GHIDRA_HOME)/support/pyghidraRun" -H $(GHIDRA_PROJ_DIR) pharaoh \
		-import "$(GAME_EXE)" -overwrite \
		-scriptPath tools/ghidra -postScript CreateTableFunctions.py -postScript ExportFunctions.py $(GHIDRA_EXPORT)

provenance: ## Provenance report for core/
	python3 tools/check_provenance.py

check-files: ## Fail if game content is tracked by git
	python3 tools/check_no_game_files.py

# ---- core/ build (C11, no dependencies) ----
CC      ?= cc
BUILD   ?= build
CFLAGS  ?= -O2
CORE_CFLAGS := -std=c11 -Wall -Wextra -Werror -Icore $(CFLAGS)
CORE_SRC := $(shell find core -name '*.c')
CORE_OBJ := $(CORE_SRC:%.c=$(BUILD)/%.o)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CORE_CFLAGS) -c $< -o $@

$(BUILD)/libimhotep_core.a: $(CORE_OBJ)
	ar rcs $@ $^

PLATFORM_SRC := platform/game_files.c platform/exe_image.c
TOOL_CFLAGS := $(CORE_CFLAGS) -I.

$(BUILD)/terrain_check: tests/differential/terrain_check.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/save_check: tests/differential/save_check.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/desirability_check: tests/differential/desirability_check.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/save_dump: tools/extract/save_dump.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/terrain_dump: tools/extract/terrain_dump.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/state_run: tools/oracle/state_run.c $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/state_load: tools/oracle/state_load.c $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(PLATFORM_SRC) $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/state_save: tools/oracle/state_save.c $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(BUILD)/libimhotep_core.a -o $@

$(BUILD)/state_replay: tools/oracle/state_replay.c $(BUILD)/libimhotep_core.a
	$(CC) $(TOOL_CFLAGS) $< $(BUILD)/libimhotep_core.a -o $@

tools: $(BUILD)/terrain_dump $(BUILD)/save_dump $(BUILD)/state_run $(BUILD)/state_load $(BUILD)/state_save $(BUILD)/state_replay ## Build the C tools used by tools/extract (terrain_dump, save_dump)

core: $(BUILD)/libimhotep_core.a ## Build core/ (static library)

unit: core ## Unit tests (none yet; builds core/)
	@echo "unit: core/ builds; no unit tests yet"

differential: $(BUILD)/terrain_check $(BUILD)/save_check $(BUILD)/desirability_check ## Differential tests against data produced by the original game
	@if [ -d "$(IMHOTEP_GAME_DIR)/Maps" ]; then ./$(BUILD)/terrain_check && ./$(BUILD)/terrain_check --campaign && ./$(BUILD)/save_check && ./$(BUILD)/desirability_check; \
	else echo "differential: skipped ($(IMHOTEP_GAME_DIR) not found)"; fi
	@if [ -x local/venv/bin/python ] && [ -d "$(IMHOTEP_GAME_DIR)/Maps" ]; then $(MAKE) -s oracle-check; \
	else echo "oracle checks: skipped (make oracle-env)"; fi

oracle-check: $(BUILD)/save_dump $(BUILD)/desirability_check $(BUILD)/state_run $(BUILD)/state_load $(BUILD)/state_save $(BUILD)/state_replay ## Differential tests against the original run by the oracle (needs make oracle-env)
	local/venv/bin/python tests/differential/oracle_load_check.py --pak | tail -1
	local/venv/bin/python tests/differential/oracle_load_check.py | tail -1
	local/venv/bin/python tests/differential/oracle_function_check.py | tail -1
	local/venv/bin/python tests/differential/oracle_save_check.py | tail -1
	local/venv/bin/python tests/differential/oracle_hover_check.py | tail -1
	local/venv/bin/python tests/differential/oracle_command_check.py -j 8 | tail -1
	local/venv/bin/python tools/oracle/lockstep.py local/saves/downloaded/1592/39-Actium-Dec30c.sav --ticks 51 --chained | tail -1
	local/venv/bin/python tools/oracle/lockstep.py local/saves/downloaded/1564/CaesarAlan-Iken.sav --ticks 51 --chained | tail -1

oracle-env: ## Python venv in local/venv with Unicorn and Capstone (tools/oracle, docs/oracle.md)
	python3 -m venv local/venv
	local/venv/bin/pip install -q unicorn==2.1.4 capstone

corpus: $(BUILD)/save_dump ## Reference save corpus: player saves advanced 12 months by the original, headless (tools/oracle/corpus.py)
	find local/saves/downloaded -name '*.sav' -print0 | xargs -0 -n 1 -P 8 local/venv/bin/python tools/oracle/corpus.py --months 12

test: check-files provenance unit differential ## All checks
