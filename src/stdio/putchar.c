/*===-- putchar.c - putchar, puts, getchar on the platform hooks -----------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * The CROOK platform has its own versions of these three (libcrook), so this
 * file is not built there.
 */
#include <stdio.h>

int putchar(int c) { return __stdout_putc(c); }

int puts(const char *s) {
  while (*s)
    if (__stdout_putc((unsigned char)*s++) == EOF)
      return EOF;
  return __stdout_putc('\n') == EOF ? EOF : 0;
}

int getchar(void) { return __stdin_getc(); }
