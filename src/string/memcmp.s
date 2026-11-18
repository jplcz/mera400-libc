;===-- memcmp.s - compare bytes -------------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int memcmp(const void *s1, const void *s2, size_t n);
;
; Returns the difference of the first pair of bytes that differ, both taken
; as unsigned values (so the result is -255..255), or 0 when all n bytes are
; equal.
;
; Registers on entry:  r1 = s1, r2 = s2, r3 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: the same "end pointer, negative index" loop as memcpy. r5 holds
; s1[i] with a zero high octet (it is cleared once; lb replaces only the
; low octet). cb compares the low octet of r5 with the byte s2[i] as
; unsigned values and sets the E flag when they are equal; jn jumps when E
; is clear. Only after a mismatch are the pointers needed again, so the
; mismatching byte of s2 can then be loaded into r1.
;
; Fast path: when both pointers are even and n >= 2, whole words are compared
; (cw). The first differing word is then compared octet by octet (the high
; octet is the lower address), and a final odd byte is compared separately.

	.section .text.memcmp,"ax"
	.globl memcmp
memcmp:
	cwt r3, 0
	jn .start
	lwt r1, 0		; n == 0: equal
	uj r4
.start:
	awt r7, -2
	rw r5, r7
	rw r6, 1+r7
	cl r3, 2
	jls .bytes
	lw r5, r1
	or r5, r2
	nr r5, 1
	cwt r5, 0
	jn .bytes		; some pointer is odd
	lw r6, r3
	srz r6			; r6 = number of whole words
	srz r1
	srz r2
	aw r1, r6
	aw r2, r6
	nga r6
.wloop:
	tw r5, r1+r6
	cw r5, [r2+r6]
	jn .wdiff
	irb r6, .wloop
	lw r5, r3
	nr r5, 1
	cwt r5, 0
	jes .equal		; n is even
	slz r1			; byte addresses of the last byte
	slz r2
	lwt r5, 0
	lb r5, r1
	cb r5, r2
	jn .diff
.equal:
	lwt r1, 0
	ujs .exit
.wdiff:
	aw r1, r6
	aw r2, r6
	slz r1			; byte addresses of the differing words
	slz r2
	lwt r5, 0
	lb r5, r1
	cb r5, r2
	jn .diff		; the high octets differ
	awt r1, 1
	awt r2, 1
	lb r5, r1		; the low octets differ
	ujs .diff
.bytes:
	lwt r5, 0
	aw r1, r3		; r1 = s1 + n
	aw r2, r3		; r2 = s2 + n
	nga r3			; r3 = -n
.loop:
	lb r5, r1+r3		; r5 = s1[i]
	cb r5, r2+r3		; compare with s2[i]
	jn .bdiff
	irb r3, .loop
	lwt r1, 0		; all bytes equal
	ujs .exit
.bdiff:
	aw r2, r3		; r2 = address of s2[i]
.diff:				; r5 = s1[i], r2 = address of s2[i]
	lwt r1, 0
	lb r1, r2		; r1 = s2[i]
	sw r5, r1		; r5 = s1[i] - s2[i]
	lw r1, r5
.exit:
	tw r6, 1+r7
	tw r5, r7
	awt r7, 2
	uj r4
