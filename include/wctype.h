/*===-- wctype.h - wide character classes (declarations only) -- ===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_WCTYPE_H
#define MERA400_WCTYPE_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Declarations only, so that C++ headers parse; nothing is implemented. */
typedef int wctype_t;
typedef int wctrans_t;

int iswalnum(wint_t c);
int iswalpha(wint_t c);
int iswblank(wint_t c);
int iswcntrl(wint_t c);
int iswdigit(wint_t c);
int iswgraph(wint_t c);
int iswlower(wint_t c);
int iswprint(wint_t c);
int iswpunct(wint_t c);
int iswspace(wint_t c);
int iswupper(wint_t c);
int iswxdigit(wint_t c);
int iswctype(wint_t c, wctype_t t);
wctype_t wctype(const char *name);
wint_t towlower(wint_t c);
wint_t towupper(wint_t c);

#ifdef __cplusplus
}
#endif

#endif
