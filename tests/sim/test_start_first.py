#===-- test_start_first.py - _start must be the first thing in the image --===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""The CPU begins executing at the first word of the image, so _start must
sit exactly there. The linker script guarantees it through a dedicated
section; this test proves it by linking the same program with the crt0
object in every position among the other inputs and checking the ELF
symbol, the section start and the first word of the flat binary.

usage: test_start_first.py <ld.lld> <linker-script> <workdir> <crt0.o> <obj-or-lib>...
"""

import subprocess
import sys

from libtest import Checker
from m400sim import Elf

ld, script, work, crt0, *others = sys.argv[1:]
ck = Checker("start-first")


def link(inputs, out, *extra):
    subprocess.run([ld, "-m", "mera400elf", "-T", script, *extra, *inputs, "-o", out],
                   check=True)


objs = [o for o in others if o.endswith(".o")]
libs = [o for o in others if o.endswith(".a")]
orders = []
for pos in range(len(objs) + 1):
    for perm in {tuple(objs), tuple(reversed(objs))}:
        p = list(perm)
        p.insert(pos, crt0)
        orders.append(p + libs)

expected_word = None
for n, order in enumerate(orders):
    elf, binf = "%s/order%d.elf" % (work, n), "%s/order%d.bin" % (work, n)
    link(order, elf)
    link(order, binf, "--oformat=binary")
    e = Elf(elf)
    base = e.symbols["__image_base"]
    ck.check(e.symbols["_start"] == base, "order %d: _start %#x != image base %#x" %
             (n, e.symbols["_start"], base))
    ck.check(e.segments[0][0] == base, "order %d: first segment not at the base" % n)
    ck.check(e.entry == e.symbols["_start"], "order %d: ELF entry is not _start" % n)
    with open(binf, "rb") as f:
        first = f.read(2)
    if expected_word is None:
        expected_word = first
    ck.check(first == expected_word and first == e.segments[0][1][:2],
             "order %d: first word of the binary differs" % n)

ck.check(len(orders) >= 3, "too few link orders tried")
ck.finish()
