# Executive Summary

This review covered all **144 files** that existed in the repository at the review baseline and completed two independent passes. The repository is a collection of unrelated header-only libraries and standalone utilities rather than one cohesive product. It contains **52 findings**, including **9 critical** and **26 serious** findings. The highest risks are memory corruption in public headers and file parsers, destructive backup behavior, and concurrent lifetime defects.

| Severity | Count |
|---|---:|
| 5 — Critical | 9 |
| 4 — Serious | 26 |
| 3 — Moderate | 16 |
| 2 — Minor | 1 |
| 1 — Cosmetic | 0 |

| Category | Count |
|---|---:|
| Confirmed Bug | 34 |
| Likely Bug | 6 |
| Potential Issue | 1 |
| Reliability Concern | 10 |
| Design Concern | 1 |
| Code Smell | 0 |

Evidence was obtained through source inspection, compiler diagnostics, successful builds where dependencies allowed, sanitizer demonstrations, controlled temporary-directory tests, archive/image inspection, and cross-file contract tracing. `Tetris` built successfully and all 42 ordinary ThreadPool tests passed; those successes do not cover the defects documented below. `CWave` could not be fully built on this host because PulseAudio development headers are absent. Several language runtimes are also unavailable; those ports received static review plus syntax checks where a toolchain was installed.

# Repository Overview

## Inventory

All non-VCS directories:

```text
.
CWave/
CWave/src/
CWave/src/platform/
CWave/src/platform/linux/
CWave/src/platform/macos/
CWave/src/platform/windows/
Input/
Tetris/
Tetris/src/
descend_calc/
imf/
sizeof/
traffic_light/
```

| Directory | Direct files | Recursive files |
|---|---:|---:|
| `.` | 52 | 144 |
| `CWave` | 3 | 18 |
| `CWave/src` | 7 | 15 |
| `CWave/src/platform` | 1 | 8 |
| `CWave/src/platform/linux` | 2 | 2 |
| `CWave/src/platform/macos` | 3 | 3 |
| `CWave/src/platform/windows` | 2 | 2 |
| `Input` | 6 | 6 |
| `Tetris` | 2 | 11 |
| `Tetris/src` | 9 | 9 |
| `descend_calc` | 27 | 27 |
| `imf` | 10 | 10 |
| `sizeof` | 15 | 15 |
| `traffic_light` | 5 | 5 |

There are 110 language source/header files: 38 C++, 35 C/C++ headers, 5 C, 3 Python, 2 Dart, 2 Zig, 2 Nim, and one each of Assembly, Arduino, C#, F#, Go, Haskell, Java, JavaScript, Kotlin, Lua, MATLAB, Objective-C, Objective-C++, Perl, PHP, R, Ruby, Rust, shell, Swift, TypeScript, V, and Visual Basic. The remaining files comprise four build files, five documentation/text files, two ignore files, nine images, two circuit archives, and twelve PE executables.

Build systems detected:

- CMake projects: `CWave/CMakeLists.txt`, `Tetris/CMakeLists.txt`.
- Make projects: `Input/makefile`, `sizeof/makefile`.
- Most root utilities are ad-hoc, single-translation-unit builds with no root build orchestration.

Header-only libraries:

- Project headers: `Core.h`, `DataTypes.h`, `Hash.h`, `InputHandler.h`, `Profiler.h`, `ProgressBar.h`, `Result.h`, `String.h`, `ThreadPool.hpp`, `Vector.h`, `array.h`, `cstring.h`, `stack.h`, `stack_vector.h`, `Input/Input.h`, and `Input/tc.h`.
- Vendored headers: `exprtk.hpp`, `zip_file.h`, `imf/stb_image.h`, `imf/stb_image_write.h`, and root `stb_image_write.h`.
- CWave and Tetris also expose component headers, but require their corresponding implementation files.

Standalone applications include the root C/C++ programs, `CWave`, `Tetris`, `Input`, all 27 descend-calculator ports, `imf`, the `sizeof` utility, and the Arduino traffic-light sketch. Test sources are `hash_test.cpp`, `thread_pool_test.cpp`, and `TestString.h`; only the first two are standalone test programs, and `TestString.h` currently cannot compile through `String.h`. There is no dedicated `examples/` project. Example-like clients are the hash benchmarks, `Input/main.cpp`, `imf/imf.cpp`, the language ports, and the Arduino sketch. Utility scripts are `hash_test_generator.py`, `nks.py`, and the Python/shell/other interpreted descend-calculator variants.

The twelve files under `sizeof/*.exe` are valid PE executables and their x86/x64 machine types match their names. They are committed binary outputs, so source equivalence and reproducibility cannot be established from repository evidence.

## Architecture

The repository has no global application layer. Its principal dependency relationships are:

```text
Hash.h ──> hash.cpp, hash tests/benchmarks, repacker.cpp, repacker-zip.cpp
InputHandler.h + ProgressBar.h ──> Convert2Binary.cpp
ProgressBar.h ──> backup.cpp, data.cpp
exprtk.hpp ──> sum.cpp
STB image headers ──> image_to_ascii.cpp, imf/imf.cpp
Input/Input.h + Input/tc.h ──> Input example/Snake
CWave interfaces ──> Linux PulseAudio / macOS CoreAudio / Windows WASAPI backends
Tetris model (Board/Tetromino/Game) ──> Renderer ──> terminal main
zip_file.h + Hash.h ──> repacker-zip.cpp
```

Public APIs are primarily the custom single-header types and C interfaces: hash functions and hash classes, `basic_string`, C `string`, vector/array/stack containers, `Result`, `ThreadPool`, profiler/progress functions, input handlers, CWave's `AudioCaptureBase`, and the Tetris model classes. Internal modules are the hash CLI parser/work scheduler, repacker code generators, backup inventory/copy/delete phases, CWave FFT/visualizer/platform captures, Tetris game loop/renderer, STB/exprtk/miniz implementation bodies, and per-program parsers.

Design quality varies sharply. `ThreadPool.hpp` has clear contracts and unusually broad tests, Tetris has reasonable model separation, and CWave cleanly abstracts platform capture. Conversely, many header APIs rely on assertions for runtime preconditions, several utilities combine parsing, destructive filesystem mutation, and presentation in one translation unit, and root-level build/dependency declarations are absent.

## Risk Assessment

- Highest correctness/UB likelihood: `String.h`, `cstring.h`, `Result.h`, `Hash.h`, `Vector.h`, `ThreadPool.hpp`, `BrainfuckInterpreter.c`, `AssetFinder.cpp`, `Convert2Binary.cpp`, `Input/Input.h`, and `imf/imf.cpp`.
- Highest robustness risk: `backup.cpp`, both repackers, `data.cpp`, `hangman.cpp`, `tictactoe.cpp`, `snake_console.c`, and the CWave runtime configuration/audio pipeline.
- Highest portability risk: `Core.h`, `zip_file.h`, `sizeof/sizeof.c`, the Windows-only root utilities, CWave backends/build flags, `snake_console.c`, and the 27 language ports.
- Highest maintenance risk: duplicated vendored STB headers, the generated 21 MiB hash test, embedded duplicate `zip_file.h` source text, committed PE outputs, and the lack of a root build/test manifest.

# Findings By Severity

## Severity 5 — Critical

`REV-001`, `REV-004`, `REV-013`, `REV-014`, `REV-018`, `REV-022`, `REV-024`, `REV-027`, `REV-028`.

## Severity 4 — Serious

`REV-002`, `REV-005`, `REV-006`, `REV-007`, `REV-010`, `REV-012`, `REV-015`, `REV-019`, `REV-023`, `REV-025`, `REV-026`, `REV-029`, `REV-030`, `REV-031`, `REV-032`, `REV-033`, `REV-035`, `REV-036`, `REV-037`, `REV-038`, `REV-039`, `REV-040`, `REV-041`, `REV-042`, `REV-043`, `REV-045`.

## Severity 3 — Moderate

