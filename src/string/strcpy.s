;===-- strcpy.s - copy a string -------------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strcpy(char *dest, const char *src);
;
; Registers on entry:  r1 = dest, r2 = src, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: r3 is the index i. Each round copies src[i] to dest[i] through
; r5 (cleared once, so its high octet is zero and the whole word can be
; compared with zero), advances i and repeats while the copied byte was not
; the terminator. dest is never moved, so it is already the result.

	.section .text.strcpy,"ax"
	.globl strcpy
strcpy:
	awt r7, -1
	rw r5, r7
	lwt r5, 0
	lwt r3, 0
.loop:
	lb r5, r2+r3
	rb r5, r1+r3
	awt r3, 1
	cwt r5, 0
	jn .loop
	tw r5, r7
	awt r7, 1
	uj r4
