/*===-- test_mem.c - mem* and str* functions on the target -----------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Compiled for the MERA-400 and run on the simulator (tests/sim/test_c.py;
 * main returns 0 or the source line of the first failed check). Every
 * routine is compared with a plain byte loop for all small sizes, all four
 * combinations of odd and even addresses and all overlaps. The same checks
 * run on a global buffer (low memory) and on a buffer on the stack, which in
 * the large memory model is above word 0x8000, where only whole-word access
 * works.
 */
#include <string.h>

static int first_failure;
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond) && !first_failure)                                           \
      first_failure = __LINE__;                                              \
  } while (0)

#define SIZE 48

static unsigned char want[SIZE]; /* the expected contents */
static unsigned seed = 12345;

static unsigned char rnd(void) {
  seed = seed * 25173u + 13849u;
  return (unsigned char)((seed >> 8) | 1u); /* never zero: usable in strings */
}

static void fill(unsigned char *p) {
  int i;
  for (i = 0; i < SIZE; ++i)
    p[i] = rnd() | (i & 1 ? 0x80 : 0);
}

static void ref_move(unsigned char *d, const unsigned char *s, int n) {
  unsigned char tmp[16];
  int i;
  for (i = 0; i < n; ++i)
    tmp[i] = s[i];
  for (i = 0; i < n; ++i)
    d[i] = tmp[i];
}

static int same(const unsigned char *a, const unsigned char *b) {
  int i;
  for (i = 0; i < SIZE; ++i)
    if (a[i] != b[i])
      return 0;
  return 1;
}

static int sign(int x) { return (x > 0) - (x < 0); }

static void run(unsigned char *A, unsigned char *B) {
  int n, da, sa, delta, i;

  /* memcpy between two buffers, memset */
  for (n = 0; n <= 15; ++n)
    for (da = 0; da < 4; ++da)
      for (sa = 0; sa < 4; ++sa) {
        fill(A);
        fill(B);
        for (i = 0; i < SIZE; ++i)
          want[i] = A[i];
        ref_move(want + da + 8, B + sa + 8, n);
        CHECK(memcpy(A + da + 8, B + sa + 8, n) == A + da + 8);
        CHECK(same(A, want));
      }
  for (n = 0; n <= 15; ++n)
    for (da = 0; da < 4; ++da) {
      fill(A);
      for (i = 0; i < SIZE; ++i)
        want[i] = A[i];
      for (i = 0; i < n; ++i)
        want[da + 8 + i] = 0xA5;
      CHECK(memset(A + da + 8, 0x1A5, n) == A + da + 8);
      CHECK(same(A, want));
    }

  /* memmove: every overlap, both directions */
  for (n = 0; n <= 15; ++n)
    for (da = 0; da < 2; ++da)
      for (delta = -9; delta <= 9; ++delta) {
        fill(A);
        for (i = 0; i < SIZE; ++i)
          want[i] = A[i];
        ref_move(want + 16 + da + delta, A + 16 + da, n);
        CHECK(memmove(A + 16 + da + delta, A + 16 + da, n) == A + 16 + da + delta);
        CHECK(same(A, want));
      }

  /* memcmp and memchr: every position of the first difference */
  for (n = 1; n <= 11; ++n)
    for (da = 0; da < 2; ++da)
      for (sa = 0; sa < 2; ++sa)
        for (i = -1; i < n; ++i) {
          int r;
          fill(A);
          for (r = 0; r < SIZE; ++r)
            B[r] = A[r];
          /* the second buffer is shifted to the alignment sa */
          for (r = n - 1; r >= 0; --r)
            B[8 + sa + r] = A[8 + da + r];
          if (i >= 0)
            B[8 + sa + i] ^= (i & 1) ? 0x40 : 0x81;
          r = memcmp(A + 8 + da, B + 8 + sa, n);
          if (i < 0)
            CHECK(r == 0);
          else
            CHECK(sign(r) == sign((int)A[8 + da + i] - (int)B[8 + sa + i]));
          CHECK(memchr(A + 8 + da, A[8 + da + n - 1], n) != 0);
        }

  /* string functions */
  for (n = 0; n <= 9; ++n)
    for (da = 0; da < 4; ++da) {
      fill(A);
      A[8 + da + n] = 0;
      CHECK(strlen(A + 8 + da) == (size_t)n);
      for (sa = 0; sa < 2; ++sa) {
        for (i = 0; i < SIZE; ++i)
          B[i] = 0x55;
        CHECK(strcpy(B + 8 + sa, (char *)A + 8 + da) == (char *)B + 8 + sa);
        CHECK(strcmp((char *)A + 8 + da, (char *)B + 8 + sa) == 0);
        CHECK(B[8 + sa + n] == 0 && B[8 + sa + n + 1] == 0x55);
        if (n) {
          B[8 + sa + n - 1] ^= 0x10;
          CHECK(strcmp((char *)A + 8 + da, (char *)B + 8 + sa) != 0);
          CHECK(strncmp((char *)A + 8 + da, (char *)B + 8 + sa, n - 1) == 0);
          CHECK(strncmp((char *)A + 8 + da, (char *)B + 8 + sa, n) != 0);
          CHECK(strchr((char *)A + 8 + da, A[8 + da + n - 1]) != 0);
        }
        CHECK(strchr((char *)A + 8 + da, 0) == (char *)A + 8 + da + n);
        CHECK(strrchr((char *)A + 8 + da, 0) == (char *)A + 8 + da + n);
      }
      /* strncpy pads with zeros, strcat/strncat append */
      for (i = 0; i < SIZE; ++i)
        B[i] = 0x55;
      CHECK(strncpy((char *)B + 8, (char *)A + 8 + da, 12) == (char *)B + 8);
      for (i = 0; i < 12; ++i)
        CHECK(B[8 + i] == (i < n ? A[8 + da + i] : 0));
      CHECK(B[20] == 0x55);
      B[8] = 'x';
      B[9] = 0;
      CHECK(strcat((char *)B + 8, (char *)A + 8 + da) == (char *)B + 8);
      CHECK(strlen((char *)B + 8) == (size_t)(n + 1));
      B[8] = 'y';
      B[9] = 0;
      strncat((char *)B + 8, (char *)A + 8 + da, 3);
      CHECK(strlen((char *)B + 8) == (size_t)(n < 3 ? n + 1 : 4));
    }
}

static unsigned char glob_a[SIZE], glob_b[SIZE];

int main(void) {
  unsigned char la[SIZE], lb[SIZE];
  run(glob_a, glob_b); /* low memory */
  run(la, lb);         /* the stack: above word 0x8000 in the large model */
  run(la, glob_b);
  return first_failure;
}
