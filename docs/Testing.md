# Testing the code

How TimerTwo is tested: which kind of test proves what, how a test is named and
written, and what the mock of the Arduino core may do. The code of a test follows
[CodingStyle.md](CodingStyle.md), but for the exceptions `test/.clang-tidy` makes and gives its
reasons for. Like CodingStyle.md, each rule has an id and says what checks it.

## Three kinds of test

| Kind | Where | Proves | Run by |
|---|---|---|---|
| **On the host** | `test/*_test.cpp`, GoogleTest against the mock in `test/mock/` | What the library computes: every function, every branch, every edge | `make tests`, `make clang`, `make sanitize`, `make coverage` |
| **What must not compile** | `test/compile_fail/*.cpp` | That a misuse is stopped by the compiler, with the message it names | `make tests` |
| **On the target** | `examples/SelfCheck`, in simavr for the AVR | What only the target shows: the width of its `int`, text in its flash, the variants as its compiler builds them | `make self-check` |

Where host and target differ, a test on the host passes and the device fails. What differs, and
what catches it:

| Difference | Host | Smallest target | Caught by |
|---|---|---|---|
| Width of `int` | 32 bits | 16 bits | CodingStyle T-1 and T-19, a check in SelfCheck where it matters |
| Width of a pointer | 64 bits | 16 bits | CodingStyle T-9 |
| Width of `double` | 64 bits | 32 bits | CodingStyle H-7 |
| Constant data | in RAM | in flash, read by its own functions | the profile, SelfCheck |
| Compiler and standard | a recent GCC and clang | the target's, as avr-gcc 7 in C++11 | `make examples`, which alone sees what that compiler reports, as `warn_unused_result` through a cast (CodingStyle FN-10) |

| Id | Rule | Checked by |
|---|---|---|
| TS-1 | Every behaviour is tested on the host; the target checks only what the host cannot show. A check in SelfCheck that the host could make is a test on the host instead. | Review |
| TS-2 | A misuse that a `static_assert` stops has a case in `test/compile_fail/`, whose line `// Expected: ...` names the message. | `add_compile_fail_tests()` |
| TS-3 | A bug is fixed after a test that shows it fails, and the test stays. | Review |
| TS-4 | Every line and every branch of `src/` is reached by a test on the host. The tests follow what the documentation promises, not what the code does; a line they miss is first a question whether the promise is complete, then one for a test. A branch no test can reach is a question about the code, not about the test; where the code keeps it, as a way out the hardware may take, it is excluded with `GCOVR_EXCL_LINE: ` and the reason, and an exclusion without one fails. | `make coverage` |
| TS-24 | Every test on the host passes under AddressSanitizer and UBSan as well, which find what a test passes by chance: a read beyond an array, an overflow of a signed type. | `make sanitize` |

## Files and names

| Id | Rule | Checked by |
|---|---|---|
| TS-5 | One test file per header, in the folder of `test/` that mirrors the header's in `src/`, named as the class it tests and `Test`: `src/Text/MaxMatrixText.h` is tested in `test/Text/MaxMatrixTextTest.cpp`, as its suite is `MaxMatrixTextTest`. A family of functions in lower case takes `_test`: `clamp.h` in `clamp_test.cpp`. CMake takes every such file below `test/`. | Review |
| TS-6 | A test lies in the library's namespace and an anonymous one inside it, as the skeleton's does. | Review |
| TS-7 | The suite is the entity and `Test`, the test is what it does, as a sentence in the third person: `TEST(ClampTest, LimitsAValueBelow)` reads "Clamp limits a value below". No underscore, which GoogleTest reserves; no "Test", "Works" or "Correct" in the name of a test. | Review |
| TS-8 | A test reaches the library through its API only, as a sketch does: no `friend`, no `#define private public`, no header of `internal` unless the test is of `internal`. | Review |

## A test

| Id | Rule | Checked by |
|---|---|---|
| TS-9 | One behaviour per test. A test may check it with several expectations, but a second behaviour is a second test. | Review |
| TS-10 | Arrange, act, assert, in that order, each a paragraph of its own; no comment names them. | Review |
| TS-11 | `EXPECT_*` by default; `ASSERT_*` only where the rest of the test would make no sense, as an index after a size that is wrong. | Review |
| TS-12 | The value under test first, the expected second: `EXPECT_EQ(Clamp(42, 0, 10), 10)`. | Review |
| TS-13 | The expected value is written down, not computed: a test that computes it as the code does repeats the code's mistake. | Review |
| TS-14 | No logic in a test: no `if`, no loop over cases. A table of cases is a parameterized test, `TEST_P` with `INSTANTIATE_TEST_SUITE_P`, each case with a name. | Review |
| TS-15 | The edges are tested, each on its own: the least and the greatest value, one beyond each, empty and full, and an overflow where one can happen, as of the clock (H-2). | Review, `make coverage` |
| TS-16 | A `constexpr` function is also tested where the compiler computes it, with a `static_assert` in a test. | Review |
| TS-17 | A fixture only for what two tests or more set up alike. No state shared between tests: each test can run alone and in any order. | `gtest_discover_tests`, Review |
| TS-18 | Deterministic: no real time, no random number, no `sleep`. Time comes from the mock, set by the test. | Review |
| TS-25 | Each function of the API has a test with an argument outside its contract as well as one inside, where the contract says what happens then: the error it returns, the value it clamps to, the call it ignores. | Review |
| TS-26 | Calls with an order, as `Begin()` before `Update()`, are tested in the wrong one too: no `Begin()`, `Begin()` twice, `Update()` after `End()`. A state machine has a test for each transition, and one for each event that must change nothing. | Review |

## The mock of the Arduino core

| Id | Rule | Checked by |
|---|---|---|
| TS-19 | `test/mock/` gives what the library uses of the core, with the core's signatures and nothing more; it grows as the library comes to use more. | `make tests`, `make examples` |
| TS-20 | The mock records what the library does, as `Serial` its output, and lets the test set what the library reads, as the time of the clock or the level of a pin. It has no behaviour of its own beyond the core's. | Review |
| TS-21 | A test resets what it set in the mock, or the fixture does: the next test starts from the core's state after reset. | Review |

## On the target

| Id | Rule | Checked by |
|---|---|---|
| TS-22 | A check of SelfCheck prints one line, `pass` or `FAIL` and what it checks, and the run ends with the number that failed; `SelfCheck.expected` is what a correct run prints. | `make self-check` |
| TS-23 | The input of a check is `volatile`, so that the target computes it and not the compiler. | Review |
