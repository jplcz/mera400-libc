/*===-- ashlsi3.c - 32-bit shift left --------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/* The processor shifts 16-bit registers only, and the compiler expands a
   32-bit shift by one place inline. A shift by a variable amount is this
   loop of one-place shifts, so it never calls itself. A count of 32 or more
   gives 0, like the 16-bit helper. The count is a 32-bit value (not int, as
   in other runtimes) because that is how the compiler passes it. */
long __ashlsi3(long a, long n) {
  if ((unsigned long)n >= 32)
    return 0;
  while (n--)
    a <<= 1;
  return a;
}
