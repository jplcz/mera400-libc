# src/string: string and memory functions in assembly

One function per file, so a program links only what it calls (the archive
`libc.a` has one object per file). Every file starts with a comment giving
the C prototype, the registers used, and the algorithm.

Conventions (the *ABI*, application binary interface):

* arguments in `R1`, `R2`, `R3`; result in `R1`;
* `R4` holds the return address (call is `rj r4, f`, return is `uj r4`);
* `R5` and `R6` must be preserved (the functions save them on the stack when
  they use them), `R7` is the stack pointer, `R0` is the flags register and is
  never used for data.

Pointers are byte addresses. `lb` loads a byte into the *low* half of a
register without touching the high half, so loops that compare whole
registers clear a temporary first. `memcmp`, `strcmp` and `strncmp` return
the difference of the first differing bytes treated as unsigned (so bytes
above `0x7F` compare correctly).

`strextra.c` holds the functions that are simple in C; it is built only when
the C compiler is available.

`memcpy`, `memmove`, `memset` and `memcmp` have a fast path: when both
pointers are even (word aligned) and the length is at least 2 they work a
whole 16-bit word at a time, finishing with one odd byte; otherwise they use
the byte loop. `memmove` copies backwards when the destination is above the
source.

**Large model:** these assembly files assume 16-bit byte pointers. The large
build uses `large/string.c` instead (C compiled with `-mlarge-memory`, 32-bit
pointers and `size_t`, same word fast paths).
