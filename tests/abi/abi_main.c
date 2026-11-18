/*===-- abi_main.c - the ABI documented in docs/abi.md, checked --------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Compile time checks (_Static_assert) cover the sizes and alignments of the
 * types. At run time the program calls the assembly functions of abi_asm.s
 * with every kind of argument and result, calls C functions from assembly,
 * and looks at how values are laid out in memory. main returns 0 when all is
 * well, otherwise the line number of the first failed check.
 */

#include <stdarg.h>
#include <stddef.h>

/* ---- type sizes and alignments (checked by the compiler) ---------------- */

_Static_assert((char)-1 > 0, "plain char is unsigned");
_Static_assert(sizeof(char) == 1 && sizeof(short) == 2 && sizeof(int) == 2,
               "char, short, int");
_Static_assert(sizeof(long) == 4 && sizeof(long long) == 8, "long, long long");
_Static_assert(sizeof(float) == 8 && sizeof(double) == 8 &&
                   sizeof(long double) == 8,
               "floating point values take four words in memory");
_Static_assert(_Alignof(long) == 2 && _Alignof(long long) == 2 &&
                   _Alignof(double) == 2 && _Alignof(long double) == 2,
               "nothing is aligned to more than a word");
#ifdef __MERA400_LARGE__
_Static_assert(sizeof(void *) == 4 && sizeof(void (*)(void)) == 4 &&
                   sizeof(size_t) == 4 && sizeof(ptrdiff_t) == 4,
               "large model: 32 bit pointers and size_t");
#else
_Static_assert(sizeof(void *) == 2 && sizeof(void (*)(void)) == 2 &&
                   sizeof(size_t) == 2 && sizeof(ptrdiff_t) == 2,
               "small model: 16 bit pointers and size_t");
#endif
_Static_assert(sizeof(wchar_t) == 2, "wchar_t");

struct mixed { char a; int b; char c; };
_Static_assert(sizeof(struct mixed) == 6 && offsetof(struct mixed, b) == 2 &&
                   offsetof(struct mixed, c) == 4,
               "struct layout");
struct longs { char a; long l; };
_Static_assert(sizeof(struct longs) == 6 && offsetof(struct longs, l) == 2,
               "a long needs only word alignment");
struct dbl { char a; double d; };
_Static_assert(sizeof(struct dbl) == 10 && offsetof(struct dbl, d) == 2,
               "a double is 8 bytes and word aligned");
struct chars3 { char c[3]; };
_Static_assert(sizeof(struct chars3) == 3 && _Alignof(struct chars3) == 1,
               "char arrays are packed");

struct point { int x, y; };

/* ---- assembly functions (abi_asm.s) ------------------------------------- */

int asm_add3(int a, int b, int c);
long asm_ladd(long a, long b);
long long asm_ll_add(long long a, long long b);
int asm_sum_words(const int *p, int n);
int asm_byte_at(const char *p);
char *asm_buf_ptr(void);
extern char asm_buf[];
int asm_apply_twice(int (*f)(int), int x);
int asm_double(int x);
int asm_vsum(int n, ...);
int asm_call_vmixed(void);
double asm_dbl_twice(double x);
long asm_shl32(long x, int n);
int asm_point_sum(struct point p);
struct point asm_make_point(int x, int y);
int asm_sum_chars(unsigned char a, signed char b);
signed char asm_schar_minus2(void);
int asm_check_saved(void);
int asm_bump_counter(void);

/* ---- C functions called from assembly ------------------------------------ */

int c_counter;

int c_vmixed(int tag, ...) {
  va_list ap;
  va_start(ap, tag);
  int i = va_arg(ap, int);
  long l = va_arg(ap, long);
  double d = va_arg(ap, double);
  va_end(ap);
  return tag + i + (int)(l >> 16) + (int)(l & 0xFFFF) + (int)d;
}

/* Uses a lot of registers, so the compiler has to save r5 and r6 itself. */
int c_clobber(int a, int b, int c) {
  volatile int v[8];
  int s = 0;
  for (int i = 0; i < 8; i++)
    v[i] = a * i + b;
  for (int i = 0; i < 8; i++)
    s += v[i] * c + a;
  return s;
}

static int c_inc(int x) { return x + 1; }

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond))                                                               \
      return __LINE__;                                                         \
  } while (0)

static int int_args(void) {
  CHECK(asm_add3(1, 2, 3) == 6);
  CHECK(asm_add3(-1, 1, 0) == 0);
  CHECK(asm_ladd(0x0001FFFFL, 1L) == 0x00020000L);
  CHECK(asm_ladd(-1L, 1L) == 0L);
  CHECK(asm_ladd(0x12345678L, 0x11111111L) == 0x23456789L);
  CHECK(asm_ll_add(0x00010000FFFFFFFFLL, 1LL) == 0x0001000100000000LL);
  CHECK(asm_ll_add(-1LL, 1LL) == 0LL);
  CHECK(asm_ll_add(0x1111222233334444LL, 0x0101010101010101LL) ==
        0x1212232334344545LL);
  CHECK(asm_shl32(1L, 17) == 0x20000L);
  CHECK(asm_shl32(0x00010003L, 4) == 0x00100030L);
  CHECK(asm_sum_chars(200, -5) == 195);
  CHECK(asm_schar_minus2() == -2);
  signed char c = asm_schar_minus2();
  CHECK((int)c == -2);
  return 0;
}

