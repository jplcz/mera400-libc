;===-- mulhi3.s - 16-bit multiplication -----------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int16_t __mulhi3(int16_t a, int16_t b);           (a * b, low 16 bits)
;
; The low 16 bits of a product are the same for signed and unsigned
; operands, so one routine serves both.
;
; Registers on entry:  r1 = a, r2 = b, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r5, r6, r7 (r3 is scratch)
;
; Algorithm: shift and add. r3 accumulates. While b is not zero, its lowest
; bit is shifted out into the Y flag (srz); if it was 1 the current a is
; added; a doubles each round (slz). The loop runs for as many rounds as b
; has significant bits.

	.text
	.globl __mulhi3
__mulhi3:
	lwt r3, 0
.loop:
	cwt r2, 0
	jes .done
	srz r2			; Y = lowest bit of b
	jys .add
.next:
	slz r1
	ujs .loop
.add:
	aw r3, r1
	ujs .next
.done:
	lw r1, r3
	uj r4
