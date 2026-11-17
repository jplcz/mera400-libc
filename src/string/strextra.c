/*===-- strextra.c - string functions written in portable C ----------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <string.h>

size_t strnlen(const char *s, size_t n) {
  size_t i = 0;
  while (i < n && s[i])
    ++i;
  return i;
}

size_t strspn(const char *s, const char *accept) {
  size_t i = 0;
  while (s[i] && strchr(accept, s[i]))
    ++i;
  return i;
}

size_t strcspn(const char *s, const char *reject) {
  size_t i = 0;
  while (s[i] && !strchr(reject, s[i]))
    ++i;
  return i;
}

char *strpbrk(const char *s, const char *accept) {
  s += strcspn(s, accept);
  return *s ? (char *)s : NULL;
}

char *strstr(const char *haystack, const char *needle) {
  size_t n = strlen(needle);
  if (n == 0)
    return (char *)haystack;
  for (; *haystack; ++haystack)
    if (*haystack == *needle && strncmp(haystack, needle, n) == 0)
      return (char *)haystack;
  return NULL;
}
