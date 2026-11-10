#!/bin/sh
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The editor metadata in devcontainer-feature.json is the whole of this feature;
# the tools are installed by ../Dockerfile, and the Arduino AVR core by the
# postCreateCommand, as it lives in a volume rather than in the image.

set -e

echo "TimerTwo shared configuration: nothing to install."
