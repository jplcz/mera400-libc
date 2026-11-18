# CROOK platform layer (`src/crook`)

Wrappers around the CROOK operating system's extracodes. Background and the
how-to are in the top-level README; this file is the reference.

An **extracode** is a MERA-400 instruction (`EXL n`, n = 0200..0307 octal)
that traps into the operating system; the argument is in register R4.

## Why and how (for newcomers)

CROOK has no C library and no system calls in the Unix sense. A program asks
the operating system for a service with the **extracode** instruction
`EXL n`; the operating system takes over, does the work and returns to the
next instruction. This directory turns each extracode into an ordinary C
callable function named `__crook_<name>`, so C and assembly programs need not
build `EXL` instructions by hand. Declarations are in `include/crook.h`.

Pointer arguments of these functions are **word** addresses (CROOK counts
words, a C pointer counts bytes); see the note in `crook.h`. Most calls take
the address of a **parameter block** in R4; the block's word 0 usually reports
errors (non-zero on entry: an error stops the process with an alarm; zero: the
error number is stored there and the program goes on). The sequential-input
calls report end of file in the **carry flag** (the C bit of R0).

Abbreviations: FIL = file/stream description block, REC = character record
block, BLOCK = sector/memory transfer block, XOSL = the command interpreter
(the "parent" of a program), RAM file = file kept in memory, tick = 100 ms.

## Wrappers (numbers and argument layouts checked against the CROOK manuals)

"v5" marks CROOK-5 only calls: `crook.h` declares them only when
`__CROOK_VERSION__` is 5, so a CROOK-4 build cannot call them.

| Group | Function | Extracode | What it does, argument, result |
| --- | --- | --- | --- |
| any | `__crook_exl` | any | generic call: number, R4 value, optional carry out; returns R4 |
| files | `__crook_asg` | ASG 0200 | assign a stream to an existing file ("open"); FIL; returns error word |
| files | `__crook_casg` | CASG 0201 | create a file and assign a stream; FIL; error word |
| files | `__crook_setp` | SETP 0202 | set file parameters; FIL; error word |
| files | `__crook_loap` | LOAP 0203 | load file parameters into the block; FIL; error word |
| files | `__crook_nasg` | NASG 0205 | assign a stream, hiding one with the same id; FIL; error word |
| files | `__crook_cprf` | CPRF 0233 | create the special file for loading a process into its own block; FIL; error word |
| files | `__crook_eras` | ERAS 0210 | remove all local streams; no argument, no result |
| files | `__crook_reld` | RELD 0251 | release the character devices; no argument, no result |
| records | `__crook_inpr` | INPR 0212 | read a record; REC; returns carry (1 = end of file) |
| records | `__crook_prir` | PRIR 0213 | write a record with terminator; REC; carry |
| records | `__crook_pinp` | PINP 0214 | write 2 prompt characters, read a record; REC; carry |
| records | `__crook_prin` | PRIN 0215 | write a record without terminator; REC; carry |
| blocks | `__crook_tmem` | TMEM 0204 | attach a 4 K-word block of a RAM file (address 0 releases); BLOCK; word 0 |
| blocks | `__crook_wadr` | WADR 0222 | write address fields (privileged); BLOCK; word 0 |
| blocks | `__crook_ovl` | OVL 0223 | load an overlay of the process's own file; BLOCK; word 0 |
| blocks | `__crook_reap` | REAP 0224 | read the special (swap) file of a process; BLOCK; word 0 |
| blocks | `__crook_wrip` | WRIP 0225 | write the special file of a process; BLOCK; word 0 |
| blocks | `__crook_read` | READ 0226 | read sectors; BLOCK; word 0 |
| blocks | `__crook_writ` | WRIT 0227 | write sectors; BLOCK; word 0 |
| memory | `__crook_core` | CORE 0232 | resize the process memory block to R4 words (0 = query); returns last address |
| memory | `__crook_gmem` | GMEM 0270 | v5: add 4 K-word blocks (bit map); returns the new map |
| memory | `__crook_rmem` | RMEM 0271 | v5: release blocks (bit map); returns the map |
| memory | `__crook_lram` | LRAM 0272 | v5: fetch RAM-file information into a table (layout undocumented); returns R4 |
| time | `__crook_wait` | WAIT 0247 | sleep R4 ticks |
| time | `__crook_stop` | STOP 0250 | stop until the parent runs the process again or a child alarms; non-zero R4 also gives a timeout in ticks |
| time | `__crook_date` | DATE 0252 | year, month, day to a 3-word table |
| time | `__crook_time` | TIME 0253 | hour, minute, second to a 3-word table |
| exit | `exit`, `_exit`, `_Exit` | END 0373 | end of program (status in R4) |
| exit | `__crook_exit_finish` | EOSL 0377 | end, the system prints "FINISH" |
| exit | `abort`, `__crook_abo` | ABO 0375 | delete the program |
| exit | `__crook_kill` | KILL 0376 | delete the program, prints "DELETED" |
| exit | `__crook_back` | BACK 0374 | XOSL continues, the program runs on in the background (returns) |
| text | `__crook_from_c`, `__crook_to_c` | none | C string <-> CR-terminated text |
| stdio | `putchar`, `puts`, `getchar` | via REC calls | |

Source files: `exl.s`, `exit.s`, `fil.s` (FIL calls, ERAS, RELD), `rec.s`,
`block.s`, `value.s` (CORE, GMEM, RMEM, LRAM, WAIT, STOP, DATE, TIME),
`convert.s`, `stdio.s`.

`__crook_time` is the raw TIME extracode; there is no C `time()` because the
epoch/year base of DATE is unknown.

The wrappers are in separate sections, so with `--gc-sections` an unused one is
not in the image.

## Not wrapped: use `__crook_exl` (or assembly)

Extracodes whose argument layout the manuals do not give. Call them with
`__crook_exl(number, r4, &carry)` once you know the layout:

| Extracode | Why |
| --- | --- |
| ERF 0206 (delete file), ERS 0207 (close stream), FBOF 0211 (to start), FEOF 0217 (to end), EOF 0216 (end mark), TAKS 0266, RELS 0267 | argument is only called "STR" (stream id or pointer), no layout |
| SDIR 0235, TDIR 0236, CDIR 0237 | directory block ("DIR") layout not given |
| TLAB 0260 | disk label block ("MET") layout not given |
| SCON 0371, TCON 0372 | XOSL status word set/get: argument not given |
| OES 0230, ERR 0231, process calls 0234, 0240-0246, 0255, PINF 0261, system-admin 0262-0265, 0275-0277, tape 0300-0307 | outside the groups wrapped here |
| INAM 0220, INUM 0221 | the result comes back in R1-R3 and by skipping 0, 1 or 2 words after the EXL; neither `__crook_exl` nor a C wrapper can express that, so they need assembly, and the INUM skip layout is not documented |
| WAIS 0256, SIGN 0257 | the amount is a word placed after the EXL; needs assembly |

TMEM, READ, WRIT, OVL and the others share the BLOCK layout: word 0 =
words transferred / error, 1 = stream id, 2 = memory address, 3 = word count,
4 = sector number from the start of the file.

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
