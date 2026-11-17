#===-- test_string.py - simulator tests of the assembly string routines ---===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs every string function on the simulator and compares the result and
every changed byte with a Python reference.

usage: test_string.py <image.elf>   (an image containing libc.a)

Covered: odd/even alignment of every pointer, n = 0, overlapping memmove in
both directions, bytes >= 0x80, objects that end at the last byte of the
address space, and padding by strncpy.
"""

import random
import sys

from libtest import Checker, DATA_LO, Image

rng = random.Random(1)
img = Image(sys.argv[1])
ck = Checker("string")
TOP = 0x10000


def fresh():
    return img.machine()


def rnd_bytes(n, nonzero=False, high=False):
    lo = 1 if nonzero else 0
    out = bytes(rng.randrange(lo, 256) for _ in range(n))
    if high and n:
        out = bytes([0x80 | b if i % 2 else b for i, b in enumerate(out)])
        if nonzero:
            out = bytes(b or 0x81 for b in out)
    return out


def cstr(n):
    return rnd_bytes(n, nonzero=True, high=rng.random() < 0.5) + b"\0"


def sgn(x):
    return (x > 0) - (x < 0)


# --- memcpy / memmove --------------------------------------------------
for n in list(range(0, 12)) + [31, 32, 33, 100]:
    for da in (0, 1):
        for sa in (0, 1):
            m = fresh()
            d, s = 0x3000 + da, 0x5000 + sa
            data = rnd_bytes(n, high=True)
            m.put(s, data)
            r = m.call("memcpy", d, s, n,
                       expect_writes={d + i: data[i] for i in range(n)},
                       what="memcpy n=%d da=%d sa=%d" % (n, da, sa), checker=ck)
            ck.check(r == d, "memcpy returns dest")

for n in list(range(0, 10)) + [40]:
    for delta in range(-9, 10):
        for al in (0, 1):
            m = fresh()
            base = 0x4000 + al
            s = base + 10
            d = s + delta
            data = rnd_bytes(n, high=True)
            m.put(s, data)
            r = m.call("memmove", d, s, n,
                       expect_writes={d + i: data[i] for i in range(n)},
                       what="memmove n=%d delta=%d al=%d" % (n, delta, al), checker=ck)
            ck.check(r == d, "memmove returns dest")

# End of address space.
m = fresh()
data = rnd_bytes(7, high=True)
m.put(0xFFFF - 6, data)
m.call("memmove", 0xFFFF - 6 - 20, 0xFFFF - 6, 7,
       expect_writes={0xFFFF - 26 + i: data[i] for i in range(7)},
       what="memmove from top", checker=ck)
m = fresh()
m.put(0x6000, data)
m.call("memcpy", 0xFFFF - 6, 0x6000, 7,
       expect_writes={0xFFFF - 6 + i: data[i] for i in range(7)},
       what="memcpy to top", checker=ck)

# --- memset ------------------------------------------------------------
for n in list(range(0, 10)) + [33, 64]:
    for da in (0, 1):
        for c in (0, 0x41, 0x80, 0xFF, 0x1A5):
            m = fresh()
            d = 0x3000 + da
            r = m.call("memset", d, c, n,
                       expect_writes={d + i: c & 0xFF for i in range(n)},
                       what="memset n=%d da=%d c=%#x" % (n, da, c), checker=ck)
            ck.check(r == d, "memset returns dest")

# --- memcmp / strcmp / strncmp ----------------------------------------
def diff(a, b):
    for x, y in zip(a, b):
        if x != y:
            return x - y
    return 0


for trial in range(300):
    n = rng.randrange(0, 12)
    a = rnd_bytes(n, high=True)
    b = bytearray(a)
    if n and rng.random() < 0.7:
        b[rng.randrange(n)] = rng.randrange(256)
    b = bytes(b)
    aa, ba = 0x3000 + rng.randrange(2), 0x5000 + rng.randrange(2)
    m = fresh()
    m.put(aa, a)
    m.put(ba, b)
    r = m.call("memcmp", aa, ba, n, expect_writes={}, what="memcmp", checker=ck)
    ck.check(sgn(r if r < 0x8000 else r - 0x10000) == sgn(diff(a, b)) and
             (r & 0xFFFF) == (diff(a, b) & 0xFFFF),
             "memcmp %r %r -> %d" % (a, b, r))

for trial in range(300):
    a = cstr(rng.randrange(0, 10))
    b = bytearray(a)
    if rng.random() < 0.7 and len(b) > 1:
        i = rng.randrange(len(b) - 1)
        b[i] = rng.randrange(1, 256)
    if rng.random() < 0.3:
        b = bytearray(cstr(rng.randrange(0, 10)))
    b = bytes(b)
    aa, ba = 0x3000 + rng.randrange(2), 0x5000 + rng.randrange(2)
    n = rng.randrange(0, 12)
    for fn, lim in (("strcmp", None), ("strncmp", n)):
        m = fresh()
        m.put(aa, a)
        m.put(ba, b)
        # reference: compare up to the NUL (and n)
        ref = 0
        for i in range(len(a) + len(b)):
            if lim is not None and i >= lim:
                break
            x = a[i] if i < len(a) else 0
            y = b[i] if i < len(b) else 0
            if x != y:
                ref = x - y
                break
            if x == 0:
                break
        args = (aa, ba) if lim is None else (aa, ba, n)
        r = m.call(fn, *args, expect_writes={}, what=fn, checker=ck)
        ck.check((r & 0xFFFF) == (ref & 0xFFFF), "%s %r %r n=%s -> %d, want %d" % (fn, a, b, lim, r, ref))

# --- memchr ------------------------------------------------------------
for trial in range(300):
    n = rng.randrange(0, 12)
    a = rnd_bytes(n, high=True)
    c = rng.choice(list(a)) if a and rng.random() < 0.7 else rng.randrange(256)
    aa = 0x3000 + rng.randrange(2)
    m = fresh()
    m.put(aa, a)
    r = m.call("memchr", aa, c, n, expect_writes={}, what="memchr", checker=ck)
    i = a.find(bytes([c]))
    ck.check(r == (aa + i if i >= 0 else 0), "memchr %r %#x -> %#x" % (a, c, r))

# --- strlen / strchr / strrchr ----------------------------------------
for n in range(0, 12):
    for al in (0, 1):
        s = cstr(n)
        m = fresh()
        m.put(0x3000 + al, s)
        r = m.call("strlen", 0x3000 + al, expect_writes={}, what="strlen", checker=ck)
        ck.check(r == n, "strlen %d -> %d" % (n, r))
for trial in range(300):
    s = cstr(rng.randrange(0, 10))
    c = rng.choice(list(s)) if rng.random() < 0.7 else rng.randrange(256)
    al = rng.randrange(2)
    m = fresh()
    m.put(0x3000 + al, s)
    i = s.find(bytes([c]))
    j = s.rfind(bytes([c]))
    r = m.call("strchr", 0x3000 + al, c, expect_writes={}, what="strchr", checker=ck)
    ck.check(r == (0x3000 + al + i if i >= 0 else 0), "strchr %r %#x -> %#x" % (s, c, r))
    r = m.call("strrchr", 0x3000 + al, c, expect_writes={}, what="strrchr", checker=ck)
    ck.check(r == (0x3000 + al + j if j >= 0 else 0), "strrchr %r %#x -> %#x" % (s, c, r))

# --- strcpy / strncpy / strcat / strncat -------------------------------
for trial in range(300):
    s = cstr(rng.randrange(0, 10))
    d0 = cstr(rng.randrange(0, 8))
    da, sa = 0x3000 + rng.randrange(2), 0x5000 + rng.randrange(2)
    n = rng.randrange(0, 14)

    m = fresh()
    m.put(sa, s)
    r = m.call("strcpy", da, sa, expect_writes={da + i: s[i] for i in range(len(s))},
               what="strcpy", checker=ck)
    ck.check(r == da, "strcpy returns dest")

    m = fresh()
    m.put(sa, s)
    body = (s + b"\0" * n)[:n]
    r = m.call("strncpy", da, sa, n, expect_writes={da + i: body[i] for i in range(n)},
               what="strncpy n=%d %r" % (n, s), checker=ck)
    ck.check(r == da, "strncpy returns dest")

    m = fresh()
    m.put(sa, s)
    m.put(da, d0)
    at = da + len(d0) - 1
    r = m.call("strcat", da, sa, expect_writes={at + i: s[i] for i in range(len(s))},
               what="strcat", checker=ck)
    ck.check(r == da, "strcat returns dest")

    m = fresh()
    m.put(sa, s)
    m.put(da, d0)
    part = s[:-1][:n] + b"\0"
    r = m.call("strncat", da, sa, n, expect_writes={at + i: part[i] for i in range(len(part))},
               what="strncat n=%d %r" % (n, s), checker=ck)
    ck.check(r == da, "strncat returns dest")

ck.finish()
