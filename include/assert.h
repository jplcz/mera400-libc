/*===-- assert.h - run-time assertions -------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
/* Deliberately without an include guard: assert.h may be included again
   after changing NDEBUG. */
#undef assert

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
/* There is no stdio yet, so a failed assertion just aborts (hlt 040). */
#ifdef __cplusplus
extern "C"
#endif
void __assert_fail(const char *expr, const char *file, int line);
#define assert(expr) \
  ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__))
#endif
