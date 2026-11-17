/*===-- ashrsi3.c - 32-bit arithmetic shift right --------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/* A loop of inline one-place shifts (see ashlsi3.c) that keeps the sign;
   32 or more gives 0 or -1, like the 16-bit helper. The count is a 32-bit
   value because that is how the compiler passes it. */
long __ashrsi3(long a, long n) {
  if ((unsigned long)n >= 32)
    return a < 0 ? -1 : 0;
  while (n--)
    a >>= 1;
  return a;
}
