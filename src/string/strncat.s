;===-- strncat.s - append at most n bytes of a string ---------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; char *strncat(char *dest, const char *src, size_t n);
;
; Appends at most n bytes of src to dest and always writes a terminator.
;
; Registers on entry:  r1 = dest, r2 = src, r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (both are saved on the stack and restored)
;
; Algorithm: as strcat. r5 is a byte with a zero high octet; r6 first walks
; to the end of dest and then holds the distance from src to that position,
; so r2+r6 is where the byte at r2 goes. The copy stops at the source
; terminator or after n bytes; the terminator is stored in both cases.

	.text
	.globl strncat
strncat:
	awt r7, -2
	rw r5, 1+r7
	rw r6, r7
	lwt r5, 0
	lw r6, r1
	awt r6, -1
.find:
	awt r6, 1
	cb r5, r6
	jls .find		; r6 now points to the terminator of dest
	sw r6, r2		; r6 = dest_end - src
.copy:
	cwt r3, 0
	jes .term
	lb r5, r2
	cwt r5, 0
	jes .term
	rb r5, r2+r6
	awt r2, 1
	awt r3, -1
	ujs .copy
.term:
	lwt r5, 0
	rb r5, r2+r6
	tw r6, r7
	tw r5, 1+r7
	awt r7, 2
	uj r4
