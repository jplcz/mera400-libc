#===-- m400sim.py - host-side simulator of a MERA-400 CPU subset ----------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
"""A small instruction-level simulator of the MERA-400 central processor.

It executes the machine code that the LLVM toolchain produced (loaded from
the linked ELF file), so the tests exercise the real encodings, not the
assembly text. Instruction semantics follow the public EM400 emulator
sources (instruction decoding, argument evaluation, flags). Nothing here ran
on real hardware.

Modelled: the 16-bit word memory (no blocks, no paging, no MOD), registers
r0-r7 with the flags in r0, normal/short/byte arguments, all load, store,
arithmetic, logic, byte, compare, shift, jump and skip instructions. Not
modelled (the simulator stops with NotModelled when it meets one): I/O,
interrupts, privileged instructions, AWP, multi-word loads, MOD/modification.
"""

import struct

MASK = 0xFFFF

# Flags in r0 (bit 15 is the most significant, as in the manuals).
FZ, FM, FV, FC, FL, FE, FG, FY, FX = (
    0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100, 0x0080)

NORM = {
    0o20: "lw", 0o21: "tw", 0o22: "ls", 0o23: "ri", 0o24: "rw", 0o25: "pw",
    0o26: "rj", 0o27: "is", 0o30: "bb", 0o31: "bm", 0o32: "bs", 0o33: "bc",
    0o34: "bn", 0o35: "ou", 0o36: "in", 0o40: "aw", 0o41: "ac", 0o42: "sw",
    0o43: "cw", 0o44: "or", 0o45: "om", 0o46: "nr", 0o47: "nm", 0o50: "er",
    0o51: "em", 0o52: "xr", 0o53: "xm", 0o54: "cl", 0o55: "lb", 0o56: "rb",
    0o57: "cb",
}
AWP = ["ad", "sd", "mw", "dw", "af", "sf", "mf", "df"]
SHORT = {0o60: "awt", 0o61: "trb", 0o62: "irb", 0o63: "drb", 0o64: "cwt",
         0o65: "lwt", 0o66: "lws", 0o67: "rws"}
SHORT_JUMP = ["ujs", "jls", "jes", "jgs", "jvs", "jxs", "jys", "jcs"]
BYTE_OPS = ["blc", "exl", "brc", "nrf"]
LONG_JUMP = ["uj", "jl", "je", "jg", "jz", "jm", "jn", "lj"]
OP73 = ["hlt", "mcl", "cit", "giu", "lip", "cron", "?", "?"]
OP75 = ["ld", "lf", "la", "ll", "td", "tf", "ta", "tl"]
OP76 = ["rd", "rf", "ra", "rl", "pd", "pf", "pa", "pl"]
OP77 = ["mb", "im", "ki", "fi", "sp", "md", "rz", "ib"]
SINGLE = {
    (0, 0): "ric", (0, 1): "zlb", (0, 2): "sxu", (0, 3): "nga", (0, 4): "slz",
    (0, 5): "sly", (0, 6): "slx", (0, 7): "sry", (0, 8): "ngl", (0, 9): "rpc",
    (1, 0): "rky", (1, 1): "zrb", (1, 2): "sxl", (1, 3): "ngc", (1, 4): "svz",
    (1, 5): "svy", (1, 6): "svx", (1, 7): "srx", (1, 8): "srz", (1, 9): "lpc",
}

# Conditional jumps: (flag mask, required value) tested on r0.
SHORT_COND = [(0, 0), (FL, FL), (FE, FE), (FG, FG), (FV, FV), (FX, FX),
              (FY, FY), (FC, FC)]
LONG_COND = [(0, 0), (FL, FL), (FE, FE), (FG, FG), (FZ, FZ), (FM, FM),
             (FE, 0), (0, 0)]


class NotModelled(Exception):
    pass


