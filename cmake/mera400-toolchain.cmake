#===-- mera400-toolchain.cmake - CMake toolchain file for the MERA-400 ----===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
#
# Usage:  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mera400-toolchain.cmake
#         [-DLLVM_BIN_DIR=/directory/with/the/llvm/tools]
#
# The tools (llvm-mc, ld.lld, llvm-ar, llvm-objdump) are looked up in
# LLVM_BIN_DIR when it is given, otherwise in PATH. Nothing is hard coded.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mera400)

set(LLVM_BIN_DIR "" CACHE PATH
    "Directory containing llvm-mc, ld.lld, llvm-ar and llvm-objdump (empty: use PATH)")

set(_mera400_hints)
set(_mera400_no_default)
if(LLVM_BIN_DIR)
  set(_mera400_hints HINTS "${LLVM_BIN_DIR}")
  # An explicit directory must win over anything else in PATH.
  set(_mera400_no_default NO_DEFAULT_PATH)
endif()

foreach(_tool llvm-mc ld.lld llvm-ar llvm-objdump)
  string(MAKE_C_IDENTIFIER "MERA400_${_tool}" _var)
  string(TOUPPER "${_var}" _var)
  find_program(${_var} ${_tool} ${_mera400_hints} ${_mera400_no_default})
endforeach()

# Programs are never run on the build machine.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
