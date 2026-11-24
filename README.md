# MERA-400 C library

This project requires LLVM toolchain from https://github.com/jplcz/mera400-llvm

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
* **Two memory models.** "Small" means byte pointers are 16 bits, so all
  byte-addressed data must live in the lower 64 KiB (32 K words) of memory.
  "Large" means 32-bit byte pointers (`size_t` is 32 bits too), so programs
  can use the whole 1 M-word address space the CPU can reach with its
  memory-extension hardware. Choose with `-DMERA400_MODEL=small|large`; each
  build directory holds one model (`lib/small` or `lib/large`). The two models
  have different calling conventions for pointers (one register versus a
  pair), so they can never be mixed. In the large model the string routines
  are written in C (compiled with `-mlarge-memory`, with word-at-a-time fast
  paths) instead of assembly, because the byte access is done by the
  compiler there.
* **Code size matters.** The whole machine has 64 K words. Every library
  function and data object therefore sits in its own section (the C code is
  built with `-ffunction-sections -fdata-sections`, every assembly function
  has its own `.section .text.<name>`) and programs are linked with
  `--gc-sections`, so the linker drops everything a program does not use. The
  CTest `gc-sections` proves it. crt0, the `.init_array`/`.fini_array` tables
  and the entry point are protected with `KEEP()` and `ENTRY` in the linker
  script.

## What is in the box

| Directory    | Contents |
|--------------|----------|
| `src/string` | `memcpy memmove memset memcmp memchr strlen strcpy strncpy strcat strncat strcmp strncmp strchr strrchr` in assembly (small model; word-at-a-time when both pointers are even and the length is at least 2) or in C (`src/string/large`, large model), plus `strnlen strspn strcspn strpbrk strstr` in C |
| `src/ctype`, `src/stdlib` | portable C: `isalpha`..., `abs labs atoi atol strtol strtoul rand srand qsort bsearch malloc calloc realloc free`, `errno`; assembly `exit atexit abort _Exit` |
| `src/stdio` | `printf fprintf sprintf snprintf vprintf vsnprintf puts putchar fputs fputc sscanf` and friends (see `src/stdio/README.md`). All output goes through one weak hook, `__stdout_putc` |
| `src/math` | `fabs sqrt floor ceil fmod ldexp frexp modf exp log pow sin cos tan atan` and more, in C on the native double (accuracy suits the 40-bit mantissa) |
| `examples`   | demo programs (Fibonacci, printf, C++ with `new` and virtual functions); each is built as an `.elf` and a flat `.bin` memory image, see `examples/README.md` |
| `docker`     | `Dockerfile` that builds LLVM, this library and the libc++ headers into one development image, see `docker/README.md` |
| `src/cxx`    | the minimal C++ run-time (needs `clang++`): `operator new/delete` over `malloc` (a failed `new` ends the program, as there are no exceptions; the `nothrow` forms return null), `std::terminate`, `__cxa_pure_virtual`, `__cxa_guard_*`, and a stub `__cxa_atexit`. A C++ standard library such as libc++ expects these from the platform |
| `include`    | freestanding headers: `stdint inttypes limits errno assert ctype string stdlib stdio math float time locale wchar wctype` (`stddef`, `stdarg` and `stdbool` come from Clang). `time`, `locale`, `wchar`, `wctype` and parts of `stdlib`/`stdio` only declare functions so that libc++ headers compile; they are not implemented. Every header that declares functions has `extern "C"` guards, so C++ code links to the C implementations |
| `builtins`   | the helpers the compiler calls for operations the CPU lacks: 16-bit integer ones in assembly, 32/64-bit integer ones in C, and the `__m48_*` floating-point ones, in software (see `builtins/README.md`) |
| `crt`        | start-up code (`crt0.s`), which runs the constructors and destructors, and the linker script (see `crt/README.md`) |
| `src/crook`  | CROOK operating system layer (only with `MERA400_PLATFORM=crook`, see below) |
| `cmake`      | the cross-compilation toolchain file and build helpers |
| `tests`      | CTest tests and the simulator (see `tests/README.md`) |

