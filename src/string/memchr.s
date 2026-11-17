;===-- memchr.s - find a byte in memory -----------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void *memchr(const void *s, int c, size_t n);
;
; Returns a pointer to the first of the n bytes that equals (unsigned char)c,
; or a null pointer (0).
;
; Registers on entry:  r1 = s, r2 = c (only the low octet counts),
;                      r3 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (none is touched)
;
; Algorithm: "end pointer, negative index" as in memcpy. cb compares the low
; octet of r2 with the byte at r1+r3 (unsigned) and sets the E flag on a
; match; irb counts r3 up to zero. On a match r1+r3 is the address of the
; byte.

	.text
	.globl memchr
memchr:
	cwt r3, 0
	jes .null
	aw r1, r3		; r1 = s + n
	nga r3			; r3 = -n
.loop:
	cb r2, r1+r3
	jes .found
	irb r3, .loop
.null:
	lwt r1, 0
	uj r4
.found:
	aw r1, r3
	uj r4
