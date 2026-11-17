/*===-- m48_mul.c - software __m48_mul -------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_mul(double a, double b);   a * b
 *
 * The same steps as the MF instruction: multiply the magnitudes with 41
 * shift-and-add steps (one more than the mantissa width, because both
 * operands were shifted right once to make room for the sign), apply the
 * sign, normalise, round.
 */
#include "m48.h"

double __m48_mul(double a, double b) {
  m48 x = m48_unpack(a), y = m48_unpack(b);
  int negative = 0;
  i64 m = 0;

  m48_denorm(&x, 1);
  m48_denorm(&y, 1);
  if (x.m < 0) {
    negative ^= 1;
    x.m = -x.m;
  }
  if (y.m < 0) {
    negative ^= 1;
    y.m = -y.m;
  }
  x.e += y.e;
  y.m >>= 23;
  for (int i = 0; i < 41; ++i) {
    m >>= 1;
    if (y.m & 1)
      m += x.m;
    y.m >>= 1;
  }
  x.m = negative ? -m : m;
  m48_round(&x);
  return m48_pack(x);
}
