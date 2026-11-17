;===-- rec.s - CROOK extracodes that take a REC block (character records) -===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int __crook_inpr(unsigned *rec);   INPR 0212  read a record
; int __crook_prir(unsigned *rec);   PRIR 0213  write a record with its terminator
; int __crook_pinp(unsigned *rec);   PINP 0214  write 2 prompt chars, read a record
; int __crook_prin(unsigned *rec);   PRIN 0215  write a record without terminator
;
; "rec" is a word pointer to the REC block (layout in README.md); CROOK takes
; its address in r4. The result is the C flag of r0 after the call: 1 when the
; stream reached its end (end of file) during a read, otherwise 0. The record
; characters and the record length are returned in the buffer and in rec[0].
;
; Registers on entry:  r1 = rec, r4 = return address
; Registers on exit:   r1 = 1 if C was set after the extracode, else 0
; Preserved:           r5, r6, r7 (see the note in exl.s)
;
; The jump on C must follow the EXL immediately, no flag-changing instruction
; may sit between them.

	.macro REC_CALL name, number
	.section .text.\name,"ax"
	.globl \name
\name:
	awt r7, -1
	rw r4, r7		; return address
	lw r4, r1
	exl \number
	jcs .Lc_\name
	lwt r1, 0
	ujs .Ld_\name
.Lc_\name:
	lwt r1, 1
.Ld_\name:
	tw r4, r7
	awt r7, 1
	uj r4
	.endm

	REC_CALL __crook_inpr, 0212
	REC_CALL __crook_prir, 0213
	REC_CALL __crook_pinp, 0214
	REC_CALL __crook_prin, 0215
