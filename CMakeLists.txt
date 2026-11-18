#===-- CMakeLists.txt - MERA-400 C library -------------------------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.20)

# No language is enabled: CMake has no MERA-400 support, so assembly is done
# by calling llvm-mc directly (see cmake/Mera400.cmake). enable_language(C)
# is only called when a C compiler for the target exists (see below).
project(mera400-libc VERSION 0.1 LANGUAGES NONE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR
    "This is a cross build. Configure with\n"
    "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mera400-toolchain.cmake "
    "[-DLLVM_BIN_DIR=<directory with the LLVM tools>]")
endif()

# --- Memory model -----------------------------------------------------------
set(MERA400_MODEL "small" CACHE STRING
    "Memory model: small (16-bit byte pointers) or large (32-bit byte pointers)")
set_property(CACHE MERA400_MODEL PROPERTY STRINGS small large)
if(NOT MERA400_MODEL MATCHES "^(small|large)$")
  message(FATAL_ERROR "MERA400_MODEL must be small or large")
endif()

# --- Platform ---------------------------------------------------------------
# none:  bare metal, the program runs on the bare processor (crt/crt0.s).
# crook: runs under CROOK, the MERA-400 operating system (see README.md).
set(MERA400_PLATFORM "none" CACHE STRING "Target platform: none or crook")
set_property(CACHE MERA400_PLATFORM PROPERTY STRINGS none crook)
if(NOT MERA400_PLATFORM MATCHES "^(none|crook)$")
  message(FATAL_ERROR "MERA400_PLATFORM must be none or crook")
endif()
# The CROOK version the program is built for. CROOK-4 and CROOK-5 differ only
# in memory management: GMEM, RMEM and LRAM exist in CROOK-5 only.
set(MERA400_CROOK_VERSION 5 CACHE STRING
    "CROOK: operating system version to build for, 4 or 5")
set_property(CACHE MERA400_CROOK_VERSION PROPERTY STRINGS 4 5)
if(NOT MERA400_CROOK_VERSION MATCHES "^[45]$")
  message(FATAL_ERROR "MERA400_CROOK_VERSION must be 4 or 5 (CROOK-1, 2 and 3 are not supported)")
endif()

# --- C compiler (optional) --------------------------------------------------
# There is no Clang target for the MERA-400 yet, so C sources are built only
# when asked for (ON) or when the available clang reports a mera400 target
# (AUTO, the default). OFF never builds C.
set(MERA400_BUILD_C "AUTO" CACHE STRING
    "Build the C parts (ctype, stdlib): AUTO (when clang has a mera400 target), ON or OFF")
set_property(CACHE MERA400_BUILD_C PROPERTY STRINGS AUTO ON OFF)

set(MERA400_C_ENABLED OFF)
if(NOT MERA400_BUILD_C STREQUAL "OFF")
  set(_hint)
  if(LLVM_BIN_DIR)
    set(_hint HINTS "${LLVM_BIN_DIR}" NO_DEFAULT_PATH)
  endif()
  find_program(MERA400_CLANG clang ${_hint})
  find_program(MERA400_CLANGXX clang++ ${_hint})
  set(_have_target OFF)
  if(MERA400_CLANG)
    execute_process(COMMAND "${MERA400_CLANG}" --print-targets
                    OUTPUT_VARIABLE _targets ERROR_QUIET)
    if(_targets MATCHES "mera400")
      set(_have_target ON)
    endif()
  endif()
  if(_have_target)
    set(MERA400_C_ENABLED ON)
  elseif(NOT MERA400_BUILD_C STREQUAL "AUTO")
    message(FATAL_ERROR "MERA400_BUILD_C=ON but no clang with a mera400 target was found")
  else()
    message(STATUS "No clang with a mera400 target: C sources are not built")
  endif()
endif()

# --- Target triple ----------------------------------------------------------
# Under CROOK the sources are compiled for mera400-unknown-crook when the
# available clang knows that triple (it then also defines __CROOK__ and its
# driver links CROOK images by itself, see cmake/crook.cmake). Otherwise, and
# on every other platform, the bare triple is used: the code is identical.
set(MERA400_TRIPLE "mera400-unknown-none")
set(MERA400_CROOK_DRIVER OFF)
if(MERA400_PLATFORM STREQUAL "crook" AND MERA400_C_ENABLED)
  execute_process(COMMAND "${MERA400_CLANG}" --target=mera400-unknown-crook
                          -mcrook-version=${MERA400_CROOK_VERSION}
                          -E -dM -x c /dev/null
                  OUTPUT_VARIABLE _crook_macros ERROR_QUIET)
  if(_crook_macros MATCHES "__CROOK_VERSION__")
    set(MERA400_TRIPLE "mera400-unknown-crook")
    set(MERA400_CROOK_DRIVER ON)
  endif()
endif()
message(STATUS "Target triple: ${MERA400_TRIPLE}")

include(Mera400)
mera400_check_tools()

# Every C source learns the CROOK version from __CROOK_VERSION__ (crook.h
# declares the CROOK-5-only calls only for version 5). The new driver defines
# it; with an older clang the macro is defined here.
if(MERA400_PLATFORM STREQUAL "crook")
  if(MERA400_CROOK_DRIVER)
    list(APPEND MERA400_CLANG_MODEL_FLAGS -mcrook-version=${MERA400_CROOK_VERSION})
  else()
    list(APPEND MERA400_CLANG_MODEL_FLAGS -D__CROOK_VERSION__=${MERA400_CROOK_VERSION})
  endif()
endif()

