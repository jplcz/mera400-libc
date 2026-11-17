;===-- strcmp.s - compare two strings -------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int strcmp(const char *s1, const char *s2);
;
; Returns the difference of the first pair of bytes that differ (as unsigned
; values, -255..255) or 0 when the strings are equal.
;
; Registers on entry:  r1 = s1, r2 = s2, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: r3 is the index i, r5 holds s1[i] with a zero high octet. cb
; compares it with s2[i] (unsigned, E flag when equal). Equal bytes that are
; zero end the loop with result 0. On a mismatch the byte of s2 is loaded
; into r1 and subtracted.

	.section .text.strcmp,"ax"
	.globl strcmp
strcmp:
	awt r7, -1
	rw r5, r7
	lwt r5, 0
	lwt r3, 0
.loop:
	lb r5, r1+r3
	cb r5, r2+r3
	jn .diff
	awt r3, 1
	cwt r5, 0
	jn .loop
	lwt r1, 0		; equal up to the terminator
	ujs .exit
.diff:
	lw r2, r2+r3
	lwt r1, 0
	lb r1, r2		; r1 = s2[i]
	sw r5, r1
	lw r1, r5
.exit:
	tw r5, r7
	awt r7, 1
	uj r4
