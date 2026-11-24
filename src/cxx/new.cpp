/*===-- new.cpp - operator new, operator delete and the new handler --------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Allocation for C++ programs, on top of malloc and free. There are no
 * exceptions, so a failed `new` cannot throw std::bad_alloc: after calling the
 * new handler (if any) it ends the program with std::terminate. The nothrow
 * forms return a null pointer instead.
 *
 * A program may replace any of these functions by defining its own.
 * Nothing here is declared in a header: <new> comes from the C++ standard
 * library (libc++), which declares the same names.
 */
#include <stdlib.h>

namespace std {
struct nothrow_t {
  explicit nothrow_t() = default;
};
extern const nothrow_t nothrow;
const nothrow_t nothrow{};

typedef void (*new_handler)();
[[noreturn]] void terminate() noexcept;

static new_handler handler;
new_handler set_new_handler(new_handler h) noexcept {
  new_handler old = handler;
  handler = h;
  return old;
}
new_handler get_new_handler() noexcept { return handler; }
} // namespace std

typedef __SIZE_TYPE__ size_type;

// malloc(0) may return null, but new must give every object its own address.
static void *allocate(size_type n) {
  for (;;) {
    void *p = malloc(n ? n : 1);
    if (p)
      return p;
    if (!std::handler)
      return nullptr;
    std::handler();
  }
}

void *operator new(size_type n) {
  void *p = allocate(n);
  if (!p)
    std::terminate();
  return p;
}
void *operator new[](size_type n) { return operator new(n); }
void *operator new(size_type n, const std::nothrow_t &) noexcept {
  return allocate(n);
}
void *operator new[](size_type n, const std::nothrow_t &) noexcept {
  return allocate(n);
}

void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_type) noexcept { free(p); }
void operator delete[](void *p, size_type) noexcept { free(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { free(p); }
