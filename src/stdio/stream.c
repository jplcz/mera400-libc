/*===-- stream.c - FILE objects and character/string output ----------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * There is no buffering and no real file: a FILE only tells stdin, stdout
 * and stderr apart. Both output streams end in __stdout_putc.
 */
#include <stdio.h>

struct __FILE {
  int fd;
};

/* Defined only by platforms that buffer (CROOK). */
extern int __stdout_flush(void) __attribute__((weak));

static FILE in_file = {0}, out_file = {1}, err_file = {2};
FILE *stdin = &in_file;
FILE *stdout = &out_file;
FILE *stderr = &err_file;

int fputc(int c, FILE *f) {
  (void)f;
  return __stdout_putc(c);
}

int putc(int c, FILE *f) { return fputc(c, f); }

int fputs(const char *s, FILE *f) {
  while (*s)
    if (fputc((unsigned char)*s++, f) == EOF)
      return EOF;
  return 0;
}

int fflush(FILE *f) {
  (void)f;
  if (__stdout_flush)
    return __stdout_flush();
  return 0;
}

int fgetc(FILE *f) {
  (void)f;
  return __stdin_getc();
}

int getc(FILE *f) { return fgetc(f); }
