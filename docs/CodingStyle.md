# Writing the code

How the code of TimerTwo is written: how a file is split, how large a function may be,
which parts of C++ are used on a microcontroller and which are not. It holds for `src/`; the
tests and the examples follow it where a rule does not say otherwise. How a comment and a
Doxygen block are written is in [Documentation.md](Documentation.md), how the code is tested in
[Testing.md](Testing.md).

The rules hold for every target. What holds for one target only, as its way to put text into
flash or to turn interrupts off, is in its profile in [targets/](targets/): for this project
[targets/avr.md](targets/avr.md).

This file is for a person and for a coding agent alike. `AGENTS.md` points every agent here.

## How to read a rule

Each rule has an id, so that a review, a commit or a `NOLINT` can name it, and says what checks
it:

- **A tool**, as clang-tidy, clang-format or cpplint: the rule is enforced by `make lint` and
  `make tidy`, and its configuration is the authority; this file only says why.
- **Review**: no tool can check it. The author checks it, and the reviewer, human or agent.

## Principles

- **P-1 The smallest target is the measure.** The code runs on targets from an 8-bit
  microcontroller with 2 KiB of RAM to a host with gigabytes: an `int` may have 16 bits, there
  may be no floating point unit, no heap, no exceptions and no standard library. Code that is
  right on the host but costly or wrong on the smallest target is wrong. Its profile in
  `targets/` says what that target lacks.
- **P-2 The compiler and the tools decide what they can.** A rule a tool can check is a setting
  of the tool, not a paragraph here. A rule here that a tool could check is a gap to close.