def decode(ir):
    """Return (mnemonic, kind) for an instruction word.

    kind is "norm" (has a normal argument, one extra word when C is 0),
    "short" (6-bit signed constant), "byte" or "none".
    """
    op, d = ir >> 10, (ir >> 9) & 1
    a, b, c = (ir >> 6) & 7, (ir >> 3) & 7, ir & 7
    if op in NORM:
        return NORM[op], "norm"
    if op == 0o37:
        return AWP[a], "norm"
    if op in SHORT:
        return SHORT[op], "short"
    if op == 0o70:
        return SHORT_JUMP[a], "short"
    if op == 0o71:
        return BYTE_OPS[a], "byte"
    if op == 0o72:
        if b == 2:
            return "shc", "none"
        name = SINGLE.get((d, (b << 3) | c))
        return (name or "?"), "none"
    if op == 0o73:
        return OP73[a], "short" if a == 0 else "none"
    if op == 0o74:
        return LONG_JUMP[a], "norm"
    if op == 0o75:
        return OP75[a], "norm"
    if op == 0o76:
        return OP76[a], "norm"
    if op == 0o77:
        return OP77[a], "norm"
    return "?", "none"


class Halted(Exception):
    def __init__(self, code):
        super().__init__("halt %#o" % code)
        self.code = code


