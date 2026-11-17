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
