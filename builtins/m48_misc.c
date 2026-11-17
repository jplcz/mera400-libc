/*===-- m48_misc.c - __m48_neg and __m48_cmp -------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_neg(double a);            -a
 * int    __m48_cmp(double a, double b);  -1 if a < b, 0 if equal, 1 if a > b
 *
 * There are no NaNs in this format, so the comparison is total.
 */
#include "m48.h"

double __m48_neg(double a) {
  m48 x = m48_unpack(a);
  if (x.m == M48_BIT0) {
    /* -1.0 * 2^e: the negation 1.0 * 2^e is not representable, use 0.5 * 2^(e+1) */
    x.m = (i64)0x4000000000000000ULL;
    x.e += 1;
  } else {
    x.m = -x.m;
    m48_norm(&x); /* a positive power of two negates to -1/2, which is not normalised */
  }
  return m48_pack(x);
}

static int sign_of(const m48 *x) { return x->m < 0 ? -1 : x->m > 0; }

int __m48_cmp(double a, double b) {
  m48 x = m48_unpack(a), y = m48_unpack(b);
  int sx = sign_of(&x), sy = sign_of(&y);
  if (sx != sy)
    return sx < sy ? -1 : 1;
  if (sx == 0)
    return 0;
  /* Same sign, normalised: the larger exponent is the larger magnitude. */
  if (x.e != y.e)
    return (x.e < y.e) == (sx > 0) ? -1 : 1;
  if (x.m != y.m)
    return x.m < y.m ? -1 : 1;
  return 0;
}