# --- crt0 -------------------------------------------------------------------
# Built as an object (not in the archive): it must be linked first and
# explicitly, because nothing references _start.
_mera400_object(_crt0_obj "crt/crt0.s" crt0)
add_custom_target(crt0 ALL DEPENDS "${_crt0_obj}")
set_property(TARGET crt0 PROPERTY MERA400_OBJECT "${_crt0_obj}")

# --- Libraries --------------------------------------------------------------
set(MERA400_STRING_SOURCES
  src/string/memchr.s src/string/memcmp.s src/string/memcpy.s
  src/string/memmove.s src/string/memset.s src/string/strcat.s
  src/string/strchr.s src/string/strcmp.s src/string/strcpy.s
  src/string/strlen.s src/string/strncat.s src/string/strncmp.s
  src/string/strncpy.s src/string/strrchr.s)
if(MERA400_MODEL STREQUAL "large")
  # The assembly versions use 16-bit byte pointers (small model only).
  if(NOT MERA400_BUILD_C STREQUAL "OFF" AND MERA400_CLANG)
    set(MERA400_STRING_SOURCES src/string/large/string.c)
  else()
    message(FATAL_ERROR "The large memory model needs the C compiler")
  endif()
endif()

set(MERA400_BUILTINS_SOURCES
  builtins/ashlhi3.s builtins/ashrhi3.s builtins/lshrhi3.s builtins/mulhi3.s
  builtins/udivmodhi.s builtins/udivhi3.s builtins/umodhi3.s
  builtins/divhi3.s builtins/modhi3.s)

set(MERA400_C_SOURCES)
if(MERA400_C_ENABLED)
  file(GLOB MERA400_C_SOURCES CONFIGURE_DEPENDS
       src/ctype/*.c src/stdlib/*.c src/string/*.c src/stdio/*.c src/math/*.c)
  if(MERA400_PLATFORM STREQUAL "crook")
    # libcrook provides the console hooks, putchar, puts and getchar.
    list(FILTER MERA400_C_SOURCES EXCLUDE REGEX "src/stdio/(console|putchar)\\.c$")
  endif()
endif()

set(MERA400_BUILTINS_C_SOURCES)
if(MERA400_C_ENABLED)
  # 32-bit helpers are written in C (they use only one-place shifts and
  # 32-bit additions, which the compiler expands inline).
  set(MERA400_BUILTINS_C_SOURCES
    builtins/ashlsi3.c builtins/lshrsi3.c builtins/ashrsi3.c
    builtins/mulsi3.c builtins/udivmodsi4.c builtins/di3.c
    builtins/m48_addsub.c builtins/m48_mul.c builtins/m48_div.c
    builtins/m48_misc.c builtins/m48_conv.c builtins/m48_powi.c)
endif()

mera400_add_library(c SOURCES ${MERA400_STRING_SOURCES} src/stdlib/exit.s
                    ${MERA400_C_SOURCES})
# Only for tests: the string routines and the start/exit code, small enough
# to be linked as a whole into one image (the complete libc is not).
set(MERA400_CSTRING_C_SOURCES)
if(MERA400_C_ENABLED)
  file(GLOB MERA400_CSTRING_C_SOURCES CONFIGURE_DEPENDS src/string/*.c)
endif()
mera400_add_library(cstring SOURCES ${MERA400_STRING_SOURCES} src/stdlib/exit.s
                    ${MERA400_CSTRING_C_SOURCES})
mera400_add_library(builtins SOURCES ${MERA400_BUILTINS_SOURCES}
                    ${MERA400_BUILTINS_C_SOURCES})

# The platform layer is a separate library (libcrook.a) plus its own start-up
# object, so the bare-metal libc and its tests are the same on every platform.
if(MERA400_PLATFORM STREQUAL "crook")
  if(MERA400_MODEL STREQUAL "large")
    message(FATAL_ERROR
      "MERA400_PLATFORM=crook supports only MERA400_MODEL=small: CROOK programs "
      "live in one 64 K-word address space")
  endif()
  include(crook)
endif()

# --- Install: a sysroot for "clang --target=mera400-unknown-none --sysroot" ---
# Clang's MERA-400 driver looks for <sysroot>/include, <sysroot>/lib/crt0.o,
# <sysroot>/lib/mera400.ld, libc.a and libbuiltins.a, so installing into a
# directory with this layout is all a user needs to compile and link programs
# with the plain compiler command.
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
# The large model goes to lib/large so both can share one sysroot.
if(MERA400_MODEL STREQUAL "large")
  set(_libdir lib/large)
else()
  set(_libdir lib)
endif()
install(FILES "${_crt0_obj}" crt/mera400.ld DESTINATION ${_libdir})
get_target_property(_libc_a c MERA400_ARCHIVE)
get_target_property(_builtins_a builtins MERA400_ARCHIVE)
install(FILES "${_libc_a}" "${_builtins_a}" DESTINATION ${_libdir})
# The CROOK driver finds the same layout: lib/crt0-crook.o, lib/crook.ld and
# lib/libcrook.a next to libc.a and libbuiltins.a.
if(MERA400_PLATFORM STREQUAL "crook")
  get_target_property(_crook_a crook MERA400_ARCHIVE)
  install(FILES "${_crt0_crook_obj}" "${MERA400_CROOK_LINKER_SCRIPT}" "${_crook_a}"
          DESTINATION lib)
endif()

# --- Tests ------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_subdirectory(tests)
    if(MERA400_PLATFORM STREQUAL "crook")
      add_subdirectory(tests/crook)
    endif()
  else()
    message(WARNING "Python 3 not found: the simulator tests are disabled")
  endif()
endif()
