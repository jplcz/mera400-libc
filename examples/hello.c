/*===-- hello.c - printf and the console -----------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Prints a greeting and a few numbers. The bare-metal libc sends every
 * character to __console_port, a memory-mapped word that exists only in the
 * simulator used by the tests; a real machine or em400 prints nothing until a
 * platform replaces the weak __stdout_putc hook (see src/stdio/README.md).
 */
#include <stdio.h>

int main(void) {
  printf("Hello, MERA-400!\n");
  printf("%d %u %x %s\n", -42, 42u, 0xBEEF, "done");
  return 0;
}
