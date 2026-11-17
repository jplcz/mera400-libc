/*===-- float.h - floating-point characteristics ---------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_FLOAT_H
#define MERA400_FLOAT_H

/* float, double and long double all use the processor's native floating
   point format (the one the AWP arithmetic unit works on, not IEEE 754):
   a 39-bit magnitude plus sign mantissa in [1/2, 1) and an 8-bit exponent,
   no NaN, infinity or denormals. An object occupies 64 bits of storage, of
   which 48 are used. The largest value is about 1.7e38. */

#define FLT_RADIX 2
#define FLT_ROUNDS 1

#define FLT_MANT_DIG 39
#define DBL_MANT_DIG 39
#define LDBL_MANT_DIG 39

#define FLT_DIG 11
#define DBL_DIG 11
#define LDBL_DIG 11

#define FLT_MIN_EXP (-128)
#define DBL_MIN_EXP (-128)
#define LDBL_MIN_EXP (-128)
#define FLT_MAX_EXP 127
#define DBL_MAX_EXP 127
#define LDBL_MAX_EXP 127

#define FLT_MIN_10_EXP (-38)
#define DBL_MIN_10_EXP (-38)
#define LDBL_MIN_10_EXP (-38)
#define FLT_MAX_10_EXP 38
#define DBL_MAX_10_EXP 38
#define LDBL_MAX_10_EXP 38

/* 2^127 * (1 - 2^-39), 2^-129 and 2^-38. */
#define FLT_MAX 1.70141183e+38
#define DBL_MAX 1.70141183e+38
#define LDBL_MAX 1.70141183e+38
#define FLT_MIN 1.46936794e-39
#define DBL_MIN 1.46936794e-39
#define LDBL_MIN 1.46936794e-39
#define FLT_EPSILON 3.63797881e-12
#define DBL_EPSILON 3.63797881e-12
#define LDBL_EPSILON 3.63797881e-12

#endif
