# MERA-400 C library

A small C standard library for the MERA-400, a 16-bit Polish minicomputer
from the 1970s. It is a work in progress: the core routines exist and are
tested on a software simulator, but **nothing here has run on real hardware**.

## Why it is shaped the way it is

* **The MERA-400 addresses memory in 16-bit words, but C wants bytes.** A
  `char` here is 8 bits, so a "byte pointer" (`char *`, `void *`) is a
  16-bit *byte address*: word address × 2, plus 0 or 1 to pick the high or
  low half of the word. Word pointers (`int *`, structures) hold plain word
  addresses. The library routines that touch bytes (`memcpy`, `strlen`, ...)
  are therefore written in **assembly**: they are the hottest code in most
  programs, and byte access (the `lb` and `rb` instructions) needs care.
* **There is no C compiler for this CPU yet.** The LLVM back end (the part
  that turns LLVM's intermediate code into MERA-400 machine code) exists, but
  the Clang front end (the part that reads C) does not. So the C parts of
  the library (`ctype`, `stdlib`, a few string functions) are only built when
  a suitable `clang` is found (see `MERA400_BUILD_C`), and are checked on the
  host computer in the meantime.
* **Small memory model first.** "Small" means byte pointers are 16 bits, so
  all byte-addressed data must live in the lower 64 KiB (32 K words) of
  memory. A "large" model with 32-bit byte pointers is planned; the CMake
  option `MERA400_MEMORY_MODEL` already exists and stops with a clear error
  if you ask for `large`.

## What is in the box

| Directory    | Contents |
|--------------|----------|
| `src/string` | `memcpy memmove memset memcmp memchr strlen strcpy strncpy strcat strncat strcmp strncmp strchr strrchr` in assembly, plus `strnlen strspn strcspn strpbrk strstr` in C |
| `src/ctype`, `src/stdlib` | portable C: `isalpha`..., `abs labs atoi atol strtol strtoul rand srand qsort bsearch malloc calloc realloc free`, `errno` |
| `include`    | freestanding headers (no operating system needed): `stddef stdint stdbool limits errno assert ctype string stdlib float`. There is no `stdarg`, `stdio` or `math` because variable arguments and floating-point code generation are not supported by the compiler yet |
| `builtins`   | integer helpers the compiler calls for operations the CPU lacks (see `builtins/README.md`) |
| `crt`        | start-up code (`crt0.s`) and the linker script (see `crt/README.md`) |
| `cmake`      | the cross-compilation toolchain file and build helpers |
| `tests`      | CTest tests and the simulator (see `tests/README.md`) |

`float.h` describes the processor's native floating-point format (39-bit
magnitude mantissa, 8-bit exponent, largest value about 1.7e38), used for
`float`, `double` and `long double` alike, each taking 64 bits of storage.

## Building

You need CMake (3.20 or newer), Python 3, and the LLVM tools built with
MERA-400 support: `llvm-mc` (assembler), `ld.lld` (linker), `llvm-ar`
(archiver) and `llvm-objdump` (disassembler). Either put them in `PATH` or
tell CMake where they are with `LLVM_BIN_DIR`:

```sh
cmake -S . -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=cmake/mera400-toolchain.cmake \
      -DLLVM_BIN_DIR=<directory with the LLVM tools>   # omit if they are in PATH
cmake --build build
ctest --test-dir build --output-on-failure
```

Results: `build/libc.a` and `build/libbuiltins.a` (static libraries) and
`build/crt0.dir/crt0.o` (the start-up object).

Options:

| Option | Meaning |
|--------|---------|
| `LLVM_BIN_DIR` | where the LLVM tools are; empty means `PATH` |
| `MERA400_MEMORY_MODEL` | `small` (default). `large` is not implemented and fails at configure time |
| `MERA400_BUILD_C` | `AUTO` (default: build the C files only if `clang --print-targets` lists `mera400`), `ON` (require it), `OFF` |

VS Code users: `.vscode/tasks.json` has *configure*, *build* and *test*
tasks and `.vscode/launch.json` can debug the Python simulator tests.

## Using the library in your own program

Inside this project use the helpers from `cmake/Mera400.cmake`:

```cmake
include(Mera400)
mera400_add_executable(hello SOURCES hello.s LIBRARIES c builtins BINARY)
```

This links `hello.elf` (and a flat memory image `hello.bin` because of
`BINARY`) with crt0 first. By hand it is:

```sh
ld.lld -m mera400elf -T crt/mera400.ld crt0.o hello.o libc.a libbuiltins.a -o hello.elf
```

### Why crt0 must come first

When the CPU starts it fetches its first instruction from the very first word
of the program image. **crt0** (C runtime start-up code) sets up the stack,
clears uninitialised data and calls `main`, so its entry point `_start` has
to be exactly at the start of the image. A linker normally places code in
the order it sees the object files, so this is enforced twice:

1. `crt0.s` puts its code in a dedicated section, `.text.crt0`, and the linker
   script `crt/mera400.ld` places that section before all other code
   (marked `KEEP`, so it is never discarded).
2. `mera400_add_executable()` also always passes `crt0.o` as the first
   input.

The CTest `start-first` proves it: it links the same program with crt0 in
every position among the other objects and checks that `_start` is at the
image base address and that the flat binary begins with crt0's first word.

## How it was verified

* **Simulator (the main evidence).** `tests/sim/m400sim.py` is a small
  MERA-400 CPU simulator written for this project, following the behaviour
  of the EM400 emulator's source code. The tests load the *real linked
  machine code* into it and compare every function with a Python reference:
  odd and even alignment of every pointer, zero length, overlapping
  `memmove` in both directions, bytes above `0x80`, objects touching the end
  of the address space; they check that only the expected bytes change and
  that `R5`, `R6`, `R7` are preserved. The simulator's instruction decoder
  is cross-checked against `llvm-objdump` for every instruction in the
  images.
* **EM400** (the existing MERA-400 emulator) could not be built in the
  development environment (missing dependencies), so the code has **not**
  been run on it. Running these tests on EM400 is a good next step.
* **Host compiler** for the portable C code: `tests/host/test_c.c` compiles
  `ctype`, `strtol`, `qsort`, `malloc` and the C string functions with the
  build computer's compiler. This checks the algorithms, not code generation
  for the MERA-400.
* **Real hardware:** nothing has run on real hardware. The simulator is
  only as faithful as its author's reading of the documentation.

Tests (`ctest`): `string`, `builtins`, `crt0`, `decoder-vs-objdump`,
`start-first`, and `host-c-logic` (when a host C compiler exists).

## Not done yet

* The large memory model.
* 32-bit (`__mulsi3`, `__divsi3`, ...) and 64-bit compiler helpers.
* Word-at-a-time fast paths in `memcpy`/`memset` (currently one byte per
  loop iteration; simple and verifiable first).
* `stdio`, `stdarg`, `math`, `time`; building the C sources needs a Clang
  with a MERA-400 target.

## License

Apache License v2.0 with LLVM Exceptions (see `LICENSE`).
