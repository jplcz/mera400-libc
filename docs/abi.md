# The MERA-400 C/C++ ABI

This page describes how compiled C and C++ code on the MERA-400 expects to
be called and how it lays out data. It is written for two readers: someone
new to the machine and the compiler, and someone who wants to write
assembly that calls the C library (or is called by C).

**ABI** (application binary interface) is the set of rules that let separately
compiled code work together: which registers carry arguments, where the stack
is, how big a `long` is, how a structure is laid out. If assembly follows these
rules it can call, and be called by, compiled code.

Every rule below was checked against the compiler's output. The examples of
sections 12 and 13 are real, running code: `tests/abi/` assembles, links and
runs them in the simulator, in both memory models (`ctest -R abi`).

Contents: 1 Registers, 2 Stack frames, 3 Arguments and results, 4 Variable
argument lists, 5 Pointers, 6 Types, 7 Byte order, 8 Floating point,
9 Memory models, 10 Compiler helper functions, 11 C++, 12 Sections and
start-up, 13 Examples.

## 1. Registers

| Register | Role |
|----------|------|
| R0 | flags (Z zero, M minus, V overflow, C carry, L less, E equal, G greater, Y the bit shifted out, X extra flag). The compiler never allocates it. |
| R1, R2, R3 | the first arguments and the result. A 32-bit value is *high word first*: R1 = high, R2 = low. They are **caller-saved**: a call may destroy them. |
| R4 | the return address. `rj r4, f` stores the address of the next instruction in R4 and jumps to `f`; `uj r4` returns. Caller-saved. |
| R5, R6 | **callee-saved**: a function that uses them must restore them before returning. |
| R7 | the stack pointer (SP). It holds a *word* address and the stack grows downwards. |

There is no frame pointer: with a fixed frame size every local is reachable
as `n+r7`. This saves a register on a machine that has only seven.

*Why R1-R3 and not more?* The architecture has so few registers that three
for arguments is the most that leaves the code generator room to work. *Why
is R4 special?* The `rj` (return jump) instruction hard-wires it as the link
register, so a leaf function (one that calls nothing) returns without ever
touching memory.

## 2. Stack frames

R7 is a **word address**; the stack grows towards lower addresses. A function
allocates its whole frame once on entry (`awt r7, -N`) and frees it once before
returning (`awt r7, N`). Inside the frame, from the lowest address up:

```
higher addresses
    +-------------------------+
    | caller's stack arguments|   <- r7 + N + k   (k = 0, 1, ...)
    +-------------------------+
    | saved R4 (only if the   |   top of this function's frame
    |   function makes calls) |
    | saved R5, if used       |
    | saved R6, if used       |
    | locals and spill slots  |
    | outgoing argument area  |   <- 0+r7 (arguments of the calls it makes)
    +-------------------------+
lower addresses
```

* The frame is addressed as `k+r7`. On entry (before the `awt`) the caller's
  first stack argument is at `0(r7)`; after a frame of N words it is at `N+r7`.
* A leaf function needs no frame at all (`asm_add3` below).
* R4 is saved only if the function itself makes a call.
* Every stack slot is a whole word; objects on the stack are word aligned.
* Not supported: dynamic `alloca` and variable-length arrays (the compiler
  reports an error), tail calls, and frames larger than 32767 words.

The callee never pops its arguments. The caller owns the outgoing area at the
bottom of its own frame, reserved once for the largest call it makes, so there
is no per-call push/pop code.

## 3. Arguments and results

Arguments are assigned left to right to R1, R2, R3, then to stack words, one
word each, **lowest address first** (the first stack argument is at `0(r7)` at
the moment of the call). Multi-word values are always **most significant word
first**, the same as in memory (the machine is big-endian, section 7).

| Type | In registers | Result |
|------|--------------|--------|
| `char`, `short`, `int` (and `_Bool`, enums) | one register or stack word | R1 |
| `long`, `unsigned long` (32 bits) | two registers, high first | R1:R2 |
| `long long` (64 bits) | up to four words R1.. then stack | hidden pointer, see below |
| small-model pointers (16 bits) | one register | R1 |
| large-model pointers (32 bits) | two registers, high first | R1:R2 |
| `float`, `double`, `long double` | three words R1:R2:R3 | R1:R2:R3 |
| `struct`, `union`, arrays | the address of a copy (see below) | hidden first argument |