`REV-003`, `REV-008`, `REV-009`, `REV-011`, `REV-016`, `REV-017`, `REV-020`, `REV-021`, `REV-034`, `REV-044`, `REV-046`, `REV-047`, `REV-048`, `REV-050`, `REV-051`, `REV-052`.

## Severity 2 — Minor

`REV-049`.

# Findings By File

| File/component | Finding IDs |
|---|---|
| `String.h`, `TestString.h` | REV-001–REV-003 |
| `cstring.h` | REV-004–REV-005 |
| `Result.h` | REV-006–REV-007 |
| `array.h`, `stack.h` | REV-008 |
| `stack_vector.h` | REV-009 |
| `Vector.h` | REV-010–REV-011 |
| `Hash.h` | REV-012–REV-015 |
| `hash.cpp` | REV-016 |
| `Hash.dart` | REV-017 |
| `ThreadPool.hpp`, `thread_pool_test.cpp` | REV-018–REV-019 |
| `Core.h`, `README.md` | REV-020 |
| `Profiler.h`, `ProgressBar.h` | REV-021 |
| Brainfuck interpreter/compiler | REV-022–REV-023 |
| `AssetFinder.cpp` | REV-024–REV-025 |
| `Convert2Binary.cpp`, `InputHandler.h` | REV-026 |
| `backup.cpp` | REV-027–REV-029 |
| CWave | REV-030–REV-032 |
| Tetris | REV-033–REV-034 |
| `Input/*` | REV-035 |
| `snake_console.c` | REV-036 |
| `hangman.cpp`, `tictactoe.cpp` | REV-037 |
| `image_to_ascii.cpp` | REV-038 |
| `imf/imf.cpp` | REV-039 |
| `repacker.cpp` | REV-040–REV-041 |
| `repacker-zip.cpp`, `zip_file.h` | REV-041–REV-042 |
| `data.cpp` | REV-043 |
| `sum.cpp`, `reminder.cpp`, `time.cpp` | REV-044 |
| `descend_calc/descend_calc.s` | REV-045 |
| other descend calculator ports | REV-046 |
| `sizeof/*` | REV-047 |
| `nks.py` | REV-048 |
| build/docs/tests/assets | REV-049 |
| Windows desktop utilities | REV-050–REV-051 |
| `ohmslaw.nim` | REV-052 |

# Detailed Findings

ID: REV-001  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `String.h`  
Line(s): 260–266, 387–395  
Title: Default construction and reallocation operate on nonexistent objects

Description:  
`m_Str` starts null with capacity zero. `Deallocate` nevertheless destroys `m_Capacity + 1` elements and deallocates the pointer. Initial growth calls this path, and destruction of an untouched default string does too. The code also treats allocated raw storage as an array of constructed characters without consistently starting lifetimes.

Impact:  
Ordinary construction, assignment, or destruction can execute undefined behavior; stateful/debug allocators can crash or diagnose invalid deallocation.

Trigger Conditions:  
Destroy a default string or perform the first allocation, once the independent parse defect is repaired.

Root Cause:  
Capacity bookkeeping is confused with constructed-object count, and null/zero allocation is not special-cased.

Recommendation:  
Implement allocator-aware construction/destruction around a valid empty representation, and never destroy/deallocate null storage.

ID: REV-002  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `String.h`, `TestString.h`  
Line(s): `String.h` 345–346; `TestString.h` 1–8  
Title: The string header cannot be parsed by a conforming compiler

Description:  
Dependent `allocator_traits<Alloc>::size_type` and `difference_type` names omit `typename`. GCC and Clang reject the header before use. The test header also depends on `iostream`/`cassert` being included elsewhere.

Impact:  
The advertised header-only library and its tests are unusable as committed.

Trigger Conditions:  
Include `String.h` in any standard C++ translation unit.

Root Cause:  
Template dependent-name rules and self-contained-header requirements were not validated.

Recommendation:  
Correct the dependent names, make tests self-contained, and add clean-translation-unit builds to CI.

ID: REV-003  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `String.h`  
Line(s): 418–419, 491–497, 518–531  
Title: Move and modifier APIs violate their documented semantics

Description:  
The move constructor delegates to the copy constructor; the allocator move constructor passes a named lvalue to an rvalue-only helper. `reserve` shrinks and then reallocates again because the shrink branch does not return. Erasing a suffix sets the whole string size to zero rather than preserving the prefix.

Impact:  
Valid programs fail to instantiate, lose data, or perform redundant allocation.

Trigger Conditions:  
Move with an allocator, reserve below current capacity, or erase through the end from a nonzero position.

Root Cause:  
Incorrect forwarding and branch/state calculations.

Recommendation:  
Specify and test every constructor/modifier against `std::basic_string`-like edge cases.

ID: REV-004  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `cstring.h`  
Line(s): 37–55, 190–221, 535–540  
Title: Empty strings dereference null and direct `realloc` assignment destroys recovery

Description:  
The empty representation uses `data == NULL`, while `string_clear` and empty assignment write `data[0]`. Multiple functions assign `realloc` directly to the sole pointer; allocation failure loses the old allocation and leaves size/capacity inconsistent.

Impact:  
Null-pointer writes, leaks, and corrupted objects occur in core operations.

Trigger Conditions:  
Clear/assign an empty string, or experience allocation failure during mutation.

Root Cause:  
Inconsistent empty invariants and failure-unsafe allocation updates.

Recommendation:  
Use a consistent allocated-empty or guarded-null invariant and commit `realloc` only through a temporary pointer.

ID: REV-005  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `cstring.h`  
Line(s): 105–175, 426–464  
Title: Search, erase, copy, and substring algorithms return incorrect ranges

Description:  
`string_find` never rejects a candidate after a character mismatch. The not-of searches implement the wrong quantifier. Suffix erase clears the entire string. Copy/substr use `len + pos` as the amount, over-reading/copying beyond the requested span; `npos` arithmetic can overflow.

Impact:  
Callers receive false matches, lose prefixes, or expose memory outside the requested logical range.

Trigger Conditions:  
Partial-prefix search mismatches, nonzero copy positions, suffix erasure, and `npos` defaults.

Root Cause:  
Range endpoints and loop predicates are calculated incorrectly.

Recommendation:  
Centralize checked `[first,last)` calculations and add differential tests against standard string behavior.

ID: REV-006  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Result.h`  
Line(s): 143–157  
Title: Formatted errors write outside the string's logical size and typed formatting does not compile

Description:  
The variadic constructor calls `reserve` then writes through `data()` without `resize`, so `m_What` remains logically empty and the write is outside its element range. The typed variadic overload omits the parameter name and initializes from `type`, which resolves incorrectly; its delegated temporary is discarded.

Impact:  
Error text is lost and memory is modified outside the string's valid element range; typed errors fail to instantiate.

Trigger Conditions:  
Construct `Error("%d", value)` or the typed variadic equivalent.

Root Cause:  
Capacity was mistaken for size and constructor delegation was written as a body expression.

Recommendation:  
Size the string before formatting and use a named `type` with proper delegating constructors.

ID: REV-007  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Result.h`  
Line(s): 225–282  
Title: Union copy/move and alternative-changing assignment violate object lifetimes

Description:  
Copy/move constructors assign into inactive union storage instead of placement-constructing the selected member. Assignments change `m_Valid` before destroying the prior member and then assign into the inactive alternative. `noexcept` is unconditional even when member moves can throw.

Impact:  
Nontrivial result/error types encounter undefined behavior, leaks, skipped destructors, or termination.

Trigger Conditions:  
Copy/move any result with nontrivial members, especially assignment from Ok to Err or vice versa.

Root Cause:  
Manual variant lifetime management is incomplete.

Recommendation:  
Use `std::variant` or correctly construct/destroy alternatives with conditional exception specifications.

ID: REV-008  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `array.h`, `stack.h`  
Line(s): `array.h` 531–539; `stack.h` 125–153  
Title: Latent tuple/stack APIs fail when instantiated

Description:  
Rvalue `get` refers to nonexistent `_Elems` rather than `m_Elems`. Free stack `swap` takes const references then calls a mutating member. The `uses_allocator` specialization attempts to inherit from a value/member form rather than a valid trait type.

