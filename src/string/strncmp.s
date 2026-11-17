;===-- strncmp.s - compare at most n bytes of two strings -----------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int strncmp(const char *s1, const char *s2, size_t n);
;
; Like strcmp, but looks at no more than n bytes.
;
; Registers on entry:  r1 = s1, r2 = s2, r3 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: memcmp's "end pointer, negative index" loop with one extra test:
; when the equal bytes are zero the strings ended together and the result
; is 0, even if fewer than n bytes were compared.

	.section .text.strncmp,"ax"
	.globl strncmp
strncmp:
	cwt r3, 0
	jn .start
	lwt r1, 0		; n == 0: equal
	uj r4
.start:
	awt r7, -1
	rw r5, r7
	lwt r5, 0
	aw r1, r3		; r1 = s1 + n
	aw r2, r3		; r2 = s2 + n
	nga r3			; r3 = -n
.loop:
	lb r5, r1+r3
	cb r5, r2+r3
	jn .diff
	cwt r5, 0
	jes .equal		; both strings ended
	irb r3, .loop
.equal:
	lwt r1, 0
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
