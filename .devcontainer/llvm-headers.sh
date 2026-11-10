#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Puts the headers of LLVM, clang and clang-tidy of one major version into a folder: what the
# clang-tidy module in tools/lib/tidy_module/ is compiled against. The clang-tidy from PyPI loads a
# module (--load) and exports the symbols one needs, but comes without headers. The Dockerfile
# runs this, and the action llvm-headers in CI, so that both have the same.
#
# Usage:
#   llvm-headers.sh MAJOR DEST
#
# The headers come from apt.llvm.org, from llvm-MAJOR-dev and libclang-MAJOR-dev, unpacked and
# not installed: they depend on all of LLVM, and only the headers are wanted. apt reads the
# archive with a state and a configuration of its own in a temporary folder, so the system's
# sources, lists and hooks are left as they are, and no root is needed but to write DEST. Its
# signature is checked against the key whose fingerprint is below, the trust anchored in this
# file as the Dockerfile anchors its downloads.
#
# Not pinned to a patch release: apt.llvm.org keeps only the newest build of a release branch,
# and an older one is gone the day a newer comes. Within a major version LLVM keeps its ABI, so
# headers of 23.1.6 build a module that the clang-tidy 23.1.4 of uv.lock loads.
# tools/lib/tidy_module.sh checks the major version.

set -euo pipefail

# Sylvestre Ledru's key, which signs apt.llvm.org.
readonly KEY_URL="https://apt.llvm.org/llvm-snapshot.gpg.key"
readonly KEY_FINGERPRINT="6084F3CF814B57C1CF12EFD515CF4D18AF4F7421"

main() {
  if [[ $# -ne 2 ]]; then
    echo "Usage: $0 MAJOR DEST" >&2
    exit 2
  fi
  local major="$1"
  local dest="$2"
  local codename
  codename="$(sed -n 's/^VERSION_CODENAME=//p' /etc/os-release)"

  local tmp
  tmp="$(mktemp -d)"
  # shellcheck disable=SC2064  # expanded now, as tmp is local
  trap "rm -rf '${tmp}'" EXIT

  # gpg's home in the temporary folder as well, so that none is left in the image.
  export GNUPGHOME="${tmp}/gnupg"
  mkdir -m 0700 "${GNUPGHOME}"
  curl -fsSL "${KEY_URL}" | gpg --quiet --dearmor > "${tmp}/llvm.gpg"
  local fingerprint
  fingerprint="$(gpg --quiet --show-keys --with-colons "${tmp}/llvm.gpg" \
    | awk -F: '$1 == "fpr" { print $10; exit }')"
  if [[ "${fingerprint}" != "${KEY_FINGERPRINT}" ]]; then
    echo "llvm-headers: the key at ${KEY_URL} is ${fingerprint}, not ${KEY_FINGERPRINT}" >&2
    exit 1
  fi

  echo "deb [signed-by=${tmp}/llvm.gpg] https://apt.llvm.org/${codename}/" \
    "llvm-toolchain-${codename}-${major} main" > "${tmp}/sources.list"
  mkdir -p "${tmp}/lists/partial" "${tmp}/cache/archives/partial" "${tmp}/debs" \
    "${tmp}/apt.conf.d"
  # Through APT_CONFIG, which apt reads before /etc/apt/apt.conf.d: on the command line, the
  # parts would be read already, and with them the docker-clean of an Ubuntu image, whose hook
  # empties the system's cache after an update.
  cat > "${tmp}/apt.conf" <<EOF
Dir::Etc::Parts "${tmp}/apt.conf.d";
Dir::Etc::SourceList "${tmp}/sources.list";
Dir::Etc::SourceParts "-";
Dir::State::Lists "${tmp}/lists";
Dir::Cache "${tmp}/cache";
Dir::Cache::Archives "${tmp}/cache/archives";
APT::Sandbox::User "root";
EOF
  export APT_CONFIG="${tmp}/apt.conf"
  apt-get -q update >/dev/null
  (cd "${tmp}/debs" && apt-get -q download "llvm-${major}-dev" "libclang-${major}-dev" >/dev/null)

  local deb
  for deb in "${tmp}"/debs/*.deb; do
    dpkg-deb -x "${deb}" "${tmp}/root"
  done
  # usr/lib/llvm-MAJOR/include links to usr/include/llvm-MAJOR for llvm and llvm-c: -L copies what
  # the links point to.
  rm -rf "${dest}"
  mkdir -p "${dest}"
  cp -RL "${tmp}/root/usr/lib/llvm-${major}/include/." "${dest}/"
  echo "llvm-headers: $(sed -n 's/^#define LLVM_VERSION_STRING "\(.*\)"/\1/p' \
    "${dest}/llvm/Config/llvm-config.h") in ${dest}"
}

main "$@"
