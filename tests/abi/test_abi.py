#===-- test_abi.py - run the ABI test program on the simulator ------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Starts a linked program (crt0 + the C and assembly sources of tests/abi +
libc + builtins) at its first word. main returns 0 when every check passed,
otherwise the source line of the first failed check; crt0 then halts with
"hlt 077" and r1 = that value.

usage: test_abi.py <program.elf> <name of the C or C++ source file>
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sim"))

from libtest import Checker, Image  # noqa: E402
from m400sim import Cpu  # noqa: E402

ck = Checker("abi")
img = Image(sys.argv[1])
cpu = Cpu(list(img.base))
cpu.ic = img.elf.symbols["__image_base"] // 2
# Software floating point and 64-bit arithmetic take many steps.
code = cpu.run(limit=200_000_000)
ck.check(code == 0o77, "halt code %#o, want 0o77" % code)
ck.check(cpu.r[1] == 0, "first failed check is at %s line %d" % (sys.argv[2], cpu.r[1]))
ck.finish()
