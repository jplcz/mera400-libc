/*===-- explog.c - exponential, logarithm, power, hyperbolic functions -----===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <math.h>

/* ln 2 in two parts: the high part has few mantissa bits, so k * LN2_HI is
   exact for the small integers k used below. */
#define LN2_HI 0.693145751953125
#define LN2_LO 1.42860682030941723212e-6

double exp(double x) {
  double k, r, term, sum;
  int i;
  if (x > 88.72283)
    return HUGE_VAL;
  if (x < -89.5)
    return 0;
  k = floor(x * M_LOG2E + 0.5);
  r = (x - k * LN2_HI) - k * LN2_LO;
  /* Taylor series of exp(r), |r| <= 0.35 */
  sum = 1.0;
  term = 1.0;
  for (i = 1; i < 14; ++i) {
    term = term * r / i;
    sum += term;
  }
  return ldexp(sum, (int)k);
}

double exp2(double x) {
  double k;
  if (x >= 127.99)
    return HUGE_VAL;
  if (x < -129)
    return 0;
  k = floor(x + 0.5);
  return ldexp(exp((x - k) * M_LN2), (int)k);
}

double exp10(double x) { return exp(x * M_LN10); }

double log(double x) {
  double m, s, s2, sum, term;
  int e, i;
  if (x <= 0)
    return x == 0 ? -HUGE_VAL : 0;
  m = frexp(x, &e);
  if (m < M_SQRT1_2) {
    m *= 2.0;
    --e;
  }
  /* log(m) = 2 * (s + s^3/3 + s^5/5 + ...), s = (m - 1) / (m + 1) */
  s = (m - 1.0) / (m + 1.0);
  s2 = s * s;
  term = s;
  sum = 0;
  for (i = 1; i < 24; i += 2) {
    sum += term / i;
    term *= s2;
  }
  return e * LN2_HI + (e * LN2_LO + 2.0 * sum);
}

double log2(double x) { return x <= 0 ? log(x) : log(x) * M_LOG2E; }
double log10(double x) { return x <= 0 ? log(x) : log(x) * M_LOG10E; }

static double powi(double x, long n) {
  double r = 1.0;
  int neg = n < 0;
  unsigned long u = neg ? 0UL - (unsigned long)n : (unsigned long)n;
  while (u) {
    if (u & 1)
      r *= x;
    u >>= 1;
    if (u)
      x *= x;
  }
  return neg ? 1.0 / r : r;
}

double pow(double x, double y) {
  double yi;
  if (y == 0)
    return 1.0;
  if (x == 0)
    return y > 0 ? 0 : HUGE_VAL;
  yi = trunc(y);
  if (yi == y && fabs(y) < 30000.0) {
    double r = powi(x, (long)y);
    return r;
  }
  if (x < 0)
    return 0; /* a negative base with a fractional power has no real result */
  return exp(y * log(x));
}

/* exp(a) / 2 without overflowing for a up to 89.4 */
static double half_exp(double a) {
  if (a - M_LN2 > 88.72283)
    return HUGE_VAL;
  return exp(a - M_LN2);
}

double sinh(double x) {
  double a = fabs(x), r;
  if (a < 0.1) {
    double x2 = a * a;
    r = a * (1.0 + x2 / 6.0 * (1.0 + x2 / 20.0 * (1.0 + x2 / 42.0)));
  } else if (a > 20.0) {
    r = half_exp(a);
  } else {
    double e = exp(a);
    r = 0.5 * (e - 1.0 / e);
  }
  return x < 0 ? -r : r;
}

double cosh(double x) {
  double a = fabs(x);
  if (a > 20.0)
    return half_exp(a);
  a = exp(a);
  return 0.5 * (a + 1.0 / a);
}

double tanh(double x) {
  double a = fabs(x), r;
  if (a > 14.0)
    r = 1.0;
  else if (a < 0.1) {
    double x2 = a * a;
    r = a * (1.0 - x2 / 3.0 * (1.0 - 2.0 * x2 / 5.0 * (1.0 - 17.0 * x2 / 42.0)));
  } else {
    double e = exp(2.0 * a);
    r = (e - 1.0) / (e + 1.0);
  }
  return x < 0 ? -r : r;
}