**Narrow integers.** The *caller* extends an 8- or 16-bit argument to a whole
word (sign-extended for `signed char`/`short`, zero-extended for the unsigned
kinds), and the *callee* extends a result. So neither side masks what it
receives. `asm_schar_minus2` returns `-2` as the full word `0xFFFE` because the
C return type is `signed char`.

**The "torn" rule.** A 32-bit value (a `long`, or a pointer in the large
model) is never split between R3 and the stack. If only R3 is left, the whole
value goes to the stack, and the remaining registers are considered used, so
later arguments do **not** fill the hole:

| Call | R1 | R2 | R3 | stack words 0, 1, ... |
|------|----|----|----|-----------------------|
| `f(long a, long b)` | a hi | a lo | unused | b hi, b lo |
| `f(int a, int b, long c)` | a | b | unused | c hi, c lo |
| `f(int a, long b)` | a | b hi | b lo | |
| `f(int a, int b, double c, int d)` | a | b | unused | c (3 words), d |

*Why?* It keeps the callee's code simple: a 32-bit parameter is either in two
adjacent registers or a two-word stack object, never half and half. A 64-bit
`long long` is not subject to this rule: its words just continue R1, R2, R3,
stack 0, ....

**`long long` results.** Four words do not fit in R1:R3, so the caller passes
a hidden pointer to the result in R1, a **word address** in both models; the
visible arguments then shift by one register. For `long long f(long long a,
long long b)`: R1 = result pointer, `a` = R2, R3, stack 0, 1; `b` = stack 2-5.
The callee writes the result words high first. (`asm_ll_add`, section 13.)

**Floating point.** `float`, `double` and `long double` are all the same
native 3-word value (section 8): passed in R1:R2:R3 and returned there. If
fewer than three registers are free the entire value goes to the stack (same
torn rule). The fourth word in memory is padding and is never passed.
`float` and `double` have the same format, so conversions between them are free.

**Aggregates.** A `struct`, `union` or array argument is passed as the
**word address of a copy** made by the caller, in an ordinary argument slot.
There is no "by value" stack copy, so the callee may modify its copy. A function
returning an aggregate gets a hidden *first* argument: a normal C data pointer
(a **byte** address: R1 in the small model, R1:R2 in the large) to where to
write the result, with the real arguments following; it returns nothing.
For `struct P f(struct P a)`: small model R1 = result byte address, R2 = word
address of the copy; large model R1:R2 = result address, R3 = copy.

## 4. Variable argument lists

For a function such as `int f(int n, ...)` the *named* arguments go as usual,
but **every variadic argument goes on the stack**, never in registers: an
`int` takes 1 word, a `long` 2 (high first), a `double` 3 words. They start at
`0(r7)` after any stack words the named arguments used. `char` and `short` are
promoted to `int`, `float` to `double`, as in C.

*Why?* `va_arg` can then just walk memory with one pointer; there is no
register save area to build. `va_list` is a C data pointer (a byte address) to
the next slot; `va_arg` advances it by the type's size in bytes (6 for a
double). `asm_vsum` and `asm_call_vmixed` in `tests/abi/abi_asm.s` read and
build such argument lists.

## 5. Pointers

The machine addresses memory in 16-bit **words**, but C needs to address
single characters. There are therefore two kinds of address:

* A **word address** counts 16-bit words. Hardware instructions (`tw`, `rw`,
  `lw reg, symbol`) use it, and the assembler gives symbols as word addresses.
* A **byte address** counts 8-bit bytes: `2 * word address + k`, where `k = 0`
  is the high (first) byte of the word and `k = 1` the low byte.

| Pointer | Representation |
|---------|----------------|
| every C/C++ data pointer (`int *`, `char *`, `void *`, ...) | **byte address**: 16-bit (small model) or 32-bit (large) |
| pointer to a function, small model | 16-bit **word** address of the code; `rj r4, reg` calls it directly |
| pointer to a function, large model | 32-bit **byte** address; shift right by one to call |
| the address of an aggregate argument copy, the `long long` result pointer | 16-bit word address (both models) |
| stack pointer R7 | word address |

All data pointers have the same form so that `void *`, `char *` and `int *` are
interchangeable and casts are free. The cost is a shift whenever assembly
wants to *use* one. To turn a byte address into a word address shift right by
one:

```
    ; small: r1 = byte pointer
    srz r1                  ; r1 = word address, ready for  tw r2, r1
    ; large: r1:r2 = byte pointer (high:low)
    srz r1                  ; shift the 32-bit value right by one;
    sry r2                  ; the bit that falls out of r1 enters r2
                            ; r2 = word address (r1 is no longer needed)
```

