/*===-- test_c.c - host-side check of the portable C library code ----------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * There is no C compiler for the MERA-400 yet, so the C sources cannot run
 * on the target. This program compiles the *logic* of src/ctype, strtol.c,
 * qsort.c, malloc.c and strextra.c with the build machine's compiler and
 * checks it. It proves the algorithms, not the code generation: the host's
 * int is wider than 16 bits, and the compiler is not ours.
 *
 * The library headers are used (-nostdinc), so what is tested is what the
 * target will see, apart from the type sizes.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int printf(const char *, ...);

/* The heap that the linker script provides on the target. */
__asm__(".data\n.globl __heap_start\n.globl __heap_end\n.balign 16\n"
        "__heap_start:\n.skip 4096\n__heap_end:\n.skip 1\n.text\n");

static int checks, failures;
#define CHECK(cond)                                                          \
  do {                                                                       \
    ++checks;                                                                \
    if (!(cond)) {                                                           \
      ++failures;                                                            \
      printf("FAIL line %d: %s\n", __LINE__, #cond);                         \
    }                                                                        \
  } while (0)

static int cmp_int(const void *a, const void *b) {
  return *(const int *)a - *(const int *)b;
}

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
  CHECK(strtol("abc", &end, 10) == 0 && end[0] == 'a');
  CHECK(strtol("2147483647", NULL, 10) == 2147483647L);
  errno = 0;
  CHECK(strtol("99999999999999999999", NULL, 10) == LONG_MAX && errno == ERANGE);
  CHECK(atoi("42") == 42 && atol("-7") == -7);

  {
    int a[] = {5, 3, 9, 1, 1, 7, 0, -4, 8, 2, 6};
    int n = sizeof a / sizeof a[0], key = 7;
    qsort(a, n, sizeof a[0], cmp_int);
    for (i = 1; i < n; ++i)
      CHECK(a[i - 1] <= a[i]);
    CHECK(bsearch(&key, a, n, sizeof a[0], cmp_int) != NULL);
    key = 100;
    CHECK(bsearch(&key, a, n, sizeof a[0], cmp_int) == NULL);
    for (i = 0; i < n; ++i)
      CHECK(bsearch(&a[i], a, n, sizeof a[0], cmp_int) != NULL);
  }

  CHECK(strnlen("abcdef", 3) == 3 && strnlen("ab", 9) == 2);
  CHECK(strspn("aabbcx", "abc") == 5 && strcspn("hello world", " ") == 5);
  CHECK(strpbrk("hello", "lo") != NULL && strpbrk("hello", "xyz") == NULL);
  CHECK(strstr("find the needle", "needle") != NULL);
  CHECK(strstr("find", "") != NULL && strstr("find", "nope") == NULL);

  {
    char *p[40];
    int j;
    for (i = 0; i < 40; ++i) {
      p[i] = malloc(10 + i);
      CHECK(p[i] != NULL && ((unsigned long)p[i] & 3) == 0);
      memset(p[i], i, 10 + i);
    }
    for (i = 0; i < 40; ++i)
      for (j = 0; j < 10 + i; ++j)
        CHECK(p[i][j] == (char)i);
    for (i = 0; i < 40; i += 2)
      free(p[i]);
    for (i = 1; i < 40; i += 2)
      free(p[i]);
    /* Everything is free again, so one big block must be available. */
    p[0] = malloc(3000);
    CHECK(p[0] != NULL);
    CHECK(malloc(3000) == NULL);
    free(p[0]);
    p[0] = malloc(1000);
    memset(p[0], 7, 1000);
    p[0] = realloc(p[0], 2000);
    CHECK(p[0] != NULL && p[0][999] == 7);
    free(p[0]);
    p[0] = calloc(10, 10);
    for (j = 0; j < 100; ++j)
      CHECK(p[0][j] == 0);
  }

  printf("host C logic: %d checks, %d failed\n", checks, failures);
  return failures != 0;
}
