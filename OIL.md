# Open Issue List

What is planned for TimerTwo, in the order it can be done. A pull request that works on an item
sets its status and links itself in the same PR; an item is `done` once that PR is merged. New
items go at the end with the next free number, so a number never changes meaning.

Status: `open`, `in progress`, `done`, `dropped` (with the reason in the item).

## Items

| # | Item | Depends on | Status | PR |
|---|------|------------|--------|----|
| 1 | On the template embedded-cpp-template | – | done | #4 |
| 2 | The known bugs, each shown by a test first | 1 | done | #5 |
| 3 | Version 2.0.0: a namespace, constants for macros, the template's checks | 2 | done | #6 |
| 4 | Release 2.0.0 and the Library Manager | 3 | in progress | |
| 5 | SelfCheck in simavr | 1 | open | |
| 6 | Only the AVRs whose Timer2 has OC2B on pin 3 | 3 | open | |

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

### 2 The known bugs, each shown by a test first

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
  forever. `init()` with a period it refuses still leaves the timer initialized.

Found while fixing them, and fixed with them; neither can a test on the host show:

- `attachInterrupt()` writes the pointer to the callback while the interrupt may call it, and
  the AVR writes it a byte at a time.
- The duty is computed in `int`, which has 16 bits on the AVR: 255 * 255 overflows it.

The tests of the clocks run at 1, 16 and 20 MHz, each a build of its own.


### 3 Version 2.0.0: a namespace, constants for macros, the template's checks

`timer_two::timer2` with Google's names, in `src/timer_two/`, without `StandardTypes.h`;
`constexpr` for the macros that are constants. A class a test can construct, so that the tests
no longer need a process each. `src/.clang-tidy` goes, and the groups in
`examples/.clang-tidy` with it. A sketch has to change its names: no aliases for the old ones,
a clean break.

Decided on 2026-11-10:

- A refusal is a `Status` of an `enum class Error`, and `Read()` returns a `Result`, as in
  MaxMatrix; not `bool`, which says not why, nor Utl's, as every error comes from the library
  itself and Utl would be a dependency of every user (MaxMatrix OIL 28).
- The hardware is a parameter of a template, `Timer<Registers>`, as XmegaTimer has it, with one
  object for Timer/Counter2.
- The PWM takes no pin: there is only pin 3, as OCR2A is the top.
- `TimeType` stays, 16 bits from 8 MHz and 32 below.

Found on the way, from the datasheet and the core's `init()`, and fixed with a test: the flag of
an overflow stays set while the interrupt is disabled, and `init()` runs Timer2 before `setup()`,
so the interrupt came at once when `Start()` enabled it. It is cleared first now. Not on
hardware.

### 4 Release 2.0.0 and the Library Manager

The README of 2.0.0 with the table of flash and RAM between `<!-- sizes -->` markers, the tag,
`release.yml`, and the request to the Arduino Library Manager.

### 5 SelfCheck in simavr

`examples/SelfCheck`, which tools/self_check.sh runs in simavr: the period counted in callbacks
and the PWM measured on pin 3. Not possible with simavr as it is, measured on 2026-11-10 with the
registers alone, without TimerTwo, in simavr 1.6 of the dev container and in 1.8, the latest
release, built from source:

- Timer2 counts in the modes normal, CTC and fast PWM, but TCNT2 stays at 0 in both phase
  correct modes, 1 and 5, though 1.8 lists them in its table of Timer2.
- In mode 5, OC2B never drives pin 3: low in 61 397 samples over 100 ms at a duty of 64.
- With the overflow interrupt on in mode 5, the sketch no longer gets through a loop of 100 ms;
  the interrupt seems to fire without end.

So `start()` waits forever in simavr, and there is nothing to measure. The cause in both
releases: modes 1 and 5 are missing from the table of Timer2's modes, so TOV2 fires without end.

simavr's master after 1.8 fills the table (63aa5b192, 2026-08-09), and the counter moves; but it
counts one way only, so TOV2 comes twice as often as on the chip, and the duty on OC2B did not
come out the same in two programs. Reported with a program that shows it in
[simavr #442](https://github.com/buserror/simavr/issues/442#issuecomment-6779055513).

`SIMAVR_MCU` in `tools/project.sh` stays empty, and this item waits for a simavr release that
models phase correct PWM as the chip does; the template's image would then need it too.

### 6 Only the AVRs whose Timer2 has OC2B on pin 3

`library.properties` names every AVR with `architectures=avr`, and nothing in `src/` asks which
one it is compiled for; `Timer2Registers::kPwmPin` is 3, where the ATmega328P has OC2B. Found
on 2026-11-11 while going through the README for 4:

- The ATmega2560 of the Mega has OC2B on pin 9: the PWM comes out there, while `EnablePwm()`
  makes pin 3 an output.
- The ATmega32U4 of the Leonardo has no Timer/Counter2: a sketch fails to compile on a register
  it does not name, rather than on why.

Either the library refuses those chips with an `#error` that says why, or it hands each its pin;
which is to be decided. The README then names the boards it runs on.
