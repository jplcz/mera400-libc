;===-- block.s - CROOK extracodes that take a BLOCK (sector I/O, memory) --===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; unsigned __crook_read(unsigned *blk);   READ 0226  read sectors (random
;                                                    access) / receive message
; unsigned __crook_writ(unsigned *blk);   WRIT 0227  write sectors
; unsigned __crook_tmem(unsigned *blk);   TMEM 0204  attach a 4 K-word block
;                                                    of a RAM file (resident
;                                                    processes only)
; unsigned __crook_ovl(unsigned *blk);   OVL  0223  load an overlay from the
;                                                    process' own file
; unsigned __crook_reap(unsigned *blk);  REAP 0224  read the special (swap)
;                                                    file of a process
; unsigned __crook_wrip(unsigned *blk);  WRIP 0225  write the special file of
;                                                    a process
; unsigned __crook_wadr(unsigned *blk);  WADR 0222  write address fields
;                                                    (privileged programs only)
;
; The four new calls take the same BLOCK as READ and WRIT. For REAP and WRIP
; word 1 is the process id instead of the stream id, and for OVL word 1 is
; ignored. The sector in word 4 counts from the start of the file; what
; selects the module for OVL (a 4-character module name for relocatable
; modules) is not documented, so it is not described here.
;
; "blk" is a word pointer to the parameter block. For READ and WRIT the
; verified layout is: word 0 = count of words transferred (set by CROOK),
; 1 = stream id, 2 = memory (word) address, 3 = word count, 4 = sector number
; relative to the start of the file. For TMEM only words 0 (error), 1 (stream),
; 2 (address, multiple of 4 K words, 0 = release) and 4 (block number in the
; file) are documented; word 3 is unknown (see README.md).
; The functions return blk[0] unchanged in meaning: whatever CROOK left there.
;
; Registers on entry:  r1 = blk, r4 = return address
; Registers on exit:   r1 = blk[0]
; Preserved:           r5, r6, r7 (see the note in exl.s)

	.macro BLOCK_CALL name, number
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
	tw r1, r1		; first word of the block
	tw r4, r7
	awt r7, 1
	uj r4
	.endm

	BLOCK_CALL __crook_read, 0226
	BLOCK_CALL __crook_writ, 0227
	BLOCK_CALL __crook_tmem, 0204
	BLOCK_CALL __crook_wadr, 0222
	BLOCK_CALL __crook_ovl, 0223
	BLOCK_CALL __crook_reap, 0224
	BLOCK_CALL __crook_wrip, 0225
