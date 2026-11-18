;===-- crook_main.s - program for the CROOK simulator test ------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int main(void): exercises the CROOK platform layer. The expected sequence of
; extracodes is listed in tests/crook/test_crook.py, which plays the part of
; the operating system. The stream ids 3 (input) and 4 (output) are made up for
; the test. main returns 7, which crt0 passes to exit (extracode END).

	.text
	.globl main
main:
	awt r7, -1
	rw r4, r7
	lwt r1, 3
	lwt r2, 4
	rj r4, __crook_set_streams

	lw r1, msg
	aw r1, r1		; byte address of the string
	rj r4, puts		; two records: "hello", "world"
	lw r1, long_msg
	aw r1, r1
	rj r4, puts		; 254 characters without terminator, then 46 with
	lw r1, 65
	rj r4, putchar		; 'A'
	lwt r1, 10
	rj r4, putchar		; an empty record with terminator

.echo:				; copy the input to the output, character by character
	rj r4, getchar
	cwt r1, -1
	je .echo_done
	rj r4, putchar
	uj .echo
.echo_done:

	lw r1, 0247
	lw r2, 5
	lw r3, res_carry
	rj r4, __crook_exl
	rw r1, res_exl
	lw r1, 0247
	lw r2, 5
	lwt r3, 0
	rj r4, __crook_exl
	rw r1, res_exl_nocarry

	lw r1, date_buf
	rj r4, __crook_date
	lw r1, fil_block
	rj r4, __crook_asg
	rw r1, res_asg
	lwt r1, 0
	rj r4, __crook_core
	rw r1, res_core
	lw r1, rec_block
	rj r4, __crook_inpr
	rw r1, res_inpr

	.macro CALL_FIL fn, idx
	lw r1, fil_block
	rj r4, \fn
	rw r1, res_new+\idx
	.endm
	.macro CALL_BLK fn, idx
	lw r1, blk_block
	rj r4, \fn
	rw r1, res_new+\idx
	.endm

	CALL_FIL __crook_setp, 0
	CALL_FIL __crook_loap, 1
	CALL_FIL __crook_cprf, 2
	rj r4, __crook_eras
	rj r4, __crook_reld
	CALL_BLK __crook_tmem, 3
	CALL_BLK __crook_ovl, 4
	CALL_BLK __crook_reap, 5
	CALL_BLK __crook_wrip, 6
	CALL_BLK __crook_wadr, 7
	CALL_BLK __crook_read, 8
	CALL_BLK __crook_writ, 9
	lw r1, 3
	rj r4, __crook_stop
	rw r1, res_new+10
	rj r4, __crook_back

	tw r2, probe		; which one-off call to make, 0 = none
	cwt r2, 0
	je .probe_done
	cwt r2, 1
	je .probe_gmem
	cwt r2, 2
	je .probe_rmem
	cwt r2, 3
	je .probe_lram
	cwt r2, 4
	je .probe_abo
	cwt r2, 5
	je .probe_kill
	lwt r1, 8
	rj r4, __crook_exit_finish
.probe_gmem:
	lw r1, 0x0F00
	rj r4, __crook_gmem
	rw r1, res_new+11
	uj .probe_done
.probe_rmem:
	lw r1, 0x0F00
	rj r4, __crook_rmem
	rw r1, res_new+11
	uj .probe_done
.probe_lram:
	lw r1, blk_block
	rj r4, __crook_lram
	rw r1, res_new+11
	uj .probe_done
.probe_abo:
	lwt r1, 5
	rj r4, __crook_abo
.probe_kill:
	lwt r1, 6
	rj r4, __crook_kill
.probe_done:

	lwt r1, 7
	tw r4, r7
	awt r7, 1
	uj r4

	.data
msg:
	.asciiz "hello\nworld"
	.globl long_msg
long_msg:
	.rept 150
	.ascii "xx"
	.endr
	.asciiz "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"

	.globl probe
probe:
	.short 0

	.bss
	.balign 2
	.globl res_carry
res_carry:
	.space 2
	.globl res_exl
res_exl:
	.space 2
	.globl res_exl_nocarry
res_exl_nocarry:
	.space 2
	.globl res_asg
res_asg:
	.space 2
	.globl res_core
res_core:
	.space 2
	.globl res_inpr
res_inpr:
	.space 2
	.globl date_buf
date_buf:
	.space 6
	.globl fil_block
fil_block:
	.space 24
	.globl rec_block
rec_block:
	.space 10
	.globl res_new
res_new:
	.space 24
	.globl blk_block
blk_block:
	.space 10
