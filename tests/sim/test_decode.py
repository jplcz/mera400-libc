#===-- test_decode.py - cross-check the simulator's decoder ---------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""The simulator has its own instruction decoder. This test disassembles
every function in the given images with llvm-objdump and checks that the
simulator agrees on the mnemonic and the length of every instruction, so
a decoder slip cannot make a wrong test pass.

usage: test_decode.py <llvm-objdump> <image.elf>...
"""

import re
import subprocess
import sys

from libtest import Checker
from m400sim import decode

ck = Checker("decode")
line = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} ?)+)\s+(\w+)")
total = 0
for elf in sys.argv[2:]:
    out = subprocess.run([sys.argv[1], "-d", elf], capture_output=True,
                         text=True, check=True).stdout
    for l in out.splitlines():
        m = line.match(l)
        if not m:
            continue
        raw = bytes.fromhex(m.group(2).replace(" ", ""))
        word = (raw[0] << 8) | raw[1]
        name, kind = decode(word)
        length = 2 if kind != "norm" or (word & 7) else 4
        ck.check(name == m.group(3) and length == len(raw),
                 "%s: %s at %s: objdump %s/%d bytes, simulator %s/%d" %
                 (elf, m.group(2), m.group(1), m.group(3), len(raw), name, length))
        total += 1
ck.check(total > 100, "only %d instructions seen" % total)
ck.finish()
