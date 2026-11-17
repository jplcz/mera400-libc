#===-- tests/CMakeLists.txt - CTest registrations -------------------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

set(SIM "${CMAKE_CURRENT_SOURCE_DIR}/sim")

# A complete program: crt0 + main + libraries, linked by the library's own
# helper (so crt0 is first and the linker script is used).
mera400_add_executable(crt_test SOURCES crt/main_ret.s
                       LIBRARIES c builtins BINARY)

# Images for testing single libraries: every member of the archive is kept
# (--whole-archive) and there is no crt0; the test calls functions directly.
function(_add_test_image name lib entry)
  set(_elf "${CMAKE_CURRENT_BINARY_DIR}/${name}.elf")
  get_target_property(_a ${lib} MERA400_ARCHIVE)
  add_custom_command(
    OUTPUT "${_elf}"
    COMMAND "${MERA400_LD_LLD}" -m mera400elf -T "${MERA400_LINKER_SCRIPT}"
            --whole-archive "${_a}" -e ${entry} -o "${_elf}"
    DEPENDS ${lib} "${_a}" "${MERA400_LINKER_SCRIPT}"
    COMMENT "Linking ${name}.elf"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS "${_elf}")
endfunction()

_add_test_image(string_image c memcpy)
_add_test_image(builtins_image builtins __mulhi3)

set(_py ${Python3_EXECUTABLE})

add_test(NAME string COMMAND ${_py} ${SIM}/test_string.py string_image.elf)
add_test(NAME builtins COMMAND ${_py} ${SIM}/test_builtins.py builtins_image.elf)
add_test(NAME crt0 COMMAND ${_py} ${SIM}/test_crt.py crt_test.elf crt_test.bin)
add_test(NAME decoder-vs-objdump
         COMMAND ${_py} ${SIM}/test_decode.py "${MERA400_LLVM_OBJDUMP}"
                 string_image.elf builtins_image.elf crt_test.elf)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/link-orders")
add_test(NAME start-first
         COMMAND ${_py} ${SIM}/test_start_first.py "${MERA400_LD_LLD}"
                 "${MERA400_LINKER_SCRIPT}" "${CMAKE_CURRENT_BINARY_DIR}/link-orders"
                 "$<TARGET_PROPERTY:crt0,MERA400_OBJECT>"
                 "${CMAKE_CURRENT_BINARY_DIR}/crt_test.dir/main_ret.o"
                 "${CMAKE_CURRENT_BINARY_DIR}/../c.dir/strlen.o"
                 "${CMAKE_CURRENT_BINARY_DIR}/../builtins.dir/mulhi3.o"
                 "${CMAKE_CURRENT_BINARY_DIR}/../libc.a"
                 "${CMAKE_CURRENT_BINARY_DIR}/../libbuiltins.a")
set_tests_properties(string builtins crt0 decoder-vs-objdump start-first
                     PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# The portable C sources cannot run on the target without a compiler for it,
# so their logic is checked by compiling them with the build machine's
# compiler (see host/test_c.c). Skipped when no host compiler is found.
find_program(MERA400_HOST_CC NAMES cc gcc clang)
if(MERA400_HOST_CC AND NOT CMAKE_HOST_WIN32)
  set(_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
  add_custom_command(
    OUTPUT host_c_test
    COMMAND "${MERA400_HOST_CC}" -std=gnu99 -O1 -ffreestanding -fno-builtin
            -nostdinc -no-pie -I "${_root}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/host/test_c.c"
            "${_root}/src/ctype/ctype.c" "${_root}/src/stdlib/misc.c"
            "${_root}/src/stdlib/strtol.c" "${_root}/src/stdlib/qsort.c"
            "${_root}/src/stdlib/malloc.c" "${_root}/src/string/strextra.c"
            -o host_c_test
    DEPENDS host/test_c.c
    COMMENT "Building the host-side C logic test"
    VERBATIM)
  add_custom_target(host_c_test_build ALL DEPENDS host_c_test)
  add_test(NAME host-c-logic COMMAND "${CMAKE_CURRENT_BINARY_DIR}/host_c_test")
endif()
