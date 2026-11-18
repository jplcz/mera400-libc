#===-- test_gc.py - proves that --gc-sections drops unused library code ---===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""Lists the symbols of a linked program with llvm-nm and checks that the
functions the program uses are present and the functions it does not use
are not, and that _start is the first word of the image.

usage: test_gc.py <llvm-nm> <program.elf> <present,...> <absent,...>
"""

import subprocess
import sys

nm, elf, present, absent = sys.argv[1:5]
out = subprocess.run([nm, elf], capture_output=True, text=True, check=True).stdout
syms = {}
for line in out.splitlines():
    parts = line.split()
    if len(parts) == 3:
        syms[parts[2]] = int(parts[0], 16)

bad = []
for s in present.split(","):
    if s not in syms:
        bad.append("%s was removed but the program uses it" % s)
for s in absent.split(","):
    if s in syms:
        bad.append("%s is linked in although nothing uses it" % s)
if syms.get("_start") != syms.get("__image_base", 0x400):
    bad.append("_start is not at the image base")
for b in bad:
    print("FAIL:", b)
if bad:
    sys.exit(1)
print("gc: %d symbols present, %d absent as expected" %
      (len(present.split(",")), len(absent.split(","))))
