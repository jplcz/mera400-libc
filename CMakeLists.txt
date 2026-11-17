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
set(MERA400_MEMORY_MODEL "small" CACHE STRING
    "Memory model: small (16-bit byte pointers) or large (32-bit, not implemented)")
set_property(CACHE MERA400_MEMORY_MODEL PROPERTY STRINGS small large)
if(MERA400_MEMORY_MODEL STREQUAL "large")
  message(FATAL_ERROR
    "MERA400_MEMORY_MODEL=large is not implemented yet: the library only "
    "supports the small model (16-bit byte pointers). Use "
    "-DMERA400_MEMORY_MODEL=small.")
elseif(NOT MERA400_MEMORY_MODEL STREQUAL "small")
  message(FATAL_ERROR "MERA400_MEMORY_MODEL must be small or large")
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

include(Mera400)
mera400_check_tools()

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

set(MERA400_BUILTINS_SOURCES
  builtins/ashlhi3.s builtins/ashrhi3.s builtins/lshrhi3.s builtins/mulhi3.s
  builtins/udivmodhi.s builtins/udivhi3.s builtins/umodhi3.s
  builtins/divhi3.s builtins/modhi3.s)

set(MERA400_C_SOURCES)
if(MERA400_C_ENABLED)
  file(GLOB MERA400_C_SOURCES CONFIGURE_DEPENDS
       src/ctype/*.c src/stdlib/*.c src/string/*.c)
endif()

mera400_add_library(c SOURCES ${MERA400_STRING_SOURCES} ${MERA400_C_SOURCES})
mera400_add_library(builtins SOURCES ${MERA400_BUILTINS_SOURCES})

# --- Tests ------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_subdirectory(tests)
  else()
    message(WARNING "Python 3 not found: the simulator tests are disabled")
  endif()
endif()
