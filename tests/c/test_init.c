/*===-- test_init.c - constructors, destructors, atexit, exit --------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===----------------------------------------------------------------------===
 *
 * Every step prints one letter; tests/sim/test_init.py compares the whole
 * string. Expected: constructors by priority (lowest first), then the
 * default-priority ones in link order, main, then on exit the atexit
 * handlers last-registered-first and finally the destructors (.fini_array)
 * in reverse priority order.
 *
 * With -DTEST_ABORT main calls abort() instead of returning: no handler may
 * run and the processor stops with hlt 040.
 */
#include <stdio.h>
#include <stdlib.h>

static int global_value = 7;

__attribute__((constructor(102))) static void ctor_b(void) { putchar('b'); }
__attribute__((constructor(101))) static void ctor_a(void) { putchar('a'); }
__attribute__((constructor)) static void ctor_c(void) {
  putchar('c');
  global_value = 42;
}
__attribute__((destructor(101))) static void dtor_y(void) { putchar('y'); }
__attribute__((destructor(102))) static void dtor_x(void) { putchar('x'); }

static void at1(void) { putchar('1'); }
static void at2(void) { putchar('2'); }

int main(void) {
  putchar('m');
  putchar(global_value == 42 ? 'k' : '?');
  atexit(at1);
  atexit(at2);
#ifdef TEST_ABORT
  abort();
#endif
  return 3;
}
