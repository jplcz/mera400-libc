;===-- memset.s - fill bytes with a value ---------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void *memset(void *dest, int c, size_t n);
;
; Registers on entry:  r1 = dest, r2 = c (only the low octet counts),
;                      r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: dest is lowered by one and r3 counts down from n to 1 (drb), so
; rb stores the low octet of r2 at dest-1+r3, i.e. dest+n-1 first and dest
; last. dest is raised again before returning. The loop needs no scratch
; register and no masking of c, because rb ignores the high octet of r2.
;
; Fast path: when dest is even and n >= 2, the octet is replicated into both
; halves of r2 (shc = rotate by 8 and or) and whole words are stored; an odd
; last byte is stored separately. r5 is then used and saved on the stack,
; together with the original dest.

	.section .text.memset,"ax"
	.globl memset
memset:
	cwt r3, 0
	jes .done		; n == 0
	awt r7, -2
	rw r5, r7
	rw r1, 1+r7
	cl r3, 2
	jls .bytes
	lw r5, r1
	nr r5, 1
	cwt r5, 0
	jn .bytes		; dest is odd
	nr r2, 255
	lw r5, r2
	shc r5, 8
	or r2, r5		; r2 = c in both octets
	lw r5, r3
	srz r5			; r5 = number of whole words
	srz r1
	aw r1, r5
	nga r5
.wloop:
	rw r2, r1+r5
	irb r5, .wloop
	nr r3, 1
	cwt r3, 0
	jes .out
	slz r1			; byte address of the last byte
	rb r2, r1
	ujs .out
.bytes:
	awt r1, -1
.loop:
	rb r2, r1+r3
	drb r3, .loop
.out:
	tw r1, 1+r7
	tw r5, r7
	awt r7, 2
.done:
	uj r4
