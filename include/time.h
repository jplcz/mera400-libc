/*===-- time.h - time types (declarations only) -- ===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_TIME_H
#define MERA400_TIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The machine has no clock, so no time function is implemented. The types and
   declarations exist so that C++ headers (libc++ <chrono>) parse. */
typedef long time_t;
typedef long clock_t;

#define CLOCKS_PER_SEC 1000L

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
};

struct timespec {
  time_t tv_sec;
  long tv_nsec;
};

#define TIME_UTC 1

clock_t clock(void);
time_t time(time_t *t);
double difftime(time_t a, time_t b);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
int timespec_get(struct timespec *ts, int base);

#ifdef __cplusplus
}
#endif

#endif
