;===-- memcpy.s - copy bytes between non-overlapping objects --------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void *memcpy(void *dest, const void *src, size_t n);
;
; Small memory model: every pointer is a 16-bit byte address
; (word address * 2 + octet), which is what lb/rb take.
;
; Registers on entry:  r1 = dest, r2 = src, r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (r5 is saved on the stack and restored)
;
; Algorithm: "end pointer, negative index". Both pointers are moved to the
; end of their objects and the counter becomes negative; the address
; r2+counter then walks the source forward while irb (increment and branch
; if not zero) counts up to zero. That costs one instruction per loop for the
; bookkeeping. lb only replaces the low octet of its destination and rb only
; stores the low octet, so r5 never has to be cleared.
;
; Fast path: when both pointers are even (word aligned) and n >= 2, whole
; words are copied (the machine moves a word per instruction pair, twice as
; fast as bytes) with the pointers turned into word addresses by one srz
; (shift right, zero fill); a final odd byte is copied separately. Any other
; case uses the byte loop. The original dest is kept on the stack.
;
; Copying is forward, which also makes memmove with dest < src correct (both
; paths read a unit before writing a unit at or below it).

	.section .text.memcpy,"ax"
	.globl memcpy
memcpy:
	cwt r3, 0
	jes .done		; n == 0: nothing to do, r1 is already dest
	awt r7, -3
	rw r5, r7		; save r5 at 0+r7
	rw r6, 1+r7		; save r6 at 1+r7
	rw r1, 2+r7		; remember dest at 2+r7
	cl r3, 2
	jls .bytes		; n < 2
	lw r5, r1
	or r5, r2
	nr r5, 1
	cwt r5, 0
	jn .bytes		; some pointer is odd
	lw r6, r3
	srz r6			; r6 = number of whole words
	srz r1			; word addresses
	srz r2
	aw r1, r6
	aw r2, r6
	nga r6
.wloop:
	tw r5, r2+r6
	rw r5, r1+r6
	irb r6, .wloop
	lw r5, r3
	nr r5, 1
	cwt r5, 0
	jes .out		; n is even
	slz r1			; r1, r2 = byte addresses of the last byte
	slz r2
	lb r5, r2
	rb r5, r1
	ujs .out
.bytes:
	aw r1, r3		; r1 = dest + n
	aw r2, r3		; r2 = src + n
	nga r3			; r3 = -n
.loop:
	lb r5, r2+r3
	rb r5, r1+r3
	irb r3, .loop
.out:
	tw r1, 2+r7		; r1 = dest
	tw r6, 1+r7
	tw r5, r7
	awt r7, 3
.done:
	uj r4
