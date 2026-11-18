/*===-- crook_guard.c - GMEM and RMEM exist only for CROOK 5 ---------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Only compiled (-fsyntax-only), never linked or run. For CROOK 5 the program
 * calls the two memory extracodes, so the build fails if <crook.h> does not
 * declare them. For CROOK 4 it defines objects with the same names, so the
 * build fails if <crook.h> declares them: a program that calls them would
 * then not compile either.
 */
#include <crook.h>

#if !defined(__CROOK_VERSION__)
#error "__CROOK_VERSION__ is not defined"
#elif __CROOK_VERSION__ >= 5
unsigned use_memory(void) { return __crook_gmem(0) | __crook_rmem(0); }
#else
int __crook_gmem;
int __crook_rmem;
#endif
