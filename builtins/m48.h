/*===-- m48.h - shared code of the software 48-bit float helpers ----------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * The native "double" of the MERA-400 (also called m48) has three words:
 *
 *   word 0, word 1, high byte of word 2   40-bit two's complement mantissa m
 *   low byte of word 2                    8-bit two's complement exponent e
 *
 * and the value is m * 2^e / 2^39 (the mantissa is a fraction in [-1, 1)).
 * "Normalised" means the two top bits of the mantissa differ: positive
 * numbers have 2^38 <= m < 2^39, negative ones -2^39 <= m < -2^38. Zero is
 * all zero bits. A fourth word exists only in memory (padding).
 *
 * The software helpers copy the algorithms of the AWP (the hardware floating
 * point unit) bit for bit, so that the software and the AWP variants of the
 * library give identical results, including the rounding:
 *   - the mantissa is kept in the TOP 40 bits of a 64-bit integer (the low
 *     24 bits are working room), like the EM400 emulator does;
 *   - results are rounded to nearest by adding half a unit of the last place
 *     after the operation, and the final mantissa is truncated to 40 bits.
 *
 * Where the hardware raises an interrupt the software helpers choose a
 * result instead (documented in builtins/README.md):
 *   overflow             -> the largest finite number with the right sign
 *   underflow            -> zero
 *   division by zero     -> the largest finite number with the sign of the
 *                           dividend (0 / 0 gives 0)
 *   unnormalised input   -> the operand is normalised first
 */
#ifndef M48_H
#define M48_H

typedef long long i64;
typedef unsigned long long u64;

typedef union {
  double d;
  unsigned short w[4];
} m48_bits;

typedef struct {
  i64 m; /* mantissa in the top 40 bits */
  int e;
} m48;

#define M48_BIT0 ((i64)0x8000000000000000ULL)
#define M48_BIT40 ((i64)0x800000)

/* Shifts of a 64-bit value by a variable count (the compiler calls
   __ashrdi3, __lshrdi3 and __ashldi3 from di3.c for them). */
static inline i64 m48_sar(i64 v, int n) { return v >> n; }

static inline u64 m48_shr(u64 v, int n) { return v >> n; }

static inline u64 m48_shl(u64 v, int n) { return v << n; }

/* True when the two top bits differ (the normalised form). */
static inline int m48_normalised(i64 m) { return ((m ^ (m << 1)) & M48_BIT0) != 0; }

static inline void m48_norm(m48 *x) {
  while (x->m != 0 && !m48_normalised(x->m)) {
    x->m <<= 1;
    x->e -= 1;
  }
}

static inline void m48_denorm(m48 *x, int shift) {
  x->m = m48_sar(x->m, shift);
  x->e += shift;
}

static inline m48 m48_unpack(double d) {
  m48_bits b;
  m48 x;
  b.d = d;
  x.m = ((i64)b.w[0] << 48) | ((i64)b.w[1] << 32) | ((i64)(b.w[2] & 0xFF00) << 16);
  x.e = (signed char)(b.w[2] & 0xFF);
  if (x.m == 0)
    x.e = 0;
  else
    m48_norm(&x);
  return x;
}

static inline double m48_pack(m48 x) {
  m48_bits b;
  if (x.m != 0 && x.e > 127) {
    x.m = x.m < 0 ? (i64)0x8000000001000000ULL : (i64)0x7FFFFFFFFF000000ULL;
    x.e = 127;
  } else if (x.m == 0 || x.e < -128) {
    x.m = 0;
    x.e = 0;
  }
  b.w[0] = (unsigned short)(x.m >> 48);
  b.w[1] = (unsigned short)(x.m >> 32);
  b.w[2] = (unsigned short)(((x.m >> 16) & 0xFF00) | (x.e & 0xFF));
  b.w[3] = 0;
  return b.d;
}

/* Round (when bit 40 of the working mantissa is set) and normalise. */
static inline void m48_round(m48 *x) {
  m48_norm(x);
  if (x->m & M48_BIT40) {
    m48_denorm(x, 1);
    x->m += M48_BIT40;
    m48_norm(x);
  }
}

#endif
