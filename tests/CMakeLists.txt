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
  # The C parts of libc call the compiler helpers and abort (in crt0), so
  # those are linked as ordinary (not whole) archive members.
  set(_more)
  set(_more_deps)
  # crt0 (which provides abort) refers to main; the image has none because
  # the tests call functions directly, so main is aliased to the entry.
  if(ARGN)
    get_target_property(_b builtins MERA400_ARCHIVE)
    set(_more --no-whole-archive --defsym=main=${entry}
        "$<TARGET_PROPERTY:crt0,MERA400_OBJECT>" "${_b}")
    set(_more_deps builtins crt0 "${_b}")
  endif()
  add_custom_command(
    OUTPUT "${_elf}"
    COMMAND "${MERA400_LD_LLD}" -m mera400elf ${MERA400_LINK_DEFSYMS}
            -T "${MERA400_LINKER_SCRIPT}" --whole-archive "${_a}" ${_more}
            -e ${entry} -o "${_elf}"
    DEPENDS ${lib} ${_more_deps} "${_a}" "${MERA400_LINKER_SCRIPT}"
    COMMENT "Linking ${name}.elf"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS "${_elf}")
endfunction()

_add_test_image(string_image cstring memcpy WITH_RUNTIME)
_add_test_image(builtins_image builtins __mulhi3)

set(_py ${Python3_EXECUTABLE})
add_subdirectory(abi)
add_subdirectory(tools)

if(MERA400_MODEL STREQUAL "small")
  add_test(NAME string COMMAND ${_py} ${SIM}/test_string.py string_image.elf)
else()
  # The large model's C string routines, called with its calling convention.
  add_test(NAME string COMMAND ${_py} ${SIM}/test_string.py string_image.elf --large)
endif()
set_tests_properties(string PROPERTIES WORKING_DIRECTORY
                     "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME builtins COMMAND ${_py} ${SIM}/test_builtins.py builtins_image.elf)
add_test(NAME int-helpers COMMAND ${_py} ${SIM}/test_int.py builtins_image.elf
         $<$<STREQUAL:${MERA400_MODEL},large>:--large>)
add_test(NAME m48-float COMMAND ${_py} ${SIM}/test_m48.py builtins_image.elf)
add_test(NAME crt0 COMMAND ${_py} ${SIM}/test_crt.py crt_test.elf crt_test.bin)
if(MERA400_C_ENABLED)
  mera400_add_executable(c_test SOURCES c/test_c.c LIBRARIES c builtins)
  add_test(NAME c-on-target COMMAND ${_py} ${SIM}/test_c.py c_test.elf)
  set_tests_properties(c-on-target PROPERTIES WORKING_DIRECTORY
                       "${CMAKE_CURRENT_BINARY_DIR}")
  if(MERA400_PLATFORM STREQUAL "none")
    mera400_add_executable(mem_test SOURCES c/test_mem.c LIBRARIES c builtins
                           COMPILE_OPTIONS -Wno-pointer-sign)
    add_test(NAME mem-on-target COMMAND ${_py} ${SIM}/test_c.py mem_test.elf)
    set_tests_properties(mem-on-target PROPERTIES WORKING_DIRECTORY
                         "${CMAKE_CURRENT_BINARY_DIR}")
    mera400_add_executable(stdio_test SOURCES c/test_stdio.c LIBRARIES c builtins)
    add_test(NAME stdio COMMAND ${_py} ${SIM}/test_stdio.py stdio_test.elf
                                ${MERA400_MODEL}
                                $<IF:$<BOOL:${MERA400_STDIO_FLOAT}>,float,nofloat>)
    mera400_add_executable(math_test SOURCES c/test_math.c LIBRARIES c builtins)
    add_test(NAME math COMMAND ${_py} ${SIM}/test_math.py math_test.elf
                               ${MERA400_MODEL})

    # Start-up and exit: constructors, main, atexit handlers, destructors.
    mera400_add_executable(init_test SOURCES c/test_init.c LIBRARIES c builtins)
    add_test(NAME init COMMAND ${_py} ${SIM}/test_init.py init_test.elf
                               abcmk21xy 77 3 ${MERA400_MODEL})
    mera400_add_executable(init_abort_test SOURCES c/test_init.c
                           COMPILE_OPTIONS -DTEST_ABORT
                           LIBRARIES c builtins)
    add_test(NAME init-abort COMMAND ${_py} ${SIM}/test_init.py init_abort_test.elf
                                     abcmk 40 -1 ${MERA400_MODEL})
    if(MERA400_CLANGXX)
      mera400_add_executable(init_cpp_test SOURCES c/test_init_cpp.cpp
                             LIBRARIES c builtins)
      add_test(NAME init-cpp COMMAND ${_py} ${SIM}/test_init.py init_cpp_test.elf
                                     ABmba 77 0 ${MERA400_MODEL})
      mera400_add_executable(cxx_test SOURCES c/test_cxx.cpp
                             LIBRARIES c builtins)
      add_test(NAME cxx COMMAND ${_py} ${SIM}/test_init.py cxx_test.elf
                                 abcdef 77 0 ${MERA400_MODEL})
    endif()
  endif()
endif()
add_test(NAME decoder-vs-objdump
         COMMAND ${_py} ${SIM}/test_decode.py "${MERA400_LLVM_OBJDUMP}"
                 string_image.elf builtins_image.elf crt_test.elf)

# --gc-sections: crt_test only calls strlen, so no other string/helper code
# may be in the image.
get_filename_component(_llvm_bin "${MERA400_LLVM_OBJDUMP}" DIRECTORY)
find_program(MERA400_LLVM_NM llvm-nm HINTS "${_llvm_bin}" NO_DEFAULT_PATH)
if(MERA400_LLVM_NM)
  add_test(NAME gc-sections
           COMMAND ${_py} ${SIM}/test_gc.py "${MERA400_LLVM_NM}" crt_test.elf
                   main,strlen,_start,exit
                   strcpy,strcmp,memcpy,memmove,memset,strncpy,__mulhi3,__divhi3,atexit)
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/link-orders")
# strlen is its own object (strlen.s) in the small model; the large model's
# string routines are all in string.c.
if(MERA400_MODEL STREQUAL "large")
  set(_strlen_obj "${CMAKE_CURRENT_BINARY_DIR}/../c.dir/string.o")
else()
  set(_strlen_obj "${CMAKE_CURRENT_BINARY_DIR}/../c.dir/strlen.o")
endif()
add_test(NAME start-first
         COMMAND ${_py} ${SIM}/test_start_first.py "${MERA400_LD_LLD}"
                 "${MERA400_LINKER_SCRIPT}" "${CMAKE_CURRENT_BINARY_DIR}/link-orders"
                 "$<TARGET_PROPERTY:crt0,MERA400_OBJECT>"
                 "${CMAKE_CURRENT_BINARY_DIR}/crt_test.dir/main_ret.o"
                 "${_strlen_obj}"
                 "${CMAKE_CURRENT_BINARY_DIR}/../builtins.dir/mulhi3.o"
                 "${MERA400_LIB_DIR}/libc.a"
                 "${MERA400_LIB_DIR}/libbuiltins.a")
set_tests_properties(builtins crt0 decoder-vs-objdump start-first
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
            -no-pie -I "${_root}/include"
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
