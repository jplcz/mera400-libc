# tests/sim: the MERA-400 simulator

`m400sim.py` is a small, readable simulator of the MERA-400 CPU instruction
set as used by this library. It exists because no real machine or working
emulator was available during development. It reads an ELF file (the linker
output), loads its segments into 64 K words of memory and executes the code.

**Modelled:** loads/stores (`lw tw rw lb rb ...`), arithmetic and logic,
compares, all jumps, `rj`/`lj`, skips (`bb bm bs bc bn`), `irb`/`drb`, shifts,
`shc`, `nga`/`ngc`/`ngl`, `zlb`/`zrb`, `hlt`. **Not modelled** (raises
`NotModelled`): I/O, interrupts, memory blocks, the AWP floating unit, `ls`,
`ri`, `pw`.

Semantics were taken from the source of the EM400 emulator; they are
believed right but are unproven against hardware.

Files:

* `libtest.py` - loads an image into memory pre-filled with random junk (so
  reliance on zeroed memory is caught), calls a function with the C calling
  convention, and verifies the return value, that only the expected bytes
  changed, and that `R5`, `R6`, `R7` were preserved.
* `test_string.py`, `test_builtins.py`, `test_crt.py`, `test_decode.py`,
  `test_start_first.py` - the drivers run by CTest. Each prints
  "N checks passed" or the failures.

To debug a failure run a driver directly, for example
`python3 test_string.py ../../build/tests/string_image.elf`.
