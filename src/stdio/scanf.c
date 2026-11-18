/*===-- scanf.c - a minimal sscanf -----------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Conversions: %d %i %u %x %o (with h, l modifiers), %s, %c, %% and the
 * assignment suppression '*' and a maximum field width. White space in the
 * format matches any white space in the input. Returns the number of
 * assigned items, or EOF when the input ends before the first conversion.
 * No floating point and no scan sets.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int digit_value(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 99;
}

int sscanf(const char *s, const char *fmt, ...) {
  va_list ap;
  int assigned = 0;
  va_start(ap, fmt);
  for (; *fmt; ++fmt) {
    int suppress = 0, width = 0, lng = 0;
    if (isspace((unsigned char)*fmt)) {
      while (isspace((unsigned char)*s))
        ++s;
      continue;
    }
    if (*fmt != '%') {
      if (*s != *fmt)
        goto done;
      ++s;
      continue;
    }
    ++fmt;
    if (*fmt == '*') {
      suppress = 1;
      ++fmt;
    }
    while (*fmt >= '0' && *fmt <= '9')
      width = width * 10 + (*fmt++ - '0');
    for (;; ++fmt) {
      if (*fmt == 'l')
        ++lng;
      else if (*fmt == 'h')
        --lng;
      else
        break;
    }
    if (!*fmt)
      break;
    if (*fmt == '%') {
      if (*s != '%')
        goto done;
      ++s;
      continue;
    }
    if (*fmt != 'c')
      while (isspace((unsigned char)*s))
        ++s;
    if (!*s) {
      if (!assigned)
        assigned = EOF;
      goto done;
    }
    if (*fmt == 'c') {
      char *p = suppress ? 0 : va_arg(ap, char *);
      if (width == 0)
        width = 1;
      while (width-- > 0 && *s) {
        if (p)
          *p++ = *s;
        ++s;
      }
      assigned += !suppress;
    } else if (*fmt == 's') {
      char *p = suppress ? 0 : va_arg(ap, char *);
      while (*s && !isspace((unsigned char)*s) && (width == 0 || width-- > 0)) {
        if (p)
          *p++ = *s;
        ++s;
      }
      if (p)
        *p = 0;
      assigned += !suppress;
    } else if (*fmt == 'd' || *fmt == 'i' || *fmt == 'u' || *fmt == 'x' ||
               *fmt == 'X' || *fmt == 'o') {
      int base = *fmt == 'x' || *fmt == 'X' ? 16 : *fmt == 'o' ? 8 : 10;
      int neg = 0, any = 0, left = width ? width : 32767;
      unsigned long v = 0;
      if (left > 0 && (*s == '-' || *s == '+')) {
        neg = *s == '-';
        ++s;
        --left;
      }
      if (*fmt == 'i') {
        base = 10;
        if (left > 1 && s[0] == '0')
          base = (s[1] == 'x' || s[1] == 'X') ? 16 : 8;
      }
      if (base == 16 && left > 2 && s[0] == '0' &&
          (s[1] == 'x' || s[1] == 'X') && digit_value(s[2]) < 16) {
        s += 2;
        left -= 2;
      }
      while (left > 0 && digit_value((unsigned char)*s) < base) {
        v = v * base + digit_value((unsigned char)*s);
        ++s;
        --left;
        any = 1;
      }
      if (!any)
        goto done;
      if (neg)
        v = 0UL - v;
      if (!suppress) {
        if (lng >= 1)
          *va_arg(ap, long *) = (long)v;
        else if (lng < 0)
          *va_arg(ap, short *) = (short)v;
        else
          *va_arg(ap, int *) = (int)v;
        ++assigned;
      }
    } else {
      goto done;
    }
  }
done:
  va_end(ap);
  return assigned;
}
