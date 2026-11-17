;===-- ashlhi3.s - 16-bit shift left by a variable amount -----------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; uint16_t __ashlhi3(uint16_t x, uint16_t n);       (x << n)
;
; The compiler calls this for "shl i16 %x, %n" when n is not a constant (the
; CPU shifts only by one bit per instruction).
;
; Registers on entry:  r1 = x, r2 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r3, r5, r6, r7 (r3 is untouched too)
;
; The C language leaves n >= 16 undefined; here it gives 0, the value that
; shifting one bit at a time would produce.
;
; Algorithm: one "slz" (shift left, insert zero) per count, n times; drb
; (decrement and branch if not zero) is the loop counter.

	.text
	.globl __ashlhi3
__ashlhi3:
	cl r2, 16
	jls .shift		; n < 16 (unsigned)
	lwt r1, 0
	uj r4
.shift:
	cwt r2, 0
	jes .done
.loop:
	slz r1
	drb r2, .loop
.done:
	uj r4