static int pointers(void) {
  static int global_ints[4] = {1, 2, 3, 4};
  int local_ints[5] = {10, 20, 30, 40, 50};
  CHECK(asm_sum_words(global_ints, 4) == 10);
  CHECK(asm_sum_words(local_ints, 5) == 150);
  CHECK(asm_sum_words(local_ints + 2, 2) == 70);
  CHECK(asm_sum_words(local_ints, 0) == 0);

  /* Byte pointers: the first character of a string is the high byte. */
  CHECK(asm_byte_at(asm_buf) == 'A');
  CHECK(asm_byte_at(asm_buf + 1) == 'B');
  CHECK(asm_byte_at(asm_buf + 2) == 'C');
  CHECK(asm_byte_at(asm_buf + 3) == 'D');
  CHECK(asm_buf_ptr() == asm_buf);
  CHECK(asm_buf_ptr()[1] == 'B');
  const char *text = "xyz";
  CHECK(asm_byte_at(text) == 'x' && asm_byte_at(text + 1) == 'y' &&
        asm_byte_at(text + 2) == 'z');
  char buf[4] = {1, 2, 3, 4};
  CHECK(asm_byte_at(buf + 1) == 2 && asm_byte_at(buf + 2) == 3);
  return 0;
}

static int code_pointers(void) {
  CHECK(asm_apply_twice(asm_double, 3) == 12);
  CHECK(asm_apply_twice(c_inc, 5) == 7);
  int (*volatile f)(int) = c_inc;
  CHECK(asm_apply_twice(f, 40) == 42);
  return 0;
}

static int varargs(void) {
  CHECK(asm_vsum(0) == 0);
  CHECK(asm_vsum(3, 10, 20, 30) == 60);
  char small = 5;
  CHECK(asm_vsum(2, small, -1) == 4);
  CHECK(asm_call_vmixed() == 18);
  return 0;
}

static int aggregates(void) {
  struct point p = {3, 4};
  CHECK(asm_point_sum(p) == 7);
  CHECK(p.x == 3 && p.y == 4);
  struct point q = asm_make_point(5, 6);
  CHECK(q.x == 5 && q.y == 6);
  return 0;
}

static int callee_saved(void) {
  CHECK(asm_check_saved() == 1);
  c_counter = 41;
  CHECK(asm_bump_counter() == 42 && c_counter == 42);
  return 0;
}

/* Memory layout: big-endian words, floating point in four words. */
static int layout(void) {
  union {
    long l;
    unsigned short w[2];
    unsigned char b[4];
  } u;
  u.l = 0x11223344L;
  CHECK(u.w[0] == 0x1122 && u.w[1] == 0x3344);
  CHECK(u.b[0] == 0x11 && u.b[1] == 0x22 && u.b[2] == 0x33 && u.b[3] == 0x44);

  union {
    long long ll;
    unsigned short w[4];
  } v;
  v.ll = 0x0102030405060708LL;
  CHECK(v.w[0] == 0x0102 && v.w[3] == 0x0708);

  union {
    double d;
    float f;
    long double ld;
    unsigned short w[4];
  } f;
  f.d = 1.5;
  CHECK(f.w[0] == 0x6000 && f.w[1] == 0 && f.w[2] == 0x0001 && f.w[3] == 0);
  f.d = -2.0;
  CHECK(f.w[0] == 0x8000 && f.w[1] == 0 && f.w[2] == 0x0001 && f.w[3] == 0);
  f.f = 1.5f;
  CHECK(f.w[0] == 0x6000 && f.w[2] == 0x0001 && f.w[3] == 0);
  f.ld = 1.5L;
  CHECK(f.w[0] == 0x6000 && f.w[2] == 0x0001 && f.w[3] == 0);
  /* The padding word stays zero after arithmetic, and a double passes in
     three words, so a value made by asm_dbl_twice is a normal double. */
  volatile double x = 1.5;
  f.d = asm_dbl_twice(x);
  CHECK(f.d == 3.0);
  CHECK(f.w[0] == 0x6000 && f.w[2] == 0x0002 && f.w[3] == 0);
  CHECK(asm_dbl_twice(0.25) == 0.5);
  return 0;
}

int main(void) {
  int r;
  if ((r = int_args()) || (r = pointers()) || (r = code_pointers()) ||
      (r = varargs()) || (r = aggregates()) || (r = callee_saved()) ||
      (r = layout()))
    return r;
  return 0;
}
