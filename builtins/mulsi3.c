/*===-- mulsi3.c - 32-bit multiplication -----------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/* Shift and add. Written with one-place shifts and additions only, because
   a multiplication here would call this function again. The low 32 bits of
   the product are the same for signed and unsigned operands. */
long __mulsi3(long a, long b) {
  unsigned long x = a, y = b, r = 0;
  while (y) {
    if (y & 1)
      r += x;
    x <<= 1;
    y >>= 1;
  }
  return r;
}
