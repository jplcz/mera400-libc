;===-- ashrhi3.s - 16-bit arithmetic shift right by a variable amount -----===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int16_t __ashrhi3(int16_t x, uint16_t n);         (x >> n, sign filled)
;
; Registers on entry:  r1 = x, r2 = n, r4 = return address
; Registers on exit:   r1 = result
; Preserved:           r3, r5, r6, r7
;
; n >= 16 behaves like n = 16: the result is 0 or -1 (undefined in C).
;
; Algorithm: there is no arithmetic shift instruction, but "srx" shifts right
; inserting the X flag, and "sxu" sets X from the sign bit. X is not changed
; by srx, so n times srx replicates the sign. n is clamped to 16 first.

	.text
	.globl __ashrhi3
__ashrhi3:
	cl r2, 16
	jls .ok
	lwt r2, 16		; clamp: 16 shifts already give all sign bits
.ok:
	cwt r2, 0
	jes .done
	sxu r1			; X = sign bit of x
.loop:
	srx r1
	drb r2, .loop
.done:
	uj r4
