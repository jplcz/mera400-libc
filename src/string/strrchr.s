;===-- strrchr.s - find the last occurrence of a character ----------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strrchr(const char *s, int c);
;
; Returns a pointer to the last byte equal to (char)c, or a null pointer
; (0). strrchr(s, 0) returns a pointer to the terminator.
;
; Registers on entry:  r1 = s, r2 = c (only the low octet counts),
;                      r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: one pass over the whole string. r3 remembers the address of the
; latest match (initially 0 = none); r5 is zero and serves as the terminator
; test. The match test precedes the terminator test, so for c == 0 the
; terminator itself is recorded.

	.section .text.strrchr,"ax"
	.globl strrchr
strrchr:
	awt r7, -1
	rw r5, r7
	lwt r5, 0
	lwt r3, 0
	awt r1, -1
.loop:
	awt r1, 1
	cb r2, r1
	jn .nomatch
	lw r3, r1		; remember the match
.nomatch:
	cb r5, r1
	jls .loop
	lw r1, r3
	tw r5, r7
	awt r7, 1
	uj r4
