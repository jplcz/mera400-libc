# builtins: compiler runtime helpers

The CPU has no instruction for some operations, so the compiler emits a call
to a small helper function instead, for example `a * b` for 16-bit values
becomes `__mulhi3(a, b)`. The names follow the usual compiler-runtime
convention: `hi` = 16-bit ("half integer"), `si` = 32-bit, `di` = 64-bit.

These helpers are called by compiler-generated code, never by name from C.
They use the ordinary calling convention: arguments in `R1`, `R2`,
result in `R1`, return address in `R4`; `R5`, `R6`, `R7` are preserved.
Edge cases are noted at the top of each file.

| Helper | Operation | Status |
|--------|-----------|--------|
| `__ashlhi3` | shift left | done |
| `__lshrhi3` | logical shift right | done |
| `__ashrhi3` | arithmetic shift right (sign kept) | done |
| `__mulhi3` | multiplication | done |
| `__udivhi3` / `__umodhi3` | unsigned divide / remainder | done |
| `__divhi3` / `__modhi3` | signed divide / remainder (toward zero) | done |
| `__ashlsi3` `__lshrsi3` `__ashrsi3` | 32-bit shifts by a variable count (the count is a 32-bit value, as the compiler passes it) | done, in C |
| `__mulsi3` | 32-bit multiplication | done, in C |
| `__udivsi3` `__umodsi3` `__divsi3` `__modsi3` | 32-bit divide / remainder | done, in C |
| `*di3` (64-bit) `__mulodi4` `__mulosi4`... | 64-bit multiply, divide, remainder, shifts | done, in C (`di3.c`) |
| `__m48_add sub mul div neg cmp` and conversions, `powi` | floating point on the native 48-bit format (3 words: 39-bit magnitude mantissa, 8-bit exponent), in software; the compiler calls them for every `float`/`double` operation | done, in C (`m48_*.c`) |

The float helpers are checked against a Python model (`tests/sim/m48ref.py`).
Overflow saturates to the largest value, underflow gives 0. The variant that
would use the hardware AWP (floating point) instructions is not implemented.

`udivmodhi.s` holds the shared internal routine `__udivmodhi` (quotient in
`R1`, remainder in `R2`) that the four division helpers use.

Behaviour worth knowing: a shift count of 16 or more gives 0 (or 0/-1 for
`__ashrhi3`); unsigned division by zero gives quotient `0xFFFF` and the
dividend as the remainder (C leaves it undefined, and the signed helpers
are not specified for it); `-32768 / -1` gives `-32768`.

All eight were run on the simulator against Python arithmetic (edge values
plus random pairs, 16164 checks), not on hardware.
