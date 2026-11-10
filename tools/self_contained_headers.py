#!/usr/bin/env python3
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compiles every header of src/ on its own, as the only include of an otherwise empty file.

A header that uses something it does not include still compiles in the build, as long as every
file that includes it happens to have included that something first. Nothing notices until the
include it leaned on goes, and then what breaks is a file nobody touched; misc-include-cleaner
cannot see it either, as it asks what a file uses, not whether the file compiles without its
neighbours.

So each header is compiled alone, by the compiler and with the flags of a compilation database:
the host's, which CMake writes (g++, gnu++11, against the mock of the core), and the board's,
which tools/tidy.sh writes from arduino-cli's (avr-g++, the core and the board's pins). An #if
that only the board takes is so compiled for the board.

Usage:
    tools/self_contained_headers.py build/host build/tidy/default
"""

import argparse
import concurrent.futures
import json
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# What a compile of one source is told about its output and its input; a syntax check writes
# nothing, and -MF would write a dependency file over the build's own.
WITH_VALUE = {'-o', '-MF', '-MT', '-MQ', '-include', '-x'}
ALONE = {'-c', '-MD', '-MMD'}


def arguments_of(entry):
    return entry['arguments'] if 'arguments' in entry else shlex.split(entry['command'])


def library_entry(database):
    """The flags the library is compiled with: a source of src/, else the mock of the core,
    which the host's library target compiles with the same flags, else a sketch, whose
    `-include Arduino.h` is dropped below, as a header has to include what it uses itself."""
    def under(entry, part, suffix):
        path = (Path(entry['directory']) / entry['file']).resolve().as_posix()
        return part in path and path.endswith(suffix)

    for part, suffix in (('/src/', '.cpp'), ('/test/mock/', '.cpp'), ('/examples/', '.ino')):
        for entry in database:
            if under(entry, part, suffix):
                return entry
    raise SystemExit('self_contained_headers.py: no source of the library in the database')


def syntax_check(arguments, source):
    """The compile of `source` as C++ with the same compiler and flags, writing nothing."""
    result = [arguments[0]]
    skip = False
    for argument in arguments[1:]:
        if skip:
            skip = False
        elif argument in WITH_VALUE:
            skip = True
        elif argument in ALONE or argument.endswith(('.cpp', '.c', '.ino')):
            pass
        elif argument.startswith('-isystem'):
            # tools/tidy.sh makes the core's folders -isystem, so that clang-tidy reports nothing
            # of them; but avr-gcc reads a header there as extern "C", and Arduino.h then
            # conflicts with avr-libc. As arduino-cli gives them: -I.
            result.append('-I' + argument[len('-isystem'):])
        else:
            result.append(argument)
    return result + ['-x', 'c++', '-fsyntax-only', str(source)]


def check(header, arguments, directory, scratch):
    """The first error of compiling `header` alone, or None."""
    name = header.relative_to(ROOT).as_posix().replace('/', '_')
    source = Path(scratch) / f'{name}.cpp'
    source.write_text(f'#include "{header}"\n')
    result = subprocess.run(syntax_check(arguments, source), cwd=directory,
                            capture_output=True, text=True, check=False)
    if result.returncode == 0:
        return None
    errors = [line for line in result.stderr.splitlines() if 'error' in line]
    return errors[0] if errors else result.stderr.strip().splitlines()[-1]


def check_database(build, headers):
    """The headers that do not compile alone with the flags of `build`, with their errors."""
    database = json.loads((build / 'compile_commands.json').read_text())
    entry = library_entry(database)
    arguments = arguments_of(entry)
    print(f'self_contained_headers.py: {len(headers)} headers by '
          f'{os.path.basename(arguments[0])}, as {build}', flush=True)
    with tempfile.TemporaryDirectory() as scratch, \
            concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as pool:
        results = pool.map(
            lambda header: (header, check(header, arguments, entry['directory'], scratch)),
            headers)
        return [(header, error) for header, error in results if error]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('builds', nargs='+', type=Path,
                        help='the directories holding a compile_commands.json')
    arguments = parser.parse_args()

    headers = sorted((ROOT / 'src').rglob('*.h'))
    failures = []
    for build in arguments.builds:
        failures += [(build, header, error)
                     for header, error in check_database(build, headers)]
    for build, header, error in failures:
        print(f'{header.relative_to(ROOT)}: does not compile on its own, as {build}\n'
              f'    {error}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