To make a byte pointer from a symbol, shift left:

```
    lw  r1, asm_buf         ; r1 = WORD address of the symbol
    slz r1                  ; small: r1 = byte address
    ; large: lw r2, sym; lwt r1, 0; slz r2; sly r1
```

`lb` (load byte) takes a small-model byte address and replaces only the low
byte of its register, so zero the register first. In the large model `lb`
cannot reach all of memory; load the word and extract the byte with shifts.
Code can use the low bit of the byte address (`and r, 1`) to choose which
half of the word it is.

In the **small model** the 16-bit byte address limits byte-addressable data
(the whole program image, stack and heap) to the lower 32K words (64 KiB of
bytes). The large model has 32-bit pointers and the stack starts at word
`0xFFFF`.

## 6. Types: sizes and alignment

| Type | Bits | Alignment (bytes) |
|------|------|-------------------|
| `char` (**unsigned** by default) | 8 | 1 |
| `short`, `int` | 16 | 2 |
| `long` | 32 | 2 |
| `long long` | 64 | 2 |
| `float`, `double`, `long double` | 64 in memory (3 value words + 1 zero padding word) | 2 |
| data pointer, `size_t`, `ptrdiff_t`, `intptr_t`, function pointer | 16 (small), 32 (large) | 2 |
| `wchar_t` | 16 (unsigned) | 2 |

* Nothing needs more than word alignment, because that is the machine's
  granule. Structures are padded accordingly:
  `struct {char a; int b; char c;}` is 6 bytes, `struct {char a; long l;}` is 6,
  `struct {char c[3];}` is 3 (alignment 1).
* `char` is unsigned because the hardware's byte load zero-extends.
* `int` is 16 bits (a machine word), so `long` is needed for 32-bit values:
  write `1L << 20`, not `1 << 20`.
* The sizes and alignments above are checked at build time by
  `_Static_assert`s in `tests/abi/abi_main.c`.

Predefined macros: `__mera400__`, `__MERA400__`, `__BIG_ENDIAN__`, and
`__MERA400_LARGE__` in the large model.

## 7. Byte order and packing

The MERA-400 is **big-endian**: the most significant part comes first in
memory, whether the unit is a byte in a word or a word in a long value.

* A `long` `0x11223344` is stored as the words `0x1122`, `0x3344` at
  increasing word addresses.
* Two characters share a word, the **first character in the high byte**: the
  string `"ABCD"` is the words `0x4142`, `0x4344`. The byte address `2w` is the
  high byte, `2w+1` the low byte.
* A `long long` is four words, most significant first.

## 8. Floating point

The machine has no floating point hardware, so the compiler calls software
routines (section 10) that implement the machine's own 48-bit-class format
(a 40-bit mantissa and an 8-bit exponent):

* three words: a 40-bit two's-complement **mantissa** (word 0, word 1, high
  byte of word 2) followed by an 8-bit two's-complement **exponent** (low
  byte of word 2);
* value = `mantissa / 2^39 * 2^exponent`; normalized when the two top mantissa
  bits differ; zero is all bits zero.
* in memory a fourth word of zeros follows, so every `double` is 4 words.

Examples (memory words): `1.5` = `0x6000 0x0000 0x0001 0x0000`; `-2.0` =
`0x8000 0x0000 0x0001 0x0000`.

There are no infinities, NaNs or denormals; overflow saturates to the
largest finite value and underflow gives zero. `<float.h>`: 39 mantissa
bits, 11 decimal digits, exponents -128..127, max about 1.7e38, epsilon about
3.6e-12. The compiler does not define `__STDC_IEC_559__`.

## 9. Small and large memory models

| | small (default) | large (`-mlarge-memory`) |
|---|-----------------|--------------------------|
| data and function pointers | 16 bits | 32 bits |
| `size_t`, `ptrdiff_t` | 16 bits | 32 bits |
| address space | 32K words for byte pointers | the whole 64K words |
| stack top | word `0x7FFE` | word `0xFFFF` |

Everything else (registers, frame layout, type sizes of non-pointers, the
float format) is the same. Programs and libraries of different models must not
be linked together. The assembler is told the model with `--defsym=MERA400_LARGE=1`,
which is why the example assembly has `.ifdef MERA400_LARGE` blocks for each
place where pointers change width and therefore argument positions.

## 10. Compiler helper functions

