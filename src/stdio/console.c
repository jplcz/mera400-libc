/*===-- console.c - the console of platform "none" -------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * The bare processor has no console, so a simulator or board model supplies
 * one as a memory-mapped word: the linker symbol __console_port is its byte
 * address (the top word of memory by default). Writing a word stores the
 * character in its low octet to the output; reading it gives the next input
 * character in the low octet, or 0xFFFF at the end of the input.
 *
 * Both functions are weak: a program may define its own and the linker
 * takes that one. (The CROOK platform does not build this file at all.)
 */
#include <stdio.h>

extern volatile unsigned short __console_port[];

__attribute__((weak)) int __stdout_putc(int c) {
  __console_port[0] = (unsigned char)c;
  return (unsigned char)c;
}

__attribute__((weak)) int __stdin_getc(void) {
  unsigned short v = __console_port[0];
  return v == 0xFFFFu ? EOF : (int)(v & 0xFF);
}
