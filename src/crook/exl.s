;===-- exl.s - generic CROOK extracode call ---------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int __crook_exl(unsigned number, unsigned r4, unsigned *carry);
;
; Executes the extracode "number" (0-255) with r4 as its argument and returns
; the value r4 has after the call. If "carry" is not NULL the C flag of r0
; after the call is stored there (1 or 0): CROOK sets C to report end of file
; after a sequential read. "carry" is a word pointer (an unsigned *).
;
; Registers on entry:  r1 = number, r2 = r4 value, r3 = carry, r4 = return address
; Registers on exit:   r1 = r4 after the extracode
; Preserved:           r5, r6, r7. CROOK does not document which registers an
;                      extracode keeps; its kernel restores the interrupted
;                      process, so only the declared outputs are assumed to
;                      change. r3 and the return address are kept on the
;                      stack anyway.
;
; The EXL instruction holds its number inside the instruction word (opcode
; 0xE500 + number), so the helper builds the instruction at run time in the
; word "slot" and runs it. This is possible because CROOK has one memory
; space for code and data. The slot lives in the .data section, followed by
; a jump back. The instruction after the EXL must be a plain jump (it does
; not change the flags), so that the C flag still reaches ".resume".
;
; The fixed-number wrappers in this directory do not need this trick; use
; those whenever the number is known.

	.section .text.__crook_exl,"ax"
	.globl __crook_exl
__crook_exl:
	awt r7, -1
	rw r4, r7		; keep the return address
	awt r7, -1
	rw r3, r7		; keep the carry pointer
	nr r1, 255
	or r1, 0xE500		; EXL number
	rw r1, .slot
	lw r4, r2		; the extracode argument
	uj .slot
.resume:
	jcs .carry
	lwt r2, 0
	ujs .store
.carry:
	lwt r2, 1
.store:
	tw r3, r7
	awt r7, 1
	cwt r3, 0
	jes .nostore
	rw r2, r3
.nostore:
	lw r1, r4
	tw r4, r7
	awt r7, 1
	uj r4

	.section .data.crook_exl,"awx"
.slot:
	.short 0		; patched: EXL number
	uj .resume
