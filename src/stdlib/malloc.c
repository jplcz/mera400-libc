/*===-- malloc.c - a simple first-fit allocator ----------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#include <stdlib.h>
#include <string.h>

/* The heap is the region between two linker script symbols. Every block
   starts with a header holding its size (a whole number of headers, so the
   data is word aligned) and a free flag; blocks follow each other, so the
   next block is found by adding the size. free() merges the block with a
   free successor, and malloc() merges runs of free blocks while it
   searches, so no separate free list is needed. */
typedef struct block {
  unsigned int size; /* bytes of data after the header */
  unsigned int free;
} block;

extern char __heap_start[];
extern char __heap_end[];

static block *first;

static void init(void) {
  size_t total = (size_t)(__heap_end - __heap_start);
  first = (block *)__heap_start;
  first->size = total - sizeof(block);
  first->free = 1;
}

static block *next(block *b) {
  block *n = (block *)((char *)(b + 1) + b->size);
  return (char *)n < __heap_end ? n : NULL;
}

void *malloc(size_t n) {
  block *b;
  if (!first)
    init();
  n = (n + 3) & ~3u; /* two words: keeps every block word aligned */
  if (n == 0)
    n = 4;
  for (b = first; b; b = next(b)) {
    block *nb;
    if (!b->free)
      continue;
    while ((nb = next(b)) && nb->free)
      b->size += sizeof(block) + nb->size;
    if (b->size < n)
      continue;
    if (b->size >= n + sizeof(block) + 4) {
      block *rest = (block *)((char *)(b + 1) + n);
      rest->size = b->size - n - sizeof(block);
      rest->free = 1;
      b->size = n;
    }
    b->free = 0;
    return b + 1;
  }
  return NULL;
}

void free(void *p) {
  block *b;
  if (!p)
    return;
  b = (block *)p - 1;
  b->free = 1;
}

void *calloc(size_t count, size_t size) {
  size_t total;
  void *p;
  if (size && count > (size_t)-1 / size)
    return NULL;
  total = count * size;
  p = malloc(total);
  if (p)
    memset(p, 0, total);
  return p;
}

void *realloc(void *p, size_t n) {
  block *b;
  void *q;
  if (!p)
    return malloc(n);
  b = (block *)p - 1;
  if (b->size >= n)
    return p;
  q = malloc(n);
  if (q) {
    memcpy(q, p, b->size);
    free(p);
  }
  return q;
}
