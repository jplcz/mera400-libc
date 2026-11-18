;===-- memmove.s - copy bytes between possibly overlapping objects --------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void *memmove(void *dest, const void *src, size_t n);
;
; Registers on entry:  r1 = dest, r2 = src, r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7
;
; Algorithm: with dest < src a forward copy never overwrites a source byte
; that has not been read yet, so the work is handed to memcpy (a tail jump:
; r4 still holds the caller's return address). Otherwise (dest > src, or
; dest == src which needs no work) the bytes are copied backwards.
;
; The backward loops address unit r2+r3 and r1+r3 with r3 counting down
; from n to 1 (drb = decrement and branch if not zero). Both pointers are
; first lowered by one unit, so the first access is at offset n-1 and the
; last at offset 0. The pointer comparison is unsigned (cl), because byte
; addresses use the whole 16-bit range.
;
; Fast path: when both pointers are even and n >= 2 the copy goes by whole
; words, backwards; an odd last byte is copied first (it is the highest
; one). Otherwise the byte loop is used.

	.section .text.memmove,"ax"
	.globl memmove
memmove:
	cl r1, r2
	jl memcpy		; dest < src: copy forward
	je .done		; dest == src: nothing to do
	cwt r3, 0
	jes .done		; n == 0 (the drb loop needs n >= 1)
	awt r7, -3
	rw r5, r7
	rw r6, 1+r7
	rw r1, 2+r7		; remember dest
	cl r3, 2
	jls .bytes
	lw r5, r1
	or r5, r2
	nr r5, 1
	cwt r5, 0
	jn .bytes		; some pointer is odd
	lw r5, r3
	nr r5, 1
	cwt r5, 0
	jes .even
	awt r3, -1		; copy the odd last byte first
	lb r5, r2+r3
	rb r5, r1+r3
	awt r3, 1
.even:
	lw r6, r3
	srz r6			; r6 = number of whole words
	srz r1			; word addresses, lowered by one word
	srz r2
	awt r1, -1
	awt r2, -1
.wloop:
	tw r5, r2+r6
	rw r5, r1+r6
	drb r6, .wloop
	ujs .out
.bytes:
	awt r1, -1		; bias so that offset n maps to byte n-1
	awt r2, -1
.loop:
	lb r5, r2+r3
	rb r5, r1+r3
	drb r3, .loop
.out:
	tw r1, 2+r7		; r1 = dest
	tw r6, 1+r7
	tw r5, r7
	awt r7, 3
.done:
	uj r4
