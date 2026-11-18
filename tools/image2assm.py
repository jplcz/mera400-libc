#!/usr/bin/env python3
"""image2assm.py - turn a flat CROOK image into ASSM source text.

WHY
  CROOK (the MERA-400 operating system) has no loader and no linker; its own
  assembler, ASSM, writes a program straight into a disk file. The layout of
  such a file on disk is not documented, so this tool does not write one.
  Instead it writes the program as ASSM *source*: a few directives and then
  every word of the image as a numeric constant. Assemble that text with
  CROOK's own ASSM and ASSM produces the real program file.

INPUT
  A flat big-endian word image: 16-bit words, high byte first, as made by
  `clang --target=mera400-unknown-crook -o program.bin` (or by
  `llvm-objcopy -O binary`). The size must be a whole number of words.

OUTPUT (all of it from https://mera400.pl/J%C4%99zyk_symboliczny_(assembler)_ASSM,
"ASSM" below; every item is verified there unless marked UNVERIFIED)
  FILE*<title>,<type>,<base>,<attribute>.
        Names the output file. Type, base and attribute may be omitted from
        the end; defaults are 0, 0 and 050 (octal). The base is the lowest
        address of the program.
  MEM* <address>.
        "The largest address the program can use at run time".
  S* <address>.
        Sets the location counter S, the address of the next word.
  <expression>.
        A word: an expression ended by a full stop is placed at S and S grows
        by one. Constants are 16-bit; a number that starts with the digit 0 is
        octal; negative numbers are two's complement. This tool writes every
        word as an octal constant with a leading 0, e.g. 0177400. (the page's
        own example uses 0177400. and 0377.), and an octal 0 as 0.
  RES* <count>.
        Puts <count> zero words at S (the second argument is the fill value;
        omitted it is zero). Used for long runs of zeros, such as the .bss,
        heap and stack padding at the end of an image.
  PROG* and FINPROG* (only with --prog-block)
        The top-level block brackets. ASSM says a program is enclosed in
        them, but not that they are required: UNVERIFIED, hence optional.

UNVERIFIED, deliberately left open
  * The allowed characters and length of <title>. This tool accepts letters,
    digits and "_" only (a maximum length is NOT enforced).
  * What <type> means to CROOK (0 is ASSM's default).
  * Whether the location counter S starts at the base address; the tool
    writes S* <base> to be independent of that.
  * The relation between MEM* and the memory quanta (4K-word blocks) CROOK
    keeps in the file label. ASSM documents MEM* as an ADDRESS. --mem-quanta N
    writes the last address of N blocks (N * 4096 - 1): an inference.
  * The initial instruction counter of the finished program.
  Lines are at most 150 characters (ASSM's limit). A line ends with LF unless
  --eol cr is given (CROOK itself ends lines with CR; ASSM treats CR as the end
  of a line). Nothing here is a comment, to stay inside what is verified.
"""

import argparse
import re
import sys

BLOCK_WORDS = 4096  # one CROOK memory block ("quantum")
MAX_LINE = 150


def read_words(data):
    if len(data) % 2:
        raise ValueError("image size (%d bytes) is not a whole number of words"
                         % len(data))
    return [(data[i] << 8) | data[i + 1] for i in range(0, len(data), 2)]


def octal(n):
    """An ASSM number: octal with a leading 0, so 0 itself is "0"."""
    return "0%o" % n if n else "0"


def number(text, radix):
    """Parses a command line number; 0x/0o/0b prefixes are accepted."""
    if re.match(r"^0[xXoObB]", text):
        return int(text, 0)
    return int(text, radix)


