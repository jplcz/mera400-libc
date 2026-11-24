/*===-- inttypes.h - integer format macros -- ===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_INTTYPES_H
#define MERA400_INTTYPES_H

#include <stdint.h>

/* int is 16 bits, long 32, long long 64. The "fast" and "least" types map onto
   those, as in stdint.h. */
#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "ld"
#define PRId64 "lld"
#define PRIi8 "i"
#define PRIi16 "i"
#define PRIi32 "li"
#define PRIi64 "lli"
#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "lu"
#define PRIu64 "llu"
#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "lx"
#define PRIx64 "llx"
#define PRIX8 "X"
#define PRIX16 "X"
#define PRIX32 "lX"
#define PRIX64 "llX"
#define PRIo8 "o"
#define PRIo16 "o"
#define PRIo32 "lo"
#define PRIo64 "llo"
#define PRIdPTR "d"
#define PRIuPTR "u"
#define PRIxPTR "x"
#define PRIdMAX "lld"
#define PRIuMAX "llu"

#endif
