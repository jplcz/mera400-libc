#===-- tests/crook/CMakeLists.txt - CTest registrations for CROOK ---------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Only added when MERA400_PLATFORM=crook. A complete program (crt0_crook +
# main + libraries) that the simulator runs against a fake CROOK.
crook_add_executable(crook_test SOURCES crook_main.s)

add_test(NAME crook
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_crook.py
                 crook_test.elf crook_test.bin)
set_tests_properties(crook PROPERTIES
                     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
