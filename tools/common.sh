#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Shared by the scripts in tools/: where things lie, and how an example is compiled in each
# variant. The defaults below are what a project gets that says nothing else; tools/project.sh,
# sourced after them, is where it says otherwise. This file comes from the template and is
# brought up to date with `copier update`; what is the project's goes into tools/project.sh.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly BUILD_DIR="${ROOT_DIR}/build"

# The defaults, each described in tools/project.sh.
# shellcheck disable=SC2034  # used by the scripts that source this file
{
  NAMESPACE=""
  FQBN="arduino:avr:uno"
  ARDUINO_CORE="arduino:avr"
  ARDUINO_BOARD_URLS=""
  SIMAVR_MCU="atmega328p"
  SIMAVR_FREQUENCY=16000000
  DATABASE_EXAMPLE="Basics"
  SELF_CHECK_EXAMPLE="SelfCheck"
  VARIANTS=(default)
  TIDY_VARIANTS=(default)
  DOCS_INTERNAL_API=""
  CPPCHECK=1
  HOST_CODE=()
}

# The hooks, which do nothing unless tools/project.sh defines them again, and
# then are never called: SC2329.
# shellcheck disable=SC2329
{
  variant_flags() {
    :
  }

  prepare() {
    :
  }

  check_elf() {
    :
  }
}

# shellcheck source=tools/project.sh
source "${ROOT_DIR}/tools/project.sh"

err() {
  echo "$*" >&2
}

# Prints the name of every example, one per line.
list_examples() {
  local dir
  for dir in "${ROOT_DIR}"/examples/*/; do
    basename "${dir}"
  done
}

# Prints the bin directory of the avr-gcc that arduino-cli uses for FQBN.
avr_gcc_bin_dir() {
  local properties
  properties="$(arduino-cli compile --fqbn "${FQBN}" --show-properties \
    "${ROOT_DIR}/examples/${DATABASE_EXAMPLE}" 2>/dev/null)"
  sed -n 's/^compiler\.path=//p' <<< "${properties}"
}

# Fills the array named first with the files git has below the pathspecs whose paths match the
# regex: tracked ones and new ones not yet added, but not what .gitignore names, so that a file
# is checked before its first commit. Fails where git fails or no file matches, as a check over
# no file would pass.
# Arguments:
#   array: the name of the array to fill
#   regex: an extended regular expression, such as '\.(cpp|h)$'
#   pathspec...: what git lists, such as 'src/*'
list_files() {
  local -n list_files_into="$1"
  local regex="$2"
  shift 2
  local listed
  listed="$(git ls-files --cached --others --exclude-standard -- "$@")"
  if ! listed="$(grep -E "${regex}" <<< "${listed}")"; then
    err "No file in $* matches ${regex}."
    return 1
  fi
  # shellcheck disable=SC2034  # a nameref: what it fills is the caller's array
  mapfile -t list_files_into <<< "${listed}"
}

# Fills the array named first with the folders of HOST_CODE, each without the / at its end. Fails
# where one is no folder, as a misspelt one would be checked by nothing, without a word.
# Arguments:
#   array: the name of the array to fill
host_code_dirs() {
  local -n host_code_dirs_into="$1"
  host_code_dirs_into=()
  local dir
  for dir in "${HOST_CODE[@]}"; do
    dir="${dir%/}"
    if [[ ! -d "${ROOT_DIR}/${dir}" ]]; then
      err "HOST_CODE in tools/project.sh names ${dir}, which is no folder of the project."
      return 1
    fi
    host_code_dirs_into+=("${dir}")
  done
}

# Compiles an example in a variant into BUILD_DIR/<variant>/<example> and prints what
# arduino-cli prints. The flags of the variant go into compiler.cpp.extra_flags, which reaches
# every C++ file of the build, the core's as well. Its callers test its status, which turns off
# set -e within it, so it returns on a failure itself.
# Arguments:
#   variant: one of VARIANTS
#   example: the name of a folder in examples/
compile_example() {
  local variant="$1"
  local example="$2"
  local args=(
    --fqbn "${FQBN}"
    --library "${ROOT_DIR}"
    --warnings all
    --build-path "${BUILD_DIR}/${variant}/${example}"
  )
  local flags
  flags="$(variant_flags "${variant}")" || return
  flags="${flags//$'\n'/ }"
  if [[ -n "${flags}" ]]; then
    args+=(--build-property "compiler.cpp.extra_flags=${flags}")
  fi
  arduino-cli compile "${args[@]}" "${ROOT_DIR}/examples/${example}" 2>&1
}

# Prints the path of the ELF file that compile_example made.
elf_of() {
  local variant="$1"
  local example="$2"
  echo "${BUILD_DIR}/${variant}/${example}/${example}.ino.elf"
}
