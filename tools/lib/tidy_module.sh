#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the clang-tidy module in tools/lib/tidy_module/, the checks of our own, and prints the path
# of the shared library that clang-tidy loads with --load. Built again only when a source has
# changed; tools/tidy.sh calls this before it runs clang-tidy.
#
# Usage:
#   tools/lib/tidy_module.sh [OUTPUT_DIR]
#
# OUTPUT_DIR is build/tidy/module if not given.
#
# The module is compiled against the headers of the LLVM whose major version the clang-tidy of
# uv.lock has, which the dev container keeps in /opt/llvm-<major>/include and CI's action
# llvm-headers in LLVM_INCLUDE_DIR; .devcontainer/llvm-headers.sh puts them there. The clang-tidy
# from PyPI exports the symbols of LLVM and clang that a module needs. Without RTTI, as LLVM is
# built.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly ROOT_DIR
readonly SOURCE_DIR="${ROOT_DIR}/tools/lib/tidy_module"

err() {
  echo "$*" >&2
}

main() {
  local output_dir="${1:-${ROOT_DIR}/build/tidy/module}"
  local major
  major="$(uv run --frozen --project "${ROOT_DIR}" clang-tidy --version \
    | sed -n 's/.*LLVM version \([0-9]*\)\..*/\1/p')"
  local include="${LLVM_INCLUDE_DIR:-/opt/llvm-${major}/include}"
  local config="${include}/llvm/Config/llvm-config.h"
  if [[ ! -f "${config}" || ! -f "${include}/clang-tidy/ClangTidyCheck.h" ]]; then
    err "tidy_module: no headers of LLVM ${major} in ${include}. The dev container has them" \
      "once ARG LLVM_MAJOR of .devcontainer/Dockerfile is ${major}; or run" \
      ".devcontainer/llvm-headers.sh ${major} DIR and set LLVM_INCLUDE_DIR=DIR."
    exit 1
  fi
  local header_major
  header_major="$(sed -n 's/^#define LLVM_VERSION_MAJOR \([0-9]*\)/\1/p' "${config}")"
  if [[ "${header_major}" != "${major}" ]]; then
    err "tidy_module: the headers in ${include} are of LLVM ${header_major}, clang-tidy is of" \
      "LLVM ${major}."
    exit 1
  fi

  local module="${output_dir}/house.so"
  local listed
  listed="$(find "${SOURCE_DIR}" -name '*.cpp' | sort)"
  local sources
  mapfile -t sources <<< "${listed}"
  # Again when a source, a header of ours or the headers of LLVM are newer than the module.
  local newer=""
  if [[ -f "${module}" ]]; then
    newer="$(find "${SOURCE_DIR}" "${config}" -newer "${module}" -print -quit)"
  fi
  if [[ ! -f "${module}" || -n "${newer}" ]]; then
    mkdir -p "${output_dir}"
    "${CXX:-c++}" -std=c++17 -O2 -fPIC -shared -fno-rtti \
      -Wall -Wextra -Werror \
      -isystem "${include}" \
      "${sources[@]}" -o "${module}.tmp"
    mv "${module}.tmp" "${module}"
  fi
  echo "${module}"
}

main "$@"
