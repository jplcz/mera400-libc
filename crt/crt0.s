;===-- crt0.s - C runtime start-up code ------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; The first code that runs. It
;   1. points the stack pointer r7 at __stack_top (a linker symbol; the stack
;      grows downward from it),
;   2. clears the .bss section (__bss_start .. __bss_end, supplied by the
;      linker script; a flat binary does not contain .bss),
;   3. calls main(0, 0) (argc = 0, argv = 0),
;   4. halts with r1 = main's return value ("exit status") when main returns.
;
; "lw rX, symbol" loads the word address of a symbol: the linker converts
; the symbol's byte address into words, so r7 and the .bss bounds are word
; addresses, which is what rw and r7-relative addressing use.
;
; exit and abort are here because they are only a halt on this machine.
; hlt 077 is the conventional "finished" stop code of MERA-400 test
; programs and hlt 040 is used for abort.

; Everything here lives in its own section, .text.crt0. The linker script
; places that section first in the image, so _start is the very first word
; (where the CPU starts executing) no matter in which order the other
; objects are given to the linker.

	.section .text.crt0,"ax"
	.globl _start
_start:
	lw r7, __stack_top
	lw r1, __bss_start
	lw r2, __bss_end
	lwt r3, 0
.clear:
	cl r1, r2
	jes .cleared
	rw r3, r1
	awt r1, 1
	ujs .clear
.cleared:
	lwt r1, 0		; argc
	lwt r2, 0		; argv
	rj r4, main
	; main's result stays in r1 and is the exit status

	.globl exit
exit:
	hlt 077
	ujs exit		; resuming the CPU must not run into other code

	.globl abort
abort:
	hlt 040
	ujs abort
