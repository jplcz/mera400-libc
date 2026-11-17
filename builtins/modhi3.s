;===-- modhi3.s - signed 16-bit remainder ---------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int16_t __modhi3(int16_t a, int16_t b);           (a % b)
;
; The remainder has the sign of the dividend, so that
; (a / b) * b + a % b == a with division truncating toward zero.
;
; Registers on entry:  r1 = a, r2 = b, r4 = return address
; Registers on exit:   r1 = remainder
; Preserved:           r5, r6, r7 (r5 and r4 are saved on the stack)
;
; Algorithm: divide the absolute values with __udivmodhi, take the remainder
; it leaves in r2 and negate it when a was negative (r5 remembers a).

	.text
	.globl __modhi3
__modhi3:
	awt r7, -2
	rw r4, 1+r7
	rw r5, r7
	lw r5, r1		; sign of a
	cwt r1, 0
	jgs .a_positive
	nga r1			; |a|
.a_positive:
	cwt r2, 0
	jgs .b_positive
	nga r2			; |b|
.b_positive:
	rj r4, __udivmodhi
	lw r1, r2		; remainder of the absolute values
	cwt r5, 0
	jgs .done
	nga r1			; a was negative
.done:
	tw r5, r7
	tw r4, 1+r7
	awt r7, 2
	uj r4
