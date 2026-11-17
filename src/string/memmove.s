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
; The backward loop addresses byte r2+r3 and r1+r3 with r3 counting down
; from n to 1 (drb = decrement and branch if not zero). Both pointers are
; first lowered by one, so the first access is at offset n-1 and the last at
; offset 0; r1 is raised again before returning. The pointer comparison is
; unsigned (cl), because byte addresses use the whole 16-bit range.

	.section .text.memmove,"ax"
	.globl memmove
memmove:
	cl r1, r2
	jl memcpy		; dest < src: copy forward
	je .done		; dest == src: nothing to do
	cwt r3, 0
	jes .done		; n == 0 (the drb loop needs n >= 1)
	awt r7, -1
	rw r5, r7
	awt r1, -1		; bias so that offset n maps to byte n-1
	awt r2, -1
.loop:
	lb r5, r2+r3
	rb r5, r1+r3
	drb r3, .loop
	awt r1, 1
	tw r5, r7
	awt r7, 1
.done:
	uj r4
