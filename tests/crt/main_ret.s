;===-- main_ret.s - tiny test program for crt0 -----------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; main() for the crt0 test: it checks that .bss was cleared (it reads
; "counter", which crt0 must have zeroed), stores a marker in .bss, calls
; strlen through the C calling convention and returns 42 plus the length of
; "hello". The simulator checks r1 == 47 and the marker.

	.text
	.globl main
main:
	awt r7, -1
	rw r4, r7
	tw r1, counter		; must be zero (cleared .bss)
	cwt r1, 0
	jes .ok
	hlt 040			; .bss was not cleared
.ok:
	lwt r1, 1
	rw r1, counter
	lw r1, text_bytes	; word address of the string...
	aw r1, r1		; ...times two = byte address
	rj r4, strlen
	awt r1, 42
	tw r4, r7
	awt r7, 1
	uj r4

	.data
	.globl text_bytes
text_bytes:
	.asciiz "hello"

	.bss
	.globl counter
counter:
	.res 1