Impact:  
Tuple-style access, const swap calls, and allocator-trait use produce compilation failures.

Trigger Conditions:  
Instantiate those otherwise advertised templates.

Root Cause:  
Template paths were not compile-tested.

Recommendation:  
Add compile-only API coverage for cv/ref combinations, swapping, and standard traits.

ID: REV-009  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `stack_vector.h`  
Line(s): 432–492, 593–597  
Title: Equality ignores size and release-mode resize can exceed storage

Description:  
Equality compares only `[left.begin(), left.end())` against `right.begin()` and never checks sizes, making a prefix equal to a longer vector. `resize(count)` relies only on `assert(count <= N)` and writes/records beyond capacity in release builds. Pop/clear also retain resources because all `N` elements remain constructed.

Impact:  
Incorrect comparisons, possible out-of-bounds writes, and surprising resource retention.

Trigger Conditions:  
Compare unequal-length prefix values or call oversize `resize` with `NDEBUG`.

Root Cause:  
Capacity-oriented storage semantics were mixed with vector-like logical semantics.

Recommendation:  
Check both sizes, enforce capacity in all builds, and document or redesign element lifetime behavior.

ID: REV-010  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Vector.h`  
Line(s): 1289, 1321, 1340, 1503, 2220  
Title: Multiple public vector operations are syntactically invalid on instantiation

Description:  
Examples include dereferencing a scalar component, passing a pointer where a reference is required, assigning to a const local, and constructing Vector3/Vector4 results with the wrong component count. These errors remain hidden until templates are used.

Impact:  
Substantial portions of the advertised API cannot compile.

Trigger Conditions:  
Instantiate affected projection, normalization, rotation, or clamp operations.

Root Cause:  
Only parsing, not representative template instantiation, appears to have been tested.

Recommendation:  
Build an explicit instantiation matrix for every public operation and scalar type.

ID: REV-011  
Category: Potential Issue  
Confidence: High  
Severity: 3  
File: `Vector.h`  
Line(s): arithmetic operations throughout; projection/normalization families  
Title: Numeric APIs do not define zero, overflow, or invalid-shift behavior

Description:  
Integral dot/length operations can overflow; division, modulo, projections, and normalization lack zero-denominator handling; shifts accept invalid counts; and clamping assumes ordered bounds. For signed integers several paths are C++ undefined behavior.

Impact:  
Inputs at numeric boundaries can produce UB, infinities/NaNs, or incorrect geometry.

Trigger Conditions:  
Large integer components, zero-length vectors, zero divisors, invalid shifts, or reversed bounds.

Root Cause:  
Generic operators expose operations whose preconditions differ substantially by scalar category.

Recommendation:  
Constrain operations and specify checked behavior for invalid numeric domains.

ID: REV-012  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Hash.h`  
Line(s): 696–721  
Title: The advertised C interface is not C-compatible

Description:  
The C-enabled portion uses `std::size_t` and `std::memcpy`. A C11 translation unit fails to parse the header.

Impact:  
C consumers cannot use any interface despite the header's language/configuration claims.

Trigger Conditions:  
Include `Hash.h` from C with the default C interface enabled.

Root Cause:  
C++ namespace qualifiers leaked into shared implementation code.

Recommendation:  
Compile the header under both strict C and C++ modes in CI.

ID: REV-013  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `Hash.h`  
Line(s): 3918–3965  
Title: SHAKE caller-buffer mode overflows a fixed stack buffer

Description:  
When the caller supplies `buffer`, the binary output remains pointed at a fixed 32-byte `intBuff` even when `outsizeBytes / 2` is larger. Keccak writes the requested binary length into it. In allocation mode, allocation is unchecked and the returned pointer points into the allocation, so it cannot be correctly freed.

Impact:  
Stack memory corruption and unmanageable leaked heap allocations.

Trigger Conditions:  
Request a SHAKE output greater than 64 hex characters with a caller buffer, or request a large easy-mode output.

Root Cause:  
Output storage selection accounts for large results only when `buffer == NULL` and exposes an interior pointer ownership contract.

Recommendation:  
Write binary output to correctly sized storage, validate multiplication/allocation, and use an explicit caller-owned output API.

ID: REV-014  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `Hash.h`  
Line(s): 799–802, 851–861, 2742–2775  
Title: SHA-512/t validation permits invalid lengths and release builds write out of bounds

Description:  
Validation is assertion-only and permits sizes up to 2048 (the template even states 8192), although the produced SHA-512 state contains only 512 bits. `hash_sha512t_hexdigest` always formats 128 hex characters and then writes a terminator at `t / 4`; with assertions removed, large `t` writes beyond the 513-byte static buffer.

Impact:  
Global-buffer memory corruption and invalid/nonstandard digests.

Trigger Conditions:  
Call the public runtime-size API with an invalid large `t` in an `NDEBUG` build.

Root Cause:  
Algorithm constraints are incorrect and enforced only by debug assertions.

Recommendation:  
Reject invalid `t` at runtime, constrain templates at compile time, and size output from validated limits.

ID: REV-015  
Category: Reliability Concern  
Confidence: High  
Severity: 4  
File: `Hash.h`  
Line(s): 121–159, 2100–2160, digest buffers throughout  
Title: File and convenience hash paths lose errors, leak resources, and are not thread-safe

Description:  
The C file loader does not close on allocation failure and does not check seek/tell/read results. File hash streaming does not distinguish EOF from read error. C++ path overloads pass `string_view::data()` to APIs requiring termination. Numerous easy functions return shared static buffers.

Impact:  
Wrong digests can be presented as valid, descriptors/memory leak, nonterminated path views over-read, and concurrent calls race/corrupt results.

Trigger Conditions:  
I/O failure, allocation failure, sliced path views, or concurrent convenience calls.

Root Cause:  
Failure and ownership channels are represented as empty strings/raw static pointers rather than explicit results.

Recommendation:  
Propagate I/O status, use path/string objects with known length, close on all exits, and make output caller-owned.

ID: REV-016  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `hash.cpp`  
Line(s): 209–215, 232–240, 283–287  
Title: Long options are unreachable and valid search hashes are rejected

Description:  
The comparisons for `--no-decorator` and `--no-threads` include an initial space, so real command-line options fall through as input. Search digest text length is compared with a byte digest length, rejecting valid hex digests. Thread parsing accepts numeric suffixes and has no practical upper bound.

Impact:  
Documented CLI behavior fails and search cannot be used with valid digest text.

Trigger Conditions:  
Use either long option or pass a normal hexadecimal digest to `--search`.

Root Cause:  
Literal typos and confusion between binary and encoded sizes.

Recommendation:  
Use a table-driven parser and algorithm metadata with distinct byte/hex lengths.

ID: REV-017  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `Hash.dart`  
Line(s): 181–188  
Title: String hashing is not UTF-8 and fails for non-Latin-1 text

Description:  
`UpdateString` assigns UTF-16 code units into a `Uint8List`. Values above 255 raise a range error; even accepted non-ASCII text is not encoded as UTF-8.

Impact:  
Unicode strings cannot be reliably hashed and disagree with other implementations.

Trigger Conditions:  
Hash a string containing characters outside the Latin-1 byte range.

Root Cause:  
Character code units are treated as encoded bytes.

Recommendation:  
Encode strings explicitly as UTF-8 and document the encoding.

ID: REV-018  
Category: Likely Bug  
Confidence: High  
Severity: 5  
File: `ThreadPool.hpp`, `thread_pool_test.cpp`  
Line(s): `ThreadPool.hpp` 519–538, 947–1004; test 959–1003  
Title: Concurrent resize can free a worker selected by submission

Description:  
General submission selects a raw `WorkerState*` under `m_WorkersMutex`, releases that shared lock, then locks the worker mutex. Concurrent shrink can acquire the unique container lock, move/remove the owning slot, join it, and destroy the state before the submitter dereferences the raw pointer. The indexed submission path correctly retains a `shared_ptr`, highlighting the mismatch.

Impact:  
A documented thread-safe operation pair can cause use-after-free, mutex access on freed storage, and process corruption.

