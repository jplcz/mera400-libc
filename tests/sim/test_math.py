#===-- test_math.py - libm accuracy against Python's math module ----------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs tests/c/test_math.c, which prints each function's arguments and
result, and compares every line with Python's IEEE double result. The
target's double has a 39-bit mantissa (about 1e-12 relative accuracy), no
infinity and no NaN, so the expected values follow the library's conventions
(math.h): overflow gives 1.7e38, underflow 0, a domain error 0.

usage: test_math.py <program.elf> [large]
"""

import math
import sys

import m48ref
from libtest import Checker, ConsoleMem, Image
from m400sim import Cpu

HUGE = float(2**127 - 2**88)
TINY = 1.4693679e-39
REL = 3e-10
ABS = 2e-11


def dom0(f):
    def g(*a):
        try:
            return f(*a)
        except (ValueError, ZeroDivisionError):
            return 0.0
    return g


def safe_log(f):
    def g(x):
        if x == 0:
            return -HUGE
        if x < 0:
            return 0.0
        return f(x)
    return g


def pow_(x, y):
    if y == 0:
        return 1.0
    if x == 0:
        return 0.0 if y > 0 else HUGE
    if x < 0 and y != math.trunc(y):
        return 0.0
    try:
        return math.pow(x, y)
    except OverflowError:
        return HUGE if x > 0 or y % 2 == 0 else -HUGE
    except ValueError:
        return 0.0


def fmod_(x, y):
    return 0.0 if y == 0 else math.fmod(x, y)


def sqrt_(x):
    return 0.0 if x <= 0 else math.sqrt(x)


def round_(x):
    return math.copysign(math.floor(abs(x) + 0.5), x)


FUNCS = {
    "sqrt": sqrt_, "cbrt": lambda x: math.copysign(abs(x) ** (1 / 3), x) if x else 0.0,
    "floor": math.floor, "ceil": math.ceil, "trunc": math.trunc,
    "round": round_, "fabs": abs, "exp": lambda x: math.exp(x) if x < 700 else HUGE,
    "log": safe_log(math.log), "sin": math.sin, "cos": math.cos, "tan": math.tan,
    "atan": math.atan, "sinh": lambda x: math.sinh(x) if abs(x) < 700 else math.copysign(HUGE, x),
    "cosh": lambda x: math.cosh(x) if abs(x) < 700 else HUGE,
    "tanh": math.tanh, "log2": safe_log(math.log2), "log10": safe_log(math.log10),
    "exp2": lambda x: math.pow(2, x) if x < 500 else HUGE,
    "asin": dom0(math.asin), "acos": dom0(math.acos),
    "pow": pow_, "fmod": fmod_, "atan2": math.atan2, "hypot": math.hypot,
    "copysign": math.copysign,
}
TRIG = {"sin", "cos", "tan"}

large = len(sys.argv) > 2 and sys.argv[2] == "large"
ck = Checker("math")
img = Image(sys.argv[1])
cpu = Cpu(ConsoleMem(img.base, 0xFFFF if large else 0x7FFF))
cpu.ic = img.elf.symbols["__image_base"] // 2
code = cpu.run(limit=3_000_000_000)
ck.check(code == 0o77, "halt code %#o, want 0o77" % code)
ck.check(cpu.r[1] == 0, "in-program check at test_math.c line %d failed" % cpu.r[1])

worst = {}
for line in bytes(cpu.mem.output).decode("latin-1").splitlines():
    p = line.split()
    name = p[0]
    nums = [m48ref.to_float([int(v[i:i + 4], 16) for i in (0, 4, 8)]) for v in p[1:]]
    args, got = nums[:-1], nums[-1]
    if name in TRIG and abs(args[0]) > 1000:
        continue
    try:
        want = FUNCS[name](*args)
    except OverflowError:
        want = HUGE
    want = float(want)
    if abs(want) > HUGE:
        want = math.copysign(HUGE, want)
    if 0 < abs(want) < TINY:
        want = 0.0
    tol = REL * max(abs(want), 1.0) + ABS
    if name in TRIG or name == "pow":
        tol = 2e-9 * max(abs(want), 1.0)
    if name == "tan":
        tol = 2e-9 * max(abs(want), 1.0) ** 2
    err = abs(got - want)
    worst[name] = max(worst.get(name, 0.0), err / max(abs(want), 1.0))
    ck.check(err <= tol, "%s(%s) = %r, want %r" % (name, args, got, want))
print("worst relative error per function:",
      " ".join("%s=%.1e" % kv for kv in sorted(worst.items())))
ck.finish()
