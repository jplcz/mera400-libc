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
                 crook_test.elf crook_test.bin ${MERA400_CROOK_VERSION})
set_tests_properties(crook PROPERTIES
                     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# GMEM, RMEM and LRAM are declared by <crook.h> for CROOK 5 only.
if(MERA400_C_ENABLED)
  add_test(NAME crook-version-guard
           COMMAND ${MERA400_CLANG} --target=${MERA400_TRIPLE} -ffreestanding
                   -nostdlibinc ${MERA400_CLANG_MODEL_FLAGS}
                   -isystem ${MERA400_ROOT}/include
                   -isystem ${MERA400_CROOK_INCLUDE_DIR}
                   -fsyntax-only ${CMAKE_CURRENT_SOURCE_DIR}/crook_guard.c)
endif()

# A program links from the INSTALLED sysroot with no -L: install into the build
# tree, then run the plain driver command and the same fake-CROOK test on the
# result.
if(MERA400_CROOK_DRIVER)
  set(_sysroot "${CMAKE_CURRENT_BINARY_DIR}/sysroot")
  add_test(NAME crook-install
           COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${_sysroot}")
  add_test(NAME crook-sysroot-link
           COMMAND ${MERA400_CLANG} --target=${MERA400_TRIPLE} --sysroot=${_sysroot}
                   ${CMAKE_CURRENT_SOURCE_DIR}/crook_main.s
                   -mcrook-version=${MERA400_CROOK_VERSION}
                   -o ${CMAKE_CURRENT_BINARY_DIR}/sysroot_test.bin)
  add_test(NAME crook-sysroot-image
           COMMAND ${CMAKE_COMMAND} -E compare_files
                   ${CMAKE_CURRENT_BINARY_DIR}/sysroot_test.bin
                   ${CMAKE_CURRENT_BINARY_DIR}/crook_test.bin)
  set_tests_properties(crook-install PROPERTIES FIXTURES_SETUP crook_sysroot)
  set_tests_properties(crook-sysroot-link PROPERTIES FIXTURES_SETUP crook_sysroot_bin
                       FIXTURES_REQUIRED crook_sysroot)
  set_tests_properties(crook-sysroot-image PROPERTIES FIXTURES_REQUIRED crook_sysroot_bin)
endif()

# The raw image can be turned into ASSM text and back without loss.
add_test(NAME image2assm-crook-image
         COMMAND ${Python3_EXECUTABLE} -B
                 ${CMAKE_CURRENT_SOURCE_DIR}/../tools/test_image2assm.py
                 crook_test.bin)
set_tests_properties(image2assm-crook-image PROPERTIES
                     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# printf, puts and fflush through the portable stdio and libcrook's hooks.
if(MERA400_C_ENABLED)
  crook_add_executable(crook_printf SOURCES crook_printf.c
                       IMAGE_WORDS 24576)
  add_test(NAME crook-stdio
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/test_crook_stdio.py crook_printf.elf)
  set_tests_properties(crook-stdio PROPERTIES
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
