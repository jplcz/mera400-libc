/*===-- di3.c - 64-bit multiplication, division, shifts -------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * The 64-bit ("di" = double integer) helpers. They are rare in real programs,
 * so they are plain C. Inside, a 64-bit number is a pair of 32-bit halves and
 * only 32-bit operations are used: a 64-bit operation here would make the
 * compiler call the very helper being written. 64-bit values appear only at
 * the interface, to split into / join from halves (shifts by 32 are free).
 *
 * Division by zero gives quotient all ones and the dividend as remainder,
 * like the 16- and 32-bit helpers (C leaves it undefined).
 */

typedef unsigned long u32;
typedef unsigned long long u64;
typedef long long i64;

typedef struct {
  u32 hi, lo;
} pair;

static pair split(u64 x) {
  pair p = {(u32)(x >> 32), (u32)x};
  return p;
}

static u64 join(pair p) { return ((u64)p.hi << 32) | p.lo; }

/* 32 x 32 -> 64 bit product from 16-bit limbs. */
static pair mul32(u32 a, u32 b) {
  unsigned al = (unsigned)a, ah = (unsigned)(a >> 16);
  unsigned bl = (unsigned)b, bh = (unsigned)(b >> 16);
  u32 ll = (u32)al * bl, lh = (u32)al * bh, hl = (u32)ah * bl,
      hh = (u32)ah * bh;
  u32 mid = (ll >> 16) + (lh & 0xFFFF) + (hl & 0xFFFF);
  pair r;
  r.lo = (ll & 0xFFFF) | (mid << 16);
  r.hi = hh + (lh >> 16) + (hl >> 16) + (mid >> 16);
  return r;
}

u64 __muldi3(u64 a, u64 b) {
  pair x = split(a), y = split(b);
  pair r = mul32(x.lo, y.lo);
  r.hi += x.hi * y.lo + x.lo * y.hi;
  return join(r);
}

static int ge(pair a, pair b) {
  return a.hi > b.hi || (a.hi == b.hi && a.lo >= b.lo);
}

static u64 udivmod(u64 n, u64 d, u64 *rem) {
  pair nn = split(n), dd = split(d), q = {0, 0}, r = {0, 0};
  if (!dd.hi && !dd.lo) {
    if (rem)
      *rem = n;
    return ~(u64)0;
  }
  for (int i = 0; i < 64; ++i) {
    r.hi = (r.hi << 1) | (r.lo >> 31);
    r.lo = (r.lo << 1) | (nn.hi >> 31);
    nn.hi = (nn.hi << 1) | (nn.lo >> 31);
    nn.lo <<= 1;
    q.hi = (q.hi << 1) | (q.lo >> 31);
    q.lo <<= 1;
    if (ge(r, dd)) {
      u32 borrow = r.lo < dd.lo;
      r.lo -= dd.lo;
      r.hi -= dd.hi + borrow;
      q.lo |= 1;
    }
  }
  if (rem)
    *rem = join(r);
  return join(q);
}

u64 __udivmoddi4(u64 n, u64 d, u64 *rem) { return udivmod(n, d, rem); }
u64 __udivdi3(u64 n, u64 d) { return udivmod(n, d, 0); }

u64 __umoddi3(u64 n, u64 d) {
  u64 r;
  udivmod(n, d, &r);
  return r;
}

static u64 neg(u64 x) { return join((pair){~split(x).hi + !split(x).lo, -split(x).lo}); }
static int isneg(i64 x) { return split((u64)x).hi >> 31; }

i64 __divdi3(i64 n, i64 d) {
  int sign = isneg(n) != isneg(d);
  u64 q = udivmod(isneg(n) ? neg(n) : n, isneg(d) ? neg(d) : d, 0);
  return sign ? neg(q) : q;
}

i64 __moddi3(i64 n, i64 d) {
  u64 r;
  udivmod(isneg(n) ? neg(n) : n, isneg(d) ? neg(d) : d, &r);
  return isneg(n) ? neg(r) : r;
}

u64 __ashldi3(u64 a, int n) {
  pair x = split(a);
  if (n >= 64)
    return 0;
  if (n >= 32) {
    x.hi = x.lo << (n - 32);
    x.lo = 0;
  } else if (n > 0) {
    x.hi = (x.hi << n) | (x.lo >> (32 - n));
    x.lo <<= n;
  }
  return join(x);
}

u64 __lshrdi3(u64 a, int n) {
  pair x = split(a);
  if (n >= 64)
    return 0;
  if (n >= 32) {
    x.lo = x.hi >> (n - 32);
    x.hi = 0;
  } else if (n > 0) {
    x.lo = (x.lo >> n) | (x.hi << (32 - n));
    x.hi >>= n;
  }
  return join(x);
}

i64 __ashrdi3(i64 a, int n) {
  pair x = split((u64)a);
  long fill = (long)x.hi < 0 ? -1L : 0;
  if (n >= 64) {
    x.hi = x.lo = (u32)fill;
  } else if (n >= 32) {
    x.lo = (u32)((long)x.hi >> (n - 32));
    x.hi = (u32)fill;
  } else if (n > 0) {
    x.lo = (x.lo >> n) | (x.hi << (32 - n));
    x.hi = (u32)((long)x.hi >> n);
  }
  return join(x);
}

/* Signed multiplication with overflow flag: *overflow is set to 1 when the
   true product does not fit the result type. The result is the wrapped
   product either way. */
long __mulosi4(long a, long b, int *overflow) {
  i64 p = (i64)a * b;
  *overflow = p != (i64)(long)p;
  return (long)p;
}

i64 __mulodi4(i64 a, i64 b, int *overflow) {
  u64 ua = isneg(a) ? neg(a) : a, ub = isneg(b) ? neg(b) : b;
  u64 p = __muldi3(ua, ub);
  int negative = isneg(a) != isneg(b);
  int ov = 0;
  if (ua && ub) {
    ov = udivmod(p, ub, 0) != ua || (split(p).hi >> 31 && !(negative && p == (u64)1 << 63));
  }
  *overflow = ov;
  return __muldi3(a, b);
}
