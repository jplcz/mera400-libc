/*===-- stdio.h - formatted output and minimal streams ---------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Only stdout and stderr exist (both go to the same console) and stdin
 * reads from the platform's input hook. All output ends in one function,
 * int __stdout_putc(int c), which a platform provides; see src/stdio/README.md.
 */
#ifndef MERA400_STDIO_H
#define MERA400_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)

typedef struct __FILE FILE;
extern FILE *stdin, *stdout, *stderr;

int printf(const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *f, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

int putchar(int c);
int fputc(int c, FILE *f);
int putc(int c, FILE *f);
int puts(const char *s);
int fputs(const char *s, FILE *f);
int fflush(FILE *f);

int getchar(void);
int fgetc(FILE *f);
int getc(FILE *f);

int sscanf(const char *s, const char *fmt, ...);

/* The platform hooks (weak defaults in src/stdio/console.c). */
int __stdout_putc(int c);
int __stdin_getc(void);

#endif
