#!/usr/bin/env python3
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks the table of flash and RAM in README.md against the one the last build measured.

A size written in prose goes stale the same way every time: the code grows and the sentence
does not, and nothing notices. So the table of the examples that tools/compile_examples.sh
prints is not copied into the README by hand but checked: what stands between the markers

    <!-- sizes -->
    | Example | Flash, ... |
    ...
    <!-- /sizes -->

has to be the table of the last `make examples`, which writes it to build/sizes.md, byte for
byte. Exactly, not within a tolerance: CI and the dev container compile with the same pinned
core, ARDUINO_CORE of tools/project.sh, and so with the same avr-gcc. A README without the
markers has nothing to check. --fix writes the measured table in.

Only the table is checked. A size in a sentence is left alone: it is usually a comparison or a
measurement of a sketch changed for the purpose, history rather than the state of the build.

Usage:
    tools/documented_sizes.py [--fix] [build/sizes.md]
"""

import argparse
import difflib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
README = ROOT / 'README.md'
BLOCK = re.compile(r'(<!-- sizes -->\n)(.*?)(<!-- /sizes -->)', re.DOTALL)


def shown(path):
    """The path as the project names it, where it lies in the project."""
    path = path.resolve()
    return path.relative_to(ROOT) if path.is_relative_to(ROOT) else path


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('table', type=Path, nargs='?', default=ROOT / 'build' / 'sizes.md',
                        help='the table of the last make examples (default: build/sizes.md)')
    parser.add_argument('--fix', action='store_true', help='write the measured table into README.md')
    arguments = parser.parse_args()

    text = README.read_text(encoding='utf-8')
    blocks = list(BLOCK.finditer(text))
    if not blocks:
        print('documented_sizes.py: README.md has no <!-- sizes --> block, nothing to check.')
        return 0
    if len(blocks) > 1:
        print('documented_sizes.py: README.md has more than one <!-- sizes --> block; '
              'the table of the examples belongs in one place.', file=sys.stderr)
        return 1
    if not arguments.table.is_file():
        print(f'documented_sizes.py: {shown(arguments.table)} is missing; `make examples` writes it '
              'when every example compiles.', file=sys.stderr)
        return 1

    block = blocks[0]
    documented = block.group(2)
    measured = arguments.table.read_text(encoding='utf-8')
    if not measured.endswith('\n'):
        measured += '\n'
    if documented == measured:
        print('documented_sizes.py: the table in README.md is as measured.')
        return 0

    diff = difflib.unified_diff(documented.splitlines(keepends=True),
                                measured.splitlines(keepends=True),
                                'README.md, as documented', f'{shown(arguments.table)}, as measured')
    sys.stdout.writelines(diff)
    # Before the verdict on stderr, which is not buffered.
    sys.stdout.flush()
    if arguments.fix:
        README.write_text(text[:block.start(2)] + measured + text[block.end(2):], encoding='utf-8')
        print('documented_sizes.py: the measured table is written into README.md.')
        return 0
    print('\ndocumented_sizes.py: the table in README.md is not what the build measures; '
          '`make sizes FIX=1` writes it in.', file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
