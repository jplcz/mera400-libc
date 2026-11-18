/*===-- test_c.c - the C library code and runtime helpers, on the target ---===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Compiled by Clang for the MERA-400 and run on the simulator (see
 * tests/sim/test_c.py). There is no output channel, so main returns 0 when
 * every check passed, or the source line of the first failed check (which
 * the simulator test reads from r1). The same checks run on the build
 * machine in tests/host/test_c.c, which shows the algorithms are right; this
 * file shows that the target code generation and the 16-bit types are.
 */
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int first_failure;
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond) && !first_failure)                                           \
      first_failure = __LINE__;                                              \
  } while (0)

static int cmp_int(const void *a, const void *b) {
  return *(const int *)a - *(const int *)b;
}

/* volatile: the compiler must not fold the arithmetic away, so the helpers
   (__mulsi3, __udivsi3, ...) really run. */
static volatile long vl = 123456L;
static volatile unsigned long vu = 4000000000UL;
static volatile int vn = 5;

int main(void) {
  char *end;
  int i;

  CHECK(isalpha('a') && !isalpha('1') && isdigit('7') && isspace('\n'));
  CHECK(toupper('q') == 'Q' && tolower('Q') == 'q' && toupper('1') == '1');
  CHECK(ispunct('!') && !ispunct('a') && isxdigit('F') && !isxdigit('g'));

  CHECK(strtol("  -123xyz", &end, 10) == -123 && *end == 'x');
  CHECK(strtol("0x1F", &end, 0) == 31 && *end == 0);
  CHECK(strtol("077", NULL, 0) == 63);
  CHECK(strtol("zz", &end, 36) == 1295);
  CHECK(strtol("2147483647", NULL, 10) == 2147483647L);
  CHECK(strtoul("4294967295", NULL, 10) == 4294967295UL);
  CHECK(atoi("42") == 42 && atol("-70000") == -70000L);

  {
    int a[] = {5, 3, 9, 1, 1, 7, 0, -4, 8, 2, 6};
    int n = sizeof a / sizeof a[0], key = 7;
    qsort(a, n, sizeof a[0], cmp_int);
    for (i = 1; i < n; ++i)
      CHECK(a[i - 1] <= a[i]);
    CHECK(bsearch(&key, a, n, sizeof a[0], cmp_int) != NULL);
    key = 100;
    CHECK(bsearch(&key, a, n, sizeof a[0], cmp_int) == NULL);
  }

  srand(1);
  {
    int r1 = rand(), r2 = rand();
    CHECK(r1 >= 0 && r1 <= RAND_MAX && r2 >= 0 && r2 <= RAND_MAX && r1 != r2);
    srand(1);
    CHECK(rand() == r1 && rand() == r2);
  }

  CHECK(strstr("find the needle", "needle") != NULL);
  CHECK(strspn("aabbcx", "abc") == 5 && strcspn("hello world", " ") == 5);

  {
    char *p[20];
    int j;
    for (i = 0; i < 20; ++i) {
      p[i] = malloc(10 + i);
      CHECK(p[i] != NULL);
      memset(p[i], i, 10 + i);
    }
    for (i = 0; i < 20; ++i)
      for (j = 0; j < 10 + i; ++j)
        CHECK(p[i][j] == (char)i);
    for (i = 0; i < 20; ++i)
      free(p[i]);
    /* Everything is free again, so one big block is available. */
    p[0] = malloc(30000);
    CHECK(p[0] != NULL);
#ifdef __MERA400_LARGE__
    /* The large model's heap exceeds 64 KiB and sizes are 32 bits wide. */
    {
      char *big = malloc(70000UL);
      CHECK(big != NULL);
      big[69999] = 7;
      big[0] = 1;
      CHECK(big[69999] == 7 && big[0] == 1);
      free(big);
    }
#else
    CHECK(malloc(40000) == NULL);
#endif
    free(p[0]);
    p[0] = calloc(10, 10);
    for (j = 0; j < 100; ++j)
      CHECK(p[0][j] == 0);
  }

  /* 32-bit runtime helpers. */
  CHECK(vl * 7 == 864192L);
  CHECK(vl * -3 == -370368L);
  CHECK(vl / 1000 == 123 && vl % 1000 == 456);
  CHECK(-vl / 1000 == -123 && -vl % 1000 == -456);
  CHECK(vu / 65536UL == 61035UL && vu % 65536UL == 10240UL);
  CHECK((vl << vn) == 3950592L && (vl >> vn) == 3858L);
  CHECK((-vl >> vn) == -3858L && (vu >> vn) == 125000000UL);

  return first_failure;
}
