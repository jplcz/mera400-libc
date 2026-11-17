;===-- stdio.s - minimal stdio on CROOK record extracodes -------------------===;
;
; Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
; Exceptions. See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;
;===----------------------------------------------------------------------===;
;
; int putchar(int c);
; int puts(const char *s);
; int getchar(void);
; void __crook_set_streams(int in, int out);
;
; Built on the record extracodes PRIR/PRIN (write) and INPR (read), see
; rec.s, and on the conversions in convert.s.
;
; Streams: CROOK addresses files by a "stream id" chosen by the program. The
; ids of the standard streams of a program under XOSL (INT = input, OUT =
; output) are NOT known (open question in README.md), so the library does not
; guess: both ids start as -1 ("not set"), putchar/puts/getchar then return
; EOF (-1) without calling CROOK, and the program must call
; __crook_set_streams(in, out) first with ids it knows (for example ones it
; assigned itself with __crook_asg).
;
; Line ends: '\n' and '\r' written to the output end the current record
; (CROOK's line terminator is CR). Input records are delivered with their
; CR translated to '\n'. A NUL byte cannot be read (it marks the end of the
; buffered line). Lines longer than 255 characters are read in pieces.
;
; Output: putchar sends one record (the character, without terminator; for a
; newline an empty record with terminator). puts sends the string line by
; line; a piece is sent without terminator when it is cut at 254 characters,
; and with terminator at '\n' and at the end of the string (which is puts's
; own newline). What exactly PRIN and PRIR write for the record is the part
; of the REC block description in README.md that is not fully verified.
;
; Input: getchar reads a record with INPR into a buffer that was filled with
; CR first, so the line is delimited correctly whether or not INPR stores its
; terminator; then it translates the line in place.
;
; Registers: standard (r1-r3 arguments/scratch, r4 return address, r5/r6/r7
; preserved).

	.section .text.send_record,"ax"

; send_record: write the record in out_buf (text followed by CR).
;   r1 = 0: PRIN (without terminator), r1 != 0: PRIR (with terminator).
;   Returns r1 = 0, or -1 when the output stream is not set.
send_record:
	awt r7, -1
	rw r4, r7
	tw r2, __crook_stdout_stream
	cwt r2, -1
	jes .send_unset
	lw r3, out_rec
	lwt r4, 0
	rw r4, r3		; rec[0] = 0: start at character 0, no flags
	awt r3, 1
	rw r2, r3		; rec[1] = stream id
	awt r3, 1
	lw r4, out_buf
	rw r4, r3		; rec[2] = buffer (word address)
	awt r3, 1
	lw r4, 0x0DFF		; CR terminator, at most 255 characters
	rw r4, r3		; rec[3]
	cwt r1, 0
	jes .send_prin
	lw r1, out_rec
	rj r4, __crook_prir
	ujs .send_ok
.send_prin:
	lw r1, out_rec
	rj r4, __crook_prin
.send_ok:
	lwt r1, 0
	ujs .send_ret
.send_unset:
	lwt r1, -1
.send_ret:
	tw r4, r7
	awt r7, 1
	uj r4

	.section .text.putchar,"ax"
	.globl putchar
putchar:
	awt r7, -1
	rw r4, r7
	awt r7, -1
	rw r5, r7
	nr r1, 255
	lw r5, r1		; the character, returned on success
	lw r2, out_buf
	aw r2, r2		; byte address of the buffer
	lwt r3, 13
	cwt r1, 10
	jes .pc_newline
	cwt r1, 13
	jes .pc_newline
	rb r1, r2		; buf[0] = c
	awt r2, 1
	rb r3, r2		; buf[1] = CR
	lwt r1, 0		; PRIN
	rj r4, send_record
	ujs .pc_done
.pc_newline:
	rb r3, r2		; buf[0] = CR: an empty record
	lwt r1, 1		; PRIR
	rj r4, send_record
.pc_done:
	cwt r1, 0
	jes .pc_ok
	lwt r1, -1
	ujs .pc_ret
.pc_ok:
	lw r1, r5
.pc_ret:
	tw r5, r7
	awt r7, 1
	tw r4, r7
	awt r7, 1
	uj r4

	.section .text.puts,"ax"
	.globl puts
puts:
	awt r7, -1
	rw r4, r7
	awt r7, -1
	rw r5, r7
	lw r5, r1		; s
.ps_line:
	lw r1, out_buf
	aw r1, r1
	lw r2, r5
	lw r3, 254
	rj r4, __crook_from_c	; r1 = n copied
	lwt r2, 0
	lb r2, r5+r1		; why it stopped
	aw r5, r1
	cwt r2, 0
	jes .ps_end
	cwt r2, 10
	jes .ps_nl
	cwt r2, 13
	jes .ps_nl
	lwt r1, 0		; cut at the limit: no terminator, same string goes on
	rj r4, send_record
	cwt r1, 0
	jes .ps_line
	ujs .ps_ret
.ps_nl:
	awt r5, 1		; skip the newline
	lwt r1, 1
	rj r4, send_record
	cwt r1, 0
	jes .ps_line
	ujs .ps_ret
.ps_end:
	lwt r1, 1		; puts's own newline
	rj r4, send_record
.ps_ret:
	tw r5, r7
	awt r7, 1
	tw r4, r7
	awt r7, 1
	uj r4

; fill_line: read the next record into in_buf as a C string ('\n' ended).
;   Returns r1 = 0, or -1 on end of file or when the input stream is not set.
	.section .text.fill_line,"ax"
fill_line:
	awt r7, -1
	rw r4, r7
	tw r2, __crook_stdin_stream
	cwt r2, -1
	jes .fill_fail
	lw r3, in_rec
	lwt r1, 0
	rw r1, r3		; rec[0]
	awt r3, 1
	rw r2, r3		; rec[1] = stream id
	awt r3, 1
	lw r1, in_buf
	rw r1, r3		; rec[2] = buffer
	awt r3, 1
	lw r1, 0x0DFF
	rw r1, r3		; rec[3] = CR terminator, at most 255 characters
	lw r3, in_buf		; fill the buffer with CR
	lw r2, -130
	lw r1, 0x0D0D
.fill_loop:
	rw r1, r3
	awt r3, 1
	irb r2, .fill_loop
	lw r1, in_rec
	rj r4, __crook_inpr
	cwt r1, 0
	jn .fill_fail		; C set: end of file
	lw r1, in_buf
	aw r1, r1
	lw r2, r1
	lw r3, 255
	rj r4, __crook_to_c
	lwt r1, 0
	ujs .fill_ret
.fill_fail:
	lwt r1, -1
.fill_ret:
	tw r4, r7
	awt r7, 1
	uj r4

	.section .text.getchar,"ax"
	.globl getchar
getchar:
	awt r7, -1
	rw r4, r7
	awt r7, -1
	rw r5, r7
	tw r5, in_pos		; byte address of the next character, 0 = none
	cwt r5, 0
	jes .gc_refill
.gc_have:
	lwt r1, 0
	lb r1, r5
	cwt r1, 0
	jes .gc_refill		; end of the buffered line
	awt r5, 1
	cwt r1, 10
	jn .gc_store
	lwt r5, 0		; a line ends with '\n': read the next record next time
.gc_store:
	rw r5, in_pos
	ujs .gc_ret
.gc_refill:
	rj r4, fill_line
	cwt r1, 0
	jes .gc_filled
	lwt r5, 0
	rw r5, in_pos
	lwt r1, -1		; EOF
	ujs .gc_ret
.gc_filled:
	lw r5, in_buf
	aw r5, r5
	ujs .gc_have
.gc_ret:
	tw r5, r7
	awt r7, 1
	tw r4, r7
	awt r7, 1
	uj r4

	.section .text.__crook_set_streams,"ax"
	.globl __crook_set_streams
__crook_set_streams:
	rw r1, __crook_stdin_stream
	rw r2, __crook_stdout_stream
	lwt r3, 0
	rw r3, in_pos
	uj r4

	.section .data.__crook_stdin_stream,"aw"
	.globl __crook_stdin_stream
__crook_stdin_stream:
	.short 0xFFFF		; not set
	.section .data.__crook_stdout_stream,"aw"
	.globl __crook_stdout_stream
__crook_stdout_stream:
	.short 0xFFFF		; not set
	.section .data.in_pos,"aw"
in_pos:
	.short 0

	.section .bss.out_rec,"aw",@nobits
	.balign 2
out_rec:
	.space 10		; REC block: 5 words
	.section .bss.in_rec,"aw",@nobits
	.balign 2
in_rec:
	.space 10
	.section .bss.out_buf,"aw",@nobits
	.balign 2
out_buf:
	.space 260		; 130 words: up to 255 characters and the terminator
	.section .bss.in_buf,"aw",@nobits
	.balign 2
in_buf:
	.space 260
