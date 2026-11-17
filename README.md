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
* **The C parts need Clang with the MERA-400 target.** The `ctype`, `stdlib`
  and some string functions are C, built with the project's `clang` (found in
  `LLVM_BIN_DIR`; see `MERA400_BUILD_C`). The compiler's own headers supply
  `stddef.h`, `stdarg.h` and `stdbool.h`, so the library does not ship them.
  Their code is also checked on the host computer, and on the simulator as
  compiled for the MERA-400 (`tests/c/test_c.c`).
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
| `include`    | freestanding headers (no operating system needed): `stdint limits errno assert ctype string stdlib float` (`stddef`, `stdarg` and `stdbool` come from Clang). There is no `stdio` or `math` |
| `builtins`   | integer helpers the compiler calls for operations the CPU lacks: 16-bit ones in assembly, 32-bit ones in C (see `builtins/README.md`) |
| `crt`        | start-up code (`crt0.s`) and the linker script (see `crt/README.md`) |
| `src/crook`  | CROOK operating system layer (only with `MERA400_PLATFORM=crook`, see below) |
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
| `MERA400_PLATFORM` | `none` (default, bare metal) or `crook` (also builds `libcrook.a`, `crt0_crook`, the CROOK test; see "CROOK platform") |
| `MERA400_CROOK_BASE`, `MERA400_CROOK_IMAGE_WORDS`, `MERA400_CROOK_STACK_WORDS` | crook only: load address, image size and stack size, all in 16-bit words (defaults 0, 8192, 512) |

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

## CROOK platform

**CROOK** is an operating system for the MERA-400. It has no loader and no
linker: a program is a raw memory image (as produced by its assembler,
**ASSM**) placed in one 64 K-word address space. A program talks to the
system with **extracodes**, special instructions `EXL n` (n = 0200..0307
octal) whose argument goes in register R4, and leaves with the **XOSL**
extracodes `END` (0373) or `EOSL` (0377). CROOK gives a program no stack, so
our start-up code makes one.

Build for it (the triple `mera400-unknown-crook` does not exist in LLVM yet,
so the code is assembled for `mera400-unknown-none`):

```sh
cmake -S . -B build-crook -DCMAKE_TOOLCHAIN_FILE=cmake/mera400-toolchain.cmake \
      -DMERA400_PLATFORM=crook -DLLVM_BIN_DIR=<directory with the LLVM tools>
cmake --build build-crook && ctest --test-dir build-crook --output-on-failure
```

(`.vscode/tasks.json` has the same as *configure/build/test (CROOK)*.)

Pieces: `crt/crt0-crook.s` (sets R7 from `__stack_top`, clears `.bss`, calls
`main`, then `exit`), `crt/crook.ld` (layout), `src/crook` (extracode
wrappers, `putchar puts getchar`, text conversion; reference in
`src/crook/README.md`, headers in `src/crook/include`), `cmake/crook.cmake`.
Make a program with `crook_add_executable(name SOURCES ... )`: it links
`crt0_crook` first and runs `llvm-objcopy -O binary` to get `name.bin`.

**The image.** `name.bin` is a sequence of 16-bit words, each stored high
byte first (big-endian), so the file is *twice as many bytes as words*
(default: 8192 words = 16384 bytes). The first word is crt0. The file is
padded with zeros up to `MERA400_CROOK_IMAGE_WORDS`: CROOK sizes a process
by the highest address in the file, so the padding is the program's `.bss`,
heap and stack (stack at the top, growing down). Linker scripts count bytes,
CMake options count words; `cmake/crook.cmake` converts. With base 0 a NULL
pointer equals the start of the code.

**Text.** C strings (one char per byte, NUL-terminated) already have CROOK's
packing (two chars per word, high byte first). CROOK ends lines with CR, so
output turns `'\n'` into a record end and input turns CR into `'\n'`. NUL
cannot be read via `getchar`.

**Tested** only on the Python simulator in `tests/crook`, with a fake CROOK
handler for EXL (exit, file assignment, record I/O, date, core).

### CROOK: open questions

Not guessed; each needs a source or a real CROOK:

- Where a CROOK program starts executing (IC at start) and the file header /
  how the image gets onto a CROOK disk.
- What END/EOSL's R4 means as an exit status.
- Numeric ids of the standard input/output streams (INT/OUT) and the R40
  character alphabet. `putchar/puts/getchar` return EOF until
  `__crook_set_streams(in, out)` is called.
- REC word 0 (flags/last index), whether INPR stores the terminator, exact
  PRIN/PRIR semantics (we assume PRIR ends the line, PRIN does not).
- STR-argument calls: ERS (close), ERF, FBOF, FEOF, EOF: not implemented.
- TMEM word 3, BLOCK word-0 error meanings, which registers EXL preserves
  (assumed: only outputs change), the skip-return conventions of UNL/LOD.
- Epoch of DATE (so no `time()`), and the program attribute (010 vs 050)
  for 17-bit byte addressing.

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

Tests (`ctest`): `string`, `builtins`, `crt0`, `c-on-target` (the C code and
the 32-bit helpers compiled by Clang and run on the simulator; needs the C
build), `decoder-vs-objdump`, `start-first`, and `host-c-logic` (when a host C
compiler exists).

## Not done yet

* The large memory model.
* 64-bit compiler helpers.
* Word-at-a-time fast paths in `memcpy`/`memset` (currently one byte per
  loop iteration; simple and verifiable first).
* `stdio`, `math`, `time`.

## License

Apache License v2.0 with LLVM Exceptions (see `LICENSE`).
