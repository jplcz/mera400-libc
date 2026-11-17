;===-- lshrhi3.s - 16-bit logical shift right by a variable amount --------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; uint16_t __lshrhi3(uint16_t x, uint16_t n);       (x >> n, zero filled)
;
; Registers on entry:  r1 = x, r2 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r3, r5, r6, r7
;
; n >= 16 gives 0 (undefined in C).
;
; Algorithm: n times "srz" (shift right, insert zero), counted by drb.

	.section .text.__lshrhi3,"ax"
	.globl __lshrhi3
__lshrhi3:
	cl r2, 16
	jls .shift		; n < 16 (unsigned)
	lwt r1, 0
	uj r4
.shift:
	cwt r2, 0
	jes .done
.loop:
	srz r1
	drb r2, .loop
.done:
	uj r4