`float.h` describes the processor's native floating-point format (39-bit
magnitude mantissa, 8-bit exponent, largest value about 1.7e38), used for
`float`, `double` and `long double` alike, each taking 64 bits of storage.

The calling convention and data layout (for writing assembly against the
library) are described in [docs/abi.md](docs/abi.md).

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

Results: `build/lib/<model>/libc.a`, `libcstring.a` and `libbuiltins.a`
(static libraries) and `build/crt0.dir/crt0.o` (the start-up object).
`libcstring.a` is the same library without stdio and math, for small programs
(a program that pulls in all of printf and the float code needs most of the
small model's 64 KiB byte reach).

Both models and both platforms (each in its own build directory):

```sh
cmake -S . -B build        ... && cmake --build build        && ctest --test-dir build        # none, small
cmake -S . -B build-large  ... -DMERA400_MODEL=large      && ...                              # none, large
cmake -S . -B build-crook  ... -DMERA400_PLATFORM=crook   && ...                              # crook (5), small
cmake -S . -B build-crook4 ... -DMERA400_PLATFORM=crook -DMERA400_CROOK_VERSION=4 && ...      # crook 4, small
```

(`...` is the toolchain and `LLVM_BIN_DIR` options above.) CROOK with the
large model is refused at configure time. `cmake --install <dir> --prefix
<sysroot>` installs headers, `lib/` (small) or `lib/large/` (large).

Options:

| Option | Meaning |
|--------|---------|
| `LLVM_BIN_DIR` | where the LLVM tools are; empty means `PATH` |
| `MERA400_MODEL` | `small` (default) or `large` |
| `MERA400_BUILD_C` | `AUTO` (default: build the C files only if `clang --print-targets` lists `mera400`), `ON` (require it), `OFF` |
| `MERA400_PLATFORM` | `none` (default, bare metal) or `crook` (also builds `libcrook.a`, `crt0_crook`, the CROOK test; see "CROOK platform") |
| `MERA400_CROOK_BASE`, `MERA400_CROOK_IMAGE_WORDS`, `MERA400_CROOK_STACK_WORDS` | crook only: load address, image size and stack size, all in 16-bit words (defaults 0, 8192, 512) |
| `MERA400_CROOK_VERSION` | crook only: `5` (default) or `4`, the CROOK version to build for (see "CROOK versions: 4 and 5") |

The CROOK configuration also needs Python 3 for `tools/image2assm.py` (see
"CROOK platform").

VS Code users: `.vscode/tasks.json` has configure, build and test tasks for
all three configurations (small, large, CROOK) and `.vscode/launch.json` can
debug the Python simulator tests.

## Using the library in your own program

Inside this project use the helpers from `cmake/Mera400.cmake`:

```cmake
include(Mera400)
mera400_add_executable(hello SOURCES hello.s LIBRARIES c builtins)
```

This links `hello.elf` and also a flat memory image `hello.bin` (the bytes of
the image without ELF headers, which is what an emulator loads) with crt0
first. By hand it is:

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

Build for it. With a Clang that knows the target `mera400-unknown-crook` (the
one in this LLVM tree) everything is compiled for that triple, which also
defines `__CROOK__`, and the Clang driver links the program by itself; with an
older Clang the code is built for `mera400-unknown-none` (the machine code is
identical) and `ld.lld` is called directly.

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

**Linking by hand with the Clang driver.** The driver looks for `crt0-crook.o`,
`crook.ld`, `libcrook.a`, `libc.a` and `libbuiltins.a` in `-L` directories and
in `<sysroot>/lib`. `cmake --install <build> --prefix <sysroot>` in a CROOK
configuration installs all of them (`crt0-crook.o`, `crook.ld`, `libcrook.a`
next to `libc.a` and `libbuiltins.a`), so with the installed sysroot no `-L`
is needed:

```sh
clang --target=mera400-unknown-crook --sysroot=<sysroot> hello.c -o hello.bin
```

From a build tree without installing, point `-L` at it:

```sh
B=build-crook      # the build directory
clang --target=mera400-unknown-crook -O2 -c hello.c -isystem include -nostdlibinc -ffreestanding
clang --target=mera400-unknown-crook -L$B/crt0_crook.dir -Lcrt -L$B/lib/small \
      -mcrook-base=0 -mcrook-image=8192 -mcrook-stack=512 hello.o -o hello.bin
```

The driver links `crt0-crook.o`, your objects, `libcrook.a`, `libc.a` and
`libbuiltins.a` with `crook.ld` and `--gc-sections`. The three `-mcrook-*`
options count **words** (defaults 0, 8192, 512) and become the linker symbols
`__image_base`, `__image_size`, `__stack_size` (in bytes). `-mcrook-version=4|5`
(default 5) selects the CROOK version (see "CROOK versions: 4 and 5"). An output file name
ending in `.bin` gives the raw image instead of an ELF file.

**The image.** `name.bin` is a sequence of 16-bit words, each stored high
byte first (big-endian), so the file is *twice as many bytes as words*
(default: 8192 words = 16384 bytes). The first word is crt0. The file is
padded with zeros up to `MERA400_CROOK_IMAGE_WORDS`: CROOK sizes a process
by the highest address in the file, so the padding is the program's `.bss`,
heap and stack (stack at the top, growing down). Linker scripts count bytes,
CMake options count words; `cmake/crook.cmake` converts. With base 0 a NULL
pointer equals the start of the code.

### Getting the image onto CROOK: `tools/image2assm.py`

*Why.* CROOK has no loader. Its assembler, **ASSM** (the MERA-400 assembler
that comes with CROOK), writes a program straight into a disk file, and the
byte layout of such a file is not documented. A host tool that wrote the file
itself would have to guess. So instead we hand ASSM what it understands: the
image as ASSM **source text** (a file header, a memory size and every word as
a number). You copy that text to CROOK (type it in with CROOK's editor, or
transfer it by a DOS floppy) and assemble it there; ASSM, not us, decides the
on-disk layout.

*How.* The input is the flat big-endian word image (`name.bin`):

```sh
python3 tools/image2assm.py hello.bin --title HELLO -o hello.asm
python3 tools/image2assm.py hello.bin --title HELLO --base 0 --attr 050 \
        --mem-quanta 2 --eol cr -o hello.asm
```

Options: `--title` (the CROOK file name), `--type` (file type, octal, 0),
`--base` (word address of the first word, 0), `--attr` (program attribute,
octal, 050 = ASSM's default), `--mem-quanta` (memory size in 4K-word blocks;
default: just enough), `--eol cr` (CROOK ends lines with CR), `--no-res`
(write zero words one by one), `--prog-block` (wrap in `PROG*`/`FINPROG*`).
The output looks like

```
FILE*HELLO,0,0,050.
MEM* 017777.
S* 0.
0071007. 0000000. ...
RES* 7900.
```

`FILE*` names the file and gives type, base address and attribute, `MEM*` the
largest address the program may use, `S*` the address of the next word, a
number ended by a full stop is one word (a leading 0 means octal) and
`RES* n.` is *n* zero words. All of these are documented on the ASSM page
<https://mera400.pl/J%C4%99zyk_symboliczny_(assembler)_ASSM>. **Not verified**
(see the comment at the top of the tool): the characters allowed in a title,
the meaning of the type, whether ASSM needs `PROG*`/`FINPROG*`, how `MEM*`
relates to the memory *quanta* in the file label (`--mem-quanta N` writes
`N*4096-1`, a guess), and where CROOK starts the program. The CTest
`image2assm` (and `image2assm-crook-image` with a real image) checks that
image → text → words gives the original back; it cannot check that ASSM
accepts the text.

**Text.** C strings (one char per byte, NUL-terminated) already have CROOK's
packing (two chars per word, high byte first). CROOK ends lines with CR, so
output turns `'\n'` into a record end and input turns CR into `'\n'`. NUL
cannot be read via `getchar`.

**Tested** only on the Python simulator in `tests/crook`, with a fake CROOK
handler for EXL (exit, file assignment, record I/O, date, core).

### CROOK versions: 4 and 5

*Why.* CROOK exists in several versions. This library supports **CROOK-4** and
**CROOK-5** (CROOK-1, 2 and 3 are not supported). As far as a compiled program
can see they are the same system: the same extracodes (numbers and parameter
blocks), streams, text conventions and way of ending a program. They differ
in memory management only. CROOK-5 has the extracodes **GMEM** (0270, get
memory blocks), **RMEM** (0271, release memory blocks) and **LRAM** (0272,
information about a RAM file), and a memory map that need not be contiguous.
CROOK-4 has none of them: a process owns one contiguous block that **CORE**
(0232) can resize.

*How.* Choose the version when you configure, with the CMake option
`MERA400_CROOK_VERSION` (`4` or `5`, default `5`), and when you build by hand
with Clang's `-mcrook-version=4` or `-mcrook-version=5`. Clang then defines the
macro `__CROOK_VERSION__` (4 or 5), and `<crook.h>` declares `__crook_gmem`,
`__crook_rmem` and `__crook_lram` only for version 5. Calling one in a CROOK-4
build is a compile error, so the mistake shows up on your computer and not as
an alarm on the CROOK machine. The program image
is the same for both versions; there is one library, not two.

```sh
cmake -S . -B build-crook4 ... -DMERA400_PLATFORM=crook -DMERA400_CROOK_VERSION=4
clang --target=mera400-unknown-crook -mcrook-version=4 --sysroot=<sysroot> hello.c -o hello.bin
```

`malloc` does not call CROOK on either version: its heap is the static region
between `.bss` and the stack, sized by `MERA400_CROOK_IMAGE_WORDS`. Growing
it with CORE is not done.

*Tests.* In a CROOK-4 configuration the fake CROOK of `tests/crook` has no
GMEM, RMEM or LRAM: if the program calls one the test fails. The test
`crook-version-guard` compiles `tests/crook/crook_guard.c` to check that
`<crook.h>` declares GMEM, RMEM and LRAM exactly for version 5. A CROOK-4 image
cannot be run on a real system here: no CROOK-4 disk image and no emulator
support for it are known, so CROOK-4 is tested against the fake CROOK only.

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

Tests (`ctest`): see `tests/README.md` for the full list.

## Conventions and limits

* **Floating point** is the native 48-bit format, software only (`__m48_*`
  helpers, ported from the EM400 AWP algorithms and checked against a Python
  reference with random and edge-case vectors). There is no NaN or infinity:
  overflow saturates to the largest value (`HUGE_VAL`), underflow gives 0,
  division by zero gives the largest value, `errno` is never set. An
  assembly variant using the hardware AWP instructions is **not
  implemented**. `printf("%f")` digits are trustworthy to about 11 significant
  digits.
* **Heap:** `malloc` uses the area from the end of `.bss` up to the stack
  reserve (`__heap_start` to `__heap_end` from the linker script).
* **Linker symbols:** `--defsym` options (the large model's
  `__stack_top=0x1FFFE`, the CROOK image size, ...) must come *before* `-T`
  on the `ld.lld` command line; the script supplies defaults with `PROVIDE`.
  `cmake/` does this for you.

## Not done yet

* The assembly (AWP hardware) floating-point variant.
* Assembly versions of the large-model string routines (C is used).
* `time`, files other than the console, `strtod`/`scanf` beyond the minimal set.
* Running on EM400 or real hardware.

## License

Apache License v2.0 with LLVM Exceptions (see `LICENSE`).
