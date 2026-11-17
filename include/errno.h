/*===-- errno.h - error codes ----------------------------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */
#ifndef MERA400_ERRNO_H
#define MERA400_ERRNO_H

extern int errno;

#define ENOMEM 12
#define EINVAL 22
#define EDOM 33
#define ERANGE 34

#endif
