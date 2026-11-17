# tests

Run everything with `ctest --test-dir build --output-on-failure`.

| Test | What it proves |
|------|----------------|
| `string` | all assembly string functions against Python references (7254 checks) |
| `builtins` | all eight 16-bit helpers against Python arithmetic (16164 checks) |
| `crt0` | start-up: stack, `.bss` cleared from junk, `main` result, final `hlt`; flat binary equals the ELF image |
| `decoder-vs-objdump` | the simulator decodes every instruction like `llvm-objdump` (mnemonic and length) |
| `start-first` | `_start` is at the image start for every position of crt0 on the link line |
| `host-c-logic` | the portable C sources, compiled for the build computer (algorithms only) |

Directories:

* `sim/` - the MERA-400 simulator and the Python test drivers (see its README).
* `crt/main_ret.s` - the tiny `main` used by the `crt0` and `start-first` tests.
* `host/test_c.c` - the host-side C test.

The assembly tests link a *test image* of a whole library (`--whole-archive`,
no crt0) and the simulator calls each function directly, the way compiled
code would. None of this ran on real hardware.