Trigger Conditions:  
A submitter is descheduled after line 535 while another thread shrinks past the selected worker.

Root Cause:  
Selected-worker lifetime is not retained across the lock handoff.

Recommendation:  
Retain a `shared_ptr<WorkerState>` or keep the container read lock until the worker lock/lifetime is secured; add sanitizer/model-check stress.

ID: REV-019  
Category: Likely Bug  
Confidence: High  
Severity: 4  
File: `ThreadPool.hpp`  
Line(s): 693–701, 741–749, 947–1004, 1007 onward  
Title: Thread creation is not exception-safe and worker-initiated lifecycle calls can self-join

Description:  
`CreateWorkerSlot` starts a thread before allocating the slot; allocation failure destroys a joinable local thread and terminates. Failure while growing during construction can likewise unwind a vector containing joinable threads without invoking `ThreadPool`'s destructor. Resize/shutdown called by a pool worker may move and then join that same thread.

Impact:  
Allocation or thread failures and reentrant lifecycle use can terminate, throw from a `noexcept` destructor, or deadlock.

Trigger Conditions:  
Resource exhaustion during creation, or a task calling shrink/shutdown.

Root Cause:  
Thread ownership is published after launch and lifecycle APIs do not guard worker context.

Recommendation:  
Use rollback guards for every started thread and reject/defer self-joining lifecycle calls.

ID: REV-020  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `Core.h`, `README.md`  
Line(s): `Core.h` 223–229, 3073, 3190, 3240; README Core example  
Title: Configuration overrides, platform detection, and documentation disagree

Description:  
User-configurable macros are unconditionally redefined to 1. The FMA alias uses an undefined macro name. Windows ARM/AMD paths include Boost.Predef headers despite the header being presented as standalone. Documentation uses nonexistent `CORE_LANG_STDCPP_C17` and contains an example missing a semicolon.

Impact:  
Feature disabling does not work, some targets fail or misdetect capabilities, and copied examples fail.

Trigger Conditions:  
Override config macros, target affected Windows/SIMD combinations, or follow documented examples.

Root Cause:  
Platform branches and documentation are not compiled as a matrix.

Recommendation:  
Honor predefinitions, remove/declare Boost dependencies, correct FMA naming, and compile documentation snippets.

ID: REV-021  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: `Profiler.h`, `ProgressBar.h`  
Line(s): `Profiler.h` 45–76; `ProgressBar.h` 45–73  
Title: Zero, decreasing, nonfinite, and concurrent states are not handled

Description:  
Profiler averages divide by a run count guarded only by an assertion and use shared static mutable state without synchronization. Progress calculation divides by `hundred`, converts unchecked floating values to `size_t`, and subtracts unsigned previous progress from decreasing input. Nonfinite/out-of-range float-to-integer conversion is undefined.

Impact:  
Division by zero, UB, huge increments, races, and nonsensical output.

Trigger Conditions:  
Log before any run, initialize total zero, report decreasing/oversize values, or use concurrently.

Root Cause:  
Display helpers assume a single valid monotonic caller and encode preconditions only informally.

Recommendation:  
Validate state in release builds, clamp progress, use a steady clock, and make state instance-owned or synchronized.

ID: REV-022  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `BrainfuckInterpreter.c`, generated code from `BrainfuckCCompiler.c`  
Line(s): interpreter 104–121; compiler generator 240–330  
Title: Tape pointer underflow and overflow access outside the allocated tape

Description:  
The tape index is `uint16_t` while the tape has 30,000 elements. `<` at zero wraps to 65535; repeated `>` can similarly reach indices beyond 29,999. Neither interpreter nor generated C checks bounds.

Impact:  
Immediate out-of-bounds memory access and exploitable memory corruption for untrusted programs.

Trigger Conditions:  
Run the one-character program `<` at the initial tape position.

Root Cause:  
Index type range was mistaken for tape bounds.

Recommendation:  
Define explicit checked/wrapping tape semantics and enforce them identically in interpreter and generator.

ID: REV-023  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `BrainfuckInterpreter.c`, `BrainfuckCCompiler.c`  
Line(s): interpreter 20–62; compiler 37–64, 378–408  
Title: Filtered input retains uninitialized code and compiler failure is reported as success

Description:  
Readers advance an output pointer only for valid Brainfuck characters but place the terminator at the original file size, leaving the filtered tail uninitialized. A `char` is compared with `EOF`, causing 0xFF confusion or nontermination depending on signedness. The compiler overwrites fixed `out.c`, ignores compilation status, and may remove output while still returning success.

Impact:  
Programs can execute/generate random instructions, hang at EOF, overwrite an unrelated file, or claim success without an executable.

Trigger Conditions:  
Source containing comments/noncommands, byte 0xFF, compilation failure, or an existing `out.c`.

Root Cause:  
Input and subprocess outcomes are not tracked independently from nominal file length.

Recommendation:  
Track filtered length with `int` EOF handling, validate brackets, use caller-selected temporary output, and propagate compiler exit status.

ID: REV-024  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `AssetFinder.cpp`  
Line(s): 336–343, 396–536  
Title: Short and malformed files drive parser reads beyond the buffer

Description:  
The loop bound `buffer.size() - 24` underflows for short input. Format parsers trust OGG segment counts/lacing and several embedded length fields before establishing that headers, bodies, or `i + size` fit. A one-byte file produces a sanitizer-detected heap-buffer overflow.

Impact:  
Arbitrary local input can crash the extractor or disclose/corrupt process memory.

Trigger Conditions:  
Input shorter than 24 bytes or crafted RIFF/OGG/PNG-like length fields.

Root Cause:  
Unsigned size arithmetic and format-specific reads occur without a checked cursor abstraction.

Recommendation:  
Use overflow-safe remaining-length checks before every read and advance.

ID: REV-025  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `AssetFinder.cpp`  
Line(s): 396, 457, 488–489, 523 and size arithmetic nearby  
Title: Binary parsing violates alignment/aliasing rules and overflows length arithmetic

Description:  
Byte-vector addresses are cast to multi-byte integer pointers and dereferenced, which can be unaligned and violates effective-type/aliasing rules. RIFF length addition and `i + fileSize` can overflow; zero-sized chunks make skip adjustments underflow.

Impact:  
Undefined behavior on strict-alignment platforms and validation bypass/crashes on crafted sizes.

Trigger Conditions:  
Unaligned signature offsets, large/zero chunk lengths, or 32-bit targets.

Root Cause:  
Host-native typed loads and unchecked arithmetic are used for serialized data.

Recommendation:  
Decode bytes explicitly with checked addition and format endianness.

