#===-- libtest.py - helpers for simulator based tests ---------------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Common code: loading an image, calling a function the way compiled C
code does (arguments in r1..r3, return address in r4), and checking that
only the expected memory changed."""

import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from m400sim import Cpu, Elf, Halted  # noqa: E402

STACK_TOP = 0x1000      # word address: the stack grows down from here
STACK_ROOM = 16         # words below STACK_TOP a function may use
RET_WORD = 0x0010       # word holding "hlt 077": the caller's return address
HLT_77 = 0xEC3F
DATA_LO = 0x2200        # byte addresses from here up are free for test data
SENTINEL = {5: 0x1234, 6: 0x5678}


class LoggedMem(list):
    """Word memory that remembers which words were written."""

    def __init__(self, items):
        super().__init__(items)
        self.written = set()

    def __setitem__(self, i, v):
        self.written.add(i)
        super().__setitem__(i, v)


class Checker:
    def __init__(self, name):
        self.name = name
        self.count = 0
        self.failures = []

    def check(self, ok, what):
        self.count += 1
        if not ok:
            self.failures.append(what)
            if len(self.failures) <= 20:
                print("FAIL:", what)

    def finish(self):
        if self.failures:
            print("%s: %d of %d checks FAILED" % (self.name, len(self.failures), self.count))
            sys.exit(1)
        print("%s: %d checks passed" % (self.name, self.count))


class Image:
    """A linked ELF image plus a junk-filled memory it is loaded into."""

    def __init__(self, path, seed=400):
        self.elf = Elf(path)
        rng = random.Random(seed)
        # Junk everywhere: a function that wrongly relies on zeroed memory,
        # or reads past the end of a string, fails.
        self.base = [rng.randrange(0x10000) for _ in range(0x10000)]
        self.elf.load(self.base)
        self.base[RET_WORD] = HLT_77

    def word(self, symbol):
        return self.elf.symbols[symbol] // 2

    def machine(self):
        return Machine(self)


class Machine:
    def __init__(self, image):
        self.image = image
        self.cpu = Cpu(LoggedMem(image.base))

    # Byte access (byte address = word * 2 + octet, octet 0 = high).
    def put(self, addr, data):
        for i, b in enumerate(data):
            self.put_byte(addr + i, b)

    def put_byte(self, addr, b):
        mem = self.cpu.mem
        w = (addr >> 1) & 0x7FFF
        v = mem[w]
        mem[w] = ((b << 8) | (v & 0xFF)) if not addr & 1 else ((v & 0xFF00) | b)

    def byte(self, addr, mem=None):
        mem = mem if mem is not None else self.cpu.mem
        w = mem[(addr >> 1) & 0x7FFF]
        return (w & 0xFF) if addr & 1 else (w >> 8)

    def get(self, addr, n):
        return bytes(self.byte(addr + i) for i in range(n))

    def call(self, symbol, *args, expect_writes=None, what=None, checker=None):
        """Call symbol(args...) and verify the effects.

        expect_writes maps byte address -> value for every byte the function
        is allowed (and required) to change; any other change outside the
        stack room is a failure. Returns r1.
        """
        cpu = self.cpu
        init = list(cpu.mem)
        cpu.mem.written.clear()
        cpu.steps = 0
        cpu.skip = False
        for i, a in enumerate(args):
            cpu.r[1 + i] = a & 0xFFFF
        cpu.r[4] = RET_WORD
        cpu.r[5], cpu.r[6] = SENTINEL[5], SENTINEL[6]
        cpu.r[7] = STACK_TOP
        cpu.ic = self.image.word(symbol)
        problems = []
        try:
            code = cpu.run()
        except Exception as e:  # runaway, unmodelled instruction, ...
            problems.append("%s" % e)
            code = None
        if code is not None and code != 0o77:
            problems.append("halted with %#o instead of returning" % code)
        if cpu.ic != RET_WORD + 1 and code == 0o77:
            problems.append("did not return to the caller")
        for reg, name in ((5, "r5"), (6, "r6")):
            if cpu.r[reg] != SENTINEL[reg]:
                problems.append("%s not preserved" % name)
        if cpu.r[7] != STACK_TOP:
            problems.append("r7 not restored: %#x" % cpu.r[7])
        if expect_writes is not None:
            problems += self._memory_diff(init, expect_writes)
        if checker is not None:
            checker.check(not problems, "%s: %s" % (what or symbol, "; ".join(problems)))
        return cpu.r[1]

    def _memory_diff(self, init, expect):
        problems = []
        cpu = self.cpu
        touched = set()
        for w in cpu.mem.written:
            if STACK_TOP - STACK_ROOM <= w < STACK_TOP:
                continue
            touched.add(2 * w)
            touched.add(2 * w + 1)
        touched.update(a & 0xFFFF for a in expect)
        for addr in sorted(touched):
            old = self.byte(addr, init)
            want = expect.get(addr, old)
            got = self.byte(addr)
            if got != want:
                problems.append("byte %#06x is %#04x, expected %#04x" % (addr, got, want))
                if len(problems) >= 3:
                    break
        return problems


def signed16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v
