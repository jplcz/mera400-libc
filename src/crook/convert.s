;===-- convert.s - C strings <-> CROOK text ---------------------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; size_t __crook_from_c(char *dst, const char *src, size_t max);
; size_t __crook_to_c(char *dst, const char *src, size_t max);
;
; CROOK text is stored two characters per 16-bit word, the first character in
; the high (left) byte; a line (record) ends with the character CR (0x0D).
; A C string is a sequence of bytes ending with NUL, and a byte pointer is a
; byte address: byte 2*w is the high half of word w, byte 2*w+1 the low half.
; The two layouts are therefore the same bytes in memory. Only the line
; terminator needs translating (C: '\n' = 0x0A, CROOK: CR = 0x0D). A buffer
; handed to CROOK must start at an even byte address (CROOK is given a word
; address).
;
; __crook_from_c: copies the first LINE of the C string src to dst and ends it
;   with CR. The line ends at the first '\n', '\r' or NUL of src (not copied)
;   or after "max" characters. Returns n, the number of characters copied
;   (the CR is at dst[n]); src[n] tells why it stopped: NUL = end of string,
;   '\n' or '\r' = line end (the caller skips it), anything else = "max"
;   reached. dst needs room for max + 1 bytes.
; __crook_to_c: copies CROOK text from src to dst up to and including the
;   first CR, which becomes '\n', or until "max" characters were copied (then
;   no '\n' is added), and ends dst with NUL. Returns the length without the
;   NUL. dst needs room for max + 2 bytes. dst == src (in place) is allowed.
;
; Registers on entry:  r1 = dst, r2 = src, r3 = max, r4 = return address
; Registers on exit:   r1 = length
; Preserved:           r5, r6, r7 (r5 and r6 are saved on the stack)
;
; r5 is the character (cleared first: lb only replaces the low byte), r6 the
; limit, r3 the index.

	.section .text.__crook_from_c,"ax"
	.globl __crook_from_c
__crook_from_c:
	awt r7, -1
	rw r5, r7
	awt r7, -1
	rw r6, r7
	lw r6, r3		; limit
	lwt r3, 0
	lwt r5, 0
.fc_loop:
	cw r3, r6
	jes .fc_done
	lb r5, r2+r3
	cwt r5, 0
	jes .fc_done
	cwt r5, 10
	jes .fc_done
	cwt r5, 13
	jes .fc_done
	rb r5, r1+r3
	awt r3, 1
	ujs .fc_loop
.fc_done:
	lwt r5, 13
	rb r5, r1+r3		; CR terminator
	lw r1, r3
	tw r6, r7
	awt r7, 1
	tw r5, r7
	awt r7, 1
	uj r4

	.section .text.__crook_to_c,"ax"
	.globl __crook_to_c
__crook_to_c:
	awt r7, -1
	rw r5, r7
	awt r7, -1
	rw r6, r7
	lw r6, r3		; limit
	lwt r3, 0
	lwt r5, 0
.tc_loop:
	cw r3, r6
	jes .tc_done
	lb r5, r2+r3
	cwt r5, 13
	jes .tc_newline
	rb r5, r1+r3
	awt r3, 1
	ujs .tc_loop
.tc_newline:
	lwt r5, 10
	rb r5, r1+r3
	awt r3, 1
.tc_done:
	lwt r5, 0
	rb r5, r1+r3		; NUL terminator
	lw r1, r3
	tw r6, r7
	awt r7, 1
	tw r5, r7
	awt r7, 1
	uj r4