ID: REV-026  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Convert2Binary.cpp`, `InputHandler.h`  
Line(s): converter 28, 65–75, 230–263, 318–336; handler 69–78, 125–175  
Title: Converter does not portably compile, console mode always fails, and partial lines over-read

Description:  
The `sprintf_s` signature is nonportable/incorrect, hex initialization uses multicharacter constants, and signed-character shifting can be undefined. Console mode tests a default unopened `ofstream` and exits. The final block walks to configured width without checking the actual remainder. Width zero divides by zero. `InputHandler::isNumber` accepts numeric prefixes and uses ctype with possibly negative chars.

Impact:  
Build failure, unusable mode, out-of-bounds reads, division by zero, and invalid input acceptance.

Trigger Conditions:  
Non-MSVC build, `-c`, partial final input block, zero/negative width, or suffixed numbers.

Root Cause:  
Platform-specific formatting and unchecked parser/loop assumptions.

Recommendation:  
Use portable formatting, byte-safe shifts, explicit mode validation, and bounded remainder loops.

ID: REV-027  
Category: Confirmed Bug  
Confidence: High  
Severity: 5  
File: `backup.cpp`  
Line(s): 42–79, 498–513  
Title: Missing source/destination containment checks can delete source data

Description:  
The program accepts overlapping trees. If destination is below source, destination content becomes part of the source inventory with a different relative prefix; mirror deletion can then remove the original source item. A controlled temporary-directory reproduction deleted the source while retaining the nested copy.

Impact:  
The backup tool can destructively delete the authoritative source.

Trigger Conditions:  
Destination equals, contains, or is contained by source in unsafe arrangements.

Root Cause:  
Canonical path relationships are never validated before mutation.

Recommendation:  
Canonicalize both roots, reject all overlap, and make validation mandatory before inventory/copy/delete.

ID: REV-028  
Category: Likely Bug  
Confidence: High  
Severity: 5  
File: `backup.cpp`  
Line(s): 42–79, 380–417, 498–513  
Title: Partial source inventory is treated as authoritative for deletion

Description:  
Per-entry indexing errors are counted and ignored. The resulting partial source set is still passed to `DeleteAllFiles`, which removes every destination entry absent from it. Thus unreadable/transiently failing source entries appear deleted.

Impact:  
Permission, I/O, race, or filesystem errors can cause valid backup data loss.

Trigger Conditions:  
Any source entry fails metadata/relative/timestamp inspection while its counterpart exists in destination.

Root Cause:  
Inventory completeness is not an invariant for the destructive phase.

Recommendation:  
Abort all deletion if either inventory is incomplete; record explicit scan status and use a staged plan/dry run.

ID: REV-029  
Category: Reliability Concern  
Confidence: High  
Severity: 4  
File: `backup.cpp`  
Line(s): copy comparison/creation paths 200–380  
Title: Timestamp-only freshness and swallowed creation failures produce false success

Description:  
Existing files are selected by timestamps without size/content verification, so same-timestamp changes are missed. Some empty-file/directory creation failures are logged but do not affect final exit status. Recursive iterator construction/increment can also throw outside the per-entry try block.

Impact:  
Destination silently diverges from source while the process exits successfully.

Trigger Conditions:  
Coarse/preserved timestamps, failed empty entry creation, or traversal errors.

Root Cause:  
No end-to-end success model or post-copy verification.

Recommendation:  
Compare at least size+timestamp (optionally hash), make all failures affect status, and verify the completed mirror before deletion/success.

ID: REV-030  
Category: Likely Bug  
Confidence: High  
Severity: 4  
File: CWave platform audio backends  
Line(s): ring-buffer enqueue/dequeue implementations in Linux, macOS, and Windows backends  
Title: Audio ring buffers cannot distinguish full from empty and may race on sample storage

Description:  
The shared design publishes only atomic read/write indices. A producer can lap the consumer, overwrite unread samples, and make equal indices appear empty. If lapping overlaps consumer access, the non-atomic float elements are concurrently read/written.

Impact:  
Dropped/corrupted audio and a C++ data race under sustained capture or stalled rendering.

Trigger Conditions:  
Producer advances by at least ring capacity before consumer catches up.

Root Cause:  
No full-state/distance accounting or overflow policy in the SPSC ring contract.

Recommendation:  
Use monotonic counters or reserve one slot, enforce acquire/release publication, and define drop-old/drop-new behavior.

ID: REV-031  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `CWave/src/main.cpp`, `CWave/src/fft.cpp`  
Line(s): main configuration and 283–359; FFT public functions  
Title: Invalid runtime configuration and one-window break lose data or invoke invalid math

Description:  
Only limited FFT/fps validation occurs. FFT size 1 makes the Hann denominator zero; invalid ranges/pivot/smoothing and nonfinite `atof` values reach logarithms and allocations. `frameMs` becomes zero above 1000 FPS. After one FFT, the loop breaks and discards all remaining samples already pulled into `pullBuf`.

Impact:  
NaNs, hot loops, failed/huge allocations, spectrum gaps, and misleading visualization.

Trigger Conditions:  
Boundary/nonfinite CLI values or a pull containing more than one processable window.

Root Cause:  
Configuration lacks a complete domain schema and sample ownership ends at the pull call despite unconsumed data.

Recommendation:  
Validate all finite ranges and retain/process every pulled sample.

ID: REV-032  
Category: Likely Bug  
Confidence: High  
Severity: 4  
File: CWave platform backends and `CWave/CMakeLists.txt`  
Line(s): WASAPI format/COM init; PulseAudio operations; CoreAudio enqueue; CMake MinGW flags  
Title: Platform backends make unsafe format/resource assumptions

Description:  
WASAPI treats every non-float format as 16-bit PCM, including 24/32-bit and extensible formats, and balances `CoUninitialize` even for changed-mode initialization. PulseAudio operation pointers are used without null checks. macOS callback enqueue errors are ignored. MinGW uses `-mwindows` although the application requires a terminal.

Impact:  
Corrupt samples, COM imbalance, null dereference, lost callbacks, or invisible terminal UI.

Trigger Conditions:  
Non-16-bit default formats, resource failures, or MinGW builds.

Root Cause:  
Backend success/format variants are narrowed without validation.

Recommendation:  
Negotiate formats explicitly, check every platform return, and align linker subsystem with UI behavior.

ID: REV-033  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Tetris/src/game.cpp`  
Line(s): 73–80 and rotation logic  
Title: Hold can install a colliding piece and counterclockwise rotation uses the wrong kick transform

Description:  
The nonempty hold swap assigns the held piece as current without validating placement. Subsequent locking can overwrite occupied cells. Counterclockwise rotation reuses the clockwise kick lookup direction rather than applying the inverse SRS transition offsets.

Impact:  
Board state corruption and rotations that disagree with expected Tetris behavior.

Trigger Conditions:  
Swap into a blocked spawn area or perform CCW rotations near walls/blocks.

Root Cause:  
State transitions commit before collision validation and kick tables are not indexed by full from/to state.

Recommendation:  
Validate/rollback hold placement and use explicit SRS transition tables for both directions.

ID: REV-034  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: Tetris game/model/renderer  
Line(s): `game.cpp` 237; public cell/type indexing; renderer constructor/layout  
Title: Restart is recursive and public indices rely on unchecked invariants

Description:  
Restart recursively calls the game loop, retaining one stack frame per game. Public cell/type/rotation access assumes valid indices. Renderer centering is computed only at construction despite documentation implying terminal resizing adapts.

Impact:  
Long sessions can exhaust stack; API misuse can read outside tables; resized output is misplaced.

Trigger Conditions:  
Many restarts, invalid public inputs, or resize after startup.

Root Cause:  
Session control and model preconditions are implicit.

Recommendation:  
Use an iterative session loop, validate public indices, and recompute layout per frame/resize.

ID: REV-035  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `Input/Input.h`, `Input/tc.h`, `Input/main.cpp`  
Line(s): `Input.h` 330–440; terminal helpers throughout  
Title: Input listener has data races, unsafe failed initialization, and blocking shutdown

Description:  
Allocation/command results are unchecked. On open/thread failure, returned `KEYL` fields can be uninitialized, yet close joins/uses them. Listener and callers access global key state without synchronization. Close sets a flag then joins while the listener may remain blocked in `read`; one return occurs while holding the stop mutex. Terminal ioctl/tcgetattr results are ignored and stdout is used for input terminal attributes.

Impact:  
Undefined behavior, hangs, invalid joins, stale terminal no-echo state, and root/TTY-dependent failure.

Trigger Conditions:  
Initialization failure, idle input during close, concurrent key polling, or redirected stdout.

Root Cause:  
Thread, terminal, and allocation lifecycles have no explicit valid state/error channel.

Recommendation:  
Return initialized status-bearing handles, use poll/self-pipe cancellation, synchronize state, and restore terminal settings with checked RAII-like cleanup.

ID: REV-036  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `snake_console.c`  
Line(s): 29–44, 80–110, 211–224, 466–535  
Title: Windows movement tests the wrong key and parser/storage calculations are unsafe

Description:  
Windows key macros put boolean `||` inside `KEY_PRESSED`, so `GetAsyncKeyState` receives 0/1 instead of key codes and movement does not register. Strict POSIX C lacks required feature declarations. Argument parsing casts `size_t*` to `long*`, ignores tails/range, and unchecked `width * height` can wrap before allocation.

Impact:  
Broken controls, compile failure, aliasing/LLP64 corruption, and undersized allocations followed by out-of-bounds writes.

