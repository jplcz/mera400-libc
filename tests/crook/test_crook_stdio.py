#===-- test_crook_stdio.py - printf on CROOK, against a fake CROOK --------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs tests/crook/crook_printf.c and checks the records that reach the fake
operating system: each output line is one PRIR record (the library collects
characters until the newline), and the program ends with END.

usage: test_crook_stdio.py <program.elf>
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sim"))
from libtest import Checker          # noqa: E402
from m400sim import Cpu, Elf, Halted  # noqa: E402

END, PRIR, PRIN = 0o373, 0o213, 0o215

ck = Checker("crook-stdio")
elf = Elf(sys.argv[1])
mem = [0] * 0x10000
elf.load(mem)
cpu = Cpu(mem)
cpu.ic = elf.symbols["__image_base"] // 2
log = []


def get_byte(baddr):
    w = mem[baddr >> 1]
    return (w & 0xFF) if baddr & 1 else (w >> 8)


def os_(cpu, n):
    r4 = cpu.r[4]
    if n in (PRIR, PRIN):
        addr = mem[r4 + 2] * 2
        text = bytearray()
        while get_byte(addr) != 0x0D:
            text.append(get_byte(addr))
            addr += 1
        log.append(("PRIR" if n == PRIR else "PRIN", bytes(text)))
    elif n == END:
        log.append(("END", r4))
        raise Halted(END)
    else:
        raise AssertionError("unexpected extracode %#o" % n)


cpu.exl_handler = os_
code = cpu.run(limit=50_000_000)
ck.check(code == END, "program did not end with END (code %r)" % code)
want = [("PRIR", b"n=42 s=ok x=beef"), ("PRIR", b"line2"), ("PRIR", b"no newline"),
        ("END", 5)]
ck.check(log == want, "records %r, want %r" % (log, want))
ck.finish()
