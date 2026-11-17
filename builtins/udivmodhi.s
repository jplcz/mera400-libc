;===-- udivmodhi.s - unsigned 16-bit division with remainder --------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; Internal helper shared by __udivhi3, __umodhi3, __divhi3 and __modhi3. It
; is not part of the compiler's library-call interface, so its name is
; private to this library.
;
; uint16_t __udivmodhi(uint16_t a, uint16_t b)
;   r1 = a / b (quotient), r2 = a % b (remainder)
;
; Registers on entry:  r1 = a (dividend), r2 = b (divisor), r4 = return address
; Registers on exit:   r1 = quotient, r2 = remainder
; Preserved:           r5, r6, r7 (r5 is saved on the stack; r3 is scratch)
;
; Division by zero (undefined in C) gives quotient 0xFFFF and remainder a.
;
; Algorithm: restoring binary long division, 16 rounds. r3 is the running
; remainder, r5 counts the rounds. Each round moves the top bit of the
; dividend (r1) into the remainder: "slz r1" shifts it out into Y and
; "sly r3" shifts it into r3 while the bit that falls out of r3 lands in Y.
; If that bit is set the remainder no longer fits in 16 bits and is
; certainly >= b; otherwise it is compared with b (unsigned, cl). When it is
; >= b the divisor is subtracted (modulo 2^16 this is also right after the
; overflow) and a 1 goes into the freed low bit of r1, which becomes the
; quotient as the dividend bits leave.

	.text
	.globl __udivmodhi
__udivmodhi:
	cwt r2, 0
	jn .go
	lw r2, r1		; b == 0: remainder = a, quotient = 0xFFFF
	lwt r1, -1
	uj r4
.go:
	awt r7, -1
	rw r5, r7
	lwt r3, 0
	lwt r5, 16
.loop:
	slz r1			; Y = top bit of the dividend
	sly r3			; r3 = r3*2 + that bit; Y = bit shifted out of r3
	jys .sub
	cl r3, r2
	jls .next		; remainder < b: quotient bit stays 0
.sub:
	sw r3, r2
	awt r1, 1		; quotient bit = 1
.next:
	drb r5, .loop
	lw r2, r3
	tw r5, r7
	awt r7, 1
	uj r4
