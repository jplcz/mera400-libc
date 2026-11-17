#===-- test_m48.py - simulator tests of the 48-bit float helpers ----------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs the compiler helpers __m48_* and the 64-bit integer helpers (*di3)
on the simulator and compares every result, bit for bit, with the host
reference in m48ref.py (floats) or Python integer arithmetic (64-bit).

The arguments are placed the way the compiler's calling convention does it:
words go to r1, r2, r3 and then the stack; a 3-word double (with its 4th
padding word) or a 2-word long is never split between r3 and the stack; a
64-bit result comes back through a pointer to a result slot passed in r1.

usage: test_m48.py <image.elf> [--awp]    (an image containing libbuiltins.a;
       --awp: the image uses the AWP variant, which the simulator models)
"""

import random
import sys

import m48ref as ref
from libtest import Checker, Image

img = Image(sys.argv[1])
ck = Checker("m48")
rng = random.Random(48)
RES = 0x6000        # word address of the result slot for sret calls
PTR = 0x6100        # word address of the "int *" / "u64 *" output slot
M16 = 0xFFFF


def words(kind, v):
    if kind == "d":
        return list(v[:3]) + [0]
    if kind == "q":
        return [(v >> s) & M16 for s in (48, 32, 16, 0)]
    if kind == "l":
        return [(v >> 16) & M16, v & M16]
    return [v & M16]


def call(sym, args, sret=False, nres=0):
    """args: list of (kind, value), kind in d, q, l, w. Returns the machine."""
    regs, stack = [], []
    free = 3 - (1 if sret else 0)
    for kind, v in args:
        ws = words(kind, v)
        if kind == "d" and 3 <= free - len(regs):
            regs += ws[:3]
        elif kind in "dl" and len(ws) > free - len(regs):
            stack += ws
        elif kind == "q":
            while ws and len(regs) < free:
                regs.append(ws.pop(0))
            stack += ws
        elif len(regs) + len(ws) <= free:
            regs += ws
        else:
            stack += ws
    m = img.machine()
    if sret:
        regs = [RES] + regs
    m.call(sym, *regs, stack=stack, checker=ck, what=sym)
    return m


def fcall(sym, *ds, extra=()):
    args = [("d", d) for d in ds] + list(extra)
    m = call(sym, args)
    return tuple(m.cpu.r[1:4]) + (0,)


def int_call(sym, args, bits):
    m = call(sym, args, sret=bits == 64)
    if bits == 64:
        return sum(m.cpu.mem[RES + i] << (48 - 16 * i) for i in range(4))
    if bits == 32:
        return (m.cpu.r[1] << 16) | m.cpu.r[2]
    return m.cpu.r[1]


def mk(m40, e):
    """A native value from a signed 40-bit mantissa and exponent, normalised."""
    return ref.pack(*ref.norm(ref.s64(m40 << 24), e))


def rand_val(wide=False):
    m = rng.randrange(-(1 << 39), 1 << 39)
    if rng.random() < 0.1:
        m = rng.choice([0, 1 << 38, -(1 << 39), (1 << 39) - 1, -(1 << 38) - 1])
    e = rng.randrange(-128, 128) if wide else rng.randrange(-30, 31)
    return mk(m, e)


EDGE = [ref.pack(0, 0), mk(1 << 38, 1), mk(-(1 << 38), 1), mk(1 << 38, 0), mk(-(1 << 39), 0),
        mk(3 << 37, 2), mk((1 << 39) - 1, 127), mk(-(1 << 39), 127),
        mk(1 << 38, -128), mk(-(1 << 39), -128), mk((1 << 38) + 1, 40),
        mk(-(1 << 38) - 1, -40), mk(0x5555555555, 3), mk(-0x2AAAAAAAAB, 3)]

pairs = [(a, b) for a in EDGE for b in EDGE]
pairs += [(rand_val(), rand_val()) for _ in range(150)]
pairs += [(rand_val(True), rand_val(True)) for _ in range(40)]
for _ in range(40):
    a = rand_val()
    am, ae = ref.unpack(a)
    # near cancellation and exponent differences around the 40-bit limit
    pairs.append((a, ref.neg(a)))
    pairs.append((a, mk((am >> 24) ^ rng.randrange(1, 8), ae)))
    pairs.append((a, mk(rng.randrange(-(1 << 39), 1 << 39), ae - rng.randrange(37, 43))))
rng.shuffle(pairs)

# The host reference against Python's float, so that the reference itself is
# not just a copy of the C code's mistakes.
for a, b in pairs[:200]:
    fa, fb = ref.to_float(a), ref.to_float(b)
    for op, fn in (("add", lambda x, y: x + y), ("sub", lambda x, y: x - y),
                   ("mul", lambda x, y: x * y)):
        r = ref.to_float(getattr(ref, op)(a, b))
        w = fn(fa, fb)
        if abs(w) < 1e-35 or abs(w) > 1e35:
            continue
        # Cancellation can lose accuracy relative to the result, never
        # relative to the operands.
        bound = abs(w) if op == "mul" else abs(fa) + abs(fb)
        ck.check(abs(r - w) <= bound * 2.0 ** -37, "ref %s %r %r" % (op, a, b))

for a, b in pairs:
    for op in ("add", "sub", "mul", "div"):
        got = fcall("__m48_" + op, a, b)
        ck.check(got == getattr(ref, op)(a, b),
                 "%s %s %s: got %s want %s" % (op, a, b, got, getattr(ref, op)(a, b)))
    m = call("__m48_cmp", [("d", a), ("d", b)])
    ck.check(m.cpu.r[1] in (ref.cmp(a, b) & M16,), "cmp %s %s -> %d" % (a, b, m.cpu.r[1]))

for a in EDGE + [rand_val(True) for _ in range(60)]:
    ck.check(fcall("__m48_neg", a) == ref.neg(a), "neg %s" % (a,))

for n in range(-6, 14):
    for x in (mk(3 << 37, 1), mk(-(5 << 37), 0), mk(1 << 38, 1)):
        got = fcall("__m48_powi", x, extra=[("w", n)])
        want = ref.from_int(1)
        k = abs(n)
        xx = x
        while k:
            if k & 1:
                want = ref.mul(want, xx)
            k >>= 1
            if k:
                xx = ref.mul(xx, xx)
        if n < 0:
            want = ref.div(ref.from_int(1), want)
        ck.check(got == want, "powi %s %d: got %s want %s" % (x, n, got, want))

ints = [0, 1, -1, 2, 3, 255, 256, 32767, -32768, 65535, 65536, 1 << 31, (1 << 31) - 1,
        -(1 << 31), (1 << 32) - 1, 1 << 38, (1 << 39) - 1, 1 << 39, (1 << 39) + 1,
        (1 << 40) + (1 << 0), (1 << 40) + (1 << 1), (1 << 40) + 3, (1 << 41) + 6,
        (1 << 53) + 1, (1 << 62), (1 << 63) - 1, -(1 << 63), (1 << 64) - 1, -(1 << 40) - 1,
        (1 << 40) + (1 << 1) - 1]
ints += [rng.randrange(-(1 << 63), 1 << 64) >> rng.randrange(0, 64) for _ in range(120)]
for v in ints:
    if -(1 << 31) <= v < (1 << 31):
        m = call("__m48_fromi32", [("l", v & 0xFFFFFFFF)])
        ck.check(tuple(m.cpu.r[1:4]) + (0,) == ref.from_int(v), "fromi32 %d" % v)
    if 0 <= v < (1 << 32):
        m = call("__m48_fromu32", [("l", v)])
        ck.check(tuple(m.cpu.r[1:4]) + (0,) == ref.from_int(v), "fromu32 %d" % v)
    if -(1 << 63) <= v < (1 << 63):
        m = call("__m48_fromi64", [("q", v & ((1 << 64) - 1))])
        ck.check(tuple(m.cpu.r[1:4]) + (0,) == ref.from_int(v), "fromi64 %d" % v)
    if 0 <= v < (1 << 64):
        m = call("__m48_fromu64", [("q", v)])
        ck.check(tuple(m.cpu.r[1:4]) + (0,) == ref.from_int(v), "fromu64 %d" % v)

conv = EDGE + [rand_val() for _ in range(80)]
conv += [mk(rng.randrange(-(1 << 39), 1 << 39), rng.randrange(0, 70)) for _ in range(60)]
conv += [mk(rng.randrange(-(1 << 39), 1 << 39), rng.randrange(-45, 1)) for _ in range(40)]
conv += [ref.from_int(v) for v in (1 << 31, (1 << 31) - 1, -(1 << 31), -(1 << 31) - 1, 1 << 32,
                                  (1 << 32) - 1, 1 << 63, (1 << 63) - 1, -(1 << 63), 1 << 64)]
for a in conv:
    for sym, bits, lo, hi in (("__m48_toi32", 32, -(1 << 31), (1 << 31) - 1),
                              ("__m48_tou32", 32, 0, (1 << 32) - 1),
                              ("__m48_toi64", 64, -(1 << 63), (1 << 63) - 1),
                              ("__m48_tou64", 64, 0, (1 << 64) - 1)):
        got = int_call(sym, [("d", a)], bits)
        want = ref.to_int(a, lo, hi) & ((1 << bits) - 1)
        ck.check(got == want, "%s %s: got %#x want %#x" % (sym, a, got, want))

ck.finish()
