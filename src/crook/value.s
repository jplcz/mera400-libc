;===-- value.s - CROOK extracodes whose argument is one value in r4 -------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; unsigned __crook_core(unsigned words); CORE 0232  resize the process memory
;                                        block to "words" words; 0 only asks.
;                                        Returns the last address (word) of it.
; unsigned __crook_gmem(unsigned map);   GMEM 0270  add 4 K-word blocks: map has
;                                        bit 15 = block 0 ... bit 0 = block 15;
;                                        the new map is old OR map; 0 only
;                                        asks. Returns the current map.
; unsigned __crook_rmem(unsigned map);   RMEM 0271  release the blocks in map.
;                                        Returns the current map.
; void __crook_wait(unsigned ticks);     WAIT 0247  sleep; a tick is 100 ms.
; void __crook_date(unsigned *d);        DATE 0252  stores year, month, day in
;                                        d[0..2].
; void __crook_time(unsigned *t);        TIME 0253  stores hour, minute, second
;                                        in t[0..2].
;
; The argument goes to CROOK in r4 (for DATE and TIME it is the word address of
; the 3-word result). Every function returns the value r4 holds after the call;
; for WAIT, DATE and TIME that value is meaningless.
; (CORE does not work for non-contiguous processes, GMEM/RMEM numbering of
; blocks is as in the process memory map.)
;
; Registers on entry:  r1 = argument, r4 = return address
; Registers on exit:   r1 = r4 after the extracode
; Preserved:           r5, r6, r7 (see the note in exl.s)

	.macro VALUE_CALL name, number
	.section .text.\name,"ax"
	.globl \name
\name:
	awt r7, -1
	rw r4, r7		; return address
	lw r4, r1
	exl \number
	lw r1, r4
	tw r4, r7
	awt r7, 1
	uj r4
	.endm

	VALUE_CALL __crook_core, 0232
	VALUE_CALL __crook_gmem, 0270
	VALUE_CALL __crook_rmem, 0271
	VALUE_CALL __crook_wait, 0247
	VALUE_CALL __crook_date, 0252
	VALUE_CALL __crook_time, 0253