class Cpu:
    def __init__(self, mem=None):
        self.mem = mem if mem is not None else [0] * 0x10000
        self.r = [0] * 8
        self.ic = 0
        self.skip = False
        self.steps = 0
        self.executed = set()

    # --- flags -----------------------------------------------------------
    def fset(self, f, on=True):
        self.r[0] = (self.r[0] | f) if on else (self.r[0] & ~f & MASK)

    def fget(self, f):
        return 1 if self.r[0] & f else 0

    def set_leg(self, a, b):
        self.r[0] &= ~(FL | FE | FG) & MASK
        self.r[0] |= FE if a == b else (FL if a < b else FG)

    def set_z(self, v):
        self.fset(FZ, v == 0)

    def alu_add(self, reg, n, carry):
        sreg = reg - 0x10000 if reg & 0x8000 else reg
        sn = n - 0x10000 if n & 0x8000 else n
        res = (sreg + sn + carry)
        ures = (reg & MASK) + (n & MASK) + carry
        self.fset(FZ, res == 0)
        self.fset(FC, bool(ures & 0x10000))
        diff = ((reg ^ n) & 0x8000) >> 15
        self.fset(FM, self.fget(FC) != diff)
        if ((ures >> 15) & 1) ^ self.fget(FM):
            self.fset(FV)
        return ures & MASK

    def alu_sub(self, reg, n):
        sreg = reg - 0x10000 if reg & 0x8000 else reg
        sn = n - 0x10000 if n & 0x8000 else n
        res = sreg - sn
        ures = (reg & MASK) + ((-n) & MASK)
        self.fset(FZ, res == 0)
        self.fset(FC, bool(ures & 0x10000) or n == 0)
        diff = ((reg ^ n) & 0x8000) >> 15
        self.fset(FM, self.fget(FC) == diff)
        if ((ures >> 15) & 1) ^ self.fget(FM):
            self.fset(FV)
        return ures & MASK

    @staticmethod
    def signed(v):
        return v - 0x10000 if v & 0x8000 else v

    # --- execution -------------------------------------------------------
    def fetch(self):
        w = self.mem[self.ic]
        self.ic = (self.ic + 1) & MASK
        return w

    def step(self):
        self.steps += 1
        ir = self.fetch()
        name, kind = decode(ir)
        op, d = ir >> 10, (ir >> 9) & 1
        a, b, c = (ir >> 6) & 7, (ir >> 3) & 7, ir & 7
        self.executed.add(name)

        # Conditional jumps that do not fire are "ineffective": they skip
        # their immediate argument word like a skipped instruction does.
        cond = None
        if op == 0o70:
            cond = SHORT_COND[a]
        elif op == 0o74:
            cond = LONG_COND[a]
        ineffective = self.skip or (
            cond is not None and (self.r[0] & cond[0]) != cond[1])
        if ineffective:
            self.skip = False
            if kind == "norm" and c == 0:
                self.ic = (self.ic + 1) & MASK
            return

        if name in ("?", "ou", "in", "ri", "ls", "pw") or op in (0o37, 0o71, 0o75, 0o76, 0o77):
            raise NotModelled(name)

        if kind == "norm":
            ac = self.r[c] if c else self.fetch()
            if b:
                ac = (ac + self.r[b]) & MASK
            if d:
                ac = self.mem[ac]
        elif kind == "short":
            ac = (ir & 0x3F) * (-1 if d else 1)
            ac &= MASK
        else:
            ac = 0
        self.execute(name, ir, a, b, c, d, ac)

    def wr(self, reg, value):
        self.r[reg] = value & MASK

    def execute(self, name, ir, a, b, c, d, ac):
        r = self.r
        if name == "lw" or name == "lwt":
            self.wr(a, ac)
        elif name == "tw":
            self.wr(a, self.mem[ac])
        elif name == "rw":
            self.mem[ac] = r[a]
        elif name == "rj":
            self.wr(a, self.ic)
            self.ic = ac
        elif name == "aw" or name == "awt":
            self.wr(a, self.alu_add(r[a], ac, 0))
        elif name == "ac":
            self.wr(a, self.alu_add(r[a], ac, self.fget(FC)))
        elif name == "sw":
            self.wr(a, self.alu_sub(r[a], ac))
        elif name == "cw" or name == "cwt":
            self.set_leg(self.signed(r[a]), self.signed(ac))
        elif name == "cl":
            self.set_leg(r[a], ac)
        elif name in ("or", "nr", "er", "xr"):
            v = {"or": r[a] | ac, "nr": r[a] & ac, "er": r[a] & ~ac,
                 "xr": r[a] ^ ac}[name] & MASK
            self.set_z(v)
            self.wr(a, v)
        elif name in ("om", "nm", "em", "xm"):
            m = self.mem[ac]
            v = {"om": r[a] | m, "nm": r[a] & m, "em": ~r[a] & m,
                 "xm": r[a] ^ m}[name] & MASK
            self.set_z(v)
            self.mem[ac] = v
        elif name == "lb":
            w = self.mem[(ac >> 1) & 0x7FFF]
            byte = (w & 0xFF) if ac & 1 else (w >> 8)
            self.wr(a, (r[a] & 0xFF00) | byte)
        elif name == "rb":
            wa = (ac >> 1) & 0x7FFF
            w = self.mem[wa]
            if ac & 1:
                w = (w & 0xFF00) | (r[a] & 0xFF)
            else:
                w = ((r[a] & 0xFF) << 8) | (w & 0xFF)
            self.mem[wa] = w
        elif name == "cb":
            w = self.mem[(ac >> 1) & 0x7FFF]
            byte = (w & 0xFF) if ac & 1 else (w >> 8)
            self.set_leg(r[a] & 0xFF, byte)
        elif name == "trb":
            v = (r[a] + ac) & MASK
            self.wr(a, v)
            self.skip = v == 0
        elif name in ("irb", "drb"):
            v = (r[a] + (1 if name == "irb" else -1)) & MASK
            self.wr(a, v)
            if v != 0:
                self.ic = (self.ic + ac) & MASK
        elif name == "lws":
            self.wr(a, self.mem[(self.ic + ac) & MASK])
        elif name == "rws":
            self.mem[(self.ic + ac) & MASK] = r[a]
        elif name in SHORT_JUMP:
            if name == "jvs":
                self.fset(FV, False)
            self.ic = (self.ic + ac) & MASK
        elif name in LONG_JUMP and name != "lj":
            self.ic = ac
        elif name == "lj":
            self.mem[ac] = self.ic
            self.ic = (ac + 1) & MASK
        elif name == "is":
            m = self.mem[ac]
            if r[a] & ~m & MASK == 0:
                self.skip = True
            else:
                self.mem[ac] = r[a] | m
        elif name == "bb":
            self.skip = (~r[a] & ac & MASK) == 0
        elif name == "bm":
            self.skip = (r[a] & ~self.mem[ac] & MASK) == 0
        elif name == "bs":
            self.skip = (((r[a] ^ ac) & MASK) & r[7]) == 0
        elif name == "bc":
            self.skip = ((r[a] | ~ac) & MASK) != MASK
        elif name == "bn":
            self.skip = (r[a] & ac) == 0
        elif name == "hlt":
            raise Halted(ac)
        elif name == "zlb":
            self.wr(a, r[a] & 0xFF)
        elif name == "zrb":
            self.wr(a, r[a] & 0xFF00)
        elif name == "sxu":
            self.fset(FX, bool(r[a] & 0x8000))
        elif name == "sxl":
            self.fset(FX, bool(r[a] & 1))
        elif name == "nga":
            self.wr(a, self.alu_sub(0, r[a]))
        elif name == "ngc":
            self.wr(a, self.alu_add((~r[a]) & MASK, 0, self.fget(FC)))
        elif name == "ngl":
            v = ~r[a] & MASK
            self.set_z(v)
            self.wr(a, v)
        elif name in ("slz", "sly", "slx", "svz", "svy", "svx"):
            shift_in = {"z": 0, "y": self.fget(FY), "x": self.fget(FX)}[name[2]]
            v = ((r[a] << 1) + shift_in) & MASK
            if name[1] == "v" and (r[a] ^ v) & 0x8000:
                self.fset(FV)
            self.fset(FY, bool(r[a] & 0x8000))
            self.wr(a, v)
        elif name in ("srz", "sry", "srx"):
            shift_in = {"z": 0, "y": self.fget(FY), "x": self.fget(FX)}[name[2]]
            self.fset(FY, bool(r[a] & 1))
            self.wr(a, (r[a] >> 1) | (shift_in << 15))
        elif name == "shc":
            t = c | (d << 3)
            v = r[a]
            self.wr(a, ((v >> t) | (v << (16 - t))) & MASK)
        elif name == "ric":
            raise NotModelled(name)
        else:
            raise NotModelled(name)

    def run(self, limit=2_000_000):
        """Run until a hlt; returns the halt code."""
        try:
            while self.steps < limit:
                self.step()
        except Halted as h:
            return h.code
        raise RuntimeError("step limit exceeded (runaway program?)")


