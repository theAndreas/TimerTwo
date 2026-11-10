#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Installs the Arduino core of tools/project.sh, pinned, and builds what prepare() there builds
# beyond it. The dev container runs it when it is created, CI before it compiles an example;
# run it again after ARDUINO_CORE has changed.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

main() {
  if [[ $# -gt 0 ]]; then
    err "Usage: $0"
    exit 2
  fi

  local urls=()
  if [[ -n "${ARDUINO_BOARD_URLS}" ]]; then
    urls=(--additional-urls "${ARDUINO_BOARD_URLS}")
  fi
  arduino-cli core update-index "${urls[@]}"
  arduino-cli core install "${ARDUINO_CORE}" "${urls[@]}"
  prepare
}

main "$@"
