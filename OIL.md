# Open Issue List

What is planned for TimerTwo, in the order it can be done. A pull request that works on an item
sets its status and links itself in the same PR; an item is `done` once that PR is merged. New
items go at the end with the next free number, so a number never changes meaning.

Status: `open`, `in progress`, `done`, `dropped` (with the reason in the item).

## Items

| # | Item | Depends on | Status | PR |
|---|------|------------|--------|----|
| 1 | On the template embedded-cpp-template | – | in progress | #4 |
| 2 | The known bugs, each shown by a test first, and SelfCheck in simavr | 1 | open | |
| 3 | Version 2.0.0: a namespace, constants for macros, the template's checks | 2 | open | |
| 4 | Release 2.0.0 and the Library Manager | 3 | open | |

### 1 On the template embedded-cpp-template

The checks, CI and the dev container from the copier template
[embedded-cpp-template](https://github.com/theAndreas/embedded-cpp-template), which Utl and
MaxMatrix share, so that a change to one reaches the others with `copier update`; the template's
roadmap names it R-3. The library layout 1.5, `src/` and `examples/`, in place of the AtmelStudio
project; GPL-3.0-or-later as the licence, which TimerTwo had none of; the default branch `main`.

The code keeps its API and its behaviour: clang-format once, the warnings of `-Wconversion` and
`-Wsign-conversion`, the documentation of the API, and what cpplint finds. The groups of
clang-tidy that the code of 2023 fails wait for 3, in `src/.clang-tidy` and
`examples/.clang-tidy`. Tests on the host against a mock of Timer2's registers hold what works
today; what does not is 2.

*Done when* `make check` passes, and flash and RAM of the examples on the Uno are those of
before, or differ by what a commit names.

### 2 The known bugs, each shown by a test first, and SelfCheck in simavr

Found while bringing TimerTwo in. Each gets a test that fails on the code of 1, then the fix:

- `resume()` lets the counter go on, but leaves the state `STATE_STOPPED`: `read()` refuses, and
  `start()` starts again from 0.
- `stop()` before `init()` sets `STATE_STOPPED`, and `start()` then starts a timer that was
  never set up.
- `start()` enables the interrupt again after `detachInterrupt()`, as the callback stays set.
- A duty of 255 sets OCR2B below OCR2A, 249 of 250 at 1 ms: the output is never high all the
  time. And the duty is a share of OCR2A that `setPeriod()` does not set again.
- `read()` divides before it multiplies by the prescaler: at 16 MHz the steps below 16 are lost,
  so it reads 24 us for 25.
- `F_CPU / 2000000uL` is 0 below 2 MHz, so every period gives a top of 0; and
  `F_CPU / 1000000uL` truncates at 20 MHz, where `getPeriodMax()` is 24575 for 26214.
- A period of 0 sets a top of 0: the counter stands, and `start()` with a callback waits for it
  forever.

Then `examples/SelfCheck`, which tools/self_check.sh runs in simavr: the period counted in
callbacks and the PWM measured on pin 3, if simavr models mode 5 of Timer2 well enough;
`SIMAVR_MCU` in `tools/project.sh` is empty until then.

### 3 Version 2.0.0: a namespace, constants for macros, the template's checks

`timer_two::Timer2` with Google's names, in `src/timer_two/`, without `StandardTypes.h`;
`constexpr` for the macros that are constants. A class a test can construct, so that the tests
no longer need a process each. What a function returns when it refuses is decided then: an
`enum class` of TimerTwo's own, `bool`, or Utl's `Status`. XmegaTimer, the same API on the
ATxmega E5, is the model where it fits. `src/.clang-tidy` goes, and the groups in
`examples/.clang-tidy` with it. A sketch has to change its names: no aliases for the old ones,
a clean break.

### 4 Release 2.0.0 and the Library Manager

The README of 2.0.0 with the table of flash and RAM between `<!-- sizes -->` markers, the tag,
`release.yml`, and the request to the Arduino Library Manager.
