/*===-- udivmodsi4.c - 32-bit division and remainder ----------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/* Restoring division, one quotient bit per step, using only one-place shifts,
   comparisons and subtractions (a division here would call this function
   again). Division by zero gives quotient 0xFFFFFFFF and the dividend as the
   remainder, like the 16-bit helpers. All four public helpers are in this file
   so that they share the one loop; the linker takes the whole object. */
static unsigned long udivmod(unsigned long n, unsigned long d,
                             unsigned long *rem) {
  unsigned long q = 0, r = 0;
  if (d == 0) {
    *rem = n;
    return 0xFFFFFFFFUL;
  }
  for (int i = 0; i < 32; ++i) {
    r = (r << 1) | (n >> 31);
    n <<= 1;
    q <<= 1;
    if (r >= d) {
      r -= d;
      q |= 1;
    }
  }
  *rem = r;
  return q;
}

unsigned long __udivsi3(unsigned long n, unsigned long d) {
  unsigned long r;
  return udivmod(n, d, &r);
}

unsigned long __umodsi3(unsigned long n, unsigned long d) {
  unsigned long r;
  udivmod(n, d, &r);
  return r;
}

long __divsi3(long n, long d) {
  unsigned long r;
  unsigned long q = udivmod(n < 0 ? -(unsigned long)n : n,
                            d < 0 ? -(unsigned long)d : d, &r);
  return (n < 0) != (d < 0) ? -q : q;
}

long __modsi3(long n, long d) {
  unsigned long r;
  udivmod(n < 0 ? -(unsigned long)n : n, d < 0 ? -(unsigned long)d : d, &r);
  return n < 0 ? -r : r;
}
