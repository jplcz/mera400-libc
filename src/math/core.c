/*===-- core.c - sign, rounding, exponent and root functions ---------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * frexp and ldexp work on the words of the native double directly (see
 * builtins/m48.h for the layout): the exponent is the low byte of word 2.
 * The values the arithmetic helpers produce are always normalised, which
 * these functions rely on.
 */
#include <math.h>

typedef union {
  double d;
  unsigned short w[4];
} bits;

double fabs(double x) { return x < 0 ? -x : x; }

double copysign(double x, double y) {
  x = fabs(x);
  return y < 0 ? -x : x;
}

double fmin(double x, double y) { return y < x ? y : x; }
double fmax(double x, double y) { return y > x ? y : x; }

double frexp(double x, int *e) {
  bits b;
  b.d = x;
  if (x == 0) {
    *e = 0;
    return x;
  }
  *e = (signed char)(b.w[2] & 0xFF);
  b.w[2] &= 0xFF00;
  return b.d;
}

double ldexp(double x, int n) {
  bits b;
  int e;
  double f = frexp(x, &e);
  if (f == 0)
    return x;
  e += n;
  b.d = f;
  if (e > 127) {
    b.w[0] = f < 0 ? 0x8000 : 0x7FFF;
    b.w[1] = f < 0 ? 0x0000 : 0xFFFF;
    b.w[2] = f < 0 ? 0x0100 : 0xFF00;
    e = 127;
  } else if (e < -128 || n < -400) {
    return 0;
  }
  b.w[2] = (unsigned short)((b.w[2] & 0xFF00) | (e & 0xFF));
  b.w[3] = 0;
  return b.d;
}

double scalbn(double x, int n) { return ldexp(x, n); }

double trunc(double x) {
  /* From 2^39 on every value is an integer already. */
  if (x >= 549755813888.0 || x <= -549755813888.0)
    return x;
  return (double)(long long)x;
}

double floor(double x) {
  double t = trunc(x);
  return t > x ? t - 1.0 : t;
}

double ceil(double x) {
  double t = trunc(x);
  return t < x ? t + 1.0 : t;
}

double round(double x) {
  double t = trunc(x);
  if (x - t >= 0.5)
    return t + 1.0;
  if (t - x >= 0.5)
    return t - 1.0;
  return t;
}

double roundeven(double x) {
  double t = floor(x), d = x - t;
  if (d > 0.5 || (d == 0.5 && fmod(t, 2.0) != 0))
    return t + 1.0;
  return t;
}

double rint(double x) { return roundeven(x); }
double nearbyint(double x) { return roundeven(x); }
long lrint(double x) { return (long)roundeven(x); }
long long llrint(double x) { return (long long)roundeven(x); }
long lround(double x) { return (long)round(x); }
long long llround(double x) { return (long long)round(x); }

double modf(double x, double *ip) {
  double t = trunc(x);
  *ip = t;
  return x - t;
}

/* Exact: repeatedly subtracts the largest power-of-two multiple of |y| that
   fits (every subtraction is between numbers within a factor of two, so no
   rounding happens). The result has the sign of x. */
double fmod(double x, double y) {
  double r = fabs(x), d = fabs(y);
  int ex, ey;
  if (d == 0)
    return 0;
  while (r >= d) {
    frexp(r, &ex);
    frexp(d, &ey);
    double s = ldexp(d, ex - ey);
    if (s > r)
      s = ldexp(d, ex - ey - 1);
    r -= s;
  }
  return x < 0 ? -r : r;
}

double sqrt(double x) {
  double y;
  int e, i;
  if (x <= 0)
    return 0;
  y = frexp(x, &e);
  if (e & 1) {
    y *= 2.0;
    --e;
  }
  /* y is in [0.5, 2); a linear first guess, then Newton's iteration. */
  y = ldexp(0.4 + 0.6 * y, e / 2);
  for (i = 0; i < 6; ++i)
    y = 0.5 * (y + x / y);
  return y;
}

double hypot(double x, double y) {
  double a = fabs(x), b = fabs(y), t;
  if (a < b) {
    t = a;
    a = b;
    b = t;
  }
  if (a == 0)
    return 0;
  t = b / a;
  return a * sqrt(1.0 + t * t);
}

double cbrt(double x) {
  double a = fabs(x), y;
  int e, i;
  if (a == 0)
    return 0;
  y = frexp(a, &e);
  while (e % 3) {
    y *= 2.0;
    --e;
  }
  y = ldexp(0.5 + 0.5 * y, e / 3);
  for (i = 0; i < 8; ++i)
    y = y - (y * y * y - a) / (3.0 * y * y);
  return x < 0 ? -y : y;
}

double fma(double x, double y, double z) { return x * y + z; }
