/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */
/* CROOK operating system interface (EXL extracode wrappers).
 *
 * Pointers to blocks (FIL, REC, BLOCK, STR) are WORD addresses in CROOK,
 * whereas a C pointer is a BYTE address. Pass word addresses as
 * (unsigned)((unsigned)ptr / 2), or use blocks placed in assembly.
 * See src/crook/README.md for the layouts and the list of unverified calls. */
#ifndef CROOK_H
#define CROOK_H

/* Generic call: EXL number (0200..0307), R4 argument; the carry flag is
 * stored in *carry when carry is not NULL. Returns the R4 value after. */
unsigned __crook_exl(unsigned number, unsigned r4, unsigned *carry);

void exit(int status);
void _exit(int status);
void abort(void);
/* Program end under XOSL (none returns, except BACK): EOSL prints "FINISH",
 * ABO deletes the program, KILL deletes it and prints "DELETED". BACK lets
 * XOSL continue while this program runs on in the background. */
void __crook_exit_finish(int status);
void __crook_abo(int status);
void __crook_kill(int status);
void __crook_back(void);

/* File assignment and parameters: return word 0 of the FIL block (error
 * word). SETP sets and LOAP loads the file parameters, CPRF creates the
 * special file for loading a process into its own block. */
unsigned __crook_asg(unsigned fil);
unsigned __crook_casg(unsigned fil);
unsigned __crook_nasg(unsigned fil);
unsigned __crook_setp(unsigned fil);
unsigned __crook_loap(unsigned fil);
unsigned __crook_cprf(unsigned fil);
/* No argument: remove all local streams (ERAS), release the character
 * devices (RELD). */
void __crook_eras(void);
void __crook_reld(void);

/* Record I/O: return the carry flag (1 = end of file). */
unsigned __crook_inpr(unsigned rec);
unsigned __crook_prir(unsigned rec);
unsigned __crook_pinp(unsigned rec);
unsigned __crook_prin(unsigned rec);

/* Block I/O and memory blocks: return word 0 of the BLOCK block. OVL loads an
 * overlay, REAP/WRIP read/write the special file of a process, WADR is
 * privileged. */
unsigned __crook_read(unsigned blk);
unsigned __crook_writ(unsigned blk);
unsigned __crook_tmem(unsigned blk);
unsigned __crook_ovl(unsigned blk);
unsigned __crook_reap(unsigned blk);
unsigned __crook_wrip(unsigned blk);
unsigned __crook_wadr(unsigned blk);

/* Values returned in R4. GMEM, RMEM and LRAM exist in CROOK-5 only (CROOK-4
 * has no non-contiguous memory map), so they are declared only when the
 * program is built for CROOK 5 or later: __CROOK_VERSION__ is 4 or 5, set by
 * Clang's -mcrook-version=. A call in a CROOK-4 build is a compile error
 * instead of an alarm on the real system. */
unsigned __crook_core(unsigned arg);
#if !defined(__CROOK_VERSION__) || __CROOK_VERSION__ >= 5
unsigned __crook_gmem(unsigned arg);
unsigned __crook_rmem(unsigned arg);
unsigned __crook_lram(unsigned tab);   /* tab: word address of the table */
#endif
unsigned __crook_wait(unsigned arg);
unsigned __crook_stop(unsigned arg);
unsigned __crook_date(unsigned arg);
unsigned __crook_time(unsigned arg);

/* Text conversion. from_c: first line of a C string -> CR terminated text,
 * returns the number of characters. to_c: CR terminated -> C string with
 * '\n' for CR. dst/src are byte pointers; max limits the characters. */
unsigned __crook_from_c(char *dst, const char *src, unsigned max);
unsigned __crook_to_c(char *dst, const char *src, unsigned max);

#endif
