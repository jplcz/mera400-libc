/*===-- stdlib.h - general utilities ---------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_STDLIB_H
#define MERA400_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

/* exit, _Exit and abort: crt/crt0.s and src/stdlib/exit.s. exit runs the
   functions registered with atexit (last registered first), then the
   .fini_array (C++ static destructors), then _Exit, which stops the
   processor (hlt). */
void exit(int status);
void _Exit(int status);
void abort(void);
int atexit(void (*func)(void));

int abs(int x);
long labs(long x);
int atoi(const char *s);
long atol(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);

int rand(void);
void srand(unsigned int seed);

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *));

/* A first-fit allocator over the region [__heap_start, __heap_end). */
/* Declared so that C++ headers (libc++) parse. These are not implemented: using
   one fails at link time. */
typedef struct {
  int quot;
  int rem;
} div_t;
typedef struct {
  long quot;
  long rem;
} ldiv_t;
typedef struct {
  long long quot;
  long long rem;
} lldiv_t;

div_t div(int n, int d);
ldiv_t ldiv(long n, long d);
lldiv_t lldiv(long long n, long long d);
long long llabs(long long x);
long long atoll(const char *s);
long long strtoll(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
double atof(const char *s);
double strtod(const char *s, char **end);
float strtof(const char *s, char **end);
long double strtold(const char *s, char **end);
char *getenv(const char *name);
int system(const char *cmd);
int mblen(const char *s, size_t n);
int mbtowc(wchar_t *pwc, const char *s, size_t n);
int wctomb(char *s, wchar_t wc);
size_t mbstowcs(wchar_t *d, const char *s, size_t n);
size_t wcstombs(char *d, const wchar_t *s, size_t n);
void *aligned_alloc(size_t alignment, size_t n);
void quick_exit(int status);
int at_quick_exit(void (*func)(void));

void *malloc(size_t n);
void *calloc(size_t count, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);

#ifdef __cplusplus
}
#endif

#endif
