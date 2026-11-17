;===-- crt0-crook.s - C runtime start-up code for CROOK -------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; Start-up code for programs that run under the CROOK operating system. It
;   1. points the stack pointer r7 at __stack_top (a linker symbol, see
;      crt/crook.ld): CROOK gives user programs no stack, so the program
;      carries its own inside its image;
;   2. clears .bss (__bss_start .. __bss_end) with word stores;
;   3. runs the initialisers (__libc_init_array, src/stdlib/exit.s);
;   4. calls main(0, 0) (argc = 0, argv = 0). The values CROOK passes in
;      r1, r2, ... at start (the program parameters) are ignored;
;   5. calls exit(status) with main's return value. exit (src/stdlib/exit.s)
;      runs the atexit handlers and static destructors and calls _Exit
;      (src/crook/exit.s), which ends the program with a CROOK extracode.
;
; As in the bare-metal crt0, "lw rX, symbol" loads the WORD address of the
; symbol (the linker divides the byte address by two).
;
; This code is assumed to be the first word of the image (crook.ld puts
; section .text.crt0 first). Which address CROOK starts a program at is an
; open question, see README.md.

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
	rj r4, __libc_init_array
	lwt r1, 0		; argc
	lwt r2, 0		; argv
	rj r4, main
	rj r4, exit		; r1 = main's result = exit status
.hang:
	ujs .hang		; exit does not return
