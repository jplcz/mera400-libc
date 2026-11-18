/*===-- printf.c - the formatted output core and the printf family ---------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * One formatter, vformat(), writes into an "out" object that is either a
 * FILE or a memory buffer. Supported: flags - + space # 0, width and
 * precision (also *), length modifiers hh h l ll z t j, and the conversions
 * d i u x X o c s p % f F e E g G n.
 *
 * The floating point conversions work on the processor's native double (a
 * 39-bit mantissa, about 11 decimal digits): digits beyond the 17th are
 * printed as zeros and the last digits printed beyond 11 are not meaningful.
 * Rounding is "half up" on the decimal digits that were generated.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct out {
  FILE *f;
  char *buf;
  size_t cap; /* size of buf; 0 when there is none */
  size_t n;   /* characters produced (also those that did not fit) */
};

static void emit(struct out *o, char c) {
  if (o->f) {
    fputc((unsigned char)c, o->f);
  } else if (o->cap && o->n < o->cap - 1) {
    o->buf[o->n] = c;
  }
  ++o->n;
}

enum { F_LEFT = 1, F_PLUS = 2, F_SPACE = 4, F_ALT = 8, F_ZERO = 16 };

static void repeat(struct out *o, char c, int n) {
  while (n-- > 0)
    emit(o, c);
}

/* One converted field: padding, sign/prefix, zeros, body. */
static void field(struct out *o, int fl, int width, const char *prefix,
                  int zeros, const char *body, int len) {
  int plen = (int)strlen(prefix);
  int pad = width - plen - zeros - len;
  if (!(fl & (F_LEFT | F_ZERO)))
    repeat(o, ' ', pad);
  while (*prefix)
    emit(o, *prefix++);
  if ((fl & F_ZERO) && !(fl & F_LEFT))
    repeat(o, '0', pad);
  repeat(o, '0', zeros);
  while (len-- > 0)
    emit(o, *body++);
  if (fl & F_LEFT)
    repeat(o, ' ', pad);
}

/* --- integers ----------------------------------------------------------- */

/* The digits are produced right to left, ending at end; returns the start.
   The 32-bit and the 64-bit variant are separate because 64-bit division
   is large and slow on this machine. */
static char *digits32(unsigned long v, unsigned base, int upper, char *end) {
  const char *set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  while (v) {
    *--end = set[v % base];
    v /= base;
  }
  return end;
}

static char *digits64(unsigned long long v, unsigned base, int upper,
                      char *end) {
  const char *set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  while (v) {
    *--end = set[(unsigned)(v % base)];
    v /= base;
  }
  return end;
}

static void integer(struct out *o, int fl, int width, int prec, unsigned base,
                    int upper, int neg, int is64, unsigned long long v64) {
  char tmp[24];
  char *end = tmp + sizeof tmp, *p;
  char prefix[3];
  int zeros, len, np = 0;

  if (is64)
    p = digits64(v64, base, upper, end);
  else
    p = digits32((unsigned long)v64, base, upper, end);
  len = (int)(end - p);
  if (len == 0 && prec != 0) {
    *--p = '0';
    len = 1;
  }
  if (prec >= 0)
    fl &= ~F_ZERO;
  if (prec == 0 && v64 == 0)
    len = 0;
  zeros = prec > len ? prec - len : 0;
  if (base == 8 && (fl & F_ALT) && zeros == 0 && (len == 0 || *p != '0'))
    zeros = 1;
  if (base == 10) {
    if (neg)
      prefix[np++] = '-';
    else if (fl & F_PLUS)
      prefix[np++] = '+';
    else if (fl & F_SPACE)
      prefix[np++] = ' ';
  } else if (base == 16 && (fl & F_ALT) && v64 != 0) {
    prefix[np++] = '0';
    prefix[np++] = upper ? 'X' : 'x';
  }
  prefix[np] = 0;
  field(o, fl, width, prefix, zeros, p, len);
}

/* --- floating point ------------------------------------------------------ */

#define MAXDIG 48

/* Scales x (> 0) into [1, 10) and returns the decimal exponent. */
static int scale10(double *px) {
  static const double p10[] = {1e32, 1e16, 1e8, 1e4, 1e2, 1e1};
  static const int e10[] = {32, 16, 8, 4, 2, 1};
  double x = *px;
  int e = 0, i;
  for (i = 0; i < 6; ++i)
    while (x >= p10[i]) {
      x /= p10[i];
      e += e10[i];
    }
  for (i = 0; i < 6; ++i)
    while (x * p10[i] < 10.0) {
      x *= p10[i];
      e -= e10[i];
    }
  *px = x;
  return e;
}

/* Produces n decimal digits (n >= 0) of x in [1, 10) rounded half up.
   Returns 1 when rounding carried into a new leading digit; the digits are
   then 1000... and one more digit than n is valid (d[n] is set to 0). */