The compiler turns operations the CPU lacks into calls. They follow the
rules above and are in `libbuiltins.a`; assembly may call them too.

**16-bit** (`a` in R1, `b` in R2, result R1): `__mulhi3`, `__divhi3`,
`__modhi3`, `__udivhi3`, `__umodhi3`, and shifts `__ashlhi3`, `__lshrhi3`,
`__ashrhi3`. A shift count of 16 or more gives 0 (or 0/-1 for an arithmetic
right shift). Unsigned division by zero gives `0xFFFF`.

**32-bit** (`long a` in R1:R2, `long b` on the stack, result R1:R2):
`__mulsi3`, `__divsi3`, `__modsi3`, `__udivsi3`, `__umodsi3`, `__udivmodsi4`.
Shifts `__ashlsi3`, `__lshrsi3`, `__ashrsi3` take `(long a, long n)`: the count
is a **32-bit** value, hence on the stack (`0(r7)` high word, `1(r7)` low);
counts of 32 or more give 0.

**64-bit**: `__muldi3`, `__divdi3`, `__moddi3`, `__udivdi3`, `__umoddi3`,
`__udivmoddi4(n, d, u64 *rem)`, `__ashldi3`, `__lshrdi3`, `__ashrdi3(a, long n)`,
`__mulosi4`, `__mulodi4`. A 64-bit result uses the hidden pointer of section 3.

**Floating point** (`__m48_` is the native 48-bit-style format): `__m48_add`,
`__m48_sub`, `__m48_mul`, `__m48_div` (`a` in R1:R2:R3, `b` on the stack,
result R1:R2:R3), `__m48_neg`, `__m48_cmp` (returns -1, 0 or 1),
`__m48_fromi32`, `__m48_fromu32`, `__m48_fromi64`, `__m48_fromu64`,
`__m48_toi32`, `__m48_tou32`, `__m48_toi64`, `__m48_tou64`, `__m48_powi(double,
int)`. 16-bit integers are widened to the 32-bit forms first. Division by zero
gives the largest finite value with the dividend's sign.

**Memory**: `memcpy`, `memmove`, `memset` are ordinary C functions that return
the destination. Small model: R1 = destination, R2 = source (or the fill
value for `memset`), R3 = byte count. Large model: destination R1:R2, then
source and count on the stack, as 32-bit values. Copies of a small constant
size are expanded inline by the compiler (by words if both sides are word
aligned).

## 11. C++

C++ uses the **Itanium** C++ ABI (the common cross-platform one), with these
specifics:

* **Names** are mangled: `int Counter::plain(int)` is `_ZN7Counter5plainEi`
  (`_ZN`, then each name prefixed by its length, `E`, then parameter types).
  Use `extern "C"` for plain names. Under `-ffreestanding`, `main` must be
  `extern "C"`.
* `this` is a data pointer (byte address), passed as the first argument.
* **vtable** (virtual function table): the object's first word (small) or
  two words (large) is the **vptr**, pointing at slot 0 of the table.
  Small model: the vptr and the entries are 16-bit **word** addresses.
  Large model: 32-bit **byte** addresses. Before slot 0 there are two entries
  (offset to top, RTTI pointer) that are both zero here.
* **Pointers to member functions** are two pointer-sized fields
  `{ptr, adj}` (the "ARM" variant). For a non-virtual function `ptr` is the
  function's address (word address small, byte address large) and `adj = 0`;
  for a virtual function `ptr` is the vtable byte offset (slot * pointer size)
  and `adj = 1` (the low bit means virtual; `adj >> 1` adjusts `this`).
* **Static constructors** are called from `.init_array` by the start-up code
  before `main`; each entry is a 2-byte word address (small) or 4-byte byte
  address (large). Destructors and `atexit` run in `exit`.
* **No exceptions, no RTTI** (run-time type information: `dynamic_cast`,
  `typeid`) and no thread-safe statics: the compiler turns them off and
  rejects `-fexceptions`.

## 12. Sections, start-up and linking

* `crt0.o` (the C runtime start-up) must be **linked first**: its section
  `.text.crt0` is placed first by the linker script, because the machine
  starts executing at the first word of the image.
* The link uses `--gc-sections` (drop unused sections). So give each
  function its own section (`.section .text.name,"ax"`) and each datum
  `.data.name`, or an unused function drags the whole file in. Compiled C does
  this with `-ffunction-sections -fdata-sections`.
