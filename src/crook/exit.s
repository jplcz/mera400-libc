;===-- exit.s - program end on CROOK ----------------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void _Exit(int status);
; void _exit(int status);               same as _Exit
; void __crook_exit_finish(int status); like _Exit, but the system also prints
;                                       "FINISH" (extracode EOSL)
; void __crook_abo(int status);         ABO 0375: delete the program (silently)
; void __crook_kill(int status);        KILL 0376: delete the program, the
;                                       system prints "DELETED"
; void __crook_back(void);              BACK 0374: XOSL continues and this
;                                       program goes on in the background
; void abort(void);
;
; exit() itself is in the portable part of the library (src/stdlib/exit.s):
; it runs the atexit handlers and the static destructors and then calls
; _Exit, which is the platform specific end of the program.
;
; CROOK has no ordinary "exit" extracode. A program under the XOSL command
; shell ends itself with one of the "special" extracodes 0340-0377, which the
; operating system turns into an alarm for the parent process (XOSL):
;   END  = 0373 (decimal 251)  end of program
;   EOSL = 0377 (decimal 255)  end of program, prints "FINISH"
;   ABO  = 0375 (decimal 253)  delete the program (used by abort)
;   KILL = 0376 (decimal 254)  delete the program, prints "DELETED"
;   BACK = 0374 (decimal 252)  not an end: the program is continued in the
;                              background, so __crook_back returns (the manuals
;                              give BACK no argument; R4 is cleared)
; The extracode argument is passed in r4. The numbers are verified; what the
; parent does with the value in r4 is NOT (open question in README.md), so
; _Exit puts the status there and nothing more is promised.
;
; None of these returns, except BACK. The loop after each extracode only
; makes sure that the program cannot run on into unrelated code should it be
; resumed.

; Each function is in its own section, so that --gc-sections can drop the ones
; a program does not use.

	.section .text._Exit,"ax"
	.globl _Exit
	.globl _exit
_Exit:
_exit:
	lw r4, r1		; r4 = status
	exl 251			; END
.end:
	ujs .end

	.section .text.__crook_exit_finish,"ax"
	.globl __crook_exit_finish
__crook_exit_finish:
	lw r4, r1
	exl 255			; EOSL
.finish:
	ujs .finish

	.section .text.abort,"ax"
	.globl abort
abort:
	lwt r4, 0
	exl 253			; ABO
.abort:
	ujs .abort

	.section .text.__crook_abo,"ax"
	.globl __crook_abo
__crook_abo:
	lw r4, r1
	exl 253			; ABO
.abo:
	ujs .abo

	.section .text.__crook_kill,"ax"
	.globl __crook_kill
__crook_kill:
	lw r4, r1
	exl 254			; KILL
.kill:
	ujs .kill

	.section .text.__crook_back,"ax"
	.globl __crook_back
__crook_back:
	awt r7, -1
	rw r4, r7		; return address
	lwt r4, 0
	exl 252			; BACK
	tw r4, r7
	awt r7, 1
	uj r4
