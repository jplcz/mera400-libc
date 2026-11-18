#===-- test_stdio.py - printf, sscanf and friends on the simulator --------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Runs tests/c/test_stdio.c with a memory-mapped console and compares the
captured output with the text a C99 printf produces.

usage: test_stdio.py <program.elf> [small|large [float|nofloat]]
"""

import sys

from libtest import Checker, ConsoleMem, Image
from m400sim import Cpu

EXPECTED = """\
42|   42|42   |00042|+42| 42
-32768 -7 0
65535 beef BEEF 10 0xff 010
-2147483648 4000000000 deadbeef
-9000000000000 18000000000000000000
a|  b|c  |
abc|       abc|abc       |abc|(null)
  042||+5    |44|     7|xy
0x1234 %
"""
EXPECTED_FLOAT = """\
1.500000|-0.000|    3.14|2.3     |+7.0
1.234568e+04|1.23e-04|1.000000E+30
100000|1e+06|0.0001|1e-05|3.5
0.000000|0.000000e+00|0|123456789
0.667|0.3333333333|10.0|1000000.000000
3.|1.50000|0.000
10000000000.00 -123.46
"""
# Without floating point: the placeholder, and the next arguments still match.
EXPECTED_NOFLOAT = """\
?float?|?float?|?float?  |   ?float?|77
?float? ?float? k 123456 ok
"""
EXPECTED_TAIL = """\
puts line
x
fputs line
fprintf 99
abc
n=4
"""

large = len(sys.argv) > 2 and sys.argv[2] == "large"
with_float = len(sys.argv) > 3 and sys.argv[3] == "float"
EXPECTED += EXPECTED_FLOAT if with_float else EXPECTED_NOFLOAT
EXPECTED += EXPECTED_TAIL
ck = Checker("stdio")
img = Image(sys.argv[1])
port = 0xFFFF if large else 0x7FFF
cpu = Cpu(ConsoleMem(img.base, port))
cpu.ic = img.elf.symbols["__image_base"] // 2
code = cpu.run(limit=400_000_000)
out = bytes(cpu.mem.output).decode("latin-1")
ck.check(code == 0o77, "halt code %#o, want 0o77" % code)
ck.check(cpu.r[1] == 0, "%d in-program checks failed" % cpu.r[1])
got, want = out.splitlines(), EXPECTED.splitlines()
for i in range(max(len(got), len(want))):
    g = got[i] if i < len(got) else "<missing>"
    w = want[i] if i < len(want) else "<unexpected>"
    ck.check(g == w, "line %d: got %r want %r" % (i + 1, g, w))
ck.finish()
