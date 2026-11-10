#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the reference from the Doxygen blocks into build/docs/html and fails on a warning: a
# block Doxygen cannot read, a \param that names no parameter, an entity of the API without a
# block. Two warnings are let pass and counted. "Is not documented" in NAMESPACE::internal of
# tools/project.sh, which a sketch does not use; but not in a file DOCS_INTERNAL_API names, which
# is API after all. And a missing \return: a one-line block whose \brief says what comes back,
# "Returns the number of elements.", needs none, while every parameter needs its \param.
# Doxygen cannot be told to fail on some warnings and not others, so they go into a log and
# are sorted here. Doxygen comes from apt, in the Dockerfile and in CI alike, as it is not on
# PyPI.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly DOCS_DIR="${BUILD_DIR}/docs"
readonly LOG="${DOCS_DIR}/warnings.log"

# Prints the warnings that fail, one per line, with paths from the root.
failing_warnings() {
  # grep ends with 1 on a log without a line, which pipefail would take for
  # a failure.
  sed "s#^${ROOT_DIR}/##" "${LOG}" \
    | { grep -v '^$' || true; } \
    | INTERNAL="${NAMESPACE}::internal" API="${DOCS_INTERNAL_API}" awk '
        # From the environment, as -v would read the backslashes of the regex as escapes.
        BEGIN { internal = ENVIRON["INTERNAL"]; api = ENVIRON["API"] }
        internal != "::internal" && /is not documented\.$/ \
            && index($0, internal) && !(api != "" && $0 ~ api) { next }
        /warning: return type of member .* is not documented$/ { next }
        { print }'
}

main() {
  if [[ $# -gt 0 ]]; then
    err "Usage: $0"
    exit 2
  fi
  if ! command -v doxygen >/dev/null; then
    err "doxygen is missing; it comes from apt, see the Dockerfile."
    exit 1
  fi

  cd "${ROOT_DIR}"
  mkdir -p "${DOCS_DIR}"
  doxygen Doxyfile

  local all
  all="$(grep -c 'warning:' "${LOG}" || true)"
  local failing
  failing="$(failing_warnings)"
  if [[ -n "${failing}" ]]; then
    echo "${failing}"
  fi
  local count=0
  if [[ -n "${failing}" ]]; then
    count="$(wc -l <<< "${failing}")"
  fi
  echo "docs.sh: ${count} warnings that fail, $((all - count)) gaps let" \
    "pass: in ${NAMESPACE}::internal, or a missing return;" \
    "the reference is in ${DOCS_DIR}/html."
  if ((count > 0)); then
    exit 1
  fi
}

main "$@"
