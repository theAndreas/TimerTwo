# Base image, overridable via build.args in a devcontainer.json variant so that
# site-specific base images can be used without touching this file.
#
# The tag has no hyphen. devcontainers/cpp publishes ubuntu-24.04 and ubuntu24.04 both,
# but from 26.04 only the unhyphenated form exists, so the older spelling looks like the
# newest release when it is not. The base image carries build-essential and gdb: the host
# g++ builds the tests against the mock of the core, and whatever tools/project.sh prepares
# for the build, such as a GCC plugin against the plugin headers of the AVR core's avr-gcc.
ARG BASE_IMAGE=mcr.microsoft.com/devcontainers/cpp:ubuntu26.04
FROM ${BASE_IMAGE}

# CMake, Ninja and GoogleTest build and run the tests on the host, clang a second time beside
# the base image's g++ (make clang); simavr runs examples/SelfCheck as the board would
# (tools/self_check.sh); jq writes the compilation database of tools/tidy.sh; Doxygen builds
# the reference (tools/docs.sh), and cppcheck reads the library beside clang-tidy
# (tools/cppcheck.sh); neither of the two is on PyPI. From apt
# rather than pinned like the tools below: the distribution's versions are what CI installs
# as well, and none of them has a bug that forces a newer release.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake ninja-build libgtest-dev clang simavr jq doxygen cppcheck && \
    rm -rf /var/lib/apt/lists/*

# arduino-cli compiles the library and its examples with the real Arduino AVR core, which
# is the one check the tests on the host cannot replace: they build against a mock of
# Arduino.h, not against avr-gcc, and flash is RAM there. Not gcc-avr from apt - the AVR
# core brings its own toolchain, and that one is what the Arduino IDE compiles a user's
# sketch with.
# A pinned release tarball with its published checksum, for the same reason as gh below:
# the trust stays anchored to something visible in this file, at the cost of bumping the
# version by hand.
ARG ARDUINO_CLI_VERSION=1.5.1
ARG ARDUINO_CLI_SHA256_AMD64=28a8e119c498a25607821c36cb2dc49e8463941b261a0d99091baa7bc692dd2b
ARG ARDUINO_CLI_SHA256_ARM64=1e69e077479f300614d4551334e0a33f08ee40b04315d83b8e7e0e94f0d0ee62
RUN ARCH="$(dpkg --print-architecture)" && \
    case "${ARCH}" in \
        amd64) ASSET=64bit; SHA256="${ARDUINO_CLI_SHA256_AMD64}" ;; \
        arm64) ASSET=ARM64; SHA256="${ARDUINO_CLI_SHA256_ARM64}" ;; \
        *) echo "arduino-cli: no pinned checksum for ${ARCH}; add one or drop arduino-cli" >&2; exit 1 ;; \
    esac && \
    curl -fsSL -o /tmp/arduino-cli.tar.gz \
        "https://github.com/arduino/arduino-cli/releases/download/v${ARDUINO_CLI_VERSION}/arduino-cli_${ARDUINO_CLI_VERSION}_Linux_${ASSET}.tar.gz" && \
    echo "${SHA256}  /tmp/arduino-cli.tar.gz" | sha256sum -c - && \
    tar -xzf /tmp/arduino-cli.tar.gz -C /usr/local/bin arduino-cli && \
    rm /tmp/arduino-cli.tar.gz

# arduino-lint checks library.properties, keywords.txt and the folder layout against the
# rules the Arduino Library Manager applies, so a broken library is found here rather than
# when the registry rejects it. Pinned the same way as arduino-cli.
ARG ARDUINO_LINT_VERSION=1.3.0
ARG ARDUINO_LINT_SHA256_AMD64=181671ca174988f2601e1cdf2b40a552682db8eaa047de33117e67e70338b63e
ARG ARDUINO_LINT_SHA256_ARM64=d886fd734762f43b90990ff0ec12d65e088f74516db1a1a9c3c2de6b7fffdada
RUN ARCH="$(dpkg --print-architecture)" && \
    case "${ARCH}" in \
        amd64) ASSET=64bit; SHA256="${ARDUINO_LINT_SHA256_AMD64}" ;; \
        arm64) ASSET=ARM64; SHA256="${ARDUINO_LINT_SHA256_ARM64}" ;; \
        *) echo "arduino-lint: no pinned checksum for ${ARCH}; add one or drop arduino-lint" >&2; exit 1 ;; \
    esac && \
    curl -fsSL -o /tmp/arduino-lint.tar.gz \
        "https://github.com/arduino/arduino-lint/releases/download/${ARDUINO_LINT_VERSION}/arduino-lint_${ARDUINO_LINT_VERSION}_Linux_${ASSET}.tar.gz" && \
    echo "${SHA256}  /tmp/arduino-lint.tar.gz" | sha256sum -c - && \
    tar -xzf /tmp/arduino-lint.tar.gz -C /usr/local/bin arduino-lint && \
    rm /tmp/arduino-lint.tar.gz

# gh turns branch and pull request housekeeping into something doable from the container
# rather than by hand in a browser.
# A pinned release .deb, not the universe package - that one is 2.46, whose `gh pr edit`
# still asks for the retired Projects-classic `projectCards` field and therefore fails
# outright. dpkg rather than apt because the package Depends on git alone, which the base
# image carries - so this needs no package lists.
ARG GH_VERSION=2.97.0
ARG GH_SHA256_AMD64=7c7fa3bb890db0934baf65910d97b8c0fa437b2e590f7f7daf6bdf82c5c486d7
ARG GH_SHA256_ARM64=0ba7a76739c865d82ebde24667d875d9b8caa55db47c7597c24accdd4defd2bb
RUN ARCH="$(dpkg --print-architecture)" && \
    case "${ARCH}" in \
        amd64) SHA256="${GH_SHA256_AMD64}" ;; \
        arm64) SHA256="${GH_SHA256_ARM64}" ;; \
        *) echo "gh: no pinned checksum for ${ARCH}; add one or drop gh" >&2; exit 1 ;; \
    esac && \
    curl -fsSL -o /tmp/gh.deb \
        "https://github.com/cli/cli/releases/download/v${GH_VERSION}/gh_${GH_VERSION}_linux_${ARCH}.deb" && \
    echo "${SHA256}  /tmp/gh.deb" | sha256sum -c - && \
    dpkg -i /tmp/gh.deb && \
    rm /tmp/gh.deb

# uv makes the environment of the Python tools from pyproject.toml and uv.lock, in the
# workspace's .venv, on the first `uv run` or with `uv sync --frozen` when the container is
# created: clang-format, clang-tidy, cpplint, gcovr, shellcheck, actionlint, ruff and copier,
# at the versions CI checks with. This ARG is the one place uv's version is written:
# .github/actions/uv-sync reads it for CI. Pinned with its published checksum, as the tools
# above.
ARG UV_VERSION=0.12.0
ARG UV_SHA256_AMD64=eaf842262aa1c418d8ecc5605f02ee1ebfd369124fa48548e85f9481a47831a9
ARG UV_SHA256_ARM64=2c5d6e3092cc5223b10ff403880cc75121bf64e84644e7a0c69f643b0d89ac95
RUN ARCH="$(dpkg --print-architecture)" && \
    case "${ARCH}" in \
        amd64) TRIPLE=x86_64-unknown-linux-gnu; SHA256="${UV_SHA256_AMD64}" ;; \
        arm64) TRIPLE=aarch64-unknown-linux-gnu; SHA256="${UV_SHA256_ARM64}" ;; \
        *) echo "uv: no pinned checksum for ${ARCH}; add one or drop uv" >&2; exit 1 ;; \
    esac && \
    curl -fsSL -o /tmp/uv.tar.gz \
        "https://github.com/astral-sh/uv/releases/download/${UV_VERSION}/uv-${TRIPLE}.tar.gz" && \
    echo "${SHA256}  /tmp/uv.tar.gz" | sha256sum -c - && \
    tar -xzf /tmp/uv.tar.gz -C /usr/local/bin --strip-components=1 \
        "uv-${TRIPLE}/uv" "uv-${TRIPLE}/uvx" && \
    rm /tmp/uv.tar.gz

# clangd gives the editor completion, navigation and the findings of clang-tidy, with the
# .clang-tidy of the repository, reading the compilation databases that .clangd names.
# From PyPI through uv, pinned, as clang-tidy in tools/tidy.sh; 22 is the newest there, one
# behind the clang-tidy 23 of CI, which stays the measure. The package wraps the binary in
# a Python script whose interpreter would lie in root's home, out of the remote user's
# reach, so the link goes to the binary itself, which finds its headers next to it.
ARG CLANGD_VERSION=22.1.1
RUN UV_TOOL_DIR=/opt/uv/tools UV_PYTHON_INSTALL_DIR=/opt/uv/python \
        UV_TOOL_BIN_DIR=/opt/uv/bin \
        uv tool install --no-cache "clangd==${CLANGD_VERSION}" && \
    ln -s "$(find /opt/uv/tools/clangd -path '*/clangd/data/bin/clangd' -type f)" \
        /usr/local/bin/clangd && \
    clangd --version

