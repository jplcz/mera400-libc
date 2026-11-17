;===-- udivhi3.s - unsigned 16-bit division -------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; uint16_t __udivhi3(uint16_t a, uint16_t b);       (a / b)
;
; Registers on entry:  r1 = a, r2 = b, r4 = return address
; Registers on exit:   r1 = quotient
;
; A tail jump to __udivmodhi, which returns the quotient in r1 (and the
; remainder in r2, ignored here) straight to our caller through r4.

	.text
	.globl __udivhi3
__udivhi3:
	uj __udivmodhi
