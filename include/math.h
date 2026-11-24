/*===-- math.h - mathematical functions on the native double ---------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * float, double and long double are all the native 48-bit format (see
 * float.h), which has no infinity and no NaN. So a result that is too large
 * is the largest finite number (HUGE_VAL) and a result that is too small is
 * zero; a function called outside its domain returns 0 (log of a negative
 * number returns -HUGE_VAL). Nothing sets errno.
 *
 * The accuracy goal is a few units in the last place of the 39-bit mantissa
 * (about 11 decimal digits). Arguments of sin/cos/tan beyond about 5e5
 * radians lose precision.
 */
#ifndef MERA400_MATH_H
#define MERA400_MATH_H

#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The largest finite value, 2^127 * (1 - 2^-39), which every overflowing
   result saturates to (DBL_MAX is a few thousand units in the last place
   smaller: it has only nine digits). */
#define HUGE_VAL 1.7014118346015974672e+38
#define HUGE_VALF HUGE_VAL
#define INFINITY HUGE_VAL
/* Classes returned by fpclassify (the format has no NaN, infinity or
   subnormal numbers, so only FP_ZERO and FP_NORMAL occur). */
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

typedef float float_t;
typedef double double_t;

#define isnan(x) ((void)(x), 0)
#define isinf(x) ((void)(x), 0)
#define isfinite(x) ((void)(x), 1)

#define M_E 2.71828182845904523536
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.434294481903251827651
#define M_LN2 0.693147180559945309417
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_1_PI 0.318309886183790671538
#define M_2_PI 0.636619772367581343076
#define M_SQRT2 1.41421356237309504880
#define M_SQRT1_2 0.707106781186547524401

double fabs(double x);
double copysign(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);

double sqrt(double x);
double cbrt(double x);
double hypot(double x, double y);
double fma(double x, double y, double z);

double floor(double x);
double ceil(double x);
double trunc(double x);
double round(double x);
double roundeven(double x);
double rint(double x);
double nearbyint(double x);
long lrint(double x);
long lround(double x);
long long llrint(double x);
long long llround(double x);

double fmod(double x, double y);
double modf(double x, double *ip);
double frexp(double x, int *e);
double ldexp(double x, int e);
double scalbn(double x, int e);

double exp(double x);
double exp2(double x);
double exp10(double x);
double log(double x);
double log2(double x);
double log10(double x);
double pow(double x, double y);

double sin(double x);
double cos(double x);
double tan(double x);
double atan(double x);
double atan2(double y, double x);
double asin(double x);
double acos(double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);

#ifdef __cplusplus
}
#endif

#endif
