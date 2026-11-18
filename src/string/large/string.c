/*===-- string.c - the string functions of the large memory model ----------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Why this file exists: the assembly versions in the parent directory take
 * 16-bit byte pointers and a 16-bit n, which is the small memory model's
 * calling convention. In the large model a pointer is a 32-bit byte address
 * (two registers, high word first), size_t is 32 bits wide, and the lb/rb
 * instructions cannot reach words at or above 0x8000, so the compiler reads
 * and writes a byte by loading or storing the whole word and masking.
 *
 * These are therefore written in C and compiled with -mlarge-memory, which
 * produces exactly that code. The speed-critical routines (the mem*
 * functions and strlen) have a fast path: when everything is word aligned
 * they work on whole 16-bit words through an "unsigned short *", which the
 * compiler turns into a plain word access (the byte pointer is shifted right
 * by one). A byte at an odd address is one octet of a word, as always.
 *
 * The library is built with -ffreestanding, so the compiler does not turn
 * these loops back into calls of memcpy and friends.
 */
#include <stddef.h>
#include <string.h>

typedef unsigned short word;

static int is_even(const void *p) { return !((size_t)p & 1); }

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  if (n >= 2 && is_even(d) && is_even(s)) {
    word *dw = (word *)d;
    const word *sw = (const word *)s;
    size_t w = n >> 1;
    while (w--)
      *dw++ = *sw++;
    d = (unsigned char *)dw;
    s = (const unsigned char *)sw;
    n &= 1;
  }
  while (n--)
    *d++ = *s++;
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  if (d <= s)
    return memcpy(dest, src, n); /* forward copying is safe */
  d += n;
  s += n;
  if (n >= 2 && is_even(d) && is_even(s)) {
    word *dw = (word *)d;
    const word *sw = (const word *)s;
    size_t w = n >> 1;
    while (w--)
      *--dw = *--sw;
    d = (unsigned char *)dw;
    s = (const unsigned char *)sw;
    n &= 1;
  } else if (n >= 2 && (n & 1) && !is_even(d) && !is_even(s)) {
    /* both ends are odd, so both starts are even: copy the last byte, then
       whole words */
    word *dw;
    const word *sw;
    size_t w;
    *--d = *--s;
    --n;
    dw = (word *)d;
    sw = (const word *)s;
    w = n >> 1;
    while (w--)
      *--dw = *--sw;
    return dest;
  }
  while (n--)
    *--d = *--s;
  return dest;
}

void *memset(void *dest, int c, size_t n) {
  unsigned char *d = dest;
  if (n >= 2 && is_even(d)) {
    word fill = (word)((c & 0xFF) | ((c & 0xFF) << 8));
    word *dw = (word *)d;
    size_t w = n >> 1;
    while (w--)
      *dw++ = fill;
    d = (unsigned char *)dw;
    n &= 1;
  }
  while (n--)
    *d++ = (unsigned char)c;
  return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *p = a, *q = b;
  if (n >= 2 && is_even(p) && is_even(q)) {
    const word *pw = (const word *)p, *qw = (const word *)q;
    size_t w = n >> 1;
    while (w && *pw == *qw) {
      ++pw;
      ++qw;
      --w;
    }
    p = (const unsigned char *)pw;
    q = (const unsigned char *)qw;
    n = (w << 1) | (n & 1);
  }
  for (; n; --n, ++p, ++q)
    if (*p != *q)
      return (int)*p - (int)*q;
  return 0;
}

void *memchr(const void *s, int c, size_t n) {
  const unsigned char *p = s;
  for (; n; --n, ++p)
    if (*p == (unsigned char)c)
      return (void *)p;
  return NULL;
}

size_t strlen(const char *s) {
  const char *p = s;
  const word *w;
  while (!is_even(p)) {
    if (!*p)
      return (size_t)(p - s);
    ++p;
  }
  w = (const word *)p;
  while ((*w & 0xFF00) && (*w & 0x00FF))
    ++w;
  p = (const char *)w;
  while (*p)
    ++p;
  return (size_t)(p - s);
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; ++i)
    dest[i] = src[i];
  for (; i < n; ++i)
    dest[i] = 0;
  return dest;
}

char *strcat(char *dest, const char *src) {
  strcpy(dest + strlen(dest), src);
  return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
  char *d = dest + strlen(dest);
  for (; n && *src; --n)
    *d++ = *src++;
  *d = 0;
  return dest;
}

int strcmp(const char *a, const char *b) {
  const unsigned char *p = (const unsigned char *)a;
  const unsigned char *q = (const unsigned char *)b;
  while (*p && *p == *q) {
    ++p;
    ++q;
  }
  return (int)*p - (int)*q;
}

int strncmp(const char *a, const char *b, size_t n) {
  const unsigned char *p = (const unsigned char *)a;
  const unsigned char *q = (const unsigned char *)b;
  for (; n; --n, ++p, ++q) {
    if (*p != *q)
      return (int)*p - (int)*q;
    if (!*p)
      return 0;
  }
  return 0;
}

char *strchr(const char *s, int c) {
  for (;; ++s) {
    if (*s == (char)c)
      return (char *)s;
    if (!*s)
      return NULL;
  }
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  for (;; ++s) {
    if (*s == (char)c)
      last = s;
    if (!*s)
      return (char *)last;
  }
}
