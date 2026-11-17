;===-- umodhi3.s - unsigned 16-bit remainder ------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; uint16_t __umodhi3(uint16_t a, uint16_t b);       (a % b)
;
; Registers on entry:  r1 = a, r2 = b, r4 = return address
; Registers on exit:   r1 = remainder
; Preserved:           r5, r6, r7
;
; Calls __udivmodhi (which leaves the remainder in r2), so the return
; address in r4 is saved on the stack around the call.

	.section .text.__umodhi3,"ax"
	.globl __umodhi3
__umodhi3:
	awt r7, -1
	rw r4, r7
	rj r4, __udivmodhi
	tw r4, r7
	awt r7, 1
	lw r1, r2
	uj r4
