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
; The extracode argument is passed in r4. The numbers are verified; what the
; parent does with the value in r4 is NOT (open question in README.md), so
; _Exit puts the status there and nothing more is promised.
;
; None of these returns. The loop after each extracode only makes sure that
; the program cannot run on into unrelated code should it be resumed.

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
