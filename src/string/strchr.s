;===-- strchr.s - find a character in a string ----------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strchr(const char *s, int c);
;
; Returns a pointer to the first byte equal to (char)c, or a null pointer
; (0). The terminator counts as part of the string, so strchr(s, 0) returns
; a pointer to it.
;
; Registers on entry:  r1 = s, r2 = c (only the low octet counts),
;                      r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (none is touched)
;
; Algorithm: r3 is zero. At each byte the match is tested first (cb r2, r1
; sets E), then the terminator (cb r3, r1 sets L while the byte is not
; zero). The match test comes first so that c == 0 finds the terminator.

	.text
	.globl strchr
strchr:
	lwt r3, 0
	awt r1, -1
.loop:
	awt r1, 1
	cb r2, r1
	jes .found
	cb r3, r1
	jls .loop
	lwt r1, 0		; reached the terminator: not found
.found:
	uj r4