Trigger Conditions:  
Windows play, strict C build, invalid/large dimensions, or LLP64 parsing.

Root Cause:  
Macro parenthesization and cross-platform integer/API assumptions.

Recommendation:  
Evaluate each key state separately, define POSIX feature macros, parse into an intermediate checked unsigned type, and guard multiplication.

ID: REV-037  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `hangman.cpp`, `tictactoe.cpp`  
Line(s): hangman 270–310 and restart state; tictactoe 124–170  
Title: Interactive input and restart paths access invalid state

Description:  
Hangman does not check `fopen` before `getc`, stores input in `int8_t` so EOF is not portable, samples an inclusive line index that can select past the final word, and does not reset all guess counters on restart. Tic-tac-toe's occupied-cell retry decrements and indexes an unvalidated value, with failed stream state also unhandled.

Impact:  
Null dereference, instant/incorrect games, persistent stale state, or vector out-of-bounds access.

Trigger Conditions:  
Missing word file, trailing-newline selection, restart, EOF/non-numeric input, or retry value 0/out of range.

Root Cause:  
Input validation is applied only on the first path and session state is incompletely reset.

Recommendation:  
Centralize checked input loops and reset the complete game state for every session.

ID: REV-038  
Category: Likely Bug  
Confidence: High  
Severity: 4  
File: `image_to_ascii.cpp`  
Line(s): image members/load and pixel extraction  
Title: Failed loads and one/two-channel images lead to invalid pixel access

Description:  
Width, height, and channel members are uninitialized before `stbi_load`; failure is not treated as terminal. Extraction always reads three components, even for grayscale or gray-alpha images. The glyph calculation uses modulo by threshold, selecting only a small prefix of the 31-character ramp rather than scaling intensity.

Impact:  
Invalid loops/null or out-of-bounds reads and materially incorrect output.

Trigger Conditions:  
Missing/corrupt image, one/two-channel image, or normal brightness conversion.

Root Cause:  
Decoder status/channel contract is ignored and mapping math is incorrect.

Recommendation:  
Initialize state, fail closed, request/handle a fixed channel count, and scale intensity across the full ramp.

ID: REV-039  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `imf/imf.cpp`  
Line(s): Image copy/move assignment and filter API, approximately 50–130  
Title: Image assignment leaks ownership and const filters mutate their input

Description:  
Copy and move assignment overwrite an existing `m_Data` pointer without freeing it and do not safely handle self-assignment. Const indexing returns mutable storage; filters take `const Image&`, mutate it, and return by value, producing confusing copies and repeated leaks. Pixel count multiplication occurs in `int` before allocation sizing.

Impact:  
Leaks, self-move/copy corruption, violation of const contracts, and possible undersized allocation for large dimensions.

Trigger Conditions:  
Assign into a loaded image, chain filters, self-assign, or load sufficiently large dimensions.

Root Cause:  
Manual ownership and mutability semantics are inconsistent.

Recommendation:  
Use a vector/RAII buffer, copy-and-swap or correct rule-of-five, const-correct access, and checked `size_t` multiplication.

ID: REV-040  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `repacker.cpp`  
Line(s): 128–169 and generated extraction metadata  
Title: Content hashes are used as identities and paths are emitted into unsafe raw strings

Description:  
Generated identifiers/header names depend only on MD5 content. Two distinct paths with identical content collide, overwrite/reuse metadata, and can map extraction to the wrong name/path. File paths are inserted into `R"(...)"`; a path containing `)"` terminates the literal and breaks/injects generated C++.

Impact:  
Missing/misnamed packaged files and code-generation failure or injection through filenames.

Trigger Conditions:  
Two equal-content files or a legal filename containing the raw-string terminator.

Root Cause:  
Payload deduplication identity is conflated with per-path metadata, and generated literals are not escaped/delimited safely.

Recommendation:  
Give each path unique metadata, optionally deduplicate bytes separately, and use escaped literals or collision-free raw delimiters.

ID: REV-041  
Category: Reliability Concern  
Confidence: High  
Severity: 4  
File: `repacker.cpp`, `repacker-zip.cpp`  
Line(s): compile/wrapper command construction and temporary cleanup  
Title: Shell command construction, ignored exit status, and weak temporary naming make packaging unreliable

Description:  
Compiler and wrapper command lines concatenate executable paths/arguments without platform-correct quoting and invoke `system`. Compiler return status is ignored and main can delete intermediates/return success. Millisecond-derived temporary directory names can collide; cleanup may remove a preexisting collision path. Generated extraction writes also ignore failures before launching.

Impact:  
Wrong command execution, argument corruption, false-success packages, lost diagnostics/artifacts, or unintended directory removal.

Trigger Conditions:  
Spaces/quotes/metacharacters, compiler failure, simultaneous invocations, or extraction errors.

Root Cause:  
Process and temporary-resource management are represented as strings with no checked result model.

Recommendation:  
Use platform process APIs/argument arrays, secure unique temp creation, scoped cleanup, and propagate every status.

ID: REV-042  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `zip_file.h`, `repacker-zip.cpp`  
Line(s): `zip_file.h` 5583, 5611; repacker archive/setup paths  
Title: Zip library callback types fail on LP64 and the zip repacker lacks portable build coverage

Description:  
`detail::write_callback` uses `std::uint64_t`, which is `unsigned long` on LP64, while miniz requires an `unsigned long long` callback. The registrations are ill-formed on Linux. The header also embeds a second raw-string copy of its own source, increasing review/build drift risk.

Impact:  
`repacker-zip.cpp` and even a minimal include/use program cannot compile on common 64-bit Unix targets.

Trigger Conditions:  
Compile on an LP64 ABI.

Root Cause:  
Fixed-width and API-declared integer types were assumed interchangeable.

Recommendation:  
Match the exact callback typedef, add LP64/LLP64 builds, and generate embedded source from one canonical file.

ID: REV-043  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `data.cpp`  
Line(s): 190–203  
Title: Hardware concurrency 0/1 causes underflow or division by zero in a disk-filling utility

Description:  
`threadsToSpawn = hardware_concurrency() - 1`. A result of one produces zero, then file count is divided/modulo by zero. A permitted result of zero underflows to a huge unsigned count. Remaining-file count is narrowed to `uint8_t`. The utility intentionally sizes output to reported free disk space without reserve/safety margin.

Impact:  
Crash, attempted creation of enormous thread counts, incorrect distribution, or complete disk exhaustion affecting the system.

Trigger Conditions:  
Platforms/containers reporting 0 or 1 hardware thread, large remainder, or normal execution on a live volume.

Root Cause:  
Concurrency hints and free-space measurements are treated as safe exact work budgets.

Recommendation:  
Require explicit bounded user limits, normalize thread count to at least one, preserve wide remainders, and reserve filesystem space.

ID: REV-044  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: `sum.cpp`, `reminder.cpp`, `time.cpp`  
Line(s): sum 40–90; reminder 70–115 and 190–220; time 180–210  
Title: Small utilities ignore parser, numeric, process, and lifecycle failure paths

Description:  
`sum` ignores exprtk compile failure and accepts `stoi` prefixes/out-of-range exceptions. Reminder accepts trailing text and nonfinite `stod` values; NaN later reaches float-to-DWORD conversion, while COM initialization/cleanup and console-handler cleanup can race or become unbalanced. `time` reconstructs a Windows command line without quoting, leaks `CommandLineToArgvW`, and uses unchecked process timing results.

Impact:  
Silent wrong sums, crashes/UB, double cleanup, broken child arguments, leaks, and uninitialized timing data.

Trigger Conditions:  
Invalid expressions/numbers, NaN/Infinity, console termination races, or paths/arguments with spaces/quotes.

Root Cause:  
Convenience parsers and platform APIs are assumed successful and losslessly composable.

Recommendation:  
Require full-token parsing, finite/range checks, checked platform results, single-owner cleanup, and proper argument-vector process launch.

ID: REV-045  
Category: Confirmed Bug  
Confidence: High  
Severity: 4  
File: `descend_calc/descend_calc.s`  
Line(s): 1–90  
Title: Assembly port discards wind input and mixes signed arithmetic with unsigned division

