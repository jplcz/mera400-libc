# CROOK platform layer (`src/crook`)

Wrappers around the CROOK operating system's extracodes. Background and the
how-to are in the top-level README; this file is the reference.

An **extracode** is a MERA-400 instruction (`EXL n`, n = 0200..0307 octal)
that traps into the operating system; the argument is in register R4.

## Implemented (number and argument layout verified in `docs/crook-research.md`)

| File | Functions | Extracodes |
| --- | --- | --- |
| `exl.s` | `__crook_exl` (generic) | any |
| `exit.s` | `exit`, `_exit`, `abort`, `__crook_exit_finish` | END 0373, EOSL 0377, ABO 0375 |
| `fil.s` | `__crook_asg/casg/nasg` | ASG 0200, CASG 0201, NASG 0205 |
| `rec.s` | `__crook_inpr/prir/pinp/prin` | 0212..0215 |
| `block.s` | `__crook_read/writ/tmem` | 0226, 0227, 0204 |
| `value.s` | `__crook_core/gmem/rmem/wait/date/time` | 0232, 0270, 0271, 0247, 0252, 0253 |
| `convert.s` | `__crook_from_c`, `__crook_to_c` | none |
| `stdio.s` | `putchar`, `puts`, `getchar` | via REC calls |

`__crook_time` is the raw TIME extracode; there is no C `time()` because the
epoch/year base of DATE is unknown.

## TODO: not implemented (argument layout not verified)

ERS 0207 (close), ERF 0206, FBOF 0211, FEOF 0217, EOF 0216 (STR-type
arguments); use `__crook_exl` once the layout is known.

## Text: C strings vs CROOK records

CROOK stores two characters per 16-bit word, high byte first. MERA-400 is
big-endian and C byte pointers address the high byte first, so a C string is
already in CROOK packing; only the terminator differs: C uses NUL, CROOK uses
CR (0x0D). `__crook_from_c` stops at NL, CR or NUL and appends CR;
`__crook_to_c` turns CR into `'\n'` and appends NUL. `puts` ends the record
at each `'\n'` (PRIR = print and end the line). Consequences: `'\r'` and
`'\n'` both mean "end of line" on output, input lines come back with `'\n'`,
and NUL cannot be read through `getchar`.

REC block (4 words, word addresses): word 0 flags/index (see open questions),
word 1 stream id, word 2 buffer word address, word 3 = 0x0DFF (terminator CR,
max length 255).

## Open questions (not guessed)

See the top-level README, section "CROOK: open questions".
