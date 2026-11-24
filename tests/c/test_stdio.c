/*===-- test_stdio.c - printf, sscanf and friends, on the target -----------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Writes lines to the console; tests/sim/test_stdio.py captures them and
 * compares them with the expected text (what a C99 printf prints). main
 * returns the number of failed in-program checks (the buffer functions).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int failed;
#define CHECK(c)                                                             \
  do {                                                                       \
    if (!(c)) {                                                              \
      ++failed;                                                              \
      printf("CHECK FAILED line %d\n", __LINE__);                            \
    }                                                                        \
  } while (0)

static int fmt_into(char *buf, size_t n, const char *fmt, ...) {
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = vsnprintf(buf, n, fmt, ap);
  va_end(ap);
  return r;
}

#ifndef MERA400_STDIO_FLOAT
#define MERA400_STDIO_FLOAT 0
#endif

#if MERA400_STDIO_FLOAT
/* volatile: keep the compiler from folding the conversions away. */
static volatile double vd;
#endif

int main(void) {
  char buf[40], word[16];
  int a = 0, b = 0, n;
  unsigned h = 0;
  char c = 0;

  printf("%d|%5d|%-5d|%05d|%+d|% d\n", 42, 42, 42, 42, 42, 42);
  printf("%d %i %d\n", -32767 - 1, -7, 0);
  printf("%u %x %X %o %#x %#o\n", 65535u, 0xbeef, 0xbeef, 8, 255, 8);
  printf("%ld %lu %lx\n", -2147483647L - 1, 4000000000UL, 0xdeadbeefUL);
  printf("%lld %llu\n", -9000000000000LL, 18000000000000000000ULL);
  printf("%c|%3c|%-3c|\n", 'a', 'b', 'c');
  printf("%s|%10s|%-10s|%.3s|%s\n", "abc", "abc", "abc", "abcdef", (char *)0);
  printf("%5.3d|%.0d|%-+6d|%hhu|%*d|%.*s\n", 42, 0, 5, 300, 6, 7, 2, "xyz");
  printf("%p %%\n", (void *)0x1234);

#if MERA400_STDIO_FLOAT
  vd = 1.5;
  printf("%f|%.3f|%8.2f|%-8.1f|%+.1f\n", vd, -0.0004, 3.14159, 2.26, 7.0);
  vd = 12345.678;
  printf("%e|%.2e|%E\n", vd, 0.000123, 1e30);
  printf("%g|%g|%g|%g|%g\n", 100000.0, 1000000.0, 0.0001, 0.00001, 3.5);
  printf("%f|%e|%g|%.0f\n", 0.0, 0.0, 0.0, 123456789.0);
  vd = 2.0 / 3.0;
  printf("%.3g|%.10g|%.1f|%f\n", vd, 1.0 / 3.0, 9.96, 999999.9999999);
  printf("%#.0f|%#g|%.3f\n", 3.0, 1.5, 1e-9);
  printf("%.2f %.2f\n", 1e10, -123.456);
#else
  /* Placeholder "?float?"; each double must still use up its 3 words. */
  printf("%f|%5.1f|%-9e|%10g|%d\n", 1.5, 2.5, 3.5, 4.5, 77);
  printf("%.2E %G %c %ld %s\n", 1e30, 0.5, 'k', 123456L, "ok");
#endif

  n = snprintf(buf, 5, "%s", "abcdefgh");
  CHECK(n == 8 && strcmp(buf, "abcd") == 0);
  n = sprintf(buf, "%d-%s-%c", 12, "ab", 'z');
  CHECK(n == 7 && strcmp(buf, "12-ab-z") == 0);
#if MERA400_STDIO_FLOAT
  n = fmt_into(buf, sizeof buf, "%03d:%.2f", 5, 2.5);
  CHECK(n == 8 && strcmp(buf, "005:2.50") == 0);
#else
  n = fmt_into(buf, sizeof buf, "%03d:%.2f:%d", 5, 2.5, 6);
  CHECK(n == 13 && strcmp(buf, "005:?float?:6") == 0);
#endif
  CHECK(snprintf(0, 0, "%ld", 123456L) == 6);

  n = sscanf("12 abc Z -7 0x1f", "%d %s %c %d %x", &a, word, &c, &b, &h);
  CHECK(n == 5 && a == 12 && strcmp(word, "abc") == 0 && c == 'Z' &&
        b == -7 && h == 0x1f);
  CHECK(sscanf("x", "%d", &a) == 0);
  CHECK(sscanf("", "%d", &a) == EOF);
  /* sscanf has no floating point conversions at all (in either mode): the
     conversion fails, stops the scan and stores nothing. */
  {
    unsigned fv[2] = {5, 5}; /* room for a float: must stay untouched */
    a = b = 0;
    CHECK(sscanf("12 3.5 9", "%d %f %d", &a, fv, &b) == 1 && a == 12 &&
          b == 0 && fv[0] == 5 && fv[1] == 5);
  }

  puts("puts line");
  putchar('x');
  putchar('\n');
  fputs("fputs line\n", stderr);
  fprintf(stdout, "fprintf %d\n", 99);
  printf("n=%d\n", printf("abc\n"));
  return failed;
}
