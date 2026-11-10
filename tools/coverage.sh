#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs the tests on the host with --coverage and reports which lines of src/
# they reach: the lines no test reaches on the terminal, and all of it as HTML
# in build/coverage/html/. The variants of CMakeLists.txt count together, as
# each takes other branches of #if. gcovr runs from the environment uv.lock
# pins, like the tools of lint.sh.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly COVERAGE_DIR="${BUILD_DIR}/coverage"

# Removes the notes (.gcno) of a source that no longer exists, renamed or
# deleted: the build writes notes only for what it compiles and never
# removes old ones, and gcovr reads every one it finds, so the lines of a
# header that only such a file reached stood as missed. A note lies at
# <dir>/CMakeFiles/<target>.dir/<source>.gcno, which gives its source: <dir>
# is the build directory of the CMakeLists.txt that adds the target, empty for
# the root's and <subdir>/ for one of add_subdirectory(<subdir>), and <source>
# is relative to that CMakeLists.txt.
remove_stale_notes() {
  local notes
  notes="$(find "${COVERAGE_DIR}" -name '*.gcno')"
  local note
  local subdir
  local source
  while IFS= read -r note; do
    subdir="${note#"${COVERAGE_DIR}"/}"
    subdir="${subdir%%CMakeFiles/*}"
    source="${note#*.dir/}"
    source="${ROOT_DIR}/${subdir}${source%.gcno}"
    if [[ -n "${note}" && ! -f "${source}" ]]; then
      rm -f "${note}"
    fi
  done <<< "${notes}"
}

# Fails on a line of src/ excluded from coverage without its reason (Testing.md TS-4): a marker
# that starts an exclusion is followed by a colon and the reason, as
# `// GCOVR_EXCL_LINE: the bus may hang, no test can make it`. The end of a block needs none.
check_exclusions() {
  local bare
  bare="$(grep -rnE 'GCOVR_EXCL_(BR_)?(LINE|START)' src/ \
    | grep -vE 'GCOVR_EXCL_(BR_)?(LINE|START): [^ ]' || true)"
  if [[ -n "${bare}" ]]; then
    err "An exclusion from coverage without its reason (Testing.md TS-4):"
    err "${bare}"
    exit 1
  fi
}

main() {
  cd "${ROOT_DIR}"
  check_exclusions
  # Counts from an earlier run would add to this one.
  find "${COVERAGE_DIR}" -name '*.gcda' -delete 2>/dev/null || true
  cmake --workflow --preset coverage
  remove_stale_notes

  # No counts is a build that was not instrumented, not a library without code: a build
  # directory configured with another compiler is made anew, and CMake drops the preset's
  # options with its old cache, --coverage among them. gcovr would report 0 of 0 lines and pass.
  if ! find "${COVERAGE_DIR}" -name '*.gcda' -print -quit | grep -q .; then
    err "No coverage counts in ${COVERAGE_DIR}: the build was not instrumented."
    err "Remove ${COVERAGE_DIR} and run this again."
    exit 1
  fi

  mkdir -p "${COVERAGE_DIR}/html"
  # separate: a function under an #if of a variant lies on another line in
  # each, and is two functions, not one to merge.
  # --merge-lines: gcov counts a template per instance, and each lambda is a
  # type of its own, so a line reached by one instance stood as missed by
  # another; merged, a line counts as reached if any instance reaches it.
  # --exclude-throw-branches: on the host a call that may throw has a branch
  # to its exception, which no test takes; a sketch is built without
  # exceptions, so on the AVR there is no such branch to reach.
  uv run --frozen gcovr --root "${ROOT_DIR}" --filter 'src/' \
    --merge-mode-functions=separate \
    --merge-lines \
    --exclude-throw-branches \
    --txt-metric line --txt "${COVERAGE_DIR}/coverage.txt" \
    --html-details "${COVERAGE_DIR}/html/index.html" \
    --print-summary "${COVERAGE_DIR}"
  echo
  # The files with lines missed, and which.
  grep -v ' 100%' "${COVERAGE_DIR}/coverage.txt" || true
}

main "$@"
