;===-- memset.s - fill bytes with a value ---------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; void *memset(void *dest, int c, size_t n);
;
; Registers on entry:  r1 = dest, r2 = c (only the low octet counts),
;                      r3 = n, r4 = return address
; Registers on exit:   r1 = dest
; Preserved:           r5, r6, r7 (none is touched, so nothing is saved)
;
; Algorithm: dest is lowered by one and r3 counts down from n to 1 (drb), so
; rb stores the low octet of r2 at dest-1+r3, i.e. dest+n-1 first and dest
; last. dest is raised again before returning. The loop needs no scratch
; register and no masking of c, because rb ignores the high octet of r2.

	.text
	.globl memset
memset:
	cwt r3, 0
	jes .done		; n == 0
	awt r1, -1
.loop:
	rb r2, r1+r3
	drb r3, .loop
	awt r1, 1
.done:
	uj r4