Description:  
The program targets Windows x64 calling convention. It saves wind in `%edi`, calls input again for direction, then uses `%eax` (the direction 0/1) in the final addition/subtraction instead of wind. It uses unsigned `div` for signed user values and fails to preserve `%r15` as required.

Impact:  
Results ignore the entered wind speed, negative values calculate incorrectly/fault, and ABI callers can be corrupted.

Trigger Conditions:  
Any wind other than the direction value; negative inputs; ABI-sensitive invocation.

Root Cause:  
Register ownership/liveness and signedness were not designed across calls.

Recommendation:  
Assign stack/callee-saved storage deliberately, use signed division where intended, and add known-answer ABI tests.

ID: REV-046  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: other 26 files in `descend_calc/`  
Line(s): input loops and final arithmetic throughout  
Title: Language ports inconsistently handle EOF, overflow, parsing, and labels

Description:  
Most fixed-width ports subtract integer altitudes before conversion, producing language-specific overflow/UB/wrap/panic. Many retry loops do not handle EOF and either spin, unwrap null, or throw. Several parsers accept numeric prefixes; JS/TS exceed safe-integer precision. The Lua output labels wind-adjusted total as altitude to lose. MATLAB/function-file structure and numerous runtime assumptions reduce portability.

Impact:  
The same inputs yield different results, hangs, or crashes across the purported equivalent implementations.

Trigger Conditions:  
Extreme integers, EOF, suffix input, unsupported selector values, or large JS numbers.

Root Cause:  
No shared behavioral specification or cross-language conformance vectors.

Recommendation:  
Define domains/EOF/overflow/selector semantics and run identical generated vectors in every available implementation.

ID: REV-047  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `sizeof/sizeof.c`, `sizeof/makefile`, twelve `sizeof/*.exe` files  
Line(s): C source 21–45; makefile throughout  
Title: `sizeof` values are passed with incompatible variadic formats and build labels are unreliable

Description:  
`sizeof` produces `size_t`, but macros print it with `%lld`, `%llu`, or `%d` without casting, which is variadic type mismatch/UB. Clang is detected as GCC because GCC macros are checked first. The makefile starts with Windows `cls`, applies C++ flags to C, and labels GCC x86 targets without `-m32`. Committed executables cannot be verified against source.

Impact:  
Wrong output/UB, incorrect compiler labels, failed/nonrepresentative builds, and irreproducible artifacts.

Trigger Conditions:  
Platforms where `size_t` differs from the chosen format, Clang builds, or make targets outside the original host.

Root Cause:  
Platform formatting and artifact naming are inferred through brittle macros/commands.

Recommendation:  
Use `%zu`, order compiler detection correctly, make architecture flags explicit, and generate binaries in CI rather than versioning them.

ID: REV-048  
Category: Confirmed Bug  
Confidence: High  
Severity: 3  
File: `nks.py`  
Line(s): 1–12 and numerical implementations  
Title: Program cannot start because a required module is absent

Description:  
The top-level import requires `pi_million`, which is neither tracked nor declared as a dependency. Independently, algorithms advertised with arbitrary precision call `math.tan`/NumPy floating operations, collapsing precision to binary64.

Impact:  
Immediate import failure in a clean checkout; misleading accuracy if the missing module is supplied.

Trigger Conditions:  
Run from the repository without a coincidentally installed module.

Root Cause:  
Runtime dependencies and precision contracts are undocumented/unvalidated.

Recommendation:  
Include or declare dependencies and keep arbitrary-precision calculations within a compatible numeric library.

ID: REV-049  
Category: Design Concern  
Confidence: High  
Severity: 2  
File: repository build/docs/tests/assets/configuration  
Line(s): repository-wide  
Title: No root build/CI/dependency manifest provides reproducible coverage

Description:  
Only two subprojects have CMake and two have makefiles. Root documentation contains stale names and CWave dependency/platform contradictions. Hash tests are a 21 MiB generated source but omit SHAKE, C-interface, file-error, CLI, and SHA-512/t boundary coverage. ThreadPool tests pass but do not deterministically control the raw-pointer race window. No Tetris/CWave unit tests exist. Vendored headers have no provenance lock, and binary outputs are committed despite ignore rules.

Impact:  
Large API regions and platform branches can remain uncompilable or unsafe without detection.

Trigger Conditions:  
Fresh checkout, alternate platform/compiler, dependency update, or latent template use.

Root Cause:  
Projects evolved as isolated experiments without repository-wide verification.

Recommendation:  
Add a root manifest/CI matrix, dependency provenance, generated-test workflow, compile-only header/API tests, and sanitizer/platform jobs.

ID: REV-050  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: `autoclicker.cpp`, `mousetracker.cpp`, `content-size.cpp`  
Line(s): primary loops/platform calls  
Title: Desktop utilities ignore platform errors and hard-code environment assumptions

Description:  
Autoclicker busy-polls selection and ignores cursor/SendInput results. MouseTracker assumes a fixed 1920×1080 desktop, parses `ctime` by a fixed token index, and busy-loops at maximum CPU. Content-size silently catches per-entry errors, can overflow its total, and is Windows-only solely for color output despite filesystem logic being portable.

Impact:  
High CPU, wrong coordinates/output on scaled or multi-monitor systems, silent partial results, and needless platform lock-in.

Trigger Conditions:  
API failure, non-1080p/scaled desktops, locale/time-format changes, inaccessible trees, or very large totals.

Root Cause:  
UI conveniences are embedded in core loops without error/status abstraction.

Recommendation:  
Check calls, sleep/event-wait, query virtual desktop geometry, use chrono formatting, and report incomplete scans.

ID: REV-051  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: `pc_startup.cpp`, `red_triangle.cpp`  
Line(s): file rewrite and OpenGL setup/cleanup paths  
Title: Startup logger can destroy its prior log and OpenGL demo continues after initialization failure

Description:  
`pc_startup` reads then truncates the same file without verifying the read, so failed/partial input can erase history; it also duplicates newlines from `ctime`. `red_triangle` continues after GLEW failure, does not validate shader program link/validate results, can attach invalid shaders, leaks its VBO, and uses nonstandard `alloca`.

Impact:  
Log data loss, misleading startup history, undefined rendering behavior, and portability/resource issues.

Trigger Conditions:  
Read/I/O failure, GLEW/shader failure, repeated runs, or non-supporting compilers.

Root Cause:  
Mutation occurs before successful staging and graphics setup lacks fail-fast ownership.

Recommendation:  
Write logs atomically after verified reads and use checked RAII OpenGL initialization/cleanup.

ID: REV-052  
Category: Reliability Concern  
Confidence: High  
Severity: 3  
File: `ohmslaw.nim`, `traffic_light/traffic_light.ino`  
Line(s): numeric parser/formatter; GPIO/delay sequence  
Title: Numeric and hardware assumptions are insufficiently constrained

Description:  
The Ohm's-law input filter permits multiple decimal points, parsing exceptions are unhandled, division by zero produces nonfinite values that formatting converts to integers, and negative/exponential forms are inconsistently excluded. The traffic sketch hard-codes GPIO 0 (a boot-strapping pin on common ESP8266 boards), uses blocking delays, and does not identify the target board/electrical fail-safe assumptions.

Impact:  
Crashes or invalid electrical results, and hardware that may fail to boot or cannot safely adapt to its target.

Trigger Conditions:  
Malformed/zero numeric input or installation on affected boards.

Root Cause:  
Input and hardware domains are encoded as informal assumptions.

Recommendation:  
Use checked finite parsing/domain validation and document/configure board pins and safe state transitions.

# Top Risks

1. Stop exposing `String.h`, `cstring.h`, `Result.h`, and affected Hash APIs until their lifetime and buffer defects are repaired and sanitizer-tested.
2. Prevent all destructive `backup.cpp` execution until canonical non-overlap and inventory-completeness gates are implemented.
3. Fix `AssetFinder.cpp` and Brainfuck bounds before processing any untrusted input.
4. Close the ThreadPool worker-lifetime race before claiming concurrent resize/submission safety.
5. Correct repacker path/process generation before packaging files with externally controlled names or paths.

