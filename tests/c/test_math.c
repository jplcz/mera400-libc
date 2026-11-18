/*===-- test_math.c - libm accuracy, on the target -------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Prints "<function> <x> [<y>] <result>" lines to the console, each number
 * as the 12 hexadecimal digits of its three words (printf is not used: with
 * the software floating point it would not leave room for the math code in
 * the 64 KiB data reach of the small memory model). tests/sim/test_math.py
 * decodes them and compares with Python's math module. main returns the
 * line of the first failed in-program check (frexp, modf, ldexp, ...).
 */
#include <math.h>

extern int __stdout_putc(int c);

static int first_failure;
#define CHECK(c)                                                             \
  do {                                                                       \
    if (!(c) && !first_failure)                                              \
      first_failure = __LINE__;                                              \
  } while (0)

static volatile double xs[] = {0.0, 0.1, 0.5, 1.0, 1.3,  2.5,
                               3.14, 10.0, 17.25, 100.0, 1.0e5, 3.0e10};
static volatile double ys[] = {-3.5, 0.0, 0.4, 1.0, 2.5, 7.0};

#define NX (int)(sizeof xs / sizeof xs[0])
#define NY (int)(sizeof ys / sizeof ys[0])

static void hex4(unsigned v) {
  int i;
  for (i = 12; i >= 0; i -= 4)
    __stdout_putc("0123456789abcdef"[(v >> i) & 15]);
}

static void number(double d) {
  union {
    double d;
    unsigned short w[4];
  } u;
  u.d = d;
  __stdout_putc(' ');
  hex4(u.w[0]);
  hex4(u.w[1]);
  hex4(u.w[2]);
}

static void name(const char *s) {
  while (*s)
    __stdout_putc(*s++);
}

#define UNARY(f)                                                             \
  for (i = 0; i < NX; ++i) {                                                 \
    double x = xs[i];                                                        \
    name(#f);                                                                \
    number(x);                                                               \
    number(f(x));                                                            \
    __stdout_putc('\n');                                                     \
    x = -xs[i];                                                              \
    if (x != 0.0) {                                                          \
      name(#f);                                                              \
      number(x);                                                             \
      number(f(x));                                                          \
      __stdout_putc('\n');                                                   \
    }                                                                        \
  }

#define BINARY(f)                                                            \
  for (i = 0; i < NX; ++i)                                                   \
    for (j = 0; j < NY; ++j) {                                               \
      double x = xs[i], y = ys[j];                                           \
      name(#f);                                                              \
      number(x);                                                             \
      number(y);                                                             \
      number(f(x, y));                                                       \
      __stdout_putc('\n');                                                   \
    }

int main(void) {
  int i, j, e;
  double ip;

  UNARY(sqrt) UNARY(cbrt) UNARY(floor) UNARY(ceil) UNARY(trunc) UNARY(round)
  UNARY(fabs) UNARY(exp) UNARY(log) UNARY(sin) UNARY(cos) UNARY(tan)
  UNARY(atan) UNARY(sinh) UNARY(cosh) UNARY(tanh) UNARY(log2) UNARY(log10)
  UNARY(exp2) UNARY(asin) UNARY(acos)
  BINARY(pow) BINARY(fmod) BINARY(atan2) BINARY(hypot) BINARY(copysign)

  CHECK(frexp(8.0, &e) == 0.5 && e == 4);
  CHECK(frexp(-0.75, &e) == -0.75 && e == 0);
  CHECK(frexp(0.0, &e) == 0.0 && e == 0);
  CHECK(frexp(0.1, &e) * ldexp(1.0, e) == 0.1);
  CHECK(ldexp(3.0, 4) == 48.0 && ldexp(1.0, -3) == 0.125);
  CHECK(ldexp(1.0, 200) == HUGE_VAL && ldexp(1.0, -300) == 0.0);
  CHECK(ldexp(-1.0, 200) == -HUGE_VAL);
  CHECK(modf(-3.75, &ip) == -0.75 && ip == -3.0);
  CHECK(floor(-2.5) == -3.0 && ceil(-2.5) == -2.0 && round(-2.5) == -3.0);
  CHECK(rint(2.5) == 2.0 && rint(3.5) == 4.0 && lrint(-1.5) == -2L);
  CHECK(lround(2.5) == 3L && llround(-2.5) == -3LL);
  CHECK(fmin(1.0, 2.0) == 1.0 && fmax(1.0, 2.0) == 2.0);
  CHECK(exp(1000.0) == HUGE_VAL && exp(-1000.0) == 0.0);
  CHECK(pow(2.0, 10.0) == 1024.0 && pow(-2.0, 3.0) == -8.0 &&
        pow(2.0, -2.0) == 0.25 && pow(5.0, 0.0) == 1.0);
  return first_failure;
}