static int gen_digits(double x, int n, unsigned char *d) {
  int m = n < 17 ? n : 17, i, carry = 0;
  for (i = 0; i <= m; ++i) {
    int v = (int)x;
    if (v > 9)
      v = 9;
    d[i] = (unsigned char)v;
    x = (x - v) * 10.0;
  }
  if (d[m] >= 5) {
    for (i = m - 1; i >= 0 && d[i] == 9; --i)
      d[i] = 0;
    if (i >= 0)
      ++d[i];
    else
      carry = 1;
  }
  for (i = m; i < n; ++i)
    d[i] = 0;
  if (carry) {
    d[0] = 1;
    for (i = 1; i <= n; ++i)
      d[i] = 0;
  }
  return carry;
}

/* Decimal expansion with n significant digits: fills d, sets *nd (digits
   valid) and *e (exponent of the first digit). A negative n means the
   value rounds to zero before the first digit. */
static void expand(double x, int n, unsigned char *d, int *nd, int *e) {
  int i;
  *e = 0;
  if (x == 0 || n < 0) {
    for (i = 0; i <= (n < 0 ? 0 : n); ++i)
      d[i] = 0;
    *nd = n < 0 ? 0 : n;
    if (x != 0)
      *e = scale10(&x);
    return;
  }
  *e = scale10(&x);
  *nd = n;
  if (gen_digits(x, n, d)) {
    ++*e;
    *nd = n + 1;
  }
}

/* Writes the %f layout of the digits d (exponent e) with prec decimals. */
static int layout_f(char *b, const unsigned char *d, int nd, int e, int prec,
                    int alt) {
  int k = 0, i;
  if (e < 0)
    b[k++] = '0';
  else
    for (i = 0; i <= e; ++i)
      b[k++] = (char)('0' + (i < nd ? d[i] : 0));
  if (prec > 0 || alt)
    b[k++] = '.';
  for (i = 1; i <= prec; ++i) {
    int idx = e + i;
    b[k++] = (char)('0' + (idx >= 0 && idx < nd ? d[idx] : 0));
  }
  return k;
}

static int layout_e(char *b, const unsigned char *d, int e, int prec, int alt,
                    int upper) {
  int k = 0, i;
  b[k++] = (char)('0' + d[0]);
  if (prec > 0 || alt)
    b[k++] = '.';
  for (i = 1; i <= prec; ++i)
    b[k++] = (char)('0' + d[i]);
  b[k++] = upper ? 'E' : 'e';
  b[k++] = e < 0 ? '-' : '+';
  if (e < 0)
    e = -e;
  if (e >= 100)
    b[k++] = (char)('0' + e / 100);
  b[k++] = (char)('0' + e / 10 % 10);
  b[k++] = (char)('0' + e % 10);
  return k;
}

static void real(struct out *o, int fl, int width, int prec, int conv,
                 double x) {
  unsigned char d[MAXDIG + 2];
  char body[MAXDIG + 16 + 40];
  char prefix[2];
  int nd, e, len, upper = conv == 'E' || conv == 'G' || conv == 'F';
  char lc = (char)(conv | 0x20);

  prefix[0] = 0;
  prefix[1] = 0;
  if (x < 0) {
    prefix[0] = '-';
    x = -x;
  } else if (fl & F_PLUS) {
    prefix[0] = '+';
  } else if (fl & F_SPACE) {
    prefix[0] = ' ';
  }
  if (prec < 0)
    prec = 6;
  if (prec > 30)
    prec = 30;

  if (lc == 'f') {
    double y = x;
    int n = prec + 1;
    if (x != 0)
      n = scale10(&y) + 1 + prec;
    if (n > MAXDIG)
      n = MAXDIG;
    expand(x, n, d, &nd, &e);
    len = layout_f(body, d, nd, e, prec, fl & F_ALT);
  } else if (lc == 'e') {
    expand(x, prec + 1, d, &nd, &e);
    len = layout_e(body, d, e, prec, fl & F_ALT, upper);
  } else {
    int p = prec == 0 ? 1 : prec;
    expand(x, p, d, &nd, &e);
    if (!(fl & F_ALT)) {
      while (nd > 1 && d[nd - 1] == 0)
        --nd;
    }
    if (e < -4 || e >= p) {
      int pe = (fl & F_ALT) ? p - 1 : nd - 1;
      len = layout_e(body, d, e, pe, fl & F_ALT, upper);
    } else {
      int pf = (fl & F_ALT) ? p - 1 - e : (nd - 1 > e ? nd - 1 - e : 0);
      len = layout_f(body, d, nd, e, pf, fl & F_ALT);
    }
  }
  field(o, fl, width, prefix, 0, body, len);
}

