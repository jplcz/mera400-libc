/*===-- misc.c - errno, assertion failure, abs, rand -----------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <errno.h>
#include <stdlib.h>

int errno;

void __assert_fail(const char *expr, const char *file, int line) {
  (void)expr;
  (void)file;
  (void)line;
  abort();
}

int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }

/* A linear congruential generator in 16-bit arithmetic only (a 16-bit
   multiply is a helper call, a 32-bit one is much slower). The low bits of
   such a generator are weak, so rand() returns the high 15 bits. */
static unsigned int rand_state = 1;

void srand(unsigned int seed) { rand_state = seed; }

int rand(void) {
  rand_state = rand_state * 25173u + 13849u;
  return (int)(rand_state >> 1);
}
