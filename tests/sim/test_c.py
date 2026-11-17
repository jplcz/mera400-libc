#===-- test_c.py - run the C test program on the simulator ----------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Starts a linked program (crt0 + tests/c/test_c.c + libc + builtins) at its
first word. main returns 0 when all checks passed, otherwise the source line
of the first failed check; crt0 then halts with "hlt 077" and r1 = that value.

usage: test_c.py <program.elf>
"""

import sys

from libtest import Checker, Image
from m400sim import Cpu

ck = Checker("c")
img = Image(sys.argv[1])
cpu = Cpu(list(img.base))
cpu.ic = img.elf.symbols["__image_base"] // 2
# The C code and the software 32-bit arithmetic take many steps.
code = cpu.run(limit=200_000_000)
ck.check(code == 0o77, "halt code %#o, want 0o77" % code)
ck.check(cpu.r[1] == 0, "first failed check is at test_c.c line %d" % cpu.r[1])
ck.finish()
