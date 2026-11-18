#===-- tests/abi/CMakeLists.txt - the ABI examples of docs/abi.md ---------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Needs the compiler, and the bare machine start-up and exit (the program ends
# with crt0's "hlt 077").
if(MERA400_C_ENABLED AND MERA400_PLATFORM STREQUAL "none")
  mera400_add_executable(abi_test SOURCES abi_main.c abi_asm.s
                         LIBRARIES c builtins)
  add_test(NAME abi COMMAND ${Python3_EXECUTABLE}
           "${CMAKE_CURRENT_SOURCE_DIR}/test_abi.py" abi_test.elf abi_main.c)

  if(MERA400_CLANGXX)
    mera400_add_executable(abi_cpp_test SOURCES abi_cpp.cpp abi_cpp_asm.s
                           LIBRARIES c builtins)
    add_test(NAME abi-cpp COMMAND ${Python3_EXECUTABLE}
             "${CMAKE_CURRENT_SOURCE_DIR}/test_abi.py" abi_cpp_test.elf
             abi_cpp.cpp)
  endif()
endif()
