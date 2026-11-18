/*===-- abi_cpp.cpp - the C++ part of docs/abi.md, checked -------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Virtual calls from assembly, member function pointer layout, vtable layout
 * and a static constructor that calls assembly. main returns 0 when all is
 * well, otherwise the line number of the first failed check.
 */

#ifdef __cpp_rtti
#error "RTTI must be off on this target"
#endif
#ifdef __cpp_exceptions
#error "exceptions must be off on this target"
#endif

struct Counter {
  int n;
  virtual int add(int x);
  virtual int sub(int x);
  int plain(int x);
};

int Counter::add(int x) { return n + x; }
int Counter::sub(int x) { return n - x; }
int Counter::plain(int x) { return n * x; }

struct Derived : Counter {
  int add(int x) override { return n + 100 + x; }
};

extern "C" {
int asm_call_plain(Counter *c, int x);
int asm_call_virtual0(Counter *c, int x);
unsigned asm_vtable_header(Counter *c);
int asm_seed(void);
}

struct Seeded {
  int v;
  Seeded() : v(asm_seed()) {}
};
static Seeded g_seeded;

static_assert(sizeof(Counter) == sizeof(void *) + 2,
              "an object starts with a vptr, which is as wide as a pointer");
static_assert(sizeof(int (Counter::*)(int)) == 2 * sizeof(void *),
              "a member function pointer is a pointer and an adjustment");

union MemberPointer {
  int (Counter::*m)(int);
  unsigned short w[sizeof(int (Counter::*)(int)) / 2];
};
constexpr unsigned kWords = sizeof(int (Counter::*)(int)) / 2;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond))                                                               \
      return __LINE__;                                                         \
  } while (0)

// The build passes -ffreestanding, so main must be declared extern "C" to
// keep its name (crt0 calls "main").
extern "C" int main() {
  CHECK(g_seeded.v == 0x5EED);

  Counter c;
  c.n = 7;
  Derived d;
  d.n = 7;
  CHECK(asm_call_plain(&c, 4) == 28);
  CHECK(asm_call_virtual0(&c, 3) == 10);
  CHECK(asm_call_virtual0(&d, 3) == 110);
  CHECK(asm_vtable_header(&c) == 0);
  CHECK(asm_vtable_header(&d) == 0);

  // ARM-style member function pointers: {pointer, adjustment}. A virtual
  // function stores the byte offset of its vtable slot in the first half and
  // sets the low bit of the adjustment in the second half; a plain function
  // stores its address and a zero adjustment.
  MemberPointer vadd, vsub, plain;
  vadd.m = &Counter::add;
  vsub.m = &Counter::sub;
  plain.m = &Counter::plain;
  CHECK(vadd.w[kWords / 2 - 1] == 0 && vadd.w[kWords - 1] == 1);
  CHECK(vsub.w[kWords / 2 - 1] == sizeof(void *) && vsub.w[kWords - 1] == 1);
  CHECK(plain.w[kWords - 1] == 0);
  CHECK(plain.w[kWords / 2 - 1] != 0);

  Counter *p = &d;
  CHECK((p->*vadd.m)(1) == 108);
  CHECK((p->*vsub.m)(1) == 6);
  CHECK((p->*plain.m)(2) == 14);
  return 0;
}
