/*===-- shapes.cpp - C++ with new, delete and virtual functions ------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Freestanding C++ (no exceptions, no RTTI, no standard library): objects are
 * allocated with `new` (src/cxx, on top of malloc), used through a base
 * class, and destroyed with a virtual destructor. main returns the sum of the
 * areas, 12 + 9 = 21, in R1.
 */
struct Shape {
  virtual int area() const = 0;
  virtual ~Shape() {}
};

struct Rectangle : Shape {
  int w, h;
  Rectangle(int w, int h) : w(w), h(h) {}
  int area() const override { return w * h; }
};

struct Square : Shape {
  int side;
  explicit Square(int s) : side(s) {}
  int area() const override { return side * side; }
};

// extern "C": the build passes -ffreestanding, and then Clang mangles main
// while crt0 calls "main"
int main() {
  Shape *shapes[2] = {new Rectangle(3, 4), new Square(3)};
  int total = 0;
  for (Shape *s : shapes) {
    total += s->area();
    delete s;
  }
  return total;
}
