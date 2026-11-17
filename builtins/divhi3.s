;===-- divhi3.s - signed 16-bit division ----------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int16_t __divhi3(int16_t a, int16_t b);           (a / b, toward zero)
;
; Registers on entry:  r1 = a, r2 = b, r4 = return address
; Registers on exit:   r1 = quotient
; Preserved:           r5, r6, r7 (r5 and r4 are saved on the stack)
;
; Algorithm: divide the absolute values with __udivmodhi and negate the
; quotient when the signs of a and b differ. r5 keeps a XOR b, whose top bit
; is the sign of the result; r5 survives the call because __udivmodhi
; preserves it. The absolute value of -32768 is 32768 when read as unsigned,
; which is what the unsigned division wants. Division by zero is undefined.

	.section .text.__divhi3,"ax"
	.globl __divhi3
__divhi3:
	awt r7, -2
	rw r4, 1+r7
	rw r5, r7
	lw r5, r1
	xr r5, r2		; top bit of r5 = sign of the quotient
	cwt r1, 0
	jgs .a_positive
	nga r1			; |a|
.a_positive:
	cwt r2, 0
	jgs .b_positive
	nga r2			; |b|
.b_positive:
	rj r4, __udivmodhi
	cwt r5, 0
	jgs .done
	jes .done		; a XOR b == 0: same sign
	nga r1			; signs differed: negate the quotient
.done:
	tw r5, r7
	tw r4, 1+r7
	awt r7, 2
	uj r4
