#===-- examples/CMakeLists.txt - demo programs ----------------------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Each program becomes <name>.elf (with symbols, for debugging) and
# <name>.bin (the flat memory image) in the build directory, built with the
# bare-metal start-up code only.
if(MERA400_PLATFORM STREQUAL "none")
  mera400_add_executable(fibonacci SOURCES fibonacci.c LIBRARIES c builtins)
  mera400_add_executable(hello SOURCES hello.c LIBRARIES c builtins)
  if(MERA400_CLANGXX)
    mera400_add_executable(shapes SOURCES shapes.cpp LIBRARIES c builtins)
  endif()
endif()
