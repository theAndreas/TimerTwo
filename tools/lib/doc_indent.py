#!/usr/bin/env python3
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
"""Indents the lines that continue a command of a Doxygen block under the text after it.

The commands are \\param, \\return, \\pre and the others of docs/Documentation.md; a line that
continues one starts where the text after the command starts:

    * \\return The view of them, or IndexError::kOutOfRange if `position` lies
    *         beyond the end.

clang-format keeps such a line as it is, but when it breaks one that has grown too long, the
new line starts at the margin again; so tools/lint.sh runs this after clang-format with --fix,
and with --check fails where a line is not indented so. The paragraph after a command is
rewrapped to 80 columns, counted in characters as clang-format counts them; \\code ...
\\endcode and the lines of a list are left as they are. The block ends where */ stands, at
the start of a line or after its text, as the comment does.

Usage:
    tools/lib/doc_indent.py --fix FILE...
    tools/lib/doc_indent.py --check FILE...
"""

import argparse
import re
import sys
from pathlib import Path

WIDTH = 80
COMMAND = (r'\\(?:param(?:\[[a-z,]+\])?[ ]+\S+|tparam[ ]+\S+|retval[ ]+\S+'
           r'|return|pre|post|note|warning|stddiff|see|brief)')
# ASCII: a byte of UTF-8 is never white space, as it is not for clang-format.
BLOCK_START = re.compile(r'\s*/\*!(\s|$)', re.ASCII)
# A line that starts with it; the comment ends wherever */ stands.
BLOCK_END = re.compile(r'\s*\*/', re.ASCII)
COMMAND_LINE = re.compile(rf'(\s*)\* ({COMMAND})[ ]+(\S.*)$', re.ASCII)
CONTINUED = re.compile(r'\s*\*[ ]+(\S.*)$', re.ASCII)
NOT_CONTINUED = re.compile(r'\s*\*[ ]+(\\|-[ ])', re.ASCII)
# A code span in backticks is one word, never broken across lines; a backtick that opens none
# is a character of the word it stands in.
WORD = re.compile(r'(?:`[^`]*`|\S)+', re.ASCII)


def lines_of(text):
    """The lines of text, each with its \\n; split at \\n only, as a CR or a form feed is text."""
    lines = [line + '\n' for line in text.split('\n')]
    lines[-1] = lines[-1][:-1]
    return lines if lines[-1] else lines[:-1]


def paragraph_end(lines, start):
    """The index of the line after the paragraph that goes on from lines[start].

    The paragraph goes on to an empty line, the next command, a list or the end of the block,
    which a line that starts with */ ends after the paragraph and one that ends with it within.
    """
    end = start
    while (end < len(lines) and '*/' not in lines[end - 1] and CONTINUED.match(lines[end])
           and not NOT_CONTINUED.match(lines[end]) and not BLOCK_END.match(lines[end])):
        end += 1
    return end


def wrapped(margin, command, paragraph):
    """The lines of a command's paragraph, wrapped to WIDTH and indented under its text."""
    continued = f'{margin}* ' + ' ' * (len(command) + 1)
    out = []
    current = f'{margin}* {command} '
    empty = True
    for word in WORD.findall(paragraph):
        if not empty and len(current) + 1 + len(word) > WIDTH:
            out.append(current + '\n')
            current = continued + word
        else:
            current += ('' if empty else ' ') + word
        empty = False
    out.append(current + '\n')
    return out


def indented(lines):
    """The lines of a file with every command's paragraph indented."""
    out = []
    in_block = False
    in_code = False
    i = 0
    while i < len(lines):
        start = i
        line = lines[i]
        i += 1
        if not in_block:
            # The line that opens the block is in it: a \code there opens an example.
            in_block = bool(BLOCK_START.match(line)) and '*/' not in line
            if not in_block:
                out.append(line)
                continue
        if '\\code' in line:
            in_code = True
        match = None if in_code else COMMAND_LINE.match(line)
        if in_code and '\\endcode' in line:
            in_code = False
        if match:
            margin, command, text = match.groups()
            i = paragraph_end(lines, i)
            continued = [CONTINUED.match(lines[j])[1] for j in range(start + 1, i)]
            paragraph = ' '.join([text, *continued])
            out.extend(wrapped(margin, command, paragraph))
        else:
            out.append(line)
        if any('*/' in lines[j] for j in range(start, i)):
            in_block = in_code = False
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--fix', action='store_true', help='indent the files in place')
    mode.add_argument('--check', action='store_true', help='fail where a file is not indented')
    parser.add_argument('files', type=Path, nargs='*', metavar='FILE')
    arguments = parser.parse_args()

    failed = 0
    for file in arguments.files:
        # newline='': a line ends where it ends in the file, \r\n stays \r\n.
        with file.open(encoding='utf-8', newline='') as source:
            text = source.read()
        result = ''.join(indented(lines_of(text)))
        if result == text:
            continue
        if arguments.fix:
            with file.open('w', encoding='utf-8', newline='') as target:
                target.write(result)
        else:
            print(f'{file}: a line that continues a Doxygen command is not indented under its '
                  'text; tools/lint.sh --fix indents it')
            failed = 1
    return failed


if __name__ == '__main__':
    sys.exit(main())
