/*===-- m48_conv.c - conversions between integers and the native double ----===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_fromi32(long), __m48_fromu32(unsigned long),
 *        __m48_fromi64(long long), __m48_fromu64(unsigned long long)
 * long __m48_toi32(double), unsigned long __m48_tou32(double),
 * long long __m48_toi64(double), unsigned long long __m48_tou64(double)
 *
 * Integer to double rounds to nearest, ties to even, exactly as the
 * compiler's own conversion of constants does (32-bit integers are always
 * exact: the mantissa has 40 bits). Double to integer truncates toward zero.
 * A value that does not fit saturates to the nearest limit of the integer
 * type (C leaves it undefined); a negative value converted to an unsigned
 * type gives 0.
 */
#include "m48.h"

#define BIT39 ((u64)1 << 39)

static double from_magnitude(u64 mag, int negative) {
  m48 x;
  u64 m;
  int nb = 0;
  if (mag == 0)
    return m48_pack((m48){0, 0});
  for (u64 t = mag; t; t >>= 1)
    ++nb;
  if (nb <= 39) {
    m = m48_shl(mag, 39 - nb);
    x.e = nb;
  } else {
    int sh = nb - 39;
    u64 rem = mag & (m48_shl(1, sh) - 1), half = m48_shl(1, sh - 1);
    m = m48_shr(mag, sh);
    if (rem > half || (rem == half && (m & 1)))
      ++m;
    x.e = nb;
    if (m == BIT39) {
      m >>= 1;
      x.e += 1;
    }
  }
  x.m = (i64)(negative ? -m : m) << 24;
  m48_norm(&x);
  return m48_pack(x);
}

double __m48_fromi32(long v) {
  return from_magnitude(v < 0 ? -(u64)(i64)v : (u64)v, v < 0);
}
double __m48_fromu32(unsigned long v) { return from_magnitude(v, 0); }
double __m48_fromi64(long long v) {
  return from_magnitude(v < 0 ? -(u64)v : (u64)v, v < 0);
}
double __m48_fromu64(unsigned long long v) { return from_magnitude(v, 0); }

/* |truncated value| in *mag and its sign in *negative; returns 1 when the
   magnitude does not fit 64 bits (then only the sign is meaningful). */
static int to_magnitude(double d, u64 *mag, int *negative) {
  m48 x = m48_unpack(d);
  i64 m40 = x.m >> 24;
  u64 a;
  int s = x.e - 39;
  *negative = m40 < 0;
  a = *negative ? -m40 : m40;
  *mag = 0;
  if (a == 0)
    return 0;
  if (s >= 0) {
    if (s > 25 || (s == 25 && a >= BIT39))
      return 1;
    *mag = m48_shl(a, s);
  } else if (s > -40) {
    *mag = m48_shr(a, -s);
  }
  return 0;
}

long long __m48_toi64(double d) {
  u64 mag;
  int neg, ovf = to_magnitude(d, &mag, &neg);
  if (neg)
    return ovf || mag > ((u64)1 << 63) ? (i64)0x8000000000000000ULL : -(i64)mag;
  return ovf || mag > 0x7FFFFFFFFFFFFFFFULL ? (i64)0x7FFFFFFFFFFFFFFFULL : (i64)mag;
}

unsigned long long __m48_tou64(double d) {
  u64 mag;
  int neg, ovf = to_magnitude(d, &mag, &neg);
  if (neg)
    return 0;
  return ovf ? ~(u64)0 : mag;
}

long __m48_toi32(double d) {
  i64 v = __m48_toi64(d);
  return v > 0x7FFFFFFFLL ? 0x7FFFFFFFL : v < -0x80000000LL ? -0x7FFFFFFFL - 1 : (long)v;
}

unsigned long __m48_tou32(double d) {
  u64 v = __m48_tou64(d);
  return v > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (unsigned long)v;
}
