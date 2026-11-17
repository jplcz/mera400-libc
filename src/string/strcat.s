;===-- strcat.s - append a string -----------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strcat(char *dest, const char *src);
;
; Registers on entry:  r1 = dest, r2 = src, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: r5 is a byte with a zero high octet. First r3 walks dest to its
; terminator (cb r5, r3 sets L while the byte is not zero). Then r3 becomes
; the distance from src to that position, so r2+r3 is the destination of
; the byte at r2 and a single moving pointer (r2) drives the copy.

	.section .text.strcat,"ax"
	.globl strcat
strcat:
	awt r7, -1
	rw r5, r7
	lwt r5, 0
	lw r3, r1
	awt r3, -1
.find:
	awt r3, 1
	cb r5, r3
	jls .find		; r3 now points to the terminator of dest
	sw r3, r2		; r3 = dest_end - src
.copy:
	lb r5, r2
	rb r5, r2+r3
	awt r2, 1
	cwt r5, 0
	jn .copy
	tw r5, r7
	awt r7, 1
	uj r4
