# TimerTwo: for a coding agent

Periodic interrupt, PWM and time readout on Timer2 of the ATmega328P. An embedded C++ library for targets from an 8-bit microcontroller to the host:
C++11 as the least standard, without the standard library. This project builds and checks it on
the host by GCC and clang, and on the AVR through arduino-cli and simavr.

## Before writing code, read

- [docs/CodingStyle.md](docs/CodingStyle.md): how the code is written. Every rule has an id;
  name it where you deviate, in the `NOLINT` or in the comment that gives the reason.
- [docs/Documentation.md](docs/Documentation.md): how a comment and a Doxygen block are written.
  Everything a sketch can use is documented.
- [docs/Testing.md](docs/Testing.md): which kind of test proves what, and how a test is named
  and written.
- [docs/targets/avr.md](docs/targets/avr.md): what holds on the AVR only, beyond the rules
  that hold for every target.

They hold without exception. Where they and your habits differ, they win.

## How to work

**Quality before speed.** A change that is right and small is worth more than one that is
large and quick. Read the code you change and what calls it before you write; check what you
assume, as an API, a signature or a size, instead of guessing; run every check, not the first;
and where a decision is not yours to make, or the rules leave it open, ask rather than pick.

1. **A test first.** Write the test of the behaviour, see it fail, then write the code.
2. **Small steps.** One concern per change; what you find on the way is a note in your answer,
   not a change of its own.
   A commit holds one concern, and its message says what changed and why.
   A pull request's description says what changes for a user of the library, why, and how it
   was checked; not the code: no function names, no walk through the files, no snippets. The
   diff shows those.
3. **Only what was asked.** No refactoring, renaming or reformatting of code you were not asked
   to touch.

After every step:

```bash
make lint      # clang-format, cpplint, shellcheck, actionlint, ruff: seconds
make tidy      # clang-tidy, for the host and the board
make tests     # GoogleTest on the host
```

`make fmt` formats the code and the Doxygen blocks in place.

## Never

- **Nothing written anew that exists already**: look in the project first, then the standard
  library where every target has it, then the library like it that the project uses, as the
  ETL or Utl (M-3). Only what none has is written, once and for reuse, in a module folder of its
  own with its test; say in your answer what you wrote and why nothing existing would do.
- **No heap, no exceptions, no RTTI**: no `new`, `malloc`, `throw`, `try`, `dynamic_cast`,
  `typeid` (M-1, M-2).
- **No blocking and no `float` without its reason**: no busy wait in the library, no `double`
  at all (H-1, H-7). An interrupt's shared data is read atomically (H-6).
- **No long expressions**: split a computation or a condition into named steps (T-18).
- **No invented API.** A function of the Arduino core or of a library is used only if you have
  seen its declaration; `test/mock/` shows what the tests can call.
- **No code for later**: no unused parameter, function, overload or option "in case", no
  `TODO`, no commented-out code.
- **No comment that repeats the code.** A comment says why, Documentation.md says how.
- **No test weakened to pass**: a failing test is fixed in the code, unless the test is wrong,
  and then you say why in your answer.
- **No check silenced to pass**: no `NOLINT`, `#pragma` or change of `.clang-tidy`,
  `.clang-format`, `CPPLINT.cfg`, the `Makefile` or the CMake files to make a finding go away.
  Where a finding is wrong, suppress it for the one line, naming the check, the rule of
  CodingStyle.md and the reason (S-1), and say so in your answer.
- **No change to the template's files** (below) unless asked.

## Done

A change is done when all of these hold, not when it compiles:

- `make check` passes: it is what CI runs, all of it.
- Every line and branch the change adds is reached by a test: `make coverage` lists the lines
  missed per file.
- What a sketch can use is documented (Documentation.md), and `make docs` passes.
- The README's table of flash and RAM, if it has one, is what the build measures: after a change
  that moves them, `make examples` and `make sizes FIX=1`.
- What a user of the library notices is in `CHANGELOG.md`, if the project keeps one.
- Your answer names every rule you deviated from, and why.

## Where things are

| What | Where |
|---|---|
| The library | `src/`, the header a sketch includes is `src/TimerTwo.h` |
| Tests | `test/`, GoogleTest against a mock of the Arduino core in `test/mock/` |
| Examples | `examples/`; `SelfCheck` runs on the board in simavr |
| The project's settings | `tools/project.sh` (board, variants, hooks), `tools/project.mk` (own targets) |
| The scripts behind `make` | `tools/`; `tools/README.md` says what each is, and when a script is shell or Python |

`.clang-tidy`, `.clang-format`, `CPPLINT.cfg`, the `Makefile`, `tools/` and `.github/` come from
the template and are updated by `copier update`. Change them only when asked; a project's own
exception goes at the end of a list, with its reason.

## The project's own notes

<!-- What an agent has to know of this project beyond the template: its design, its terms, what
     not to touch. The template wrote this section once; an update keeps what is added here. -->

TimerTwo drives Timer/Counter2 of the ATmega328P in mode 5, phase correct PWM: OCR2A is the top
and sets the period with the prescaler, the overflow at the bottom calls the sketch's callback,
and OC2B on pin 3 gives the PWM. `OIL.md` is the open issue list: what is planned, in which order,
and why. A pull request that works on an item sets its status and links itself in the same PR.

- **The hardware is a parameter** (CodingStyle H-3): `timer_two::Timer<Registers>` in
  `src/timer_two/timer.h` drives any 8-bit Timer/Counter whose registers `Registers` returns;
  `src/timer_two/timer2.h` hands in Timer/Counter2 as `Timer2Registers`, and `timer2.cpp` defines
  the one object a sketch uses, `timer2`, made when compiling, and the overflow interrupt that
  calls it. A test makes a `Timer2` of its own, on the mock's registers set back in `SetUp()`.
- **Errors** are `timer_two::Error` in a `Status`, or a `Result` for `Read()`, as MaxMatrix has
  them: no conversion to `bool`, the texts in flash.
- **The mock of the registers**, `test/mock/avr/io.h`: TCNT2 steps up to OCR2A and down again
  while a clock is selected, on each read or on every n-th, so that `Start()` and `Read()`, which
  wait for the counter to move, end on the host.
- **The clocks**: `CMakeLists.txt` builds `test/ClockTest.cpp` at 1 and 20 MHz besides the
  Uno's 16, as the code computes with quotients of `F_CPU`; its expectations are computed from
  `F_CPU`, so that they hold at any clock.
- **No self check in simavr yet** (OIL 5): simavr 1.6 does not count Timer2 in mode 5.
