#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks the style of the C++ sources (clang-format, cpplint, and the indent
# of the lines that continue a Doxygen command, tools/lib/doc_indent.py), of the
# scripts (shellcheck, with the checks .shellcheckrc turns on) and of the CI
# workflow (actionlint). With --fix, formats the C++ sources instead of only
# checking them. The Python scripts are read by ruff. The tools run from the
# environment uv.lock pins, through `uv run --frozen`, so that every machine
# checks the same; doc_indent.py needs only Python's standard library.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly UV_RUN=(uv run --frozen)

main() {
  local fix=0
  if [[ "${1:-}" == "--fix" ]]; then
    fix=1
  elif [[ $# -gt 0 ]]; then
    err "Usage: $0 [--fix]"
    exit 2
  fi

  cd "${ROOT_DIR}"
  # The library, its examples and tests, and the code of HOST_CODE beside them.
  local host_code
  host_code_dirs host_code
  local paths=('src/*' 'examples/*' 'test/*')
  local dir
  for dir in "${host_code[@]}"; do
    paths+=("${dir}/*")
  done
  local sources
  list_files sources '\.(h|cpp|ino)$' "${paths[@]}"
  local scripts
  list_files scripts '\.sh$' 'tools/*.sh'

  local failed=0
  # The lines that continue a command of a Doxygen block start under its text,
  # docs/Documentation.md. clang-format keeps them so, but a line it breaks
  # starts at the margin again, so the indent comes after it.
  if ((fix)); then
    "${UV_RUN[@]}" clang-format -i "${sources[@]}"
    tools/lib/doc_indent.py --fix "${sources[@]}"
  else
    "${UV_RUN[@]}" clang-format --dry-run --Werror \
      "${sources[@]}" || failed=1
    tools/lib/doc_indent.py --check "${sources[@]}" || failed=1
  fi
  "${UV_RUN[@]}" cpplint --quiet "${sources[@]}" || failed=1
  "${UV_RUN[@]}" shellcheck -x "${scripts[@]}" || failed=1
  "${UV_RUN[@]}" actionlint || failed=1
  "${UV_RUN[@]}" ruff check --quiet || failed=1
  exit "${failed}"
}

main "$@"
