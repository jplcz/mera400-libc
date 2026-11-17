/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */
/* Minimal console I/O on CROOK. Named crook_stdio.h so it does not shadow
 * a full stdio.h. Stream ids are unknown (open question, see README), so
 * call __crook_set_streams first; until then every function returns EOF. */
#ifndef CROOK_STDIO_H
#define CROOK_STDIO_H

#define EOF (-1)

int putchar(int c);
int puts(const char *s);
int getchar(void);

/* in/out: CROOK stream ids (word 1 of the REC block). */
void __crook_set_streams(unsigned in, unsigned out);

#endif
