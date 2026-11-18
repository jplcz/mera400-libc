# crt: start-up code and linker script

**crt0.s** is the *C runtime start-up code* ("crt" = C runtime, "0" = the
very first object in a program). After reset the CPU executes the first
word of the program image; that word is `_start`, which:

1. sets the stack pointer `R7` from the linker symbol `__stack_top`;
2. clears `.bss` (the uninitialised globals; C promises they start as zero);
3. calls `main` with `R1 = R2 = 0` (no `argc`/`argv` yet);
4. falls into `exit`, which stops the CPU with `hlt 077` (the conventional
   "finished OK" stop). `abort` stops with `hlt 040`.

The code lives in the section `.text.crt0` so that the linker script can
put it first whatever the link order (see the top-level README). Before
`main`, crt0 runs `__libc_init_array` (the `.preinit_array` and `.init_array`
tables: C++ static constructors and `__attribute__((constructor))`); `exit`
runs the `atexit` functions, then the `.fini_array` destructors. In the
large model the tables hold 4-byte byte addresses (function pointers are
32 bits there); in the small model 2-byte word addresses. The linker script
marks them `KEEP` so `--gc-sections` does not drop them.

**mera400.ld** is the *linker script*: it tells `ld.lld` where things go.
All addresses in it are **byte** addresses (word address × 2):

* `__image_base` (default `0x400`): where the image is loaded. Override with
  `--defsym=__image_base=...`.
* `__stack_top` (default `0xFFFE`; the large model passes `0x1FFFE`) and `__stack_size` (default `0x400`):
  the stack grows down from `__stack_top`; the heap used by `malloc`
  (`__heap_start`..`__heap_end`) fills the space between the end of `.bss`
  and the reserved stack area.
* Section order: code (`.text`, crt0 first), `.rodata`, `.data`, `.bss`.

Link by hand with
`ld.lld -m mera400elf -T crt/mera400.ld crt0.o your.o libc.a libbuiltins.a`;
add `--oformat=binary` for a flat image of 16-bit words, high byte first.