* Section order in the image: `.text`, `.rodata`, `.preinit_array`,
  `.init_array`, `.fini_array`, `.data`, `.bss`.
* **Start-up state**: memory other than the loaded image is undefined. `_start`
  sets R7 to the stack top, zeroes `.bss`, runs the constructors, calls
  `main(0, 0)` (argc = 0, argv = null) and passes the result to `exit`.
  The program then ends with `hlt 077` (R1 = the exit status); `abort` uses
  `hlt 040`.
* Assemble: `llvm-mc -triple=mera400-unknown-none -filetype=obj file.s`
  (add `--defsym=MERA400_LARGE=1` for the large model). In CMake use
  `mera400_add_executable(name SOURCES a.c b.s LIBRARIES c builtins)`.
* **CROOK versions.** Under CROOK, `__CROOK_VERSION__` (4 or 5, set by Clang's
  `-mcrook-version=`) tells the source which CROOK it is built for. The ABI,
  data layout and image format are the same for both; only the availability
  of the CROOK-5 memory extracodes (GMEM, RMEM, LRAM) differs, see the
  README, section "CROOK versions: 4 and 5".

## 13. Examples

All the code here is from `tests/abi/`. `abi_asm.s` is the assembly (with
`.ifdef MERA400_LARGE` variants), `abi_main.c` calls each function and
checks the results, `abi_cpp.cpp` and `abi_cpp_asm.s` do the same for C++.

### Calling assembly from C

A leaf function: arguments in R1-R3, result in R1, nothing else touched.

```asm
; int asm_add3(int a, int b, int c)
asm_add3:
    aw r1, r2
    aw r1, r3
    uj r4
```

Declared in C as `int asm_add3(int a, int b, int c);` and called as usual.

A `long` argument that would be torn goes on the stack (word 0 = high word):

```asm
; long asm_ladd(long a, long b)   a = r1:r2, b = stack
asm_ladd:
    tw r3, 1+r7        ; low word of b
    aw r2, r3          ; low words first: sets the carry
    tw r3, r7          ; high word of b (loads keep the carry)
    ac r1, r3          ; add with carry
    uj r4
```

A pointer parameter must be converted from a byte to a word address:

```asm
; small model: int asm_sum_words(const int *p, int n)  p = r1, n = r2
    srz r1             ; byte -> word address
    ...
    tw r5, r1          ; load *p
```

(The large model version also shifts the 32-bit pointer; see the file.)

### Calling C from assembly

A function that makes a call saves R4 and has a frame. This calls the
variadic `int c_vmixed(int tag, ...)` with an `int`, a `long` and a `double`:

```asm
asm_call_vmixed:
    awt r7, -7         ; 6 argument words + saved r4
    rw r4, 6+r7
    lwt r1, 10
    rw r1, r7          ; int
    ...                ; long: 2 words, double: 3 words at 1+r7.. 5+r7
    lwt r1, 1          ; tag in r1
    rj r4, c_vmixed
    tw r4, 6+r7
    awt r7, 7
    uj r4
```

Callee-saved registers survive: `asm_check_saved` loads R5 and R6, calls a
C function that uses many registers, and checks they are unchanged.

Function pointers (small: word address, `rj r4, r5` calls it directly;
large: shift the 32-bit byte address first, see `asm_apply_twice`):

```asm
    lw r5, r1          ; small model: f
    lw r1, r2          ; x
    rj r4, r5
```

### Structures and floating point

```asm
; int asm_point_sum(struct point p)     r1 = WORD address of the copy
    tw r2, r1
    tw r3, 1+r1
    aw r2, r3
    lw r1, r2
    uj r4
```

`asm_make_point` writes the result through the hidden first argument (a byte
pointer: shift it right first). `asm_dbl_twice` shows a `double` in R1:R2:R3:
it copies the value to the stack as the second argument of `__m48_add`.

### C++ from assembly

```asm
; int asm_call_plain(Counter *c, int x)  =  c->plain(x)
asm_call_plain:
    uj _ZN7Counter5plainEi      ; same arguments: jump to the mangled name

; a virtual call, small model
    lw r6, r1
    srz r6              ; word address of the object
    tw r6, r6           ; vptr
    tw r6, r6           ; slot 0
    rj r4, r6           ; this and x still in their registers
```

### Running the tests

```
cmake --build build
ctest --test-dir build -R abi --output-on-failure
```

Do the same in a `-mlarge-memory` build directory. A failure prints the source
line in `abi_main.c` or `abi_cpp.cpp` of the first failed check.
