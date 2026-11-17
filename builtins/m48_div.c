/*===-- m48_div.c - software __m48_div -------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_div(double a, double b);   a / b
 *
 * The same steps as the DF instruction: divide the magnitudes with 41
 * restoring-division steps, apply the sign, normalise. Division by zero gives
 * the largest finite number with the sign of the dividend (see m48.h).
 */
#include "m48.h"

double __m48_div(double a, double b) {
  m48 x = m48_unpack(a), y = m48_unpack(b);
  int negative = 0;
  i64 rem, q = 0;

  if (y.m == 0) {
    if (x.m != 0) {
      x.e = 128; /* overflow: m48_pack saturates */
      return m48_pack(x);
    }
    return m48_pack(x);
  }
  m48_denorm(&x, 2);
  m48_denorm(&y, 1);
  if (x.m < 0) {
    negative ^= 1;
    x.m = -x.m;
  }
  if (y.m < 0) {
    negative ^= 1;
    y.m = -y.m;
  }
  x.e -= y.e;
  rem = x.m;
  for (int i = 0; i < 41; ++i) {
    rem -= y.m;
    q <<= 1;
    if (rem < 0)
      rem += y.m;
    else
      q |= M48_BIT40;
    rem <<= 1;
  }
  x.m = negative ? -q : q;
  m48_norm(&x);
  return m48_pack(x);
}
