/*===-- test_cxx.cpp - the C++ run-time: new, delete, local statics -------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Expected output: "abcdef". Each letter marks a check that passed; a
 * failed check prints nothing, so the output would be shorter.
 */
extern "C" int putchar(int c);

namespace std {
struct nothrow_t {
  explicit nothrow_t() = default;
};
extern const nothrow_t nothrow;
} // namespace std
typedef __SIZE_TYPE__ size_type;
void *operator new(size_type, const std::nothrow_t &) noexcept;
void operator delete(void *, size_type) noexcept;

struct Base {
  virtual int value() const = 0;
  virtual ~Base() {}
};
struct Derived : Base {
  int v;
  explicit Derived(int x) : v(x) {}
  int value() const override { return v; }
};

static int once() {
  putchar('e');
  return 5;
}
static int local_static() {
  static int x = once();
  return x;
}

extern "C" int main() {
  int *p = new int(7);
  if (*p == 7)
    putchar('a');
  delete p;

  int *a = new int[10];
  for (int i = 0; i < 10; i++)
    a[i] = i;
  if (a[9] == 9)
    putchar('b');
  delete[] a;

  void *none = operator new((size_type)-1, std::nothrow);
  if (!none)
    putchar('c');

  Base *b = new Derived(42);
  if (b->value() == 42)
    putchar('d');
  delete b; // virtual destructor, then sized operator delete

  // The initialiser runs once ('e' is printed once), however often it is called.
  if (local_static() + local_static() == 10)
    putchar('f');
  return 0;
}
