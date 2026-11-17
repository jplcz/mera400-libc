;===-- strncpy.s - copy a string, padding with zeros ----------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strncpy(char *dest, const char *src, size_t n);
;
; Copies at most n bytes of src. If src is shorter than n, the rest of the
; n bytes of dest is filled with zeros; if it is not, dest is not
; terminated (as the C standard demands).
;
; Registers on entry:  r1 = dest, r2 = src, r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (both are saved on the stack and restored)
;
; Algorithm: r5 is the index i and r6 is the byte, always with a zero high
; octet. The copy loop moves src[i] to dest[i] and counts r3 down with drb.
; When the terminator has been copied, r6 is zero, so the padding loop can
; store r6 for the remaining r3-1 bytes.

	.text
	.globl strncpy
strncpy:
	cwt r3, 0
	jes .ret		; n == 0: nothing to do
	awt r7, -2
	rw r5, 1+r7
	rw r6, r7
	lwt r5, 0
	lwt r6, 0
.copy:
	lb r6, r2+r5
	rb r6, r1+r5
	awt r5, 1
	cwt r6, 0
	jes .pad		; the terminator was just copied
	drb r3, .copy
	ujs .out		; n bytes copied, no terminator
.pad:
	drb r3, .fill		; r3 = bytes still to pad; none left: done
	ujs .out
.fill:
	rb r6, r1+r5
	awt r5, 1
	drb r3, .fill
.out:
	tw r6, r7
	tw r5, 1+r7
	awt r7, 2
.ret:
	uj r4
