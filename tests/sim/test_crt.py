#===-- test_crt.py - run crt0 and a tiny main on the simulator ------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Starts the image at its first word (as the CPU does after a reset) and
checks the whole start-up path: stack set from __stack_top, .bss cleared
(memory was pre-filled with junk), main called, its result in r1, the final
halt "hlt 077". Also checks that the flat binary equals the loaded ELF.

usage: test_crt.py <program.elf> <program.bin>
"""

import sys

from libtest import Checker, Image
from m400sim import Cpu

ck = Checker("crt0")
img = Image(sys.argv[1])
sym = img.elf.symbols
base = sym["__image_base"] // 2

cpu = Cpu(list(img.base))
cpu.ic = base                      # the CPU begins at the image start
code = cpu.run()
ck.check(code == 0o77, "halt code %#o, want 0o77" % code)
ck.check(cpu.r[1] == 47, "main result r1 = %d, want 47" % cpu.r[1])
ck.check(cpu.r[7] == sym["__stack_top"] // 2, "stack pointer %#x" % cpu.r[7])
ck.check(cpu.mem[sym["counter"] // 2] == 1, "main's write to .bss")
lo, hi = sym["__bss_start"] // 2, sym["__bss_end"] // 2
ck.check(hi - lo >= 1, "bss not empty")
ck.check(all(cpu.mem[w] == (1 if w == sym["counter"] // 2 else 0) for w in range(lo, hi)),
         ".bss cleared")
ck.check(cpu.mem[hi] == img.base[hi], "word after .bss untouched")
ck.check(sym["_start"] == sym["__image_base"], "_start at image base")

# The flat binary must be exactly the bytes at the image base.
with open(sys.argv[2], "rb") as f:
    flat = f.read()
mem = [0] * 0x10000
img.elf.load(mem)
want = b"".join(bytes([mem[base + i] >> 8, mem[base + i] & 0xFF]) for i in range(len(flat) // 2))
ck.check(len(flat) % 2 == 0 and flat == want, "flat binary equals the ELF image")
first = (flat[0] << 8) | flat[1]
ck.check(first == mem[base], "first word of the binary is crt0's first instruction")

ck.finish()
