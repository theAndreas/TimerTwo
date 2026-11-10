#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs clang-tidy on the library and the examples, with the flags arduino-cli compiles them with
# for the board, once for each of TIDY_VARIANTS in tools/project.sh, and on the tests on the
# host; the checks are in .clang-tidy. With --database-only it writes the compilation database
# of the first variant and stops: clangd reads it for the examples, see .clangd. clang-tidy
# runs from the environment uv.lock pins, like the tools of lint.sh.
#
# arduino-cli compiles copies of a sketch's files in its build folder, and a finding there would
# point to the copy. So the flags are taken from its compilation database, and a database of our
# own names the files in the repository; an .ino is compiled as C++ with Arduino.h included
# first, which is what arduino-cli adds to it.
#
# The library is read with the board's flags, not only through the host's database: an int has
# 16 bits on the AVR and 32 on the host, and a check that only saw the host would never meet the
# code as the board does.
#
# The checks of our own, house-*, are a module of clang-tidy's that tools/lib/tidy_module.sh builds
# from tools/lib/tidy_module/ and clang-tidy loads; the database alone does not need it.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly CLANG_TIDY=(uv run --frozen clang-tidy)
readonly TIDY_DIR="${BUILD_DIR}/tidy"

# Prints, as a JSON array, the arguments arduino-cli compiles a file of the sketch
# DATABASE_EXAMPLE with in VARIANT, without the file, its output and -w, and changed for clang.
# Compiled with the defines of the variant, as arduino-cli finds the libraries a sketch needs by
# its includes: one that only a variant includes, as SPI under a switch, has its folder in the
# database of that variant alone. Only the defines: a plugin or an optimization of avr-gcc means
# nothing to clang.
# - The include folders outside the repository, the core's and the board's, become -isystem,
#   so that nothing is reported from headers we do not own.
# - -Wall -Wextra, so that clang's own warnings are reported as well.
# - -Wno-unknown-attributes: clang does not know avr-gcc's attribute __progmem__, which
#   PROGMEM stands for, and takes the tables as in RAM. That changes nothing clang-tidy checks.
arduino_flags() {
  local variant="$1"
  local defines
  defines="$(variant_flags "${variant}" | { grep -E '^-D' || true; } | paste -sd ' ' -)"
  local args=(
    --fqbn "${FQBN}"
    --library "${ROOT_DIR}"
    --only-compilation-database
    --build-path "${TIDY_DIR}/arduino/${variant}"
  )
  if [[ -n "${defines}" ]]; then
    args+=(--build-property "compiler.cpp.extra_flags=${defines}")
  fi
  arduino-cli compile "${args[@]}" "${ROOT_DIR}/examples/${DATABASE_EXAMPLE}" >/dev/null
  jq --arg root "${ROOT_DIR}" '[.[] | select(.file | endswith(".ino.cpp"))][0]
    | .file as $file
    | .arguments
    | del(.[index("-o"), index("-o") + 1])
    | map(select(. != $file and . != "-w"))
    | map(if startswith("-I") and (startswith("-I" + $root) | not)
          then "-isystem" + .[2:] else . end)
    + ["-Wall", "-Wextra", "-Wno-unknown-attributes"]' \
    "${TIDY_DIR}/arduino/${variant}/compile_commands.json"
}

# Writes <dir>/compile_commands.json for the files given, with the flags of arduino_flags().
# Arguments:
#   dir, flags, then the files
write_database() {
  local dir="$1"
  local flags="$2"
  shift 2
  mkdir -p "${dir}"
  printf '%s\n' "$@" | jq -R . | jq -s \
    --arg dir "${ROOT_DIR}" --argjson flags "${flags}" '
    map({
      directory: $dir,
      file: .,
      arguments: ($flags
        + (if endswith(".ino") then ["-x", "c++", "-include", "Arduino.h"]
           else [] end)
        + [.])
    })' > "${dir}/compile_commands.json"
}

main() {
  local database_only=0
  if [[ "${1:-}" == "--database-only" ]]; then
    database_only=1
  elif [[ $# -gt 0 ]]; then
    err "Usage: $0 [--database-only]"
    exit 2
  fi

  cd "${ROOT_DIR}"
  mkdir -p "${TIDY_DIR}"
  local sources
  list_files sources '\.(cpp|ino)$' 'src/*' 'examples/*'
  local clang_tidy=("${CLANG_TIDY[@]}")
  if ((!database_only)); then
    local module
    module="$(tools/lib/tidy_module.sh)"
    clang_tidy+=("--load=${module}")
  fi

  # Each variant takes other branches of #if. The first is the one clangd reads, see .clangd.
  local failed=0
  local variant
  local flags
  for variant in "${TIDY_VARIANTS[@]}"; do
    flags="$(arduino_flags "${variant}")"
    write_database "${TIDY_DIR}/${variant}" "${flags}" "${sources[@]}"
    if ((database_only)); then
      echo "Wrote ${TIDY_DIR}/${variant}/compile_commands.json"
      exit 0
    fi
    echo "clang-tidy, ${variant}:"
    "${clang_tidy[@]}" -p "${TIDY_DIR}/${variant}" --quiet \
      "${sources[@]}" || failed=1
  done

  # The tests on the host, against the mock, with the database CMake writes. Only what lies in
  # test/: the tests are C++17, and the headers of the library, checked above as the gnu++11
  # they are, would be held to C++17. Not the mock in test/mock/, which stands in for the
  # Arduino core and follows it, not the Google style. Nor what lies in test/compile_fail/,
  # which must not compile and is in no database. And the code of HOST_CODE, which CMake builds
  # beside the tests, and its headers.
  local host_code
  host_code_dirs host_code
  local paths=('test/*' ':!test/mock/*' ':!test/compile_fail/*')
  local filter='test'
  local dir
  for dir in "${host_code[@]}"; do
    paths+=("${dir}/*")
    filter+="|${dir}"
  done
  local tests
  list_files tests '\.cpp$' "${paths[@]}"
  cmake --preset default >/dev/null
  echo "clang-tidy, the tests on the host:"
  "${clang_tidy[@]}" -p "${BUILD_DIR}/host" --quiet \
    --header-filter="(^|/)(${filter})/" \
    "${tests[@]}" || failed=1
  exit "${failed}"
}

main "$@"
