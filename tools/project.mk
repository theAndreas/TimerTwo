# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The project's own targets, which the Makefile includes; the template wrote this file once, and
# `copier update` leaves it alone. A target with a `## ` comment is listed by `make`, and one
# added to PROJECT_CHECKS is part of `make check`:
#
#   PROJECT_CHECKS += sizes
#
#   sizes: ## The README's table of flash and RAM against the build
#   	tools/sizes.sh
