;===-- abi_asm.s - assembly side of the ABI test --------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; Hand written functions that follow the C calling convention, so C can call
; them, and that call C functions and compiler helpers. They are the examples
; of docs/abi.md; abi_main.c calls every one of them and checks the results.
; Where the large memory model differs (C data pointers and function pointers
; are 32 bits wide, so every argument after them moves) the code is under
; ".ifdef MERA400_LARGE", which the build defines for the large model.
;
; Reminder of the instructions used (r0..r7 are the registers):
;   lw  rA, x    rA = x (a register, or a 16 bit constant/word address)
;   lwt rA, n    rA = n for a small constant n (-63..63)
;   tw  rA, a    rA = the word of memory at address a  (load)
;   rw  rA, a    the word of memory at address a = rA  (store)
;   aw/ac/sw     add / add with carry / subtract
;   rj  r4, f    r4 = address of the next instruction, jump to f (a call)
;   uj  r4       jump to r4 (a return)
; Addresses given to tw and rw are WORD addresses, and so is the value
; "lw rA, symbol" loads.

;---------------------------------------------------------------------------
; int asm_add3(int a, int b, int c)       r1 = a, r2 = b, r3 = c
; A leaf function: it touches no stack and needs no prologue.
	.section .text.asm_add3,"ax"
	.globl asm_add3
asm_add3:
	aw r1, r2
	aw r1, r3
	uj r4

;---------------------------------------------------------------------------
; long asm_ladd(long a, long b)
; a = r1:r2 (high word in r1). b would be split between r3 and the stack, so
; it goes to the stack as a whole: 0(r7) is its high word, 1(r7) its low word.
; The result is returned in r1:r2.
	.section .text.asm_ladd,"ax"
	.globl asm_ladd
asm_ladd:
	tw r3, 1+r7		; low word of b
	aw r2, r3		; low words first: this sets the carry
	tw r3, r7		; high word of b (loads keep the carry)
	ac r1, r3		; add with carry
	uj r4

;---------------------------------------------------------------------------
; long long asm_ll_add(long long a, long long b)
; A 64-bit result does not fit r1..r3, so the caller passes a hidden pointer
; to the result in r1: a WORD address, in both models. Then a is r2, r3 and
; stack words 0 and 1; b is stack words 2..5. All values have their most
; significant word first. r5 and r6 are callee-saved, so they are saved first,
; which moves the stack arguments up by two words.
	.section .text.asm_ll_add,"ax"
	.globl asm_ll_add
asm_ll_add:
	awt r7, -2
	rw r5, 1+r7
	rw r6, r7
	tw r5, 3+r7		; a[3] (stack word 1 of the caller + 2)
	tw r6, 7+r7		; b[3]
	aw r5, r6
	rw r5, 3+r1		; result[3]
	tw r5, 2+r7		; a[2]
	tw r6, 6+r7		; b[2]
	ac r5, r6
	rw r5, 2+r1
	tw r6, 5+r7		; b[1]
	ac r3, r6
	rw r3, 1+r1
	tw r6, 4+r7		; b[0]
	ac r2, r6
	rw r2, r1
	tw r6, r7
	tw r5, 1+r7
	awt r7, 2
	uj r4

;---------------------------------------------------------------------------
; int asm_sum_words(const int *p, int n)
; A C data pointer is a BYTE address; word loads need a word address, so the
; pointer is shifted right by one bit first.
;   small: p = r1, n = r2
;   large: p = r1:r2 (32 bits), n = r3
; r5 is callee-saved, so the function saves it in a one word frame.
	.section .text.asm_sum_words,"ax"
	.globl asm_sum_words
asm_sum_words:
	awt r7, -1
	rw r5, r7
.ifdef MERA400_LARGE
	srz r1			; shift the 32 bit byte address right by one...
	sry r2			; ...r2 is now the word address
	lw r1, r2
	lw r2, r3		; n
.else
	srz r1			; byte address -> word address
.endif
	lwt r3, 0		; sum
.sum_loop:
	cwt r2, 0
	jes .sum_done
	tw r5, r1
	aw r3, r5
	awt r1, 1
	awt r2, -1
	ujs .sum_loop
.sum_done:
	lw r1, r3
	tw r5, r7
	awt r7, 1
	uj r4

;---------------------------------------------------------------------------
; int asm_byte_at(const char *p)    the byte p points to, as 0..255
; Byte address = word address * 2 + (0 for the high byte, 1 for the low
; byte): the machine is big-endian and the first character of a string is
; the high byte of its word.
;   small: p = r1: the "lb" instruction takes exactly this byte address. It
;          replaces only the low byte of the register, so clear it first.
;   large: p = r1:r2. "lb" cannot reach all of memory, so load the word and
;          pick the byte by hand.
	.section .text.asm_byte_at,"ax"
	.globl asm_byte_at
