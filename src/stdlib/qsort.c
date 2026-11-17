/*===-- qsort.c - qsort and bsearch ----------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <stdlib.h>

static void swap(char *a, char *b, size_t size) {
  while (size--) {
    char t = *a;
    *a++ = *b;
    *b++ = t;
  }
}

/* Heap sort: no recursion (the stack is small), no allocation and a
   worst case of O(n log n). It is not stable. */
static void sift_down(char *base, size_t size, size_t root, size_t n,
                      int (*cmp)(const void *, const void *)) {
  for (;;) {
    size_t child = 2 * root + 1;
    if (child >= n)
      return;
    if (child + 1 < n &&
        cmp(base + child * size, base + (child + 1) * size) < 0)
      ++child;
    if (cmp(base + root * size, base + child * size) >= 0)
      return;
    swap(base + root * size, base + child * size, size);
    root = child;
  }
}

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *)) {
  char *b = (char *)base;
  size_t i;
  if (n < 2 || size == 0)
    return;
  for (i = n / 2; i-- > 0;)
    sift_down(b, size, i, n, cmp);
  for (i = n - 1; i > 0; --i) {
    swap(b, b + i * size, size);
    sift_down(b, size, 0, i, cmp);
  }
}

void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *)) {
  const char *b = (const char *)base;
  while (n) {
    size_t mid = n / 2;
    const char *p = b + mid * size;
    int r = cmp(key, p);
    if (r == 0)
      return (void *)p;
    if (r > 0) {
      b = p + size;
      n -= mid + 1;
    } else {
      n = mid;
    }
  }
  return NULL;
}
