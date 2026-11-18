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

/* File assignment: return word 0 of the FIL block (error word). */
unsigned __crook_asg(unsigned fil);
unsigned __crook_casg(unsigned fil);
unsigned __crook_nasg(unsigned fil);

/* Record I/O: return the carry flag (1 = end of file). */
unsigned __crook_inpr(unsigned rec);
unsigned __crook_prir(unsigned rec);
unsigned __crook_pinp(unsigned rec);
unsigned __crook_prin(unsigned rec);

/* Block I/O and memory blocks: return word 0 of the BLOCK block. */
unsigned __crook_read(unsigned blk);
unsigned __crook_writ(unsigned blk);
unsigned __crook_tmem(unsigned blk);

/* Values returned in R4. GMEM and RMEM exist in CROOK-5 only (CROOK-4 has no
 * non-contiguous memory map), so they are declared only when the program is
 * built for CROOK 5 or later: __CROOK_VERSION__ is 4 or 5, set by Clang's
 * -mcrook-version=. A call in a CROOK-4 build is a compile error instead of
 * an alarm on the real system. LRAM (0272, also CROOK-5 only) has no wrapper;
 * reach it with __crook_exl. */
unsigned __crook_core(unsigned arg);
#if !defined(__CROOK_VERSION__) || __CROOK_VERSION__ >= 5
unsigned __crook_gmem(unsigned arg);
unsigned __crook_rmem(unsigned arg);
#endif
unsigned __crook_wait(unsigned arg);
unsigned __crook_date(unsigned arg);
unsigned __crook_time(unsigned arg);

/* Text conversion. from_c: first line of a C string -> CR terminated text,
 * returns the number of characters. to_c: CR terminated -> C string with
 * '\n' for CR. dst/src are byte pointers; max limits the characters. */
unsigned __crook_from_c(char *dst, const char *src, unsigned max);
unsigned __crook_to_c(char *dst, const char *src, unsigned max);

#endif
