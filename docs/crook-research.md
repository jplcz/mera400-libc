# CROOK (MERA-400) research for an LLVM `mera400-unknown-crook` toolchain

Legend: **[V]** = verified in cited source; **[I]** = inference; **[?]** = unknown / not found.
Base wiki: https://mera400.pl . Wiki text is a transcription of "SYSTEM OPERACYJNY CROOK-5 (wersja 7)", Czerniak & Nikodemski, Gdańsk 1988.
PDFs were downloaded and converted with pdftotext (OCR quality is poor; CROOK-4 PDF content ≈ CROOK-5 manual).
`mera400-crook3.pdf` (19 pp) is in fact a CROOK-2/3 installation/disk-extracode note, not a user-program ABI doc (https://mera400.pl/files/mera400-crook3.pdf) – nothing useful for ABI.

## 0. Key sources
- Overview: https://mera400.pl/CROOK
- Manual index: https://mera400.pl/System_Operacyjny_CROOK-5
- Extracode list: https://mera400.pl/CROOK-5_-_Wykaz_ekstrakod%C3%B3w
- Processes/extracodes: https://mera400.pl/CROOK-5_-_Procesy
- Memory mgmt: https://mera400.pl/CROOK-5_-_Zarz%C4%85dzanie_pami%C4%99ci%C4%85_operacyjn%C4%85
- I/O: https://mera400.pl/CROOK-5_-_Podsystem_wej%C5%9Bcia-wyj%C5%9Bcia
- Files/streams: https://mera400.pl/CROOK-5_-_Organizacja_informacji_w_systemie
- Program environment: https://mera400.pl/CROOK-5_-_%C5%9Arodowisko_dla_program%C3%B3w_i_zlece%C5%84
- ASSM assembler: https://mera400.pl/J%C4%99zyk_symboliczny_(assembler)_ASSM
- CROOK-4 PDF: http://mera400.pl/files/mera400-crook4.pdf (sections III.1.6–III.4, "Ekstrakod TMEM" p.100, CSYS/LOD/UNL p.103)
- EXL instruction: https://mera400.pl/Lista_rozkaz%C3%B3w
- CPU mods (MX-16, 17-bit byte addr): https://mera400.pl/Modyfikacje_sprz%C4%99towe_procesora
- Memory map / data structs: https://mera400.pl/Mapa_pami%C4%99ci_CROOK-5 , https://mera400.pl/Struktury_danych_CROOK-5
- EM400 run: https://mera400.pl/CROOK-5_w_EM400 ; disk image: https://mera400.pl/CROOK-5_-_obraz_dysku ; EM400: https://mera400.pl/EM400
- Cross tools: https://mera400.pl/ASSEM (github.com/jakubfi/assem), https://mera400.pl/EMAS, https://mera400.pl/EMCRK (github.com/jakubfi/emcrk); EM400 source github.com/jakubfi/em400

## 1. Executable / load format and process memory
- CROOK is written entirely in assembler (ASSK mnemonics + MERA-400 extras); kernel 8/15 = 19162 words. [V] https://mera400.pl/CROOK
- **There is no linker/loader in CROOK-4/5 base software.** ASSM translates (single pass) source straight to a ready-to-run binary on disk; library code is included as source; drawback: cannot link ASSM output with other compilers' modules. [V] ASSM page ("Oprogramowanie podstawowe systemu CROOK-4 ... nie ma w nim programu łącząco-ładującego").
- Binary program = a disk file whose **contents are a raw memory image** produced by `FILE*<title>[,<type>[,<base addr>[,<attr>]]]`. Base address = lowest address of the image (default 0); attribute default 050 (octal). [V] ASSM page "Zbiór programu wynikowego" and Dodatek 2. Words are placed at the address given by location counter `S`; file image starts at base. Exact byte-level layout of the file (is there a header? is word 0 = base address?) **[?]**. Process info gives "adres startowy (adres ładowania)" (PINF word 4) [V], and kernel data has JDAD/JPAD/JLOLEN ("adres pocz źródła ładowania", "adres pocz prg", "długość ładowania") [V] https://mera400.pl/Struktury_danych_CROOK-5 – suggests loader copies a sector range to JPAD; layout still **[?]**.
- Memory size of a process: determined by the highest address the translator emitted, or enlarged by `MEM* <addr>` ("largest address available to program at run time"). File label stores **parameter MEM = size in 4K-word quanta** allocated at first launch. [V] ASSM page "Inne dyrektywy"; CROOK-4 PDF p.97-98; https://mera400.pl/CROOK-5_-_Organizacja_informacji_w_systemie (label params: type, length, par1, par2, permission bits, attribute bits, MEM).
- **No relocation for ASSM binaries**: program has absolute addresses from `FILE*` base (default 0). Only attribute-4 modules (PASCAL compiler + assembler GASS) are "relocatable modules" and use an overlay loader (OVL with 4-char module name). [V] I/O page "Dla ekstrakodu OVL ... program o atrybucie 4 (relokowalne moduły PASCAL i GASS)". Format of those relocatable modules **[?]** (GASS/Pascal on the disk image: ASSM, ASSK, GASS, BASIC, Pascal, Fortran, C, RAL, CEMMA exist — https://mera400.pl/CROOK-5_-_obraz_dysku). A "C" compiler exists on the image (its object format **[?]**).
- Memory granularity: blocks of 4K words (0x1000). A process runs in a block shared with its parent, or (via CPRF+JUMP) in its own block(s) loaded from the file. [V] Procesy page; Zarządzanie pamięcią.
- Non-contiguous address space: process memory described by one 16-bit **memory map word**, bit15 = block 0 (0–07777 oct), … bit0 = block 15 (0170000–0177777). Each bit = 4K words ⇒ a process sees up to **16×4K = 64K words** logical space. [V] Zarządzanie pamięcią. Code and data are **not** separate segments: one 64K-word space (Von Neumann); no separate I/D spaces mentioned **[I]**. "Large memory" = more physical 32K/64K blocks selectable by SR NB bits (system), user processes get a single 64K logical space; no API to get >64K words found **[?]**.
- Overlay support in ASSM: `SS* <addr>` starts overlay (new disk sector), `#S` = disk address counter, `MAX*`, `LEN*` (length of first loaded overlay / resident part). Overlays loaded by OVL extracode. [V] ASSM page "Mechanizmy dla programów nakładkowanych".
- Program attribute (6 bits, from file label; set by `FILE*` 4th arg; default 050 octal=40 dec): bit0(MSB)=1 → program NOT prepared for hardware extension of byte address space to 64K words; bits1–2 = parameter-passing method for XOSL; bit3 = PASCAL result module; bit4 = resident; bit5 = privileged. [V] Procesy page "Atrybuty procesów". XOSL attribute low 3 bits: 010 (050) params in registers, 020 (060) decoded params message, 030 (070) raw line message, 040 macro template; values in parentheses = programs NOT ready for 17-bit byte addressing. [V] Środowisko page. So for a new MX-16/17-bit-byte-aware program the attribute should be 010 (no bit0) and for original CPU 050. Default 050 from ASSM means "registers + not 64K byte-addressing aware". LB/RB/CB can reach only the first 32K words of the 64K block unless MX-16 17-bit mode is on (https://mera400.pl/Modyfikacje_sprz%C4%99towe_procesora) [V].

## 2. Extracode (syscall) mechanism
- Instruction: `EXL b` – 8-bit byte argument b = extracode number (ASSM: `EXL, END`; error 027 = "EXL argument overflow"). [V] ASSM page; Lista rozkazów.
- Hardware semantics [V] https://mera400.pl/Lista_rozkaz%C3%B3w : `S[S[97]] := IC; S[S[97]+1] := R0; +2 := SR; +3 := b; R0 := 0; IC := S[96]; SR bits 9,10 := 0 (Q:=0 ⇒ system mode); S[97] += 4`. IC saved = address of next instruction. EXL vector in word 0140 (96 dec), stack pointer in 0141 (97 dec) – CROOK: EXL handler EXST (0x60), STACK = 0x2ffc in 8F kernel (https://mera400.pl/Mapa_pami%C4%99ci_CROOK-5). em400 implements `op_71_exl` (opcode group 071 in its naming; octal/decimal of the exact 16-bit encoding **not verified**; get it from em400 `src/cpu/instructions.c` / EMAS opcode table).
- Kernel dispatch table TABEXL at 0x59E, 65 entries [V] Mapa pamięci. Numbers 0200–0310 octal used (128–199) plus 0340–0377 "special" (alarm to parent).
- **Arguments**: single register **R4** = either a number/id or the address of a parameter block (all in words). Outputs: in parameter block words, in R4, and for sequential-input extracodes bit **C of R0**. INAM/INUM return in R1–R3 and C,V of R0. [V] Procesy page "Ekstrakody systemu operacyjnego".
- **Return convention** (not uniform): normal return = instruction after EXL. Some extracodes return at IC / IC+1 (LOD, UNL: skip one word = success; INAM: IC = fail, IC+1 = number in R2 [R1:R2 if C=1], R3 = terminator, V on overflow, IC+2 = name in R1,R2 3 chars/word R40, R3 = terminator). SIGN/WAIS take an extra word *after* the EXL (amount to add/subtract); semaphore address in R4. [V] Procesy, I/O pages, CROOK-4 PDF p.103.
- **Error handling**: parameter blocks (FIL, PROC, TMEM, BLOCK…) word 0 = error word: if non-zero on entry, an error causes an alarm (process stopped, parent informed); if zero, error number is stored in word 0 and execution continues. [V] Organizacja informacji ("słowo błędów"), Procesy.
- Register preservation across EXL: not stated explicitly **[?]** (PROC block shows R1–R7 saved per process; kernel saves/restores process context, so only declared outputs change **[I]**).
- Extracodes 0340–0377 produce a system alarm to the parent regardless of OES; parent (XOSL) interprets them. [V] Procesy page.

### Extracode numbers (decimal / octal) [V] https://mera400.pl/CROOK-5_-_Wykaz_ekstrakod%C3%B3w
Param field types: FIL (stream/file), STR (stream id/ptr), REC (char record), BLOCK (sector I/O), PROC, TIM/DAT, MET, PINF, tabl, r4 = plain value in R4.
| dec | oct | name | arg | meaning |
|---|---|---|---|---|
|128|0200|ASG|FIL|assign stream to existing file ("open")|
|129|0201|CASG|FIL|create file + assign stream ("create/open")|
|130|0202|SETP|FIL|set file params|
|131|0203|LOAP|FIL|load file/stream params|
|132|0204|TMEM|TMEM|attach 4K memory block of a RAM-file to process (resident procs only)|
|133|0205|NASG|FIL|assign stream, shadowing same id|
|134|0206|ERF|STR|delete file (via stream)|
|135|0207|ERS|STR|detach/remove local stream ("close")|
|136|0210|ERAS| |remove all local streams|
|137|0211|FBOF|STR|seek stream to beginning (char pointer, 32-bit)|
|138|0212|INPR|REC|read record (up to terminator char / max len)|
|139|0213|PRIR|REC|write record incl. terminator|
|140|0214|PINP|REC|output 2 chars (prompt) then read record|
|141|0215|PRIN|REC|write record without terminator|
|142|0216|EOF|STR|write end-of-file mark / close output file|
|143|0217|FEOF|STR|seek to end (append)|
|144|0220|INAM|REC|parse name/number from buffer|
|145|0221|INUM|REC|parse decimal number from buffer|
|146|0222|WADR|BLOCK|write address fields (privileged)|
|147|0223|OVL|BLOCK|load overlay from process' own file|
|148|0224|REAP|BLOCK|read special (swap) file of a process|
|149|0225|WRIP|BLOCK|write special file of a process|
|150|0226|READ|BLOCK|read sectors (random access) or receive message|
|151|0227|WRIT|BLOCK|write sectors|
|152|0230|OES|ERR|declare own alarm handler|
|153|0231|ERR| |pass my last alarm to parent|
|154|0232|CORE|r4|resize my memory block (R4 words; returns last address in R4)|
|155|0233|CPRF|FIL|create special file for own-block load|
|156|0234|JUMP|PROC|load process body into separate block and jump|
|157–159|0235–0237|SDIR/TDIR/CDIR|DIR|directory params|
|160|0240|DEFP|PROC|define child|
|161|0241|DELP|PROC|delete child (and descendants)|
|162|0242|SREG|PROC|set child regs|
|163|0243|TREG|PROC|get child regs|
|164|0244|RUNP|PROC|run child|
|165|0245|HANG|PROC|stop child|
|166|0246|TERR|r4|pop child from alarm list (-1 = empty)|
|167|0247|WAIT|r4|sleep R4 ticks (100 ms each)|
|168|0250|STOP|r4|stop self until RUNP/child alarm; R4≠0 also timeout in ticks|
|169|0251|RELD| |release character devices|
|170|0252|DATE|TIM|3 words: year, month, day at address in R4|
|171|0253|TIME|TIM|3 words: hour, minute, second at address in R4|
|173|0255|CHPI|r4|change own priority|
|174|0256|WAIS.n|r4|P(semaphore) – amount n in word after EXL|
|175|0257|SIGN.n|r4|V(semaphore)|
|176|0260|TLAB|MET|disk label|
|177|0261|PINF|PINF|process info|
|178–181|0262–0265|CSUM,CSYS,UNL,LOD|r4|system-admin|
|182|0266|TAKS|STR|stream semaphore take|
|183|0267|RELS|STR|release|
|184|0270|GMEM|r4|add memory blocks (R4 = map; new map = old OR arg; R4 = current map; arg 0 = query)|
|185|0271|RMEM|r4|release blocks|
|186|0272|LRAM|tabl|RAM-file info|
|189–191|0275–0277|OPPI,WFPI,CAMAC|…|PI / CAMAC|
|192–199|0300–0307|RWMT…WRMT|MT|tape ops|
Numbers 172 (0254), 187–188 (0273–0274), 200–248 are not listed **[?]**.

### Process exit / termination
- **There is no dedicated `exit` extracode in the 0200–0307 range.** Programs under XOSL exit with the *special* extracodes: **END = 0373 (251)** "end of program", EOSL = 0377 (255) (end + prints "FINISH"), ABO = 0375 (253) (delete program), KILL = 0376 (254) (delete with "DELETED"), BACK = 0374 (252) (continue XOSL, run self in background), SCON 0371 / TCON 0372 = set/get XOSL status word. These are alarms delivered to the parent (XOSL), which interprets them. [V] Wykaz ekstrakodów, Środowisko page. ASSM example: `EXL, END`. Exit code passing: R4 for END (table lists `r4`) – meaning of value **[?]**; XOSL "internal value" in R2 at start (below) suggests a numeric status convention **[?]**.
- Under INI directly (no XOSL), 0373 = reject operator call, 0374–0377 = end session (R4 = terminal number). [V]
- STOP (0250) also halts the process until the parent reruns it.

## 3. Process start-up state and stack
- Entry/initial registers depend on attribute and launch mode [V] Środowisko page:
  - attribute 010/050: parameters = values of arithmetic expressions in **R1, R2, …** consecutively; remaining registers zeroed.
  - attributes 020/030/060/070: **R1 = status word, R2 = internal value, R3 = number of screen lines** (REP LIN); other registers random; params arrive as a *message* via stream **MES** (read with READ).
  - Child launch via DEFP/SREG/JUMP: IC, R0 and R1–R7 given in PROC block (words 2,3,5–11) — so parent chooses entry IC. [V] Procesy page. JUMP: negative state word → alarm 0100 after load.
  - Entry IC when XOSL launches a file: **[?]** (not documented on wiki pages; maybe base address from `FILE*`, PINF word 4 "start address" suggests the load address; ASSM sets entry by transfer address only for overlay programs). Must be determined from kernel source/disk image (or em400 `emcrk`).
- **Stack: CROOK does not set up a user stack; none is documented.** The only stack is the hardware system stack at S[97] used by interrupts/EXL in system mode [V]. The user program must create its own stack in its memory block (reserve with MEM*/CORE and use R-register pointer) **[I]**. Standard ABI registers: none defined by CROOK; RJ uses a link register (e.g. `RJ r7`) [V] ASSM example `F=7. RJ, F(PROC1)`.
- Processor mode: user programs run with Q=1 (user mode); privileged instructions illegal except NLZ (ASSM page, "legalny w trybie użytkowym"). MB, IM, MD legal. [V]
- Memory beyond program: block(s) of 4K words; `CORE` with R4=0 returns last word address of current block; R4=n sets block size in words (not allowed for non-contiguous processes). [V] Zarządzanie pamięcią. Data heap: use CORE/GMEM/RMEM; no malloc.
- Alarms: process may register handler via OES (R4 → 3-word table: word0 = handler address, word1 = saved IC, word2 = alarm number; R4=-1 disables; handler is deregistered after each alarm). Without handler, XOSL prints alarm with program name + address. [V] Procesy, Środowisko.

## 4. Character / string conventions [V]
- Files are accessed as streams of **bytes (8-bit ASCII/ISO-7)**, sector = 256 words = 512 bytes. [V] https://mera400.pl/CROOK-5_-_Charakterystyka_systemu
- Buffers hold **2 chars per word, first char in the high (left) byte** (REC word 2; ASSM text `"..."`: earlier char on higher byte). Byte address = 2*wordaddr + (0 left/1 right).
- Records ("lines") end by a terminator char given in REC word 3 high byte; low byte = max count (0 = unlimited). Line terminator is **CR**; XOSL messages use LF text CR; ASSM treats CR as end-of-line; input end = **Ctrl-T**. [V] I/O page, ASSM page, https://mera400.pl/CROOK-5_w_EM400
- ASSM string literal `"text"`: bytes packed 2/word and translator adds a start byte (default none) and end byte (default **0200 octal = 128**, `TEXT*` changes). So strings are **not NUL-terminated by default**; `!x!` / `!` gives NUL byte; ASCII "xy" constant `!xy`. [V] ASSM page.
- Names (files, directories, streams) are **R40 (Radix-40)** 3 chars/word: file title = 5 words (area, dir[2], name[2]) etc. R40 code for names; `$ABC` in ASSM. [V] Organizacja informacji, ASSM.
- Bit/word terms: R-words = 16-bit; numbers two's complement; 32-bit pointer for file char position.

## 5. File I/O summary
- Stream id = any 16-bit number; ASG/CASG/NASG with FIL block {0 err, 1 stream id, 2 type, 3 length(sectors), 4 par1, 5 par2, 6 [bits0-5 perms | 6-11 attrs | 12-15 MEM], 7-11 title(5 words)}. [V] Organizacja page.
- Title: word0 area/space name (R40; reserved: STR, MES, MEM, DEV; RAM), words1-2 directory, 3-4 file name. Special title `STR` = refer to existing stream (word 3 = id). Devices are files of special type -6 in directory `DEV//`; standard streams INT (commands in) and OUT (messages out), MES, COM, MON local terminal. [V] Organizacja page, Środowisko page.
- Sequential: REC block {0 low byte = start char index; on return whole word = last char index; bit flags in high byte; 1 stream id; 2 buffer address; 3 [terminator<<8 | max length]; 4 (PINP: 2 prompt chars)}. **C bit in R0 set on end-of-file** on read. Read/write EOF automatically removes stream. [V] I/O page.
- Random: BLOCK {0 count of words transferred; 1 stream id; 2 mem address; 3 word count; 4 sector number relative to file start}. READ/WRIT. [V]
- No explicit seek-to-offset extracode: only FBOF (begin), FEOF (end) for char access and sector addressing for BLOCK access. [V] **[I]** arbitrary char seek not available.
- Time: DATE (year, month, day), TIME (h, m, s) to words at address in R4. Tick = 100 ms (WAIT/STOP). [V]
- Memory: CORE (resize block, words), GMEM/RMEM (4K block map), TMEM (attach 4K block from a RAM file; CROOK-4 PDF p.100: block {0 err, 1 stream, 2 address (multiple of 4K) for first word or 0=release, 4 block number in file}).

## 6. Build / transfer / run in EM400
- Run CROOK-5: download https://mera400.pl/img/crook5-p8f-1.1.1.zip (MD5 a19c1fe436926ad76a181d23d238d95e, CROOK-5 8/15 for **MX-16** = modified CPU); EM400: select machine MX-16, load .img as **Winchester 1:28**, open terminal 15:4, raise START and CLOCK; login `boss` or LIBRAR (no password); prompt of OSL. [V] CROOK-5_w_EM400 and obraz_dysku.
- Image areas: **S** (system: kernel, OSL, tools, ASSM, ASSK, GASS, BASIC, Pascal, Fortran, C, RAL, CEMMA) and **D** (docs). [V]
- Ways to get a cross-built binary in (none is documented as official; combos below are **[I]**):
  1. Convert binary to text and enter it with the EXM/EDIT editor and assemble on-target with ASSM (self-hosting, supported path).
  2. DOSF program (`SEND <pattern> -B`, binary copy sets file length into params PA1/PA2) copies files between DOS floppies (360K/1.2M) and CROOK; in EM400 only SP45DE 8" floppy is emulated (https://mera400.pl/EM400) so DOS-floppy transfer on 5.25" may not be available **[?]**. https://mera400.pl/Program_pomocniczy_DOSF
  3. Patch image directly using filesystem structure knowledge: https://mera400.pl/Struktura_talerzy_dyskowych (not read), emcrk library (github.com/jakubfi/emcrk: kernel finder `crkfind`, structs for obj/file/exl/process/cfg, r40 tool) – no disk-image write tool found. **[?]**
- Cross assemblers: ASSEM (ASSM/ASSK syntax on PC) and EMAS (modern syntax). EM400 has an `L_CRK5` logging mode that decodes extracodes (src/log_crk.c) – useful to debug syscalls by number.
- Boot of kernel: 3-stage loader, kernel 5888 words loaded to 0x900 then copied to 0; not relevant to user toolchain. https://mera400.pl/%C5%81adowanie_systemu_operacyjnego

## 7. ASSM/ASSK vs EMAS differences (from sources)
ASSM (ASSK's successor, K-202 origin): one-pass, fixed token syntax, statement terminators `.` (insn), `)` after 2-word immediate, `:` label, `*` directives.
- Instruction syntax: `MNEM[,A][,C|(imm)|short][&B]['].` e.g. `LW, 3,5.` (R3:=R5), `LW, 4(-1200)`, `LW, 3(TABLE&5')`, `JG(LAB1&E)`, `F=7. RJ, F(PROC1)`; `&n` = B-modification, `'` = D (indirect), `(...)` = 16-bit immediate in 2nd word, bare number = 7-bit short arg (±63), `MD(24&6)`.
- Variables: ≤6 chars (+#,%,$ prefix); `name = expr.` plain; `name:` address label; `S` = location counter (`S* addr.`); `#S` disk address; expressions only `+`/`-` and scaling `/n`; octal if leading 0; `!ab` char pair, `$ABC` R40, `?CZ` flag-bit mask (bits ZMVCLEGY X1234567 ↔ R0 bits 0..15).
- Directives: FILE*, INT*, OUT*, LAB*/NLAB*, S*, PROG*/FINPROG*, SEG*/FINSEG*, MACRO*/FINMACRO* (no real macro generator), ALL*, RES*, TEXT*, MEM*, IF UNK*/UND*/DEF* … FI*, SS*, MAX*, LEN*, OS*. Max line 150 chars. Extracode names in a separate dictionary (`EXL, END`). Comments `[...]` and printed `<...>`.
- ASSEM (PC cross assembler) implements the same syntax but **lacks** FILE*, INT*, OUT*, LAB*, NLAB*, MEM*, IFUND*, LEN*, OS* (ASSM) and F*, T*, NTRAC*, TRAC*, NAME*, BA*, HS*, E* (ASSK) – i.e. no `MEM*`/`FILE*` produce the program file attributes (base, attr, MEM). https://mera400.pl/ASSEM
- Syntax details of EMAS (comma-free, `LW r1, r2+r3`, `.word`, `.org`, `.include`) from the 17-bit byte example and em400 tests only; full EMAS reference page not read **[?]** https://mera400.pl/EMAS .

## 8. NUCON / other compilers
- No page about "NUCON" found on the pages searched **[?]**. Compilers on image: ALGOL-1204M, CSL-CROOK, FORTRAN-CROOK, MODULA-2, PASCAL 400, C, RAL, LISP, BASIC (docs in area D/OPISY; wiki pages for ALGOL-1204M and CSL-CROOK exist). https://mera400.pl/Oprogramowanie , https://mera400.pl/CROOK-5_-_obraz_dysku
- C compiler produces input for ASSM ("przepełnienie słownika przy wołaniu przez kompilator języka 'C'", error 034) and Pascal+GASS produce relocatable modules (attribute 4) loaded by OVL. [V] ASSM errors list; I/O page.

## 9. Open gaps (priority for the toolchain)
1. Exact on-disk layout of a CROOK binary file (header? first sector? how base address and length are recorded, relation to label params PA1/PA2) and the **initial IC** of a newly started program.
2. Exit-status semantics of END (0373)/EOSL and whether R4 is returned to parent; exact EXL opcode encoding verification (take from EMAS/em400 table).
3. Whether any register-preservation/stack convention exists in system libraries (C compiler runtime of CROOK, Pascal) — inspect on the disk image (area S, C libs) or the C compiler docs in area D `OPISY`.
4. How to inject a binary into the image reliably (file-system structure page `Struktura_talerzy_dyskowych` not read; DOSF/floppy path unclear in EM400).
5. Relocatable module format (GASS/Pascal) — only mentioned, not described.
6. NUCON: no information found.
