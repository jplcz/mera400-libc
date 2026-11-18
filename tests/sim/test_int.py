#===-- test_int.py - simulator tests of the 32- and 64-bit integer helpers ==#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs the 32-bit helpers (__ashlsi3 __lshrsi3 __ashrsi3 __mulsi3 __udivsi3
__umodsi3 __divsi3 __modsi3 __mulosi4) and the 64-bit helpers (__ashldi3
__lshrdi3 __ashrdi3 __muldi3 __udivdi3 __umoddi3 __divdi3 __moddi3
__udivmoddi4 __mulodi4) on the simulator against Python integer arithmetic.

Calling convention: words go to r1, r2, r3 and then the stack; a
2-word long that does not fit in what is left of r1..r3 goes whole to the
stack, and once an argument is on the stack all later ones are too; a 4-word
long long may be split between r3 and the stack; a long long result comes
back through a (word) pointer to a result slot passed in r1. Shift counts
are 32-bit values. C data pointers (the `int *overflow`, `u64 *rem`) are byte
pointers: 16 bits in the small model, 32 bits (high word first) with --large.

usage: test_int.py <image.elf> [--large]   (an image containing libbuiltins.a)
"""

import random
import sys

from libtest import Checker, Image

img = Image(sys.argv[1])
LARGE = "--large" in sys.argv
ck = Checker("int")
rng = random.Random(32)
RES = 0x6000        # word address of the result slot for sret calls
PTR = 0x6100        # word address of the "int *" / "u64 *" output slot
M16, M32, M64 = 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF


def words(kind, v):
    if kind == "b":                      # C data pointer: a byte address
        return [v * 2 & M16] if not LARGE else [0, v * 2 & M16]
    n = {"l": 2, "q": 4, "w": 1}[kind]
    return [(v >> (16 * (n - 1 - i))) & M16 for i in range(n)]


def call(sym, args, sret=False):
    regs, stack = [], []
    free = 3 - (1 if sret else 0)
    for kind, v in args:
        ws = words(kind, v)
        if kind == "q" and not stack:
            while ws and len(regs) < free:
                regs.append(ws.pop(0))
            stack += ws
        elif not stack and len(regs) + len(ws) <= free:
            regs += ws
        else:
            stack += ws
    m = img.machine()
    if sret:
        regs = [RES] + regs
    m.call(sym, *regs, stack=stack, checker=ck, what=sym)
    return m


def r32(m):
    return (m.cpu.r[1] << 16) | m.cpu.r[2]


def r64(m, base=RES):
    return sum(m.cpu.mem[base + i] << (48 - 16 * i) for i in range(4))


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def s64(v):
    return v - (1 << 64) if v >> 63 else v


def tdiv(a, b):
    """C division (toward zero) and remainder."""
    q = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        q = -q
    return q, a - q * b


E32 = [0, 1, 2, 3, 0xFF, 0x100, 0xFFFF, 0x10000, 0x10001, 0x7FFFFFFF, 0x80000000,
       0x80000001, 0xFFFFFFFE, 0xFFFFFFFF, 0x12345678, 0xDEADBEEF]
E64 = [0, 1, 2, 0xFFFF, 0x10000, 0xFFFFFFFF, 1 << 32, (1 << 32) + 1, 0x7FFFFFFFFFFFFFFF,
       1 << 63, (1 << 63) + 1, M64, M64 - 1, 0x123456789ABCDEF0, 0xDEADBEEFCAFEF00D,
       (1 << 48) - 1, 1 << 48]


def vec32(n):
    out = [(a, b) for a in E32 for b in E32]
    out += [(rng.getrandbits(32), rng.getrandbits(32)) for _ in range(n)]
    out += [(rng.getrandbits(32), rng.getrandbits(rng.randrange(1, 32))) for _ in range(n)]
    return out


def vec64(n):
    out = [(a, b) for a in E64[::2] for b in E64]
    out += [(rng.getrandbits(64), rng.getrandbits(64)) for _ in range(n)]
    out += [(rng.getrandbits(64), rng.getrandbits(rng.randrange(1, 64))) for _ in range(n)]
    return out


# --- 32-bit ----------------------------------------------------------------
for a, b in vec32(60):
    ck.check(r32(call("__mulsi3", [("l", a), ("l", b)])) == (a * b) & M32, "mulsi3 %#x %#x" % (a, b))
    if b == 0:
        ck.check(r32(call("__udivsi3", [("l", a), ("l", b)])) == M32, "udivsi3 by 0")
        ck.check(r32(call("__umodsi3", [("l", a), ("l", b)])) == a, "umodsi3 by 0")
        continue
    ck.check(r32(call("__udivsi3", [("l", a), ("l", b)])) == a // b, "udivsi3 %#x %#x" % (a, b))
    ck.check(r32(call("__umodsi3", [("l", a), ("l", b)])) == a % b, "umodsi3 %#x %#x" % (a, b))
    if not (a == 0x80000000 and b == M32):
        q, r = tdiv(s32(a), s32(b))
        ck.check(r32(call("__divsi3", [("l", a), ("l", b)])) == q & M32, "divsi3 %#x %#x" % (a, b))
        ck.check(r32(call("__modsi3", [("l", a), ("l", b)])) == r & M32, "modsi3 %#x %#x" % (a, b))

counts = list(range(0, 36)) + [0x7FFFFFFF, 0x80000000, M32]
for a in E32 + [rng.getrandbits(32) for _ in range(10)]:
    for n in counts:
        ck.check(r32(call("__ashlsi3", [("l", a), ("l", n)])) == ((a << n) & M32 if n < 32 else 0),
                 "ashlsi3 %#x %#x" % (a, n))
        ck.check(r32(call("__lshrsi3", [("l", a), ("l", n)])) == (a >> n if n < 32 else 0),
                 "lshrsi3 %#x %#x" % (a, n))
        ck.check(r32(call("__ashrsi3", [("l", a), ("l", n)])) == (s32(a) >> min(n, 31)) & M32,
                 "ashrsi3 %#x %#x" % (a, n))

for a, b in vec32(40):
    m = call("__mulosi4", [("l", a), ("l", b), ("b", PTR)])
    p = s32(a) * s32(b)
    ck.check(r32(m) == p & M32, "mulosi4 result %#x %#x" % (a, b))
    ck.check(m.cpu.mem[PTR] == (0 if -(1 << 31) <= p < (1 << 31) else 1),
             "mulosi4 overflow %#x %#x -> %d" % (a, b, m.cpu.mem[PTR]))

# --- 64-bit ----------------------------------------------------------------
for a, b in vec64(40):
    ck.check(r64(call("__muldi3", [("q", a), ("q", b)], True)) == (a * b) & M64,
             "muldi3 %#x %#x" % (a, b))
    m = call("__udivmoddi4", [("q", a), ("q", b), ("b", PTR)], True)
    if b == 0:
        ck.check(r64(m) == M64 and r64(m, PTR) == a, "udivmoddi4 by 0")
        continue
    ck.check(r64(m) == a // b and r64(m, PTR) == a % b, "udivmoddi4 %#x %#x" % (a, b))
    ck.check(r64(call("__udivdi3", [("q", a), ("q", b)], True)) == a // b, "udivdi3 %#x %#x" % (a, b))
    ck.check(r64(call("__umoddi3", [("q", a), ("q", b)], True)) == a % b, "umoddi3 %#x %#x" % (a, b))
    if not (a == 1 << 63 and b == M64):
        q, r = tdiv(s64(a), s64(b))
        ck.check(r64(call("__divdi3", [("q", a), ("q", b)], True)) == q & M64, "divdi3 %#x %#x" % (a, b))
        ck.check(r64(call("__moddi3", [("q", a), ("q", b)], True)) == r & M64, "moddi3 %#x %#x" % (a, b))

counts = list(range(0, 70, 1)) + [0x7FFFFFFF, 0x80000000, M32]
for a in E64 + [rng.getrandbits(64) for _ in range(6)]:
    for n in counts:
        ck.check(r64(call("__ashldi3", [("q", a), ("l", n)], True)) == ((a << n) & M64 if n < 64 else 0),
                 "ashldi3 %#x %#x" % (a, n))
        ck.check(r64(call("__lshrdi3", [("q", a), ("l", n)], True)) == (a >> n if n < 64 else 0),
                 "lshrdi3 %#x %#x" % (a, n))
        ck.check(r64(call("__ashrdi3", [("q", a), ("l", n)], True)) == (s64(a) >> min(n, 63)) & M64,
                 "ashrdi3 %#x %#x" % (a, n))

for a, b in vec64(30):
    m = call("__mulodi4", [("q", a), ("q", b), ("b", PTR)], True)
    p = s64(a) * s64(b)
    ck.check(r64(m) == p & M64, "mulodi4 result %#x %#x" % (a, b))
    ck.check(m.cpu.mem[PTR] == (0 if -(1 << 63) <= p < (1 << 63) else 1),
             "mulodi4 overflow %#x %#x -> %d" % (a, b, m.cpu.mem[PTR]))

ck.finish()
