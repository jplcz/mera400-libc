/*===-- fibonacci.c - the smallest useful program --------------------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Computes fib(10) = 55 recursively and returns it from main. crt0 passes the
 * return value to exit, which leaves it in register R1 and stops the processor
 * (WAIT/HLT), so no output device is needed: look at R1 (0x37 = 55).
 */
static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

int main(void) { return fib(10); }
