# Examples

Small programs that show how to use the library. They are also a quick check
that a toolchain, the library and an emulator work together.

| Program | What it shows |
|---|---|
| `fibonacci.c` | The smallest useful program: no output device, the answer (55) is returned from `main` and ends up in register R1 |
| `hello.c` | `printf` through the console (only visible on the simulator the tests use, see below) |
| `shapes.cpp` | Freestanding C++: `new`, `delete`, virtual functions (the runtime is in `src/cxx`) |

## What you get

Every program is built twice, into the build directory (`examples/` in it):

* `<name>.elf` - the program with its symbols; use it with a debugger or to
  look at the code (`llvm-objdump -d`).
* `<name>.bin` - the flat memory image (ELF = the standard executable file
  format): just the bytes that go into memory, starting at the load address,
  with no headers. An emulator or a loader copies these bytes into memory.
  Every `mera400_add_executable` target produces both.

Build with the project's normal build (see the main README); the examples are
part of `ninja`.

## Running one on em400

The images are linked to be loaded at byte address `0x400` (word `0x200`), so
the low memory with the interrupt vectors stays free. em400 loads a binary at
word 0 and starts the CPU there, so for em400 link the same objects again
with the load address 0 and a stack that fits in em400's default memory
(the stack top below is the last byte of the 8 K words that the default
configuration has):

```sh
ld.lld -m mera400elf --defsym=__image_base=0 --defsym=__stack_top=0x1FFE \
    -T crt/mera400.ld --oformat=binary --gc-sections \
    crt0.o fibonacci.o libc.a libbuiltins.a -o fibonacci.bin
```

(`--defsym` must come before `-T`.) Then, with em400's command-line
interface, clear the machine, wait, start it and look at the registers:

```sh
(sleep 1; echo clear; sleep 1; echo start; sleep 1; echo reg; echo quit) |
    em400 -c cfg/em400.ini -u cmd -p fibonacci.bin -L
```

The result is in register `R1`: `0x37` (55) for `fibonacci`, `0x15` (21) for
`shapes`. The processor then stops in the WAIT state.

`hello` prints through `__console_port`, a memory-mapped word that exists only
in the simulator used by this project's tests. em400 has no such port (its
terminal is a serial line on a character channel), so nothing appears there.
