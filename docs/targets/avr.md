# Target profile: AVR through Arduino

What holds for TimerTwo on the AVR, built by avr-gcc through arduino-cli, beyond the
rules of [CodingStyle.md](../CodingStyle.md) that hold for every target. The rules there name
this profile where a target decides; the ids here start with `AVR-`.

## What the target is

| What | On the AVR (ATmega328P as the measure) | Consequence |
|---|---|---|
| `int` | 16 bits | Fixed widths (T-1); an overflow the host does not show |
| RAM, flash | 2 KiB, 32 KiB | No recursion (FN-6), the cost of an entity in its block (M-7) |
| Floating point | No unit; `float` in software | Fixed point first (H-7) |
| `double` | 32 bits, the same as `float` | No `double` (H-7) |
| Standard library | None | A library like it, as the ETL or Utl, or written for reuse (M-3) |
| Exceptions, RTTI | None; a sketch is built with `-fno-exceptions -fno-rtti` | M-2 |
| Heap | `malloc` exists, but fragments 2 KiB | M-1 |
| The table of `virtual` functions | Copied from flash into RAM at start, per class | C-6 |
| Compiler | avr-gcc 7.3, `gnu++11` | The least standard; `[[nodiscard]]` as `TIMERTWO_NODISCARD` (FN-10) |

## Rules

| Id | Rule | Checked by |
|---|---|---|
| AVR-1 | Constant text and tables lie in flash, as `PROGMEM`, and are read with `pgm_read_*`; a text handed to `Print` is `F("...")`. Flash and RAM are two address spaces: a pointer into flash read as if into RAM reads wrong data, and only SelfCheck shows it. | Review, `make self-check` |
| AVR-2 | A critical section is `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` from `<util/atomic.h>`, which restores the interrupt flag as it was. The AVR reads 8 bits in one access: a shared `uint16_t` or wider is read in one (H-6). | Review |
| AVR-3 | Registers and bits by the names of `<avr/io.h>`: `TIMSK1 \|= _BV(TOIE1)` (H-4). | Review |
| AVR-4 | Time from the core's `millis()` and `micros()`, handed to the library as its clock (H-2, H-3); `millis()` overflows after 49 days, `micros()` after 71 minutes. | Review, a test across the overflow |
| AVR-5 | An interrupt routine is `ISR(vector)` of `<avr/interrupt.h>`; it does not call `Serial`, `delay()` or `millis()`-based waits (H-5). | Review |
| AVR-6 | `delay()` and `delayMicroseconds()` only for the wait a datasheet asks (H-1). | Review |
| AVR-7 | `asm` only where the AVR needs it, as for a delay exact to the cycle, with its reason (MISRA 10.4.1). | Review |