asm_byte_at:
.ifdef MERA400_LARGE
	lw r3, r2
	nr r3, 1		; 0: high byte, 1: low byte
	srz r1
	sry r2			; r2 = word address
	tw r2, r2		; the word
	cwt r3, 0
	jn .low_byte
	shc r2, 8		; rotate the high byte into the low byte
.low_byte:
	nr r2, 255
	lw r1, r2
.else
	lwt r2, 0
	lb r2, r1
	lw r1, r2
.endif
	uj r4

;---------------------------------------------------------------------------
; char *asm_buf_ptr(void)     the address of the string asm_buf, as a C pointer
; "lw r1, asm_buf" gives the WORD address; C wants a BYTE address, which is
; twice as much (a 32 bit value in the large model: r1 = high word).
	.section .text.asm_buf_ptr,"ax"
	.globl asm_buf_ptr
asm_buf_ptr:
.ifdef MERA400_LARGE
	lw r2, asm_buf
	lwt r1, 0
	slz r2			; shift left, the bit shifted out goes to Y...
	sly r1			; ...and comes in on the right of the high word
.else
	lw r1, asm_buf
	slz r1
.endif
	uj r4

	.section .data.asm_buf,"aw"
	.globl asm_buf
asm_buf:
	.asciiz "ABCD"		; two characters per word, first one in the high byte

;---------------------------------------------------------------------------
; int asm_apply_twice(int (*f)(int), int x)     returns f(f(x))
; A function pointer is a WORD address of code in the small model, and a
; 32 bit BYTE address in the large one. "rj r4, rX" calls the word address in
; rX. The function calls others, so it saves r4 (its own return address) and
; keeps f in r5, which a callee must preserve.
;   small: f = r1, x = r2
;   large: f = r1:r2, x = r3
	.section .text.asm_apply_twice,"ax"
	.globl asm_apply_twice
asm_apply_twice:
	awt r7, -2
	rw r4, 1+r7
	rw r5, r7
.ifdef MERA400_LARGE
	srz r1
	sry r2			; r2 = word address of the code
	lw r5, r2
	lw r1, r3		; x
.else
	lw r5, r1
	lw r1, r2
.endif
	rj r4, r5		; r1 = f(x)
	rj r4, r5		; r1 = f(f(x))
	tw r5, r7
	tw r4, 1+r7
	awt r7, 2
	uj r4

; int asm_double(int x): a function written in assembly whose address C takes
; and passes to asm_apply_twice.
	.section .text.asm_double,"ax"
	.globl asm_double
asm_double:
	aw r1, r1
	uj r4

;---------------------------------------------------------------------------
; int asm_vsum(int n, ...)     adds the n int arguments after n
; The named argument n is in r1. EVERY variadic argument is on the stack, one
; word each, starting at 0(r7). The function needs no register save area.
	.section .text.asm_vsum,"ax"
	.globl asm_vsum
asm_vsum:
	lw r2, r7		; r2 walks the stack words
	lwt r3, 0
.vsum_loop:
	cwt r1, 0
	jes .vsum_done
	aw r3, [r2]
	awt r2, 1
	awt r1, -1
	ujs .vsum_loop
.vsum_done:
	lw r1, r3
	uj r4

;---------------------------------------------------------------------------
; int asm_call_vmixed(void)     calls  c_vmixed(1, 10, 0x00010002L, 4.0)
; The C function is "int c_vmixed(int tag, ...)". tag goes to r1; the three
; variadic arguments are stored in the outgoing area at the bottom of the
; frame, which is where the callee finds them at 0(r7): an int (1 word), a
; long (2 words, high first) and a double (3 words). Frame: 6 argument words
; plus one for the saved return address.
	.section .text.asm_call_vmixed,"ax"
	.globl asm_call_vmixed
asm_call_vmixed:
	awt r7, -7
	rw r4, 6+r7
	lwt r1, 10
	rw r1, r7		; the int
	lwt r1, 1
	rw r1, 1+r7		; the long, high word
	lwt r1, 2
	rw r1, 2+r7		; the long, low word
	tw r1, .double_four
	rw r1, 3+r7		; the double, three words
	tw r1, .double_four+1
	rw r1, 4+r7
	tw r1, .double_four+2
	rw r1, 5+r7
	lwt r1, 1		; tag
	rj r4, c_vmixed
	tw r4, 6+r7
	awt r7, 7
	uj r4
.double_four:
	.float 4.0		; three words: the native floating point format

