;===-- abi_cpp_asm.s - assembly side of the C++ ABI test ------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; Assembly that works with C++ objects (see abi_cpp.cpp and docs/abi.md).
; The object (this) is a C data pointer: a byte address, r1 (r1:r2 in the
; large model).
; struct Counter { int n; virtual int add(int); virtual int sub(int);
;                  int plain(int); };

; int asm_call_plain(Counter *c, int x)     c->plain(x)
; Same arguments as the callee, so just jump to it; it returns to our caller.
; The name is the Itanium mangled name.
	.section .text.asm_call_plain,"ax"
	.globl asm_call_plain
asm_call_plain:
	uj _ZN7Counter5plainEi

; int asm_call_virtual0(Counter *c, int x)     c->add(x), a virtual call
; The first word of the object is the vptr (virtual table pointer), which
; points at slot 0 of the table. Small model: vptr and slots are word
; addresses. Large model: both are 32 bit byte addresses.
	.section .text.asm_call_virtual0,"ax"
	.globl asm_call_virtual0
asm_call_virtual0:
	awt r7, -3
	rw r4, 2+r7
	rw r5, 1+r7
	rw r6, r7
.ifdef MERA400_LARGE
	lw r5, r1
	lw r6, r2
	srz r5
	sry r6			; r6 = word address of the object
	tw r5, r6		; vptr, high word
	tw r6, 1+r6		; vptr, low word
	srz r5
	sry r6			; r6 = word address of slot 0
	tw r5, r6		; function pointer, high word
	tw r6, 1+r6		; low word
	srz r5
	sry r6			; r6 = word address of the code
.else
	lw r6, r1
	srz r6			; word address of the object
	tw r6, r6		; vptr
	tw r6, r6		; slot 0: the function's word address
.endif
	rj r4, r6		; this and x are still in their registers
	tw r6, r7
	tw r5, 1+r7
	tw r4, 2+r7
	awt r7, 3
	uj r4

; unsigned asm_vtable_header(Counter *c)
; Before slot 0 the table has two entries: offset to top and RTTI pointer,
; both zero. An entry is one word (small) or two words (large).
; Returns the OR of those words.
	.section .text.asm_vtable_header,"ax"
	.globl asm_vtable_header
asm_vtable_header:
.ifdef MERA400_LARGE
	srz r1
	sry r2			; r2 = word address of the object
	tw r1, r2
	tw r2, 1+r2		; r1:r2 = vptr, a byte address
	srz r1
	sry r2			; r2 = word address of slot 0
	awt r2, -4
	tw r1, r2
	tw r3, 1+r2
	or r1, r3
	tw r3, 2+r2
	or r1, r3
	tw r3, 3+r2
	or r1, r3
.else
	srz r1
	tw r1, r1		; vptr = word address of slot 0
	awt r1, -2
	tw r2, r1
	tw r3, 1+r1
	lw r1, r2
	or r1, r3
.endif
	uj r4

; int asm_seed(void): called by the constructor of a global C++ object, run
; from .init_array before main.
	.section .text.asm_seed,"ax"
	.globl asm_seed
asm_seed:
	lw r1, 0x5EED
	uj r4
