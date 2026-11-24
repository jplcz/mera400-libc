/*===-- locale.h - locales (declarations only) -- ===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_LOCALE_H
#define MERA400_LOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Declarations only, so that C++ headers parse; there is only the "C" locale
   and setlocale is not implemented. */
struct lconv {
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
};

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

#ifdef __cplusplus
}
#endif

#endif