# Recommended Priorities

- P0: REV-001, REV-004, REV-013, REV-014, REV-018, REV-022, REV-024, REV-027, REV-028.
- P1: all severity-4 findings; add targeted regression tests before repair is considered complete.
- P2: make every public header parse and instantiate independently under GCC/Clang/MSVC, C and C++ where claimed.
- P2: add root CI/build metadata, sanitizer runs, filesystem fault injection, cross-language known-answer vectors, and platform-backend mocks.
- P3: improve docs, dependency provenance, binary artifact policy, and utility portability.

# Second-Pass Findings

The second pass re-read every file while assuming the first pass missed defects. It specifically traced ownership across worker-container/worker locks, source/destination inventory phases, generated-code identities, audio producer/consumer publication, public hash output ownership, and UI/model state transitions.

New or materially strengthened second-pass results were:

- REV-014: invalid SHA-512/t length becomes a static-buffer overflow with assertions disabled.
- REV-018: the ThreadPool stress test does not preserve worker lifetime; the raw pointer differs from the safe `shared_ptr` path next to it.
- REV-028: a partial source scan feeds a destructive mirror deletion phase.
- REV-030: equal audio ring indices ambiguously mean empty after producer wrap, and overwrite creates a data-race window.
- REV-033: hold state commits without collision validation and CCW kick transitions are not inverted.
- REV-040: identical payload content collides in path metadata, not merely in data deduplication.
- REV-043: `hardware_concurrency()` is a fallible hint; values 0 and 1 both break the work split.

Boundary/failure rechecks also confirmed empty/default objects, `NDEBUG` behavior, zero divisors, nonfinite values, integer multiplication/addition, EOF, nonterminated views, allocation failure, I/O short reads, process exit status, self-join, and malformed serialized-length paths.

# Coverage and Validation

## Per-file assessment rubric

Every file in the manifest below was inspected for purpose, design, correctness, reliability, robustness/error handling, edge cases, portability, API surface, and maintainability. Findings above record actionable defects. Files without a dedicated finding were still assessed:

- `DataTypes.h` is a small, portable typedef header, though its short global names can collide.
- `hash.zig` is a straightforward CLI using standard hashing; allocator cleanup is process-lifetime and no correctness defect was established.
- `exprtk.hpp`, both STB headers, and the primary miniz/miniz-cpp portion of `zip_file.h` are vendored implementations; integration contracts and build compatibility were reviewed, not re-audited algorithm-by-algorithm as original upstream projects. `zip_file.h`'s repository-specific embedded duplicate was inspected too.
- Hash tests provide broad MD5/SHA-1/SHA-2/SHA-3 known-answer and chunking coverage, but a complete run was impractical on the generated 21 MiB translation unit on this host; partial execution passed before termination. This limitation is reflected in REV-049 and no unexecuted result is claimed.
- All seven IMF sample images decode as plausible image assets; the two traffic PNGs and both circuit archives are structurally valid. The circuit archives each contain four XML members. `words.txt` is a 370,102-line CRLF dictionary and is the Hangman data source.
- CWave common code passed syntax checking; full Linux configuration stopped at the undeclared system PulseAudio dependency. Tetris configured/built successfully. The ordinary optimized ThreadPool suite passed all 42 tests.

## Reviewed original-file manifest (144/144)

```text
.gitignore
AssetFinder.cpp
BrainfuckCCompiler.c
BrainfuckInterpreter.c
CWave/.gitignore
CWave/CMakeLists.txt
CWave/README.md
CWave/src/audio_capture.h
CWave/src/config.h
CWave/src/fft.cpp
CWave/src/fft.h
CWave/src/main.cpp
CWave/src/platform/linux/audio_capture_pulseaudio.cpp
CWave/src/platform/linux/audio_capture_pulseaudio.h
CWave/src/platform/macos/audio_capture_coreaudio.cpp
CWave/src/platform/macos/audio_capture_coreaudio.h
CWave/src/platform/macos/audio_capture_coreaudio_tap.mm
CWave/src/platform/platform.h
CWave/src/platform/windows/audio_capture_wasapi.cpp
CWave/src/platform/windows/audio_capture_wasapi.h
CWave/src/visualizer.cpp
CWave/src/visualizer.h
Convert2Binary.cpp
Core.h
DataTypes.h
Hash.dart
Hash.h
Input/Input.h
Input/Snake.cpp
Input/Snake.h
Input/main.cpp
Input/makefile
Input/tc.h
InputHandler.h
Profiler.h
ProgressBar.h
README.md
Result.h
String.h
TestString.h
Tetris/CMakeLists.txt
Tetris/README.md
Tetris/src/board.cpp
Tetris/src/board.h
Tetris/src/game.cpp
Tetris/src/game.h
Tetris/src/main.cpp
Tetris/src/renderer.cpp
Tetris/src/renderer.h
Tetris/src/tetromino.cpp
Tetris/src/tetromino.h
ThreadPool.hpp
Vector.h
array.h
autoclicker.cpp
backup.cpp
content-size.cpp
cstring.h
data.cpp
descend_calc/descend_calc.c
descend_calc/descend_calc.cpp
descend_calc/descend_calc.cs
descend_calc/descend_calc.dart
descend_calc/descend_calc.fs
descend_calc/descend_calc.go
descend_calc/descend_calc.hs
descend_calc/descend_calc.java
descend_calc/descend_calc.js
descend_calc/descend_calc.kt
descend_calc/descend_calc.lua
descend_calc/descend_calc.nim
descend_calc/descend_calc.php
descend_calc/descend_calc.pl
descend_calc/descend_calc.py
descend_calc/descend_calc.r
descend_calc/descend_calc.rb
descend_calc/descend_calc.rs
descend_calc/descend_calc.s
descend_calc/descend_calc.sh
descend_calc/descend_calc.swift
descend_calc/descend_calc.ts
descend_calc/descend_calc.v
descend_calc/descend_calc.vb
descend_calc/descend_calc.zig
descend_calc/descend_calc_matlab.m
descend_calc/descend_calc_objective_c.m
exprtk.hpp
hangman.cpp
hash.cpp
hash.zig
hash_benchmark.cpp
hash_benchmark_file.cpp
hash_benchmark_file_generate.cpp
hash_test.cpp
hash_test_generator.py
image_to_ascii.cpp
imf/1.jpg
imf/2.jpg
imf/3.jpg
imf/4.png
imf/5.png
imf/6.png
imf/7.png
imf/imf.cpp
imf/stb_image.h
imf/stb_image_write.h
mousetracker.cpp
nks.py
ohmslaw.nim
pc_startup.cpp
red_triangle.cpp
reminder.cpp
repacker-zip.cpp
repacker.cpp
sizeof/clangC++x64.exe
sizeof/clangC++x86.exe
sizeof/clangCx64.exe
sizeof/clangCx86.exe
sizeof/g++C++x64.exe
sizeof/g++C++x86.exe
sizeof/gccCx64.exe
sizeof/gccCx86.exe
sizeof/makefile
sizeof/msvcC++x64.exe
sizeof/msvcC++x86.exe
sizeof/msvcCx64.exe
sizeof/msvcCx86.exe
sizeof/readme
sizeof/sizeof.c
snake_console.c
stack.h
stack_vector.h
stb_image_write.h
sum.cpp
thread_pool_test.cpp
tictactoe.cpp
time.cpp
traffic_light/circuit.cddx
traffic_light/circuit.png
traffic_light/intersection.cddx
traffic_light/intersection.png
traffic_light/traffic_light.ino
words.txt
zip_file.h
```

## Artifact and integrity checks

- `report.md`: created.
- `bug_findings/`: created with isolated minimal sources, inputs, scripts, and README instructions for demonstrable findings.
- Original source/configuration/assets: no intentional edits. Final verification compares all 144 baseline SHA-256 hashes and checks Git's tracked diff.
- No commits, pull requests, refactors, or source fixes were made.
