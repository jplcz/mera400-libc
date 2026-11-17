/*===-- m48_addsub.c - software __m48_add and __m48_sub -------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * double __m48_add(double a, double b);   a + b
 * double __m48_sub(double a, double b);   a - b
 *
 * The same steps as the AF and SF instructions (see m48.h): align the
 * exponents (a difference of 40 or more makes the smaller operand zero), make
 * room for a carry, add or subtract, normalise, round.
 */
#include "m48.h"

static double addsub(double a, double b, int sub) {
  m48 x = m48_unpack(a), y = m48_unpack(b);
  int ediff = x.e - y.e;

  if (ediff < 0) {
    if (ediff <= -40) {
      x.m = 0;
      x.e = y.e;
    } else {
      m48_denorm(&x, -ediff);
    }
  } else if (ediff > 0) {
    if (ediff >= 40) {
      y.m = 0;
      y.e = x.e;
    } else {
      m48_denorm(&y, ediff);
    }
  }
  m48_denorm(&x, 1);
  m48_denorm(&y, 1);
  x.m = sub ? x.m - y.m : x.m + y.m;
  m48_round(&x);
  return m48_pack(x);
}

double __m48_add(double a, double b) { return addsub(a, b, 0); }
double __m48_sub(double a, double b) { return addsub(a, b, 1); }