;---------------------------------------------------------------------------
; double asm_dbl_twice(double x)     returns x + x
; A double is three words: r1:r2:r3, returned the same way. __m48_add(a, b)
; takes a in r1:r2:r3 and, as no registers are left, b on the stack. The
; function is called with x already in r1..r3, so it only has to copy x to
; 0(r7)..2(r7) and call.
	.section .text.asm_dbl_twice,"ax"
	.globl asm_dbl_twice
asm_dbl_twice:
	awt r7, -4
	rw r4, 3+r7
	rw r1, r7
	rw r2, 1+r7
	rw r3, 2+r7
	rj r4, __m48_add
	tw r4, 3+r7
	awt r7, 4
	uj r4

;---------------------------------------------------------------------------
; long asm_shl32(long x, int n)     x << n, through the compiler helper
; __ashlsi3(long x, long n): x in r1:r2 and the count as a 32 bit value, which
; cannot be split between r3 and the stack, so it is on the stack: 0(r7) =
; high word (zero), 1(r7) = low word. Here n arrives in r3.
	.section .text.asm_shl32,"ax"
	.globl asm_shl32
asm_shl32:
	awt r7, -3
	rw r4, 2+r7
	rw r3, 1+r7
	lwt r3, 0
	rw r3, r7
	rj r4, __ashlsi3
	tw r4, 2+r7
	awt r7, 3
	uj r4

;---------------------------------------------------------------------------
; struct point { int x, y; };
; An aggregate argument is passed as the address of a copy that the caller
; made: a WORD address (16 bits in both models), in the next argument slot.
; int asm_point_sum(struct point p)     r1 = word address of the copy
	.section .text.asm_point_sum,"ax"
	.globl asm_point_sum
asm_point_sum:
	tw r2, r1		; p.x
	tw r3, 1+r1		; p.y
	aw r2, r3
	lw r1, r2
	uj r4

; struct point asm_make_point(int x, int y)
; An aggregate result is written through a hidden first argument: a C data
; pointer, so a BYTE address (r1, or r1:r2 in the large model); x and y follow.
;   small: dest = r1, x = r2, y = r3
;   large: dest = r1:r2, x = r3, y = 0(r7)
	.section .text.asm_make_point,"ax"
	.globl asm_make_point
asm_make_point:
.ifdef MERA400_LARGE
	srz r1
	sry r2
	lw r1, r2		; r1 = word address of the result
	rw r3, r1		; x
	tw r3, r7		; y
	rw r3, 1+r1
.else
	srz r1
	rw r2, r1		; x
	rw r3, 1+r1		; y
.endif
	uj r4

;---------------------------------------------------------------------------
; Narrow integers: the CALLER extends an argument to 16 bits (sign extension
; for signed char and short, zero extension for unsigned ones) and the CALLEE
; extends a result, so neither side masks what it receives.
; int asm_sum_chars(unsigned char a, signed char b)
	.section .text.asm_sum_chars,"ax"
	.globl asm_sum_chars
asm_sum_chars:
	aw r1, r2
	uj r4

; signed char asm_schar_minus2(void): the result must be sign extended
	.section .text.asm_schar_minus2,"ax"
	.globl asm_schar_minus2
asm_schar_minus2:
	lwt r1, -2
	uj r4

;---------------------------------------------------------------------------
; Callee-saved registers: r5 and r6 survive a call, r1..r3 and r4 do not.
; int asm_check_saved(void)     1 if c_clobber left r5 and r6 alone
	.section .text.asm_check_saved,"ax"
	.globl asm_check_saved
asm_check_saved:
	awt r7, -3
	rw r4, 2+r7
	rw r5, 1+r7
	rw r6, r7
	lw r5, 0x1234
	lw r6, 0x5678
	lwt r1, 1
	lwt r2, 2
	lwt r3, 3
	rj r4, c_clobber
	lw r2, 0x1234
	cw r5, r2
	jn .check_bad
	lw r2, 0x5678
	cw r6, r2
	jn .check_bad
	lwt r1, 1
	ujs .check_out
.check_bad:
	lwt r1, 0
.check_out:
	tw r6, r7
	tw r5, 1+r7
	tw r4, 2+r7
	awt r7, 3
	uj r4

;---------------------------------------------------------------------------
; int asm_bump_counter(void)
; Reads and writes the C global "int c_counter": "tw/rw rA, symbol" use the
; symbol as a word address, which is what the hardware wants.
	.section .text.asm_bump_counter,"ax"
	.globl asm_bump_counter
asm_bump_counter:
	tw r1, c_counter
	awt r1, 1
	rw r1, c_counter
	uj r4
