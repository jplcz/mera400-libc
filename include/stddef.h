/*===-- stddef.h - common definitions --------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_STDDEF_H
#define MERA400_STDDEF_H

/* A word is 16 bits, so size_t (the result of sizeof, counted in 8-bit
   chars) and pointer differences are 16-bit in the small memory model. */
typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef int wchar_t;

#define NULL ((void *)0)
#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
