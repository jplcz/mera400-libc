/*===-- crook_printf.c - printf on the CROOK platform ----------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Shows that the portable stdio works on CROOK: printf output reaches the
 * operating system as records (tests/crook/test_crook_stdio.py plays the
 * operating system and checks them).
 */
#include <stdio.h>

void __crook_set_streams(unsigned in, unsigned out);

int main(void) {
  __crook_set_streams(3, 4);
  printf("n=%d s=%s x=%04x\n", 42, "ok", 0xBEEF);
  puts("line2");
  printf("no newline");
  fflush(stdout);
  return 5;
}
