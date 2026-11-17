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
; end of their objects and r3 becomes -n; the address r2+r3 then walks the
; source forward while irb (increment and branch if not zero) counts r3 up
; to zero. That costs one instruction per loop for the bookkeeping, and the
; loop needs only one scratch register (r5). lb only replaces the low octet
; of its destination and rb only stores the low octet, so r5 never has to be
; cleared. Because the original dest is lost in the process, n is kept on the
; stack and subtracted again at the end. Address sums wrap modulo 2^16, so
; objects ending at the top of the byte address space work too.
;
; Copying is forward, which also makes memmove with dest < src correct.

	.text
	.globl memcpy
memcpy:
	cwt r3, 0
	jes .done		; n == 0: nothing to do, r1 is already dest
	awt r7, -2
	rw r5, r7		; save r5 at 0+r7
	rw r3, 1+r7		; remember n at 1+r7
	aw r1, r3		; r1 = dest + n
	aw r2, r3		; r2 = src + n
	nga r3			; r3 = -n
.loop:
	lb r5, r2+r3
	rb r5, r1+r3
	irb r3, .loop
	sw r1, [1+r7]		; r1 = dest + n - n
	tw r5, r7
	awt r7, 2
.done:
	uj r4
