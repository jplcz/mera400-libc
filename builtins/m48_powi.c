/*===-- m48_powi.c - __m48_powi --------------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_powi(double x, int n);   x to the integer power n
 *
 * Repeated squaring; a negative n gives 1 / x^|n|. The exponent is a 16-bit
 * "int" passed on the stack (see the compiler design notes). The multiplications
 * are the compiler's own calls to __m48_mul (or the AF family instructions).
 */
double __m48_powi(double x, int n) {
  unsigned k = n < 0 ? -(unsigned)n : (unsigned)n;
  double r = 1.0;
  while (k) {
    if (k & 1)
      r *= x;
    k >>= 1;
    if (k)
      x *= x;
  }
  return n < 0 ? 1.0 / r : r;
}