def default_mem_address(base, nwords):
    top = base + nwords
    blocks = max(1, -(-top // BLOCK_WORDS))
    return blocks * BLOCK_WORDS - 1


def image_to_assm(words, title, type_=0, base=0, attr=0o50, mem=None,
                  words_per_line=8, min_run=8, prog_block=False, eol="\n"):
    """Returns the ASSM source for the words (a list of ints 0..65535)."""
    if not re.match(r"^[A-Za-z0-9_]+$", title):
        raise ValueError("title must be letters, digits and _ only")
    if not 0 <= base < 65536 or base + len(words) > 65536:
        raise ValueError("the image does not fit in the 64K-word address space")
    if mem is None:
        mem = default_mem_address(base, len(words))
    lines = []
    if prog_block:
        lines.append("PROG*")
    lines.append("FILE*%s,%s,%s,%s." % (title, octal(type_), octal(base),
                                        octal(attr)))
    lines.append("MEM* %s." % octal(mem))
    lines.append("S* %s." % octal(base))
    i = 0
    n = len(words)
    while i < n:
        j = i
        while j < n and words[j] == 0:
            j += 1
        if j - i >= min_run:
            lines.append("RES* %d." % (j - i))
            i = j
            continue
        # Words up to the next long run of zeros (short runs stay as words).
        chunk = []
        while i < n and len(chunk) < words_per_line:
            k = i
            while k < n and words[k] == 0:
                k += 1
            if k - i >= min_run:
                break
            chunk.append(words[i])
            i += 1
        lines.append(" ".join("%s." % octal(w) for w in chunk))
    if prog_block:
        lines.append("FINPROG*")
    for line in lines:
        if len(line) > MAX_LINE:
            raise ValueError("line longer than ASSM's 150 characters; "
                             "use fewer --words-per-line")
    return eol.join(lines) + eol


def parse_assm(text):
    """Reads back the subset of ASSM that image_to_assm writes.

    Returns (header, words) where header is a dict with title, type, base,
    attr and mem. Anything else is an error, so a malformed text never
    round-trips by accident.
    """
    header = {}
    words = []
    counter = None

    def value(tok):
        if not re.match(r"^(0[0-7]*|[1-9][0-9]*|-[1-9][0-9]*)$", tok):
            raise ValueError("bad ASSM number %r" % tok)
        v = int(tok, 8) if tok.startswith("0") else int(tok, 10)
        if not -32768 <= v <= 65535:
            raise ValueError("number out of 16-bit range: %r" % tok)
        return v & 0xFFFF

    for raw in text.replace("\r", "\n").split("\n"):
        line = raw.strip()
        if not line:
            continue
        if line in ("PROG*", "FINPROG*"):
            continue
        m = re.match(r"^FILE\*([A-Za-z0-9_]+),(\w+),(\w+),(\w+)\.$", line)
        if m:
            header["title"] = m.group(1)
            header["type"], header["base"], header["attr"] = (
                value(m.group(i)) for i in (2, 3, 4))
            continue
        m = re.match(r"^MEM\* (\w+)\.$", line)
        if m:
            header["mem"] = value(m.group(1))
            continue
        m = re.match(r"^S\* (\w+)\.$", line)
        if m:
            counter = value(m.group(1))
            if counter != header.get("base"):
                raise ValueError("S* different from the base is not supported")
            continue
        m = re.match(r"^RES\* ([1-9][0-9]*)\.$", line)
        if m:
            words.extend([0] * int(m.group(1)))
            continue
        for tok in line.split(" "):
            if not tok.endswith("."):
                raise ValueError("unterminated item %r" % tok)
            words.append(value(tok[:-1]))
    if "title" not in header or counter is None:
        raise ValueError("FILE* or S* is missing")
    return header, words


def words_to_bytes(words):
    out = bytearray()
    for w in words:
        out += bytes((w >> 8, w & 0xFF))
    return bytes(out)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("image", help="flat big-endian word image (.bin)")
    p.add_argument("-o", "--output", help="ASSM file to write (default: stdout)")
    p.add_argument("--title", required=True, help="title of the CROOK file")
    p.add_argument("--type", default="0",
                   help="file type, octal (default 0)")
    p.add_argument("--base", default="0",
                   help="word address of the first word, decimal "
                        "(0x.. and 0o.. accepted; default 0)")
    p.add_argument("--attr", default="050",
                   help="program attribute, octal (default 050, ASSM's default)")
    p.add_argument("--mem-quanta", type=int,
                   help="memory size in 4K-word blocks, written as MEM* "
                        "(default: just enough for base + image)")
    p.add_argument("--words-per-line", type=int, default=8)
    p.add_argument("--no-res", action="store_true",
                   help="write zero words one by one instead of RES*")
    p.add_argument("--prog-block", action="store_true",
                   help="enclose the text in PROG* ... FINPROG* (UNVERIFIED "
                        "whether ASSM needs it)")
    p.add_argument("--eol", choices=("lf", "cr"), default="lf")
    args = p.parse_args(argv)

    try:
        with open(args.image, "rb") as f:
            words = read_words(f.read())
        mem = None
        if args.mem_quanta is not None:
            if args.mem_quanta < 1 or args.mem_quanta > 16:
                raise ValueError("--mem-quanta must be 1..16")
            mem = args.mem_quanta * BLOCK_WORDS - 1
        text = image_to_assm(
            words, args.title, number(args.type, 8), number(args.base, 10),
            number(args.attr, 8), mem, args.words_per_line,
            min_run=10**9 if args.no_res else 8, prog_block=args.prog_block,
            eol="\r" if args.eol == "cr" else "\n")
    except (OSError, ValueError) as e:
        print("image2assm: error: %s" % e, file=sys.stderr)
        return 1
    if args.output:
        with open(args.output, "w", newline="") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
