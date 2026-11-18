/*===-- test_init_cpp.cpp - C++ static constructors and destructors --------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===----------------------------------------------------------------------===
 *
 * Global objects of a freestanding C++ program: their constructors run
 * before main (from .init_array), their destructors after it (through
 * __cxa_atexit when the compiler uses it, or .fini_array; see exit.s).
 * Expected output: "AB" "m" and then the destructors "ba".
 */
extern "C" int putchar(int c);

struct Letter {
  int c;
  explicit Letter(int ch) : c(ch) { putchar(ch); }
  ~Letter() { putchar(c + ('a' - 'A')); }
};

static Letter first('A');
static Letter second('B');

// extern "C": the build passes -ffreestanding, and then Clang mangles main
// (_Z4mainv) while crt0 calls "main"
extern "C" int main() {
  putchar('m');
  return 0;
}
