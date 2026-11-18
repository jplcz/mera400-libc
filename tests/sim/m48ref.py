#===-- m48ref.py - host-side reference of the MERA-400 native floating point -===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""The native 48-bit floating point format ("m48") on the host.

A value is three 16-bit words (w0, w1, w2):
  mantissa = 40-bit two's complement: w0, w1 and the high byte of w2
  exponent = 8-bit two's complement: the low byte of w2
  value    = mantissa / 2**39 * 2**exponent        (mantissa/2**39 in [-1, 1))
Normalised: the two top mantissa bits differ (|mantissa/2**39| in [1/2, 1),
except that -1 is allowed). Zero is all bits zero.

add/sub/mul/div follow the algorithm of the EM400 emulator's AWP (src/awp.c
of the "emawp" project, which models the hardware): the mantissa is held in
the top 40 bits of a 64-bit signed integer. What the hardware answers with an
interrupt, this reference answers with the choice the library makes (see
builtins/README.md): saturate on overflow, flush to zero on underflow, divide
by zero gives the largest number with the dividend's sign.

The conversions are written differently on purpose (exact fractions), so a
mistake in the C code and a mistake in the algorithm do not cancel out.
"""

from fractions import Fraction

BIT0 = 1 << 63
BIT40 = 1 << 23
POS_MAX = (0x7FFFFFFFFF << 24, 127)
NEG_MAX = (-(2**39 - 1) << 24, 127)


def s64(x):
    x &= (1 << 64) - 1
    return x - (1 << 64) if x & BIT0 else x


def normalised(m):
    return ((m ^ s64(m << 1)) & BIT0) != 0


def norm(m, e):
    while m != 0 and not normalised(m):
        m = s64(m << 1)
        e -= 1
    return m, e


def unpack(w):
    m = s64((w[0] << 48) | (w[1] << 32) | ((w[2] & 0xFF00) << 16))
    e = w[2] & 0xFF
    e = e - 256 if e & 0x80 else e
    if m == 0:
        return 0, 0
    return norm(m, e)


def pack(m, e):
    if m != 0 and e > 127:
        m, e = POS_MAX if m > 0 else NEG_MAX
    elif m == 0 or e < -128:
        m, e = 0, 0
    mu = m & ((1 << 64) - 1)
    return ((mu >> 48) & 0xFFFF, (mu >> 32) & 0xFFFF,
            ((mu >> 16) & 0xFF00) | (e & 0xFF), 0)


def rnd(m, e):
    m, e = norm(m, e)
    if m & BIT40:
        m, e = m >> 1, e + 1
        m += BIT40
        m, e = norm(s64(m), e)
    return m, e


def addsub(a, b, sub):
    m1, e1 = unpack(a)
    m2, e2 = unpack(b)
    d = e1 - e2
    if d < 0:
        if d <= -40:
            m1, e1 = 0, e2
        else:
            m1, e1 = m1 >> -d, e2
    elif d > 0:
        if d >= 40:
            m2, e2 = 0, e1
        else:
            m2, e2 = m2 >> d, e1
    m1, m2 = m1 >> 1, m2 >> 1
    e = e1 + 1
    m = m1 - m2 if sub else m1 + m2
    return pack(*rnd(m, e))


def add(a, b):
    return addsub(a, b, 0)


def sub(a, b):
    return addsub(a, b, 1)


def mul(a, b):
    m1, e1 = unpack(a)
    m2, e2 = unpack(b)
    m1, m2 = m1 >> 1, m2 >> 1
    e = e1 + 1 + e2 + 1
    neg = 0
    if m1 < 0:
        neg ^= 1
        m1 = -m1
    if m2 < 0:
        neg ^= 1
        m2 = -m2
    m2 >>= 23
    m = 0
    for _ in range(41):
        m >>= 1
        if m2 & 1:
            m += m1
        m2 >>= 1
    m = -m if neg else m
    return pack(*rnd(m, e))


def div(a, b):
    m1, e1 = unpack(a)
    m2, e2 = unpack(b)
    if m2 == 0:
        return pack(m1, 128) if m1 else pack(0, 0)
    m1, m2 = m1 >> 2, m2 >> 1
    e = e1 + 2 - (e2 + 1)
    neg = 0
    if m1 < 0:
        neg ^= 1
        m1 = -m1
    if m2 < 0:
        neg ^= 1
        m2 = -m2
    rem, q = m1, 0
    for _ in range(41):
        rem -= m2
        q = s64(q << 1)
        if rem < 0:
            rem += m2
        else:
            q |= BIT40
        rem = s64(rem << 1)
    m = s64(-q) if neg else q
    return pack(*norm(m, e))


def neg(a):
    m, e = unpack(a)
    return pack(*norm(-m, e)) if m != -BIT0 else pack(1 << 62, e + 1)


def cmp(a, b):
    x, y = to_fraction(a), to_fraction(b)
    return -1 if x < y else 1 if x > y else 0


def to_fraction(w):
    m, e = unpack(w)
    return Fraction(m, 1 << 63) * Fraction(2) ** e


def to_float(w):
    return float(to_fraction(w))


def from_fraction(x):
    """Nearest (ties to even) native value of an exact fraction."""
    if x == 0:
        return pack(0, 0)
    sign = -1 if x < 0 else 1
    x = abs(x)
    e = 0
    while x >= 1:
        x /= 2
        e += 1
    while x < Fraction(1, 2):
        x *= 2
        e -= 1
    scaled = x * (1 << 39)
    q = scaled.numerator // scaled.denominator
    r = scaled - q
    if r > Fraction(1, 2) or (r == Fraction(1, 2) and q & 1):
        q += 1
    if q == 1 << 39:
        q, e = 1 << 38, e + 1
    return pack(*norm(s64(sign * q << 24), e))


def from_int(n):
    return from_fraction(Fraction(n))


def to_int(w, lo, hi):
    """Truncate toward zero, saturate to [lo, hi]."""
    v = to_fraction(w)
    n = v.numerator // v.denominator if v >= 0 else -((-v).numerator // (-v).denominator)
    return max(lo, min(hi, n))