# The headers of LLVM, clang and clang-tidy that tools/lib/tidy_module.sh compiles the checks of
# our own against, as a module that the clang-tidy of uv.lock loads. LLVM_MAJOR is that
# clang-tidy's major version: tools/lib/tidy_module.sh fails, saying so, when uv.lock moves on and
# this does not. CI's action llvm-headers reads it from here, so there is one place that says
# which LLVM. llvm-headers.sh says where they come from, and why not pinned closer.
ARG LLVM_MAJOR=23
COPY llvm-headers.sh /tmp/llvm-headers.sh
RUN /tmp/llvm-headers.sh "${LLVM_MAJOR}" "/opt/llvm-${LLVM_MAJOR}/include" && \
    rm /tmp/llvm-headers.sh

# The Arduino data directory and uv's cache are named volumes: the AVR core and its
# toolchain, and the checkers from PyPI, are downloaded on first use, and the volumes are
# what keep a rebuild from fetching them again. An empty named volume takes its ownership
# from the path it covers, so the path has to exist and belong to the remote user first,
# or nothing can write into it. `install -d` gives only the last component to the owner and
# creates missing parents as root, so ~/.cache is named on its own: a base image without it
# would otherwise leave it to root, and arduino-cli could not put its build cache there.
RUN install -d -o vscode -g vscode -m 0755 /home/vscode/.arduino15 && \
    install -d -o vscode -g vscode -m 0755 /home/vscode/.cache /home/vscode/.cache/uv