- **P-3 Three standards, in this order:** the C++ Core Guidelines, MISRA C++:2023
  ([Misra.md](Misra.md) says rule by rule how far), and the
  [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for everything they
  leave open, as names and layout. Where they differ, the first wins, and P-1 over all three;
  the departures from Google are listed below.
- **P-4 Clear before clever.** The next reader is a stranger, or an agent with no context.

## Files

| Id | Rule | Checked by |
|---|---|---|
| F-1 | A header holds the declarations a user needs, documented; a `.cpp` holds the definitions. | Review |
| F-2 | A header includes what its declarations need, not what its bodies need. In the class body stay only a template, which every caller has to see, a `constexpr` function, and an accessor of one statement; anything more, a state machine, an `#if` ladder, a body that would pull in a header for itself, is defined in the `.cpp`. Inlining is no reason to keep a body in the header: with `-flto` the compiler inlines across sources as readily as within one, and what a header costs is every file that includes it. A header that one source alone includes may keep its bodies, until a second one includes it. | Review, `make headers` |
| F-3 | One class, or one family of functions, per header and source. A file is named exactly as the class it holds: `RingBuffer.h` and `RingBuffer.cpp` for `RingBuffer`, `array.h` for a class that mimics the standard's `array`; a family of functions after the family, in lower case: `clamp.h`. | Review |
| F-4 | Every header compiles as the only include of an empty file. | `make headers` |
| F-5 | A file includes what it uses, and nothing it does not ("include what you use"). | `misc-include-cleaner` |
| F-6 | Header guards as `PROJECT_PATH_FILE_H_`, from the path. | cpplint |
| F-7 | Includes in Google's order, each group sorted. | clang-format |
| F-8 | Every file starts with the copyright and the SPDX licence as `//` lines. | Review |
| F-9 | A library lies in its own namespace, so that its names cannot meet those of its user; an application may stay in the global one. What is not part of a library's API lies in its namespace `internal`. No `static` and no anonymous namespace in a header. | `google-build-namespaces`, Review |
| F-10 | A source file is ASCII, with LF line ends: a display on the device may show no umlaut, and a byte that is not ASCII in a string is a byte more than its character. | `.editorconfig`, Review |
| F-11 | A `.cpp` defines its functions in the order the header declares them. | Review |
| F-12 | A header and its source lie side by side in one folder, a folder per module: `src/Text/MaxMatrixText.h` beside `src/Text/MaxMatrixText.cpp`. No tree of headers apart from the tree of sources, as `inc/` and `src/`: what is not part of the API is told by the namespace `internal` (F-9), not by a folder, and an Arduino library has everything below `src/` anyway. | Review |
| F-13 | Files are grouped in folders, not left to pile up in one: a folder holds what belongs together, and when it holds more than about a dozen files, or files of more than one module, it is split by module. The same for `test/`, which mirrors `src/`, for `tools/`, `docs/` and `examples/`. | Review |
| F-14 | No cycle of includes: two headers that need each other are one, or a third holds what both need. | Review |
| F-15 | An example is copied into a sketch as it stands, so it is code to ship: complete, checking the results it gets, and with what a user has to adapt, as a pin, named at its top. | Review, `make examples` |

## Functions

| Id | Rule | Checked by |
|---|---|---|
| FN-1 | A function does one thing, and its name says which. If the name needs an "and", it is two. | Review |
| FN-2 | At most 40 lines in its body. A test has no limit of lines, as many cases make it long but flat, and SelfCheck is a test; the other limits hold for it. | `readability-function-size` |
| FN-3 | At most 4 parameters; more are a struct. | `readability-function-size` |
| FN-4 | At most 3 levels of nesting within the body; an early return over an `else`. | `readability-function-size`, `readability-else-after-return` |
| FN-5 | A cognitive complexity of at most 15. In a test, what GoogleTest's macros expand to does not count, its own branches do. | `readability-function-cognitive-complexity` |
| FN-6 | No recursion: the depth of the stack cannot be known when compiling, and on a small target the stack is what is left of the RAM. | `misc-no-recursion` |
| FN-7 | A parameter that is cheap to copy (up to two pointers in size) is passed by value, any other by `T const&`; one the function writes by pointer or by `T&` only where a return value cannot carry it (F.16, F.20). | `performance-unnecessary-value-param`, Review |
| FN-8 | A result is returned, not written into a parameter (F.20). Several results are a struct. | Review |
| FN-9 | A function that can be `constexpr` is. In C++11 it is one `return`. | Review |
| FN-10 | A result that must not be ignored, as an error, is marked so with `TIMERTWO_NODISCARD` from `timer_two/attributes.h`: `[[nodiscard]]` from C++17, GCC's `warn_unused_result` before it. An attribute of a later standard is a macro so, chosen by `__cplusplus`. A result dropped on purpose goes to `Discard()` of the same header, with the reason in a comment: `Discard(Send(frame));  // sent again on the next tick`. Never `(void)` or `static_cast<void>`: before C++17 GCC warns through both, clang does not, so a cast that passes on the host fails the build for the target. | `-Wunused-result`; the reason by review |
| FN-11 | A function is `const` when it leaves unchanged what a user sees of the object: its value, and what it controls through a pointer, as a pin, a register or a buffer it does not own. Constness is logical, not what the compiler allows: a function that writes a register through a member pointer is not `const`, though it compiles as one; a cache it updates is `mutable` (C-11). | `readability-make-member-function-const`; what is written through a pointer by review |
| FN-12 | A default argument only where its value is self-evident, and never on a `virtual` function. | `google-default-arguments`, Review |
| FN-13 | No code copied and changed a little: what two places do alike is one function. | Review |
| FN-14 | A loop that only looks for something is a function of its own, named as a question, with early returns: `bool ContainsPin(...)`, not a flag set inside a loop. | Review |
| FN-15 | A parameter that may be absent is a pointer, and its block says that it may be null; any other the function reads or writes is a reference. A member that refers to an object the class does not own, and that may be absent, is a pointer. One that must exist is a `reference_wrapper`, taken by reference in the constructor, so that it is never null and the class stays assignable: `explicit Display(Spi& spi) : spi_{spi} {}`. Which one, by M-3: `std::reference_wrapper` where every target has `<functional>`, else the counterpart of the ETL or Utl, else one of the project's own with its interface. Where an object of the class is made when compiling, as a global one (M-5), the wrapper's constructor has to be `constexpr` as well: the standard's is so only from C++20. Never a reference or a `const` member, which take the assignment from the class. What the class changes through the member counts for FN-11. | `cppcoreguidelines-avoid-const-or-ref-data-members`, Review |
| FN-16 | Two parameters of the same type whose order means something and that a caller can swap unnoticed, as a pin and a count, take a type each: a small `struct` or an `enum class`, which costs nothing once optimized. A `using` makes no type of its own (T-16). | `bugprone-easily-swappable-parameters`, Review |

## Types and values

| Id | Rule | Checked by |
|---|---|---|
| T-1 | Integers of fixed width, `uint8_t` to `int32_t`, in `src/`, a loop counter among them: the width of `int` depends on the target, 16 bits on an 8-bit microcontroller and 32 on the host, so a test on the host misses an overflow on the device. A counter takes the type of what it is compared with, `size_t` for a size, `uint8_t` for a small range. `bool`, `char` for text and `size_t` stay; so does `int` where an API of the platform gives it. | Review, until a check of our own (ROADMAP C-5) |
| T-2 | No implicit conversion that can change a value. | `-Wconversion`, `-Wsign-conversion` |
| T-3 | Braced initialization: `int count{0};`, `Point p{1, 2};`, and in a member initializer list. | `house-braced-member-init` in the initializer list, Review elsewhere |
| T-4 | `const` to the right of what it makes constant: `char const*`, `T const&`. | clang-format |
| T-5 | Everything that does not change is `const`; a value known when compiling is `constexpr`. | `misc-const-correctness` |
| T-6 | `constexpr` before `const` before a macro. A macro only where nothing else will do, as for a conditional build or a macro the platform requires. | `cppcoreguidelines-macro-usage` |
| T-7 | `enum class`, with its underlying type: `enum class Mode : uint8_t`. | `cppcoreguidelines-use-enum-class`,`performance-enum-size` |
| T-8 | `auto` only where the type stands in the same line already, as in a cast, or cannot be written, as of a lambda. | `modernize-use-auto`, Review |
| T-9 | No C-style cast. `static_cast`; `reinterpret_cast` and `const_cast` only with a `NOLINT` and its reason. Where a pointer is cast, two things differ by target: its width, 16 bits on an 8-bit microcontroller and 64 on the host, so that an address as a number is a `uintptr_t`; and the alignment, so that a pointer is not cast to a type that needs a stricter one. No cast through `void*` to hide a change of type. | `cppcoreguidelines-pro-type-*`, `google-readability-casting`, `bugprone-casting-through-void` |
| T-10 | `nullptr`, never `NULL` or `0`. | `modernize-use-nullptr` |
| T-11 | No magic number: a value with a meaning has a name. Without one pass only what `readability-magic-numbers` lets pass, 0 to 4 and the floating point values 1.0 and 100.0. Tests and examples are exempt: their numbers are their data. | `readability-magic-numbers` |
| T-12 | A range-based `for` where the index is not needed, taking each element by `const&` unless a copy is meant: `for (uint8_t const pin : pins)`, `for (Led const& led : leds)`. | `modernize-loop-convert`, `performance-for-range-copy` |
| T-13 | A variable is declared where it is first needed, initialized, one per declaration, in the smallest scope; a new value that means something else is a new variable, not the old one reused. A data member is initialized too (C-5). | `cppcoreguidelines-init-variables`, `cppcoreguidelines-pro-type-member-init`, `readability-isolate-declaration`, Review |
| T-14 | One side effect per statement: no `++` in a condition, no assignment inside an expression, and none in the argument of `assert` or another macro, which a build may drop or evaluate twice. | `bugprone-inc-dec-in-conditions`, `bugprone-assignment-in-if-condition`, `bugprone-assert-side-effect`, `bugprone-macro-repeated-side-effects`, Review |
| T-15 | A `switch` over an `enum class` names every value and has no `default`, so that the compiler reports a value added later; any other `switch` has a `default`. Where the value may be one the `enum` does not name, as one cast from a number or read from memory, the function handles it after the `switch`, as an error it returns: the compiler's report stays, and so does the way out. A test reaches it with `static_cast<Mode>()` of an unnamed value, which is valid, as the `enum` has its underlying type (T-7). | `-Wswitch`, `bugprone-switch-missing-default-case` |
| T-16 | `using` for an alias, never `typedef`: `using Callback = void (*)(uint8_t);`. | `modernize-use-using` |
| T-17 | Never Arduino's `byte`, which is an `unsigned char` that says nothing of what it holds. A byte that is a number, a level or a count, is `uint8_t`. A byte that is raw data, as of a buffer, a register or a frame on a bus, is a type that is no number, by M-3: `std::byte` where every target has C++17, else `etl::byte` or Utl's counterpart, else one of the project's own after the same pattern, an `enum class` of `unsigned char` with `to_integer()`. Raw data then takes only bitwise operators, and is a number only through `to_integer<uint8_t>()`. | Review |
| T-18 | An expression does one thing. A computation of several steps, or a condition of several parts, is split into named steps: a `const` local for each value with a meaning, a small function for a step that needs explaining or recurs, so that each line reads as what it computes. A step whose name explains it is worth a function even when called once. Named functions called one in the argument of the next read as a pipeline and stay, as long as they fit on a line: `ToLevel(GetWeightedIntensity(GetIntensity(pixel)))`. | Review (ROADMAP C-3) |
| T-19 | Arithmetic on a type narrower than `int` is done in `int`, after the promotion, and `int` has a sign and on an 8-bit microcontroller 16 bits: there `uint8_t * uint8_t` overflows from 182 * 182 on, on the host `uint16_t * uint16_t` from 46341 * 46341, and the overflow of a signed type is undefined. An operand is widened before the operation, to the least unsigned width that holds the result: `uint32_t const area{static_cast<uint32_t>(width) * height};`. Not wider: on an 8-bit target an operation of 32 bits costs several of 16 (P-1). `~` of a `uint8_t` is a negative `int`, cast back before it is compared or shifted. | `-Wconversion` for the narrowing back, `make sanitize` for the widths of the host, Review |
| T-20 | An array is a class that knows its size, `std::array` or its counterpart by M-3, which does not decay to a pointer; a C array only for a string literal: `char const kName[] = "font";`. An element is reached by index, not by arithmetic on a pointer. | `modernize-avoid-c-arrays`, `cppcoreguidelines-pro-bounds-pointer-arithmetic` |
| T-21 | A `char` is widened to a number through `unsigned char`: whether `char` has a sign depends on the target, and where it has one, a character above 127 turns negative. | `bugprone-signed-char-misuse` |

T-18 in an example:

```cpp
// Not:
if ((now - start_ >= kInterval) && (state_ == State::kOn) && !paused_) {
// But:
bool const elapsed{now - start_ >= kInterval};
bool const running{state_ == State::kOn && !paused_};
if (elapsed && running) {
```

## Classes

| Id | Rule | Checked by |
|---|---|---|
| C-1 | `struct` for data with public members only and no invariant, `class` for everything else. | `misc-non-private-member-variables-in-classes` |
| C-2 | The order in a class: `public`, `protected`, `private`; in each, types and constants, constructors and destructor, functions, data members. | Review |
| C-3 | The rule of zero first. Else the rule of six: a class that declares a special member declares all six, the default constructor among them; `= default` and `= delete` count. | `cppcoreguidelines-special-member-functions` for five; the default constructor by review, until a check of our own (ROADMAP C-4) |
| C-4 | A constructor with one argument is `explicit`, unless the conversion is meant; then a `NOLINT` says why. | cpplint `runtime/explicit` |
| C-5 | A default the same for every constructor is a member initializer (C.48); the rest is in the initializer list (C.49), never assigned in the body. | `modernize-use-default-member-init`, `cppcoreguidelines-prefer-member-initializer` |
| C-6 | `virtual` only with its reason in a comment at the class: each class with a `virtual` function has a table, which takes RAM on some targets (see the profile), and a call through it cannot be inlined. A template, or a function reference, often does the same at no cost; the reason says why it does not here. That a test wants to replace the class is no reason: the test is handed the hardware (H-3), or the class is a parameter of a template. | Review |
| C-7 | A base class with a `virtual` function has a public virtual or protected non-virtual destructor. | `cppcoreguidelines-virtual-class-destructor` |
| C-8 | Inheritance is public and of an interface; a class is reused by holding it, not by deriving from it. | Review |
| C-9 | Data members are `private` and end in `_`. | `readability-identifier-naming` |
| C-10 | A function that overrides says `override`, or `final`, and not `virtual` again. | `modernize-use-override` |
| C-11 | `mutable` only for what is not part of the state a user sees, as a cache (FN-11). | Review |
| C-12 | `friend` only where nothing else will do, as an operator that needs the members; with its reason. | Review |
| C-13 | An operator is overloaded only where its meaning is obvious: `Point + Point`, not `Point * Point`. | `misc-unconventional-assign-operator`, Review |
| C-14 | A special member that does what the compiler's would is `= default`, not written out: `Sensor() = default;`, not `Sensor() {}`. | `modernize-use-equals-default` |

## Memory and the microcontroller

| Id | Rule | Checked by |
|---|---|---|
| M-1 | No heap: no `new`, `delete`, `malloc` or `free`, not even in `setup()`. The size of everything is known when compiling, and the RAM a build takes is the RAM it needs. | `cppcoreguidelines-no-malloc`, `cppcoreguidelines-owning-memory` |
| M-2 | No exceptions and no RTTI: no `throw`, `try`, `dynamic_cast` or `typeid`. Small targets have neither, and on the others they cost flash and time that cannot be bounded. Nor a `static_cast` from a base down to a derived class in its place: where a design needs one, the base lacks a function. | the target's build, `cppcoreguidelines-pro-type-static-cast-downcast`, Review |
| M-3 | Reuse before writing, in this order: (1) what the project has already, a function or class that does it; (2) the standard library, where every target of the project has it, and only its parts that need neither heap nor exceptions (M-1, M-2), as `std::array`, `<algorithm>`, `<type_traits>`; where a target lacks it, `std::` stands only in the code of the targets that have it; (3) a library in the style of the standard library that the project uses, as the [ETL](https://github.com/ETLCPP/etl) or Utl; (4) only where none has it, it is written: once, for reuse, with the interface of the standard library's counterpart, so that it can give way to `std::` or `etl::` later, in a module folder of its own, as `src/timer_two/Utility/`, with its test. The tests on the host may use the standard library. | Review |
| M-4 | Constant data, as text and tables, is `constexpr` or `const`, so that it can lie in flash. Where a target needs more to put it there and read it, its profile says how. A constant in a header whose address is taken, as a table read by an index known only at run time, is declared `extern` there and defined once in a `.cpp`: a `constexpr` at namespace scope gives every file that takes its address a copy of its own. A value used as a value may stay `constexpr` in the header; from C++17, `inline constexpr` does both. | Review, the profile |
| M-5 | No global object with a constructor that does work: the order of initialization across files is not defined. An object of static storage is initialized when compiling, by a `constexpr` constructor, and has no destructor of its own, which would cost flash for an end the device never reaches. What has to start with work gets a `Begin()` that the application calls in `setup()`. | `cppcoreguidelines-avoid-non-const-global-variables`, `cert-err58-cpp` |
| M-7 | The cost of an entity in flash and RAM, where it matters, is stated in its Doxygen block. | Review |

## Time, hardware and interrupts

| Id | Rule | Checked by |
|---|---|---|
| H-1 | The library does not block. Work that takes time is spread over an `Update()` that the application calls in its main loop; a busy wait only for the microseconds a datasheet asks between two steps of the hardware, with the datasheet's figure in a comment. | Review |
| H-2 | Time is an unsigned count of a fixed width from a clock the library is given, as a millisecond counter, and a span is measured as the difference: `now - start >= interval`, never `now >= start + interval`. The difference is right across the overflow of the counter, the sum is not. | Review, a test across the overflow |
| H-3 | The hardware is handed in, not written into the library: pins, ports and peripherals as parameters of the constructor or of a template. The library is then tested on the host against the mock, and runs on another board. | Review |
| H-4 | A register and its bits by the names of the vendor's header, or by a named `constexpr`, never by a number. The profile names the header. | `readability-magic-numbers`, Review |
| H-5 | An interrupt routine does the least it can: it copies a value or sets a flag, and returns. No output, no wait, no floating point, no call that takes long. | Review |
| H-6 | What an interrupt routine and the rest share is `volatile`. The rest reads or changes it in a critical section, with the interrupts off, where it is wider than what the target reads in one access or is read and written in one step, as `++count`. The profile says how a critical section is written. | Review |
| H-7 | Fixed point before `float` where the smallest target has no floating point unit, as the profile says: there the first `float` brings the soft-float routines and their flash. A `float` takes its reason in a comment. No `double`: its width differs between targets, 32 bits on some and 64 on the host, so that a test on the host does not compute what the device does. | `-Wdouble-promotion`, Review |
| H-8 | Bitwise operators on unsigned types only, the masks named: `constexpr uint8_t kReadyBit{0x04U};`. A shift is by less than the width of its type, after the promotion to `int` that a `uint8_t` takes. | `-Wshift-count-overflow`, Review |

## Errors

| Id | Rule | Checked by |
|---|---|---|
| E-1 | An error is returned, as a `Result` or an `enum class` of errors, and never ignored silently (FN-10). Each project defines its own error type; the template brings none. | Review |
| E-2 | What the caller must ensure is a precondition: a `static_assert` where the compiler can check it, a `\pre` in the block otherwise. | Review |
| E-3 | Misuse that can be found when compiling fails when compiling, and a test of what must not compile proves it: `add_compile_fail_tests()`. | `test/` |

## Names

A name says what the entity is or does, in the terms of its domain: `brightness`, not `val`;
`ReadTemperature()`, not `Process()`. No abbreviation but the common ones (`i` for an index,
`n` for a count) and those of the domain (`pwm`, `adc`). A short name is fine where its scope is
a few lines, as Google allows; the further a name is seen, the more it says. Checked by review:
`readability-identifier-length` is off for that reason.

A template parameter is named for its role, as any other name, never by a single letter, not
even where the scope is short: `template <typename Value, typename Error> class Result`,
`\tparam Element`, not `T` and `E`. A pack is plural: `typename... Args`. Checked by review; no
check of clang-tidy asks for a length of a template parameter alone, a candidate for a check of
our own (ROADMAP C-3).

As Google names, enforced by `readability-identifier-naming`; `.clang-tidy` is the authority.

| What | How | Example |
|---|---|---|
| Namespace | `lower_case` | `timer_two` |
| File | as the class it holds (F-3) | `RingBuffer.h`, `array.h`, `clamp.h` |
| Type, function | `CamelCase` | `RingBuffer`, `PushBack()` |
| Accessor | like the member it gives | `size()` |
| Variable, parameter | `lower_case` | `byte_count` |
| Private member | `lower_case_` | `head_` |
| Constant, enum value | `kCamelCase` | `kMaxSize` |
| Macro | `TIMERTWO_UPPER_CASE` | `TIMERTWO_NODISCARD` |

A type that mimics one of the standard library takes its names, as `array`, `size_type`; the
project lists it in `.clang-tidy`.

No name belongs to the compiler and its library: none with two underscores in a row, none that
begins with an underscore and a capital letter, and none at namespace scope that begins with an
underscore. The headers of a vendor are full of them. Checked by `bugprone-reserved-identifier`.

## Where we depart from Google

Google's guide is written for large programs on servers, with the standard library. Where it
does not fit a microcontroller, or MISRA asks for more, this file wins:

| Google says | Here | Why |
|---|---|---|
| `.cc` files | `.cpp` | Arduino compiles only `.cpp` from `src/`, and most embedded toolchains expect it |
| File names in lower case | Named as the class (F-3) | A file is found by the name of what it holds; Arduino names a library's header after the library anyway |
| `int` is the integer to use | Fixed width (T-1) | The width of `int` differs between targets; MISRA 6.9.2 |
| Braces around a one-line body are optional | Always braces | MISRA 9.3.1 |
| The standard library, `std::unique_ptr` for ownership | Its parts without heap and exceptions where every target has it, else a library like it (M-3); no ownership by pointer (M-1) | A small target may have no standard library, and there is no heap |
| Comments in `//`, any style | Doxygen blocks `/*! */` for the API | The reference is built from them (Documentation.md) |
| `const int` or `int const`, consistently | `int const` (T-4) | Read from right to left, as the rest of a type |
| Brace or `=` initialization | Braced (T-3) | It refuses a narrowing conversion |
| No exceptions | No exceptions | The same, for another reason: a small target has none (M-2) |

## Suppressing a check

| Id | Rule | Checked by |
|---|---|---|
| S-1 | A `NOLINT` names every check it suppresses, never none and never a pattern as `*`, and says why, on the same line or the one above: `// NOLINT(runtime/explicit): a Result is made from its value`. | Review (ROADMAP C-3) |
| S-2 | `NOLINTNEXTLINE` over `NOLINT` where the reason does not fit behind the code; never `NOLINTBEGIN` around more than one entity. | Review |
| S-3 | A check that is wrong for the whole project is turned off in `.clang-tidy`, with the reason, not suppressed line by line. | Review |
| S-4 | The code is changed first; a suppression is the last way, not the shortest. Its reason answers three questions: why the check reports the line, why the line stays as it is, and what keeps it from doing harm, as a test, a `static_assert` or a bound. "Intended" answers none. | Review |
| S-5 | A suppression of a report the tool gets wrong says so, with the tool's issue where there is one, and is looked at again when the tool is updated: once the tool is right, it goes. | Review |
