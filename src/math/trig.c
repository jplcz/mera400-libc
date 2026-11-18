/*===-- trig.c - sine, cosine, tangent, arc functions ----------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <math.h>

/* pi/2 in three parts (Cody-Waite): the first two have 19 mantissa bits
   each, so k * part is exact for k below 2^19; the third holds the rest. */
#define PIO2_1 1.570796966552734375
#define PIO2_2 (-6.397604010999202728271484375e-7)
#define PIO2_3 2.5633441515945187881889420985846996875529e-12

static double sin_poly(double r) {
  double r2 = r * r;
  /* r - r^3/3! + r^5/5! - ... - r^15/15! */
  return r * (1.0 - r2 / 6.0 * (1.0 - r2 / 20.0 * (1.0 - r2 / 42.0 *
         (1.0 - r2 / 72.0 * (1.0 - r2 / 110.0 * (1.0 - r2 / 156.0 *
         (1.0 - r2 / 210.0)))))));
}

static double cos_poly(double r) {
  double r2 = r * r;
  return 1.0 - r2 / 2.0 * (1.0 - r2 / 12.0 * (1.0 - r2 / 30.0 *
         (1.0 - r2 / 56.0 * (1.0 - r2 / 90.0 * (1.0 - r2 / 132.0 *
         (1.0 - r2 / 182.0))))));
}

/* Reduces x to r in [-pi/4, pi/4] and returns the quadrant 0..3. */
static int reduce(double x, double *r) {
  double k = floor(x * M_2_PI + 0.5);
  *r = ((x - k * PIO2_1) - k * PIO2_2) - k * PIO2_3;
  return (int)fmod(k, 4.0) & 3;
}

double sin(double x) {
  double r;
  int q = reduce(x, &r);
  switch (q) {
  case 0: return sin_poly(r);
  case 1: return cos_poly(r);
  case 2: return -sin_poly(r);
  default: return -cos_poly(r);
  }
}

double cos(double x) {
  double r;
  int q = reduce(x, &r);
  switch (q) {
  case 0: return cos_poly(r);
  case 1: return -sin_poly(r);
  case 2: return -cos_poly(r);
  default: return sin_poly(r);
  }
}

double tan(double x) {
  double r, s, c;
  int q = reduce(x, &r);
  s = sin_poly(r);
  c = cos_poly(r);
  if (q & 1)
    return c == 0 ? HUGE_VAL : -c / s;
  return c == 0 ? HUGE_VAL : s / c;
}

double atan(double x) {
  double a = fabs(x), r, a2, term, sum;
  int inv = 0, shift = 0, i;
  if (a > 1.0) {
    a = 1.0 / a;
    inv = 1;
  }
  if (a > 0.2679491924311227) { /* tan(pi/12) */
    a = (a * 1.7320508075688772 - 1.0) / (1.7320508075688772 + a);
    shift = 1;
  }
  a2 = a * a;
  term = a;
  sum = 0;
  for (i = 1; i < 31; i += 2) {
    sum += term / i;
    term = -term * a2;
  }
  r = sum + (shift ? M_PI / 6.0 : 0.0);
  if (inv)
    r = M_PI_2 - r;
  return x < 0 ? -r : r;
}

double atan2(double y, double x) {
  if (x == 0)
    return y > 0 ? M_PI_2 : y < 0 ? -M_PI_2 : 0;
  if (x > 0)
    return atan(y / x);
  return y >= 0 ? atan(y / x) + M_PI : atan(y / x) - M_PI;
}

double asin(double x) {
  double a = fabs(x);
  if (a > 1.0)
    return 0;
  return atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}

double acos(double x) {
  if (fabs(x) > 1.0)
    return 0;
  return atan2(sqrt((1.0 - x) * (1.0 + x)), x);
}
