#===-- test_init.py - start-up/exit order of a whole program --------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs a complete program (crt0, constructors, main, exit) on the simulator
with a memory-mapped console and compares the printed letters.

usage: test_init.py <program.elf> <expected output> <halt code, octal>
                    <exit status> <model: small|large>
"""

import sys

from libtest import Checker, ConsoleMem, Image
from m400sim import Cpu

elf, expected, halt, status, model = sys.argv[1:6]
ck = Checker("init")
img = Image(elf)
cpu = Cpu(ConsoleMem(img.base, 0xFFFF if model == "large" else 0x7FFF))
cpu.ic = img.elf.symbols["__image_base"] // 2
code = cpu.run(limit=2_000_000)
out = bytes(cpu.mem.output).decode("latin-1")
ck.check(out == expected, "output %r, want %r" % (out, expected))
ck.check(code == int(halt, 8), "halt code %#o, want %#o" % (code, int(halt, 8)))
if int(status) >= 0:
    ck.check(cpu.r[1] == int(status), "exit status %d, want %s" % (cpu.r[1], status))
ck.finish()
