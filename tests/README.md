# tests

Run everything with `ctest --test-dir build --output-on-failure` (use
`build-large` and `build-crook` for the other configurations; see the top
level README). Some tests exist only in some configurations.

| Test | What it proves |
|------|----------------|
| `string` | (small model) the assembly string functions against Python references, including exhaustive alignment, overlap and `memcmp` difference positions |
| `mem-on-target` | (none platform, both models) a C program running on the simulator checks every `mem*`/`str*` function exhaustively on global and stack buffers (in the large model the stack is above word 0x8000) |
| `builtins` | the 16-bit integer helpers against Python arithmetic |
| `m48-float` | the software `__m48_*` floating-point helpers against `sim/m48ref.py` with random and edge-case vectors |
| `c-on-target` | the portable C code (`qsort`, `malloc`, ...) compiled by Clang and run on the simulator |
| `stdio` | `printf`, `sprintf`, `snprintf`, `sscanf` and the console hook on the simulator |
| `math` | the `math.h` functions against Python's `math` within an accuracy bound |
| `init`, `init-abort`, `init-cpp` | constructors (with priorities), destructors, `atexit` order, `exit` and `abort`; the C++ variant uses `clang++` static objects. It checks the 1-byte destructor guard too, in both models. |
| `crt0` | start-up: stack, `.bss` cleared from junk, `main` result, final `hlt` |
| `decoder-vs-objdump` | the simulator decodes every instruction like `llvm-objdump` |
| `start-first` | `_start` is at the image start for every position of crt0 on the link line |
| `gc-sections` | a program using one function does not contain unused library functions (checked with `llvm-nm`) |
| `host-c-logic` | the portable C sources, compiled for the build computer (algorithms only) |
| `crook`, `crook-stdio` | only with `-DMERA400_PLATFORM=crook`: whole CROOK programs run against a fake operating system; `crook-stdio` checks that `printf` output arrives as line records |

Directories:

* `sim/` - the MERA-400 simulator and the Python test drivers (see its README).
* `crt/main_ret.s` - the tiny `main` used by the `crt0` and `start-first` tests.
* `host/test_c.c` - the host-side C test.
* `crook/` - the CROOK test program (`crook_main.s`) and its driver
  `test_crook.py`, which plays the part of the operating system. Real CROOK
  is not available, so every extracode (`EXL n`, a call into the operating
  system) is answered by Python code that logs the call and fills in results;
  the test compares the log with the expected sequence.

The assembly tests link a *test image* of a whole library (`--whole-archive`,
no crt0) and the simulator calls each function directly, the way compiled
code would. None of this ran on real hardware.
