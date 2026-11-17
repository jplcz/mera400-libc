;===-- strlen.s - length of a string --------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; size_t strlen(const char *s);
;
; Registers on entry:  r1 = s, r4 = return address
; Registers on exit:   r1 = length
; Preserved:           r5, r6, r7 (none is touched)
;
; Algorithm: r2 remembers the start, r3 is zero, and r1 walks the string.
; "cb r3, r1" compares the byte at r1 with 0: the L (less) flag is set
; exactly when 0 < byte, i.e. the byte is not the terminator, so jls repeats.
; The pointer is advanced before the compare and not after it, because the
; add would otherwise sit between the compare and the jump and the loop
; would depend on the add leaving the compare flags alone.

	.text
	.globl strlen
strlen:
	lw r2, r1		; r2 = s
	lwt r3, 0
	awt r1, -1
.loop:
	awt r1, 1
	cb r3, r1
	jls .loop
	sw r1, r2		; r1 = end - start
	uj r4
