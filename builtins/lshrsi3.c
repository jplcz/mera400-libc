/*===-- lshrsi3.c - 32-bit logical shift right -----------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/* A loop of inline one-place shifts (see ashlsi3.c); 32 or more gives 0. The
   count is a 32-bit value because that is how the compiler passes it. */
unsigned long __lshrsi3(unsigned long a, long n) {
  if ((unsigned long)n >= 32)
    return 0;
  while (n--)
    a >>= 1;
  return a;
}