/* --- the formatter ------------------------------------------------------- */

static int vformat(struct out *o, const char *fmt, va_list ap) {
  for (; *fmt; ++fmt) {
    int fl = 0, width = 0, prec = -1, lng = 0;
    char c;
    if (*fmt != '%') {
      emit(o, *fmt);
      continue;
    }
    ++fmt;
    for (;; ++fmt) {
      if (*fmt == '-')
        fl |= F_LEFT;
      else if (*fmt == '+')
        fl |= F_PLUS;
      else if (*fmt == ' ')
        fl |= F_SPACE;
      else if (*fmt == '#')
        fl |= F_ALT;
      else if (*fmt == '0')
        fl |= F_ZERO;
      else
        break;
    }
    if (*fmt == '*') {
      width = va_arg(ap, int);
      if (width < 0) {
        fl |= F_LEFT;
        width = -width;
      }
      ++fmt;
    } else {
      while (*fmt >= '0' && *fmt <= '9')
        width = width * 10 + (*fmt++ - '0');
    }
    if (*fmt == '.') {
      ++fmt;
      prec = 0;
      if (*fmt == '*') {
        prec = va_arg(ap, int);
        ++fmt;
      } else {
        while (*fmt >= '0' && *fmt <= '9')
          prec = prec * 10 + (*fmt++ - '0');
      }
    }
    /* lng: -2 hh, -1 h, 0 int, 1 long, 2 long long */
    for (;; ++fmt) {
      if (*fmt == 'h')
        --lng;
      else if (*fmt == 'l')
        ++lng;
      else if (*fmt == 'j')
        lng = 2;
      else if (*fmt == 'z' || *fmt == 't')
        lng = sizeof(size_t) > sizeof(int) ? 1 : 0;
      else if (*fmt == 'L')
        lng = 0;
      else
        break;
    }
    c = *fmt;
    if (!c)
      break;
    switch (c) {
    case 'd':
    case 'i': {
      long long v = lng >= 2 ? va_arg(ap, long long)
                    : lng == 1 ? va_arg(ap, long)
                               : va_arg(ap, int);
      if (lng == -1)
        v = (short)v;
      else if (lng == -2)
        v = (signed char)v;
      integer(o, fl, width, prec, 10, 0, v < 0, lng >= 2,
              v < 0 ? 0ULL - (unsigned long long)v : (unsigned long long)v);
      break;
    }
    case 'u':
    case 'x':
    case 'X':
    case 'o': {
      unsigned long long v = lng >= 2 ? va_arg(ap, unsigned long long)
                             : lng == 1 ? va_arg(ap, unsigned long)
                                        : va_arg(ap, unsigned);
      if (lng == -2)
        v = (unsigned char)v;
      integer(o, fl, width, prec, c == 'u' ? 10 : c == 'o' ? 8 : 16, c == 'X',
              0, lng >= 2, v);
      break;
    }
    case 'p': {
      void *p = va_arg(ap, void *);
      integer(o, fl | F_ALT, width, prec, 16, 0, 0, 0,
              (unsigned long long)(size_t)p);
      break;
    }
    case 'c': {
      char ch = (char)va_arg(ap, int);
      field(o, fl & ~F_ZERO, width, "", 0, &ch, 1);
      break;
    }
    case 's': {
      const char *s = va_arg(ap, const char *);
      int len = 0;
      if (!s)
        s = "(null)";
      while (s[len] && (prec < 0 || len < prec))
        ++len;
      field(o, fl & ~F_ZERO, width, "", 0, s, len);
      break;
    }
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
      real(o, fl, width, prec, c, va_arg(ap, double));
      break;
    case 'n':
      *va_arg(ap, int *) = (int)o->n;
      break;
    case '%':
      emit(o, '%');
      break;
    default:
      emit(o, '%');
      emit(o, c);
      break;
    }
  }
  return (int)o->n;
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
  struct out o = {f, 0, 0, 0};
  return vformat(&o, fmt, ap);
}

int vprintf(const char *fmt, va_list ap) { return vfprintf(stdout, fmt, ap); }

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  struct out o = {0, buf, size, 0};
  int n = vformat(&o, fmt, ap);
  if (size)
    buf[o.n < size - 1 ? o.n : size - 1] = 0;
  return n;
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
  return vsnprintf(buf, (size_t)-1, fmt, ap);
}

int printf(const char *fmt, ...) {
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vfprintf(stdout, fmt, ap);
  va_end(ap);
  return n;
}

int fprintf(FILE *f, const char *fmt, ...) {
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vfprintf(f, fmt, ap);
  va_end(ap);
  return n;
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vsnprintf(buf, (size_t)-1, fmt, ap);
  va_end(ap);
  return n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return n;
}
