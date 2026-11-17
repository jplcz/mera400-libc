#===-- test_builtins.py - simulator tests of the integer helpers ----------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs __ashlhi3 __lshrhi3 __ashrhi3 __mulhi3 __udivhi3 __umodhi3
__divhi3 __modhi3 on the simulator against Python integer arithmetic.

usage: test_builtins.py <image.elf>   (an image containing libbuiltins.a)
"""

import random
import sys

from libtest import Checker, Image, signed16

rng = random.Random(2)
img = Image(sys.argv[1])
ck = Checker("builtins")
M = 0xFFFF

edge = [0, 1, 2, 3, 7, 8, 0xFF, 0x100, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF,
        10, 100, 1000, 0x1234, 0xABCD]
pairs = [(a, b) for a in edge for b in edge]
pairs += [(rng.randrange(0x10000), rng.randrange(0x10000)) for _ in range(400)]
pairs += [(rng.randrange(0x10000), rng.randrange(1, 300)) for _ in range(200)]


def run(fn, a, b):
    m = img.machine()
    return m.call(fn, a, b, expect_writes={}, what="%s(%#x,%#x)" % (fn, a, b), checker=ck)


def ref_div(a, b):
    sa, sb = signed16(a), signed16(b)
    q = abs(sa) // abs(sb)
    if (sa < 0) != (sb < 0):
        q = -q
    return q, sa - q * sb


for a, b in pairs:
    ck.check(run("__mulhi3", a, b) == (a * b) & M, "mul %#x %#x" % (a, b))
    if b == 0:
        # Division by zero is undefined in C; ours gives all ones and the
        # dividend as remainder for the unsigned helpers.
        ck.check(run("__udivhi3", a, b) == M, "udiv by zero")
        ck.check(run("__umodhi3", a, b) == a, "umod by zero")
        run("__divhi3", a, b)
        run("__modhi3", a, b)
        continue
    ck.check(run("__udivhi3", a, b) == a // b, "udiv %#x %#x" % (a, b))
    ck.check(run("__umodhi3", a, b) == a % b, "umod %#x %#x" % (a, b))
    q, r = ref_div(a, b)
    if not (signed16(a) == -0x8000 and signed16(b) == -1):
        ck.check(run("__divhi3", a, b) == q & M, "div %d %d" % (signed16(a), signed16(b)))
        ck.check(run("__modhi3", a, b) == r & M, "mod %d %d" % (signed16(a), signed16(b)))
    else:
        ck.check(run("__divhi3", a, b) == 0x8000, "div INT_MIN / -1 wraps")
        ck.check(run("__modhi3", a, b) == 0, "mod INT_MIN % -1")

for a in edge + [rng.randrange(0x10000) for _ in range(40)]:
    for n in range(0, 20):
        ck.check(run("__ashlhi3", a, n) == (a << n) & M if n < 16 else run("__ashlhi3", a, n) == 0,
                 "ashl %#x %d" % (a, n))
        ck.check(run("__lshrhi3", a, n) == (a >> n if n < 16 else 0), "lshr %#x %d" % (a, n))
        want = (signed16(a) >> min(n, 15)) & M
        ck.check(run("__ashrhi3", a, n) == want, "ashr %#x %d" % (a, n))

ck.finish()