# The same arrangement for the logins a rebuild would otherwise throw away: Codex and Claude
# Code, whose extensions the shared feature installs, and gh, which every push and every pull request
# goes through. Mode 0700 rather than 0755, because what these hold is a credential and not
# a cache - and gh refuses to read a config directory others can enter. ~/.config is
# named on its own, as ~/.cache above, so that it does not fall to root.
RUN install -d -o vscode -g vscode -m 0700 /home/vscode/.codex && \
    install -d -o vscode -g vscode -m 0700 /home/vscode/.claude && \
    install -d -o vscode -g vscode -m 0755 /home/vscode/.config && \
    install -d -o vscode -g vscode -m 0700 /home/vscode/.config/gh

# The shell history, a volume as well, so that a rebuild keeps it. 0700 too: a command line
# can carry a token.
RUN install -d -o vscode -g vscode -m 0700 /home/vscode/.commandhistory

# GitHub's SSH host key, so that the first git operation in a fresh container verifies
# against something rather than failing with "Host key verification failed", because
# ~/.ssh belongs to the container and is empty after every rebuild.
#
# Written out rather than fetched with ssh-keyscan at build time: a scan believes whatever
# answers it, which is precisely what a known host key is there to prevent. The fingerprint
# below is the one GitHub publishes under "GitHub's SSH key fingerprints", and
# `ssh-keygen -lf /etc/ssh/ssh_known_hosts` prints it back out of the file.
#
# Three names for one key: github.com, ssh.github.com for SSH on port 443 in networks that
# block 22, and the bracketed form, because an entry is keyed by host *and* port.
ARG GITHUB_HOST_KEY="ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMqqnkVzrm0SdG6UOoqKLsabgH5C9okWi0dh2l9GKJl"
RUN printf '# SHA256:+DiY3wvvV6TuJJhbpZisF/zLDA0zPMSvHdkr4UvCOqU\n%s %s\n%s %s\n%s %s\n' \
        'github.com' "${GITHUB_HOST_KEY}" \
        'ssh.github.com' "${GITHUB_HOST_KEY}" \
        '[ssh.github.com]:443' "${GITHUB_HOST_KEY}" \
        >> /etc/ssh/ssh_known_hosts && \
    chmod 0644 /etc/ssh/ssh_known_hosts

# The project's own packages, from apt-packages.txt beside this file, which the project's
# workflows install as well. Last, so that a change to the list builds this layer alone and none
# of the downloads above. Nothing to do for an empty list.
COPY apt-packages.txt /tmp/apt-packages.txt
RUN packages="$(sed -e 's/#.*//' /tmp/apt-packages.txt | xargs)" && \
    if [ -n "${packages}" ]; then \
        apt-get update && \
        DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ${packages} && \
        rm -rf /var/lib/apt/lists/*; \
    fi && \
    rm /tmp/apt-packages.txt

ENV TZ=Europe/Berlin

# Extensions, editor settings and lifecycle commands live in the local feature
# in shared/, which every devcontainer.json variant references. A
# devcontainer.metadata label set here would not work: clients read image
# metadata from the image the final FROM points at, then overwrite the label on
# the image they build from this file.
