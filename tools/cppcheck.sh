#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs cppcheck on the library, as a second analyser beside clang-tidy: clang-tidy sees exactly
# what the compiler sees, cppcheck parses the tree by itself and works out the configurations of
# the #ifs, which is how it reaches bounds and branches the clang checks do not follow. It is
# given the include folders, not a compilation database, the mock of the core standing in for
# the real one. cppcheck comes from apt, in the Dockerfile and in CI alike, as it is not on PyPI.
#
# --enable=warning,portability: what is wrong, not how it is written, which clang-tidy and the
# style tools say. --inline-suppr, so that a finding is answered next to the code, with the
# reason there: // cppcheck-suppress <id> above the line.
#
# -D__has_include(x)=1: cppcheck knows __has_include from C++17 only and reports a syntax error
# on it before, where GCC and Clang offer it in every standard. 1, as cppcheck does not read the
# system headers anyway (missingIncludeSystem), so the branch that includes them is the one read.
#
# A project whose code cppcheck cannot parse sets CPPCHECK=0 in tools/project.sh, with the
# reason: a syntax error ends the analysis of the whole translation unit, so suppressing it
# would leave nothing checked and still pass.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

main() {
  if [[ $# -gt 0 ]]; then
    err "Usage: $0"
    exit 2
  fi
  if ((CPPCHECK == 0)); then
    echo "cppcheck.sh: CPPCHECK=0 in tools/project.sh, nothing to run."
    exit 0
  fi
  if ! command -v cppcheck >/dev/null; then
    err "cppcheck is missing; it comes from apt, see the Dockerfile."
    exit 1
  fi

  cd "${ROOT_DIR}"
  local defines=()
  local variant
  local flag
  # The defines of the variants, so that the branches each takes are read as well.
  for variant in "${TIDY_VARIANTS[@]}"; do
    while IFS= read -r flag; do
      defines+=("${flag}")
    done < <(variant_flags "${variant}" | grep -E '^-D' || true)
  done

  # The headers as well, by name: given a folder, cppcheck reads only its .cpp
  # files, and a library of templates may have none.
  local files
  list_files files '\.(cpp|h)$' 'src/*'

  mkdir -p "${BUILD_DIR}/cppcheck"
  cppcheck \
    --enable=warning,portability \
    --check-level=exhaustive \
    --language=c++ \
    --std=c++11 \
    --inline-suppr \
    --suppress=missingIncludeSystem \
    --error-exitcode=1 \
    --quiet \
    --cppcheck-build-dir="${BUILD_DIR}/cppcheck" \
    -Isrc -Itest/mock \
    '-D__has_include(x)=1' \
    "${defines[@]}" \
    "${files[@]}"
}

main "$@"
