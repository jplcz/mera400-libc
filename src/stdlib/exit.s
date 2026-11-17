;===-- exit.s - start-up initialisers, atexit and exit --------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void __libc_init_array(void);  called by crt0 before main
; int  atexit(void (*f)(void));  0 on success, -1 when 32 handlers exist
; void exit(int status);         never returns
;
; Function pointers: in the small model they are 16-bit word addresses, so
; an array entry is one word and atexit takes a word address. In the large
; model a function pointer is a 32-bit BYTE address (the value of the ELF
; symbol), so an array entry is two words (high word first), and a call
; through it shifts the address right by one to get the word address "rj"
; needs. The large version is assembled when the symbol MERA400_LARGE is
; defined (llvm-mc --defsym=MERA400_LARGE=1, done by cmake/Mera400.cmake).
;
; Each function and each data object is in its own section (.text.<name>,
; .bss.<name>), so that --gc-sections can drop what a program does not use
; (code size matters on a machine with 64K words).
;
; Why it exists: C++ static objects and functions marked
; __attribute__((constructor)) must run before main, and their destructors
; (and functions registered with atexit) after it. The compiler puts the
; addresses of the functions in the sections .preinit_array, .init_array and
; .fini_array; the linker script gathers each section between a pair of
; symbols (__init_array_start/__init_array_end, ...). The symbols are BYTE
; addresses; "lw r5, symbol" turns them into word addresses. Initialisers run
; from the first entry to the last, finalisers from the last to the first,
; and atexit handlers in reverse order of registration, as the C standard says.
;
; r5 and r6 survive calls (callee saved), so the loops keep their pointers
; there; each routine that returns restores the caller's r5 and r6.

; Calls every function whose address is stored in the array r5 .. r6-1,
; from the first to the last. A private helper: r4 = return address.
	.section .text.__libc_run_array,"ax"
__libc_run_array:
	awt r7, -1
	rw r4, r7
.fwd_loop:
	cl r5, r6
	jes .fwd_done
	jgs .fwd_done
.ifdef MERA400_LARGE
	tw r1, r5		; r1:r2 = entry, a byte address
	tw r2, 1+r5
	awt r5, 2
	srz r1			; to a word address in r2
	sry r2
	rj r4, r2		; call it
.else
	tw r1, r5		; r1 = entry
	awt r5, 1
	rj r4, r1		; call it
.endif
	ujs .fwd_loop
.fwd_done:
	tw r4, r7
	awt r7, 1
	uj r4

	.section .text.__libc_init_array,"ax"
	.globl __libc_init_array
__libc_init_array:
	awt r7, -3
	rw r4, 2+r7
	rw r5, 1+r7
	rw r6, r7
	lw r5, __preinit_array_start
	lw r6, __preinit_array_end
	rj r4, __libc_run_array
	lw r5, __init_array_start
	lw r6, __init_array_end
	rj r4, __libc_run_array
	tw r6, r7
	tw r5, 1+r7
	tw r4, 2+r7
	awt r7, 3
	uj r4

; The table holds word addresses in both models, so exit is the same.
	.section .text.atexit,"ax"
	.globl atexit
atexit:
.ifdef MERA400_LARGE
	srz r1			; r1:r2 byte address -> r2 word address
	sry r2
	lw r1, r2		; the table holds word addresses
.endif
	tw r2, __atexit_count
	cwt r2, 32
	jls .register
	lwt r1, -1		; table full
	uj r4
.register:
	rw r1, __atexit_table+r2
	awt r2, 1
	rw r2, __atexit_count
	lwt r1, 0
	uj r4

	.section .text.exit,"ax"
	.globl exit
exit:
	awt r7, -1
	rw r1, r7		; keep the status on the stack
.atexit_loop:
	tw r5, __atexit_count
	cwt r5, 0
	jes .fini
	awt r5, -1
	rw r5, __atexit_count
	tw r1, __atexit_table+r5
	rj r4, r1
	ujs .atexit_loop
.fini:
	lw r5, __fini_array_end
	lw r6, __fini_array_start
.fini_loop:
	cl r5, r6
	jes .quit
	jls .quit
.ifdef MERA400_LARGE
	awt r5, -2
	tw r1, r5		; r1:r2 = entry, a byte address
	tw r2, 1+r5
	srz r1
	sry r2
	rj r4, r2
.else
	awt r5, -1
	tw r1, r5
	rj r4, r1
.endif
	ujs .fini_loop
.quit:
	tw r1, r7
	awt r7, 1
	uj _Exit		; _Exit(status) is platform specific and never returns

	.section .bss.__atexit_count,"aw",@nobits
__atexit_count:
	.res 1
	.section .bss.__atexit_table,"aw",@nobits
__atexit_table:
	.res 32
