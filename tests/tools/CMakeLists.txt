#===-- tests/tools/CMakeLists.txt - CTest registrations for host tools ----===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# tools/image2assm.py: image -> ASSM text -> words must give the image back.
# -B keeps Python from writing .pyc files into the source tree.
add_test(NAME image2assm
         COMMAND ${Python3_EXECUTABLE} -B
                 ${CMAKE_CURRENT_SOURCE_DIR}/test_image2assm.py)
