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
| `crook` | only with `-DMERA400_PLATFORM=crook`: a whole CROOK program (crt0, `puts`, `getchar`, extracode wrappers, exit) runs on the simulator against a fake CROOK operating system; also checks the raw `.bin` image |

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
