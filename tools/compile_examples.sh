#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Compiles every example in each variant and prints flash and RAM of each, as a Markdown table.
# Fails on a warning outside the Arduino core, and where check_elf() of tools/project.sh fails
# on what was built. When every example compiles, the table goes to build/sizes.md as well, which
# tools/documented_sizes.py compares with the README's (`make sizes`); else that file is removed,
# so that no table of an older build is taken for this one's.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

# Prints the number after "<label> use(s)" in the output of arduino-cli.
size_of() {
  local label="$1"
  local output="$2"
  sed -n "s/^${label} uses* \([0-9]*\) bytes.*/\1/p" <<< "${output}"
}

main() {
  if [[ $# -gt 0 ]]; then
    err "Usage: $0"
    exit 2
  fi

  local failed=0
  local example
  local variant
  local output
  local warnings
  local elf
  local -A flash_size
  local -A ram_size
  local listed
  listed="$(list_examples)"
  local examples
  mapfile -t examples <<< "${listed}"
  local -r sizes="${BUILD_DIR}/sizes.md"
  rm -f "${sizes}"

  for example in "${examples[@]}"; do
    for variant in "${VARIANTS[@]}"; do
      if ! output="$(compile_example "${variant}" "${example}")"; then
        err "${example} (${variant}) does not compile:"
        err "${output}"
        failed=1
        continue
      fi
      warnings="$(grep 'warning:' <<< "${output}" \
        | grep -v '/cores/arduino/' || true)"
      if [[ -n "${warnings}" ]]; then
        err "${example} (${variant}) warns:"
        err "${warnings}"
        failed=1
      fi
      elf="$(elf_of "${variant}" "${example}")"
      check_elf "${variant}" "${elf}" || failed=1
      flash_size["${example}/${variant}"]="$(size_of 'Sketch' "${output}")"
      ram_size["${example}/${variant}"]="$(size_of 'Global variables' "${output}")"
    done
  done

  local header="| Example |"
  local rule="|---|"
  for variant in "${VARIANTS[@]}"; do
    header+=" Flash, ${variant} |"
    rule+="---|"
  done
  for variant in "${VARIANTS[@]}"; do
    header+=" RAM, ${variant} |"
    rule+="---|"
  done
  local table="${header}"$'\n'"${rule}"
  local row
  for example in "${examples[@]}"; do
    row="| ${example} |"
    for variant in "${VARIANTS[@]}"; do
      row+=" ${flash_size["${example}/${variant}"]:-} |"
    done
    for variant in "${VARIANTS[@]}"; do
      row+=" ${ram_size["${example}/${variant}"]:-} |"
    done
    table+=$'\n'"${row}"
  done
  echo "${table}"
  if ((failed == 0)); then
    mkdir -p "${BUILD_DIR}"
    echo "${table}" > "${sizes}"
  fi
  exit "${failed}"
}

main "$@"
