#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks the library against the rules of the Arduino Library Manager, as arduino-lint does in
# CI. Not the working tree: the Library Manager sees the repository, and the working tree has
# build/ and .venv beside it, whose links arduino-lint reports as a library's. So what git has,
# tracked or new and not ignored, is copied into a folder of the library's name in build/, and
# that is checked. With --copy-only it makes the copy, prints its path and stops, for a workflow
# that checks it with arduino-lint's action.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

main() {
  local copy_only=0
  if [[ "${1:-}" == "--copy-only" ]]; then
    copy_only=1
  elif [[ $# -gt 0 ]]; then
    err "Usage: $0 [--copy-only]"
    exit 2
  fi

  cd "${ROOT_DIR}"
  local name
  name="$(sed -n 's/^name=//p' library.properties)"
  if [[ -z "${name}" ]]; then
    err "library.properties names no library."
    exit 1
  fi
  local copy="${BUILD_DIR}/arduino-lint/${name}"
  rm -rf "${BUILD_DIR}/arduino-lint"
  mkdir -p "${copy}"
  git ls-files -z --cached --others --exclude-standard \
    | tar --null --files-from=- --create --file=- \
    | tar --extract --file=- --directory="${copy}"
  if ((copy_only)); then
    echo "${copy}"
    exit 0
  fi
  arduino-lint --library-manager submit --compliance strict "${copy}"
}

main "$@"
