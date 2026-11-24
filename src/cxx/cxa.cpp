/*===-- cxa.cpp - the C++ ABI support calls the compiler emits -------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 *
 * Compiler-generated calls of the Itanium C++ ABI, which clang uses on this
 * target. The program is single-threaded and has no exceptions, so each one
 * is tiny:
 *
 *  - std::terminate and its handler: the end of the program when something
 *    cannot be handled (a failed `new`, a pure virtual call, libc++ checks).
 *  - __cxa_pure_virtual: called through an abstract function's vtable slot.
 *  - __cxa_guard_*: protect a function-local `static T x = ...;` so it is
 *    initialised once. The MERA-400 target turns thread-safe statics off, so
 *    clang does not emit them; they exist for -fthreadsafe-statics.
 *  - __cxa_atexit: a stub. The driver passes -fno-use-cxa-atexit, so
 *    destructors of global objects normally go through atexit() (exit.s).
 *    The stub only lets code compiled without that flag link; it does not
 *    run the destructor (a bare-metal program rarely exits).
 *  - std::__libcpp_verbose_abort: where libc++ reports a failed check that
 *    would otherwise be an exception.
 */
#include <stdlib.h>

namespace std {
typedef void (*terminate_handler)();

static terminate_handler handler;

terminate_handler set_terminate(terminate_handler h) noexcept {
  terminate_handler old = handler;
  handler = h;
  return old;
}
terminate_handler get_terminate() noexcept { return handler; }

[[noreturn]] void terminate() noexcept {
  if (handler)
    handler();
  abort();
}

// libc++ declares this in its versioned namespace; it prints a message on
// other systems, here it only stops the program (no output is assumed).
inline namespace __1 {
[[noreturn]] void __libcpp_verbose_abort(const char *, ...) noexcept {
  abort();
}
} // namespace __1
} // namespace std

extern "C" {

void __cxa_pure_virtual() { std::terminate(); }

// A guard is 64 bits in the ABI; only its first byte is used: 0 = not
// initialised, 1 = done.
int __cxa_guard_acquire(char *guard) { return !*guard; }
void __cxa_guard_release(char *guard) { *guard = 1; }
void __cxa_guard_abort(char *) {}

void *__dso_handle = nullptr;
int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }

} // extern "C"
