;===-- fil.s - CROOK extracodes that take a FIL block (stream assignment) --===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; unsigned __crook_asg(unsigned *fil);    ASG  0200  assign a stream to an
;                                                    existing file ("open")
; unsigned __crook_casg(unsigned *fil);   CASG 0201  create a file and assign
;                                                    a stream to it
; unsigned __crook_nasg(unsigned *fil);   NASG 0205  assign a stream, hiding a
;                                                    stream with the same id
;
; unsigned __crook_setp(unsigned *fil);  SETP 0202  set the parameters (type,
;                                                    length, par1, par2,
;                                                    permissions, attributes)
;                                                    of the file in "fil"
; unsigned __crook_loap(unsigned *fil);  LOAP 0203  load those parameters of
;                                                    the file into "fil"
; unsigned __crook_cprf(unsigned *fil);  CPRF 0233  create the special file a
;                                                    process is loaded from
;                                                    (own memory block, used
;                                                    with JUMP)
; void __crook_eras(void);               ERAS 0210  remove all local streams
; void __crook_reld(void);               RELD 0251  release the character
;                                                    devices of the process
;
; The last two take no argument and return nothing (R4 is cleared before the
; call); the others take a FIL block as below. Deleting (ERF 0206), closing
; (ERS 0207), moving to the start or the end (FBOF 0211, FEOF 0217), the end
; mark (EOF 0216), the stream semaphores (TAKS 0266, RELS 0267), the disk
; label (TLAB 0260) and the directory calls (SDIR/TDIR/CDIR 0235-0237) have no
; wrapper: their argument is only named "STR", "MET" or "DIR" in the manuals,
; without a layout. Use __crook_exl (see README.md).
;
; "fil" is a word pointer to the 12-word FIL block (stream and file
; description, layout in README.md). CROOK takes the block address in r4.
; The functions return fil[0], the error word: if it is non-zero on entry,
; CROOK treats an error as an alarm (process stopped, parent told); if it is
; zero, the error number is stored there and the program continues.
;
; Registers on entry:  r1 = fil, r4 = return address
; Registers on exit:   r1 = fil[0]
; Preserved:           r5, r6, r7 (see the note in exl.s)

	.macro FIL_CALL name, number
	.section .text.\name,"ax"
	.globl \name
\name:
	awt r7, -1
	rw r4, r7		; return address
	awt r7, -1
	rw r1, r7		; block address
	lw r4, r1
	exl \number
	tw r1, r7
	awt r7, 1
	tw r1, r1		; error word
	tw r4, r7
	awt r7, 1
	uj r4
	.endm

	.macro NOARG_CALL name, number
	.section .text.\name,"ax"
	.globl \name
\name:
	awt r7, -1
	rw r4, r7		; return address
	lwt r4, 0
	exl \number
	tw r4, r7
	awt r7, 1
	uj r4
	.endm

	FIL_CALL __crook_asg, 0200
	FIL_CALL __crook_casg, 0201
	FIL_CALL __crook_setp, 0202
	FIL_CALL __crook_loap, 0203
	FIL_CALL __crook_nasg, 0205
	FIL_CALL __crook_cprf, 0233
	NOARG_CALL __crook_eras, 0210
	NOARG_CALL __crook_reld, 0251
