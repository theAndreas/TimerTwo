# TimerTwo

A periodic interrupt, a PWM and a time readout on Timer/Counter2 of the ATmega328P, as on the
Arduino Uno and Nano: the Timer2 counterpart of the Arduino library
[TimerOne](https://playground.arduino.cc/Code/Timer1/).

The timer runs in phase correct PWM: the period, in microseconds, sets the top and the
prescaler; the overflow at the bottom calls a function of the sketch; and pin 3 gives a PWM
whose frequency follows the period. This takes Timer2 from `analogWrite()` on pins 3 and 11 and
from `tone()`.

```cpp
#include <TimerTwo.h>

void OnTick() {
  // every millisecond, in the interrupt: keep it short
}

void setup() {
  timer_two::Discard(timer_two::timer2.Begin(1000, OnTick));
  timer_two::Discard(timer_two::timer2.EnablePwm(128));  // half of the period high
  timer_two::Discard(timer_two::timer2.Start());
}

void loop() {}
```

This is [examples/Basics](examples/Basics/Basics.ino). [examples/Read](examples/Read/Read.ino)
checks each call and reads the time.

## API

Everything lies in the namespace `timer_two`; `timer2` is the timer. A call that can refuse
returns a `Status`, and changes nothing when it refuses: `IsOk()` says whether it did what it
was asked, `GetError()` why not, and `GetErrorText()` the same in a few words for
`Serial.print()`. A `Status` must not be dropped: where a sketch does not care, `Discard()` says
so. `Read()` returns a `Result`, the time or why there is none.

| Call | What it does | Refuses with |
|---|---|---|
| `Begin(period, callback)` | Sets the timer up with a period in microseconds, 1000 if none is given, and a function the overflow calls, or none. It does not count yet. | `kBegunAlready`, `kPeriodOutOfRange` |
| `SetPeriod(period)` | Sets the period of the interrupt and of the PWM, up to `Timer2::kPeriodMax`, 32767 at 16 MHz; at least one step of the counter, 1 from 2 MHz on. The duty keeps its share. | `kNotBegun`, `kPeriodOutOfRange` |
| `Start()` | Starts the counter at 0, and the interrupt if there is a callback. | `kNotBegun`, `kRunning` |
| `Stop()` | Stops the counter where it is. | `kNotRunning` |
| `Resume()` | Lets the counter go on from where `Stop()` stopped it. | `kNotStopped` |
| `AttachInterrupt(callback)` | Sets the function the overflow calls. | `kNoCallback` |
| `DetachInterrupt()` | Disables the interrupt and forgets the function. | – |
| `EnablePwm(duty)` | Starts the PWM on pin 3: 0 is always low, 255 always high. | `kNotBegun` |
| `SetPwmDuty(duty)` | Sets the duty. | `kNotBegun` |
| `DisablePwm()` | Stops the PWM; pin 3 is a plain output again. | – |
| `Read()` | The time since the counter was last at its bottom, in microseconds. | `kNotRunning` |
| `GetState()` | `kNotBegun`, `kIdle`, `kRunning` or `kStopped`. | – |

The callback runs in the interrupt: it should do little, as setting a flag that `loop()` reads,
and what it shares with `loop()` is `volatile`. Below 8 MHz the times are `uint32_t`, as the
longest period does not fit 16 bits; from 8 MHz they are `uint16_t`. Both are `TimeType`.

## From version 1

Version 2.0.0 breaks every name, on purpose: a sketch of version 1 does not compile until it is
changed, rather than behaving otherwise without notice. There are no aliases for the old names.

| Version 1 | Version 2.0.0 |
|---|---|
| `Timer2.init(us, f)` | `timer_two::timer2.Begin(us, f)` |
| `Timer2.start()`, `stop()`, `resume()` | `Start()`, `Stop()`, `Resume()` |
| `Timer2.setPeriod(us)` | `SetPeriod(us)` |
| `Timer2.enablePwm(TimerTwo::PWM_PIN_3, duty)` | `EnablePwm(duty)`: there is only pin 3 |
| `Timer2.setPwmDuty(TimerTwo::PWM_PIN_3, duty)` | `SetPwmDuty(duty)` |
| `Timer2.disablePwm(TimerTwo::PWM_PIN_3)` | `DisablePwm()` |
| `Timer2.attachInterrupt(f)`, `detachInterrupt()` | `AttachInterrupt(f)`, `DetachInterrupt()` |
| `Timer2.read(us)` | `Read()`, which returns the time in a `Result` |
| `Timer2.getPeriodMax()` | `timer_two::Timer2::kPeriodMax` |
| `Timer2.getState()`, `TimerTwo::STATE_INIT` ... | `GetState()`, `timer_two::State::kNotBegun` ... |
| `StdReturnType`, `E_OK`, `E_NOT_OK` | `timer_two::Status`, `IsOk()`, `timer_two::Error` |
| `TimerTwo::TimeType` | `timer_two::TimeType` |
| `StandardTypes.h` | gone |

What also changed:

- The duty is a share of 255, not of 256: 255 keeps the output high, as 0 keeps it low.
- `SetPeriod()` and `SetPwmDuty()` are refused before `Begin()`, and `Stop()` returns why it
  did nothing.
- The first callback comes after a period, not at once: the flag of an overflow, which the
  core's `init()` leaves set, is cleared before the interrupt is enabled.
- The bugs of version 1 are fixed, among them `resume()`, which left the timer marked stopped,
  and the time `read()` returned, which lost its lower steps: item 2 of
  [OIL.md](https://github.com/theAndreas/TimerTwo/blob/main/OIL.md) lists them.

## Development

TimerTwo is built and checked as the projects made from
[embedded-cpp-template](https://github.com/theAndreas/embedded-cpp-template) are: `make` lists
every build and check, `make check` is what CI runs. [OIL.md](https://github.com/theAndreas/TimerTwo/blob/main/OIL.md) is what is planned, and
[AGENTS.md](https://github.com/theAndreas/TimerTwo/blob/main/AGENTS.md) what a contributor has to know.

## Licence

GPL-3.0-or-later, see [LICENSE](LICENSE).
