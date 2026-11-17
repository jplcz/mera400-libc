/*===-- strtol.c - string to integer conversion ----------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* Shared by strtol and strtoul: parses sign, base prefix and digits into an
   unsigned magnitude. Sets *overflow when the magnitude does not fit. */
static unsigned long parse(const char *s, char **end, int base, int *neg,
                           int *overflow) {
  const char *start = s;
  unsigned long value = 0;
  int any = 0;

  *neg = 0;
  *overflow = 0;
  while (isspace((unsigned char)*s))
    ++s;
  if (*s == '-' || *s == '+')
    *neg = (*s++ == '-');
  if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X') &&
      isxdigit((unsigned char)s[2])) {
    s += 2;
    base = 16;
  } else if (base == 0) {
    base = (s[0] == '0') ? 8 : 10;
  }
  if (base < 2 || base > 36) {
    if (end)
      *end = (char *)start;
    return 0;
  }
  for (;; ++s) {
    int c = (unsigned char)*s, digit;
    if (isdigit(c))
      digit = c - '0';
    else if (isalpha(c))
      digit = tolower(c) - 'a' + 10;
    else
      break;
    if (digit >= base)
      break;
    any = 1;
    if (value > (ULONG_MAX - digit) / base)
      *overflow = 1;
    else
      value = value * base + digit;
  }
  if (end)
    *end = (char *)(any ? s : start);
  return value;
}

long strtol(const char *s, char **end, int base) {
  int neg, overflow;
  unsigned long v = parse(s, end, base, &neg, &overflow);
  if (overflow || v > (neg ? (unsigned long)LONG_MAX + 1 : (unsigned long)LONG_MAX)) {
    errno = ERANGE;
    return neg ? LONG_MIN : LONG_MAX;
  }
  return neg ? (long)(0UL - v) : (long)v;
}

unsigned long strtoul(const char *s, char **end, int base) {
  int neg, overflow;
  unsigned long v = parse(s, end, base, &neg, &overflow);
  if (overflow) {
    errno = ERANGE;
    return ULONG_MAX;
  }
  return neg ? 0UL - v : v;
}

int atoi(const char *s) { return (int)strtol(s, (char **)0, 10); }
long atol(const char *s) { return strtol(s, (char **)0, 10); }