class Elf:
    """Minimal ELF32 big-endian reader: loadable segments and symbols."""

    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        assert d[:4] == b"\x7fELF" and d[4] == 1 and d[5] == 2, "not ELF32 BE"
        (self.entry,) = struct.unpack(">I", d[24:28])
        phoff, shoff = struct.unpack(">II", d[28:36])
        phentsize, phnum, shentsize, shnum = struct.unpack(">HHHH", d[42:50])
        self.segments = []
        for i in range(phnum):
            off = phoff + i * phentsize
            p_type, p_off, p_vaddr, _, p_filesz, p_memsz = struct.unpack(
                ">IIIIII", d[off:off + 24])
            if p_type == 1:
                self.segments.append((p_vaddr, d[p_off:p_off + p_filesz], p_memsz))
        self.symbols = {}
        secs = []
        for i in range(shnum):
            off = shoff + i * shentsize
            secs.append(struct.unpack(">IIIIIIIIII", d[off:off + 40]))
        for sh in secs:
            if sh[1] == 2:  # SHT_SYMTAB
                strtab = secs[sh[6]]
                for o in range(sh[4], sh[4] + sh[5], 16):
                    st_name, st_value, _, st_info, _, _ = struct.unpack(
                        ">IIIBBH", d[o:o + 16])
                    end = d.index(b"\0", strtab[4] + st_name)
                    name = d[strtab[4] + st_name:end].decode()
                    if name and (st_info >> 4) == 1:  # global
                        self.symbols[name] = st_value

    def load(self, mem):
        """Copy the segments into word memory; bss (memsz > filesz) is
        left as it is, so tests can fill memory with junk beforehand."""
        for vaddr, data, memsz in self.segments:
            assert vaddr % 2 == 0
            if len(data) % 2:
                data += b"\0"
            for i in range(0, len(data), 2):
                mem[vaddr // 2 + i // 2] = (data[i] << 8) | data[i + 1]

    def text_range(self):
        for vaddr, data, _ in self.segments:
            return vaddr, vaddr + len(data)
        return 0, 0
