/*===-- stdout.c - the stdio hooks on CROOK --------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * printf and friends write one character at a time to __stdout_putc. CROOK
 * transfers whole records (lines), so characters are collected here and sent
 * with puts() (which adds the line end) at each '\n' or when the buffer is
 * full. Output that does not end in '\n' stays in the buffer until
 * fflush() or the next newline, so end messages with '\n'.
 * Streams must have been set with __crook_set_streams (see stdio.s).
 */
#include <stdio.h>

#define LINE_MAX_CHARS 200

static char line[LINE_MAX_CHARS + 1];
static unsigned fill;

int __stdout_flush(void) {
  int r = 0;
  if (fill) {
    line[fill] = 0;
    fill = 0;
    r = puts(line);
  }
  return r;
}

int __stdout_putc(int c) {
  if (c == '\n') {
    int r;
    line[fill] = 0;
    fill = 0;
    r = puts(line);
    return r < 0 ? EOF : '\n';
  }
  line[fill++] = (char)c;
  if (fill == LINE_MAX_CHARS && __stdout_flush() < 0)
    return EOF;
  return (unsigned char)c;
}
