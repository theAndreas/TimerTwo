# The index of what builds and checks this repository.
#
# Nothing is decided here: the CMake builds are presets in CMakePresets.json, the Python tools
# are pinned in uv.lock, and each check is a script in tools/. This file only names them, so
# that a developer and CI type the same: every step of the workflows that builds or checks
# something is one of these targets. `make` alone lists them.
#
# Warnings are errors in CI and under `make check`, and not otherwise: the ci- presets, chosen
# by STRICT. GitHub Actions sets CI=true in every job, which is where the default comes from.
# The other checks fail on a finding always: clang-tidy, cppcheck and the compiled examples.
#
# This file comes from the template and is brought up to date with `copier update`. A target of
# the project's own goes into tools/project.mk, included at the top; added to PROJECT_CHECKS, it
# is part of `make check` as well.

SHELL := bash
.SHELLFLAGS := -e -o pipefail -c
.DEFAULT_GOAL := help
# One tool at a time: several of them configure the same build directory, and each runs its
# compiler or analyser on every core already.
.NOTPARALLEL:

STRICT ?= $(CI)
PRESET = $(if $(STRICT),ci-)
PROJECT_CHECKS :=

# Before the rules, as `check` reads PROJECT_CHECKS when it is read; its targets are listed after
# these all the same, as `make help` reads the files in turn.
-include tools/project.mk

.PHONY: help check lint fmt setup tests clang sanitize coverage examples sizes self-check \
        tidy cppcheck headers docs arduino-lint clean

help: ## This list
	@awk 'BEGIN { FS = ":.*## " } /^## / { printf "\n%s\n", substr($$0, 4) } \
	      /^[a-z0-9-]+:.*## / { printf "  make %-12s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@echo
	@echo '  STRICT=1 for warnings as errors, as in CI.'

## Everything at once

# Cheap first, so that a typo fails in seconds rather than after the analysers.
check: STRICT := 1
check: ## What CI runs, all of it
check: lint arduino-lint tests clang tidy cppcheck headers docs sanitize coverage examples \
       sizes self-check $(PROJECT_CHECKS)

lint: ## The checks of seconds: clang-format, cpplint, shellcheck, actionlint, ruff
	tools/lint.sh

fmt: ## Format the C++ sources and the Doxygen blocks in place
	tools/lint.sh --fix

setup: ## The pinned Arduino core, and what tools/project.sh prepares beyond it
	tools/setup.sh

## Builds, each with its tests

tests: ## The tests on the host, by GCC
	cmake --workflow --preset $(PRESET)default

clang: ## The same tests by clang, which warns about other things
	cmake --workflow --preset $(PRESET)clang

sanitize: ## The tests under AddressSanitizer and UBSan
	cmake --workflow --preset sanitize

# Into the job's summary as well when there is one, which is CI's.
coverage: ## The tests with --coverage, and what they reach of src/
	tools/coverage.sh
	if [[ -n "$${GITHUB_STEP_SUMMARY:-}" ]]; then \
	    { echo '```text'; cat build/coverage/coverage.txt; echo '```'; } >> "$$GITHUB_STEP_SUMMARY"; \
	fi

examples: ## Every example in each variant, failing on a warning; flash and RAM as a table
	tools/compile_examples.sh | tee -a "$${GITHUB_STEP_SUMMARY:-/dev/null}"

# The table of the last `make examples`, not a build of its own, so that `make check` compiles
# the examples once; FIX=1 writes it into the README.
sizes: ## The table of flash and RAM in README.md against the last make examples
	python3 tools/documented_sizes.py $(if $(FIX),--fix)

self-check: ## examples/SelfCheck in simavr, in each variant
	tools/self_check.sh

## Analysers

tidy: ## clang-tidy: the library and the examples with the board's flags, and the tests
	tools/tidy.sh

cppcheck: ## cppcheck over the library
	tools/cppcheck.sh

# The host's database from CMake, the board's from tools/tidy.sh: each header is compiled by
# the compiler that database names, so an #if only the board takes is compiled for the board.
headers: ## Every header of src/ compiled on its own, for the host and for the board
	cmake --preset default >/dev/null
	tools/tidy.sh --database-only >/dev/null
	python3 tools/self_contained_headers.py build/host build/tidy/default

docs: ## The reference from the Doxygen blocks, failing on a gap
	tools/docs.sh

# What git has, tracked or new, copied into build/arduino-lint/<name>: the Library Manager sees
# the repository, not the working tree with its build/ and .venv, whose links arduino-lint would
# report.
arduino-lint: ## The rules of the Arduino Library Manager
	tools/arduino_lint.sh

## Housekeeping

clean: ## Every build directory
	rm -rf build
