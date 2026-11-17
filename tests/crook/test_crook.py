#===-- test_crook.py - run a program against a fake CROOK -----------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Starts a CROOK program image at its first word on the simulator and plays
the operating system: every EXL (extracode) is handled by a Python function
that records the call and answers the way the CROOK manuals describe. Checks
the start-up, the extracode numbers and parameter blocks the library produces
(exit, console write and read, memory and time calls, the generic helper), the
CR/NL translation, and the raw image file.

The program is run twice: once with a fake INPR that stores the record's CR
terminator in the buffer and once with one that does not, because the CROOK
manual does not say which it is (the library must work with both).

usage: test_crook.py <program.elf> <program.bin>
"""

import os
import random
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sim"))
from libtest import Checker          # noqa: E402
from m400sim import Cpu, Elf, Halted, FC  # noqa: E402

END, ASG, DATE, CORE, WAIT, INPR, PRIR, PRIN = 0o373, 0o200, 0o252, 0o232, 0o247, 0o212, 0o213, 0o215

ck = Checker("crook")
elf = Elf(sys.argv[1])
sym = elf.symbols                      # BYTE addresses


def W(name):
    """Word address of a symbol."""
    return sym[name] // 2


def get_byte(mem, baddr):
    w = mem[baddr >> 1]
    return (w & 0xFF) if baddr & 1 else (w >> 8)


def set_byte(mem, baddr, v):
    w = mem[baddr >> 1]
    mem[baddr >> 1] = (w & 0xFF00) | v if baddr & 1 else (v << 8) | (w & 0xFF)


def text_at(mem, baddr):
    """Characters from baddr up to (not including) the first CR."""
    out = bytearray()
    while get_byte(mem, baddr) != 0x0D:
        out.append(get_byte(mem, baddr))
        baddr += 1
        assert len(out) < 1000, "runaway record"
    return bytes(out)


def run(store_terminator):
    rng = random.Random(400)
    mem = [rng.randrange(0x10000) for _ in range(0x10000)]   # junk everywhere
    elf.load(mem)
    cpu = Cpu(mem)
    cpu.ic = sym["__image_base"] // 2
    log = []
    inputs = [b"Hi", None]                                     # None: end of file

    def os_(cpu, n):
        r4 = cpu.r[4]
        if n in (PRIR, PRIN):
            rec = mem[r4:r4 + 4]
            log.append(("PRIR" if n == PRIR else "PRIN", rec[1], text_at(mem, rec[2] * 2),
                        rec[0], rec[3]))
        elif n == INPR:
            rec = mem[r4:r4 + 4]
            log.append(("INPR", rec[1], rec[3], rec[0]))
            line = inputs.pop(0) if rec[1] == 3 else None
            if line is None:
                cpu.fset(FC)                # end of file
                return
            for i, c in enumerate(line):
                set_byte(mem, rec[2] * 2 + i, c)
            if store_terminator:
                set_byte(mem, rec[2] * 2 + len(line), 0x0D)
            mem[r4] = len(line)
            cpu.fset(FC, False)
        elif n == WAIT:
            log.append(("WAIT", r4))
            cpu.r[4] = 0x1234
            cpu.fset(FC)
        elif n == DATE:
            log.append(("DATE", r4))
            mem[r4:r4 + 3] = [2026, 11, 17]
        elif n == ASG:
            log.append(("ASG", r4))
            mem[r4] = 0x55
        elif n == CORE:
            log.append(("CORE", r4))
            cpu.r[4] = 0x2FFF
        elif n == END:
            log.append(("END", r4))
            raise Halted(END)
        else:
            raise AssertionError("unexpected extracode %#o" % n)

    cpu.exl_handler = os_
    code = cpu.run()
    return cpu, mem, log, code


for store_terminator in (True, False):
    tag = "[terminator %s] " % ("stored" if store_terminator else "not stored")
    cpu, mem, log, code = run(store_terminator)
    ck.check(code == END, tag + "program did not end with END (code %r)" % code)

    s = bytearray()
    a = sym["long_msg"]
    while get_byte(mem, a):
        s.append(get_byte(mem, a))
        a += 1
    s = bytes(s)
    ck.check(len(s) > 254 and len(s) < 508, tag + "long string length %d" % len(s))

    # What the operating system should have seen. Records are (kind, stream,
    # text, start index/flags word, terminator<<8 | max length).
    def rec(kind, text):
        return (kind, 4, text, 0, 0x0DFF)

    want = [
        rec("PRIR", b"hello"), rec("PRIR", b"world"),
        rec("PRIN", s[:254]), rec("PRIR", s[254:]),
        rec("PRIN", b"A"), rec("PRIR", b""),
        ("INPR", 3, 0x0DFF, 0),
        rec("PRIN", b"H"), rec("PRIN", b"i"), rec("PRIR", b""),
        ("INPR", 3, 0x0DFF, 0),
        ("WAIT", 5), ("WAIT", 5), ("DATE", W("date_buf")),
        ("ASG", W("fil_block")), ("CORE", 0), ("INPR", 9, 0x0DFF, 0),
        ("END", 7),
    ]
    # INPR for stream 9 reads rec_block, which the program never filled: the
    # fake reports end of file for it and only the call itself is checked.
    got = log[:]
    ck.check(len(got) == len(want), tag + "%d extracode calls, want %d" % (len(got), len(want)))
    for i, (g, w) in enumerate(zip(got, want)):
        if g[0] == "INPR" and w[1] == 9:
            ck.check(g[0] == "INPR", tag + "call %d" % i)
            continue
        ck.check(g == w, tag + "call %d: %r, want %r" % (i, g, w))

    ck.check(mem[W("res_exl")] == 0x1234, tag + "__crook_exl result %#x" % mem[W("res_exl")])
    ck.check(mem[W("res_carry")] == 1, tag + "__crook_exl carry")
    ck.check(mem[W("res_exl_nocarry")] == 0x1234, tag + "__crook_exl with NULL carry")
    ck.check(mem[W("date_buf"):W("date_buf") + 3] == [2026, 11, 17], tag + "DATE result")
    ck.check(mem[W("res_asg")] == 0x55, tag + "__crook_asg returns the error word")
    ck.check(mem[W("res_core")] == 0x2FFF, tag + "__crook_core returns r4")
    ck.check(mem[W("res_inpr")] == 1, tag + "__crook_inpr returns the C flag")
    ck.check(cpu.r[7] == sym["__stack_top"] // 2, tag + "stack balanced: r7=%#x" % cpu.r[7])
    ck.check(cpu.r[4] == 7, tag + "END argument r4 = %d, want 7" % cpu.r[4])

# The raw image: big-endian words from the base address on, whole image size.
with open(sys.argv[2], "rb") as f:
    flat = f.read()
mem = [0] * 0x10000
elf.load(mem)
base = sym["__image_base"] // 2
size = sym["__stack_top"] - sym["__image_base"]
ck.check(sym["_start"] == sym["__image_base"], "_start at the image base")
ck.check(len(flat) == size, "image file is %d bytes, want %d" % (len(flat), size))
want = b"".join(bytes([mem[base + i] >> 8, mem[base + i] & 0xFF]) for i in range(len(flat) // 2))
ck.check(flat == want, "raw image equals the linked ELF image")
ck.check(((flat[0] << 8) | flat[1]) == mem[base], "first word is crt0's first instruction")
ck.check(sym["__heap_start"] <= sym["__heap_end"] <= sym["__stack_top"], "heap below stack")

ck.finish()
