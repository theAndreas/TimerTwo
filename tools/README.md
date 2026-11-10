# tools/

The scripts behind the targets of the `Makefile`, which a developer and CI alike call them
through.

## What lies where

| File | What it is |
|---|---|
| `setup.sh` | `make setup`: the pinned Arduino core, and what `prepare()` of `project.sh` builds |
| `lint.sh` | `make lint` and `make fmt`: clang-format, cpplint, the Doxygen indent, shellcheck, actionlint, ruff |
| `compile_examples.sh` | `make examples`: every example in each variant; the table of flash and RAM |
| `documented_sizes.py` | `make sizes`: the README's table against the last `make examples` |
| `self_check.sh` | `make self-check`: examples/SelfCheck in simavr |
| `coverage.sh` | `make coverage`: the tests with --coverage |
| `tidy.sh` | `make tidy`: clang-tidy, and the board's compilation database |
| `cppcheck.sh` | `make cppcheck` |
| `self_contained_headers.py` | `make headers`: every header compiled on its own |
| `docs.sh` | `make docs`: Doxygen, failing on a gap |
| `arduino_lint.sh` | `make arduino-lint` |
| `common.sh` | What the shell scripts share: the paths, the defaults of `project.sh`, compiling an example. A script of the project's own sources it as well |
| `project.sh` | The project's: the board, the core, the variants and the hooks |
| `project.mk` | The project's: its own targets of the `Makefile` |
| `lib/` | What the scripts above call, a target of none: `doc_indent.py` for `lint.sh`, `tidy_module.sh` and the clang-tidy module in `tidy_module/` for `tidy.sh` |

`project.sh` and `project.mk` are the project's own: the template wrote them once, and
`copier update` leaves them alone. Everything else here comes from the template and is brought up
to date with `copier update`; a change of the project's own goes into `project.sh`, `project.mk`
or a script of its own beside them.

## Shell or Python

- **Shell drives the tools:** a script that calls arduino-cli, cmake, clang-tidy or simavr,
  passes their flags on and reports their status. Bash, with `set -euo pipefail`; it sources
  `common.sh`.
- **Python reads and writes data:** a compilation database, a table, the text of a source. The
  standard library only, so that it runs without `uv sync`.

shellcheck reads every `.sh` here, with the checks `.shellcheckrc` turns on: the output of a
command goes into a variable before it is used, as `mapfile < <(...)` never sees the command
fail; a list of files comes from `list_files` in `common.sh`. ruff reads every `.py`.
