#===-- crook.cmake - build helpers for the CROOK platform -----------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
#
# Included by the top level CMakeLists.txt when MERA400_PLATFORM=crook.
# CROOK is the operating system of the MERA-400. It has no linker or loader:
# a program file is a RAW MEMORY IMAGE (the words of the program, placed at
# a base address). So besides the ELF file the build produces
#
#   <name>.bin   the raw image: 16-bit words, each stored HIGH BYTE FIRST
#                (big-endian), so the size in bytes is twice the size in
#                words. The first word is the first instruction of crt0.
#
#   crook_add_executable(<name> SOURCES <files...> [LIBRARIES <libs...>]
#                        [LINK_OPTIONS <options...>] [BASE <words>]
#                        [IMAGE_WORDS <words>] [STACK_WORDS <words>])
#       Links <name>.elf with crt0_crook FIRST, crook.ld and the libraries
#       (default: crook c builtins), then <name>.bin with llvm-objcopy -O binary.
#       With a clang that knows mera400-unknown-crook the link is done by
#       the clang driver (-mcrook-base/-image/-stack, which it turns into
#       --defsym options placed before -T); otherwise by ld.lld directly.
#       BASE, IMAGE_WORDS and STACK_WORDS override the cache variables below
#       for one program.
#
# Configuration (cache variables, all in WORDS, not bytes):
#   MERA400_CROOK_BASE         address of the first word of the image (0)
#   MERA400_CROOK_IMAGE_WORDS  size of the image including the zero padding
#                              that is the program's .bss, heap and stack
#   MERA400_CROOK_STACK_WORDS  words reserved at the top for the stack
#   MERA400_CROOK_VERSION      4 or 5 (default 5): the CROOK version to build
#                              for; defined in the top level CMakeLists.txt
#                              (CROOK-4 has no GMEM, RMEM or LRAM)
# The linker script works in bytes, so these are doubled when passed on.

set(MERA400_CROOK_BASE 0 CACHE STRING
    "CROOK: word address of the start of the program image")
set(MERA400_CROOK_IMAGE_WORDS 8192 CACHE STRING
    "CROOK: image size in words (the file is padded with zeros to this size)")
set(MERA400_CROOK_STACK_WORDS 512 CACHE STRING
    "CROOK: words reserved for the stack at the top of the image")

foreach(_v MERA400_CROOK_BASE MERA400_CROOK_IMAGE_WORDS MERA400_CROOK_STACK_WORDS)
  if(NOT ${_v} MATCHES "^[0-9]+$")
    message(FATAL_ERROR "${_v} must be a non-negative decimal number of words")
  endif()
endforeach()

set(MERA400_CROOK_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../crt/crook.ld"
    CACHE INTERNAL "")
set(MERA400_CROOK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../src/crook/include"
    CACHE INTERNAL "")

set(_hint)
if(LLVM_BIN_DIR)
  set(_hint HINTS "${LLVM_BIN_DIR}" NO_DEFAULT_PATH)
endif()
find_program(MERA400_LLVM_OBJCOPY llvm-objcopy ${_hint})
if(NOT MERA400_LLVM_OBJCOPY)
  message(FATAL_ERROR
    "llvm-objcopy not found (needed to make the raw CROOK image). Put it in "
    "PATH or pass -DLLVM_BIN_DIR=<dir>.")
endif()

# The platform layer: libcrook.a (extracode wrappers, exit, minimal stdio)
# and its start-up object crt0_crook (not in the archive, linked first).
mera400_add_library(crook SOURCES
  src/crook/exit.s src/crook/exl.s src/crook/fil.s src/crook/rec.s
  src/crook/block.s src/crook/value.s src/crook/convert.s src/crook/stdio.s
  src/crook/stdout.c)
_mera400_object(_crt0_crook_obj "crt/crt0-crook.s" crt0_crook)
add_custom_target(crt0_crook ALL DEPENDS "${_crt0_crook_obj}")
set_property(TARGET crt0_crook PROPERTY MERA400_OBJECT "${_crt0_crook_obj}")

function(crook_add_executable name)
  cmake_parse_arguments(ARG "" "BASE;IMAGE_WORDS;STACK_WORDS"
                        "SOURCES;LIBRARIES;LINK_OPTIONS" ${ARGN})
  # Per-program overrides of the cache variables (all in words).
  foreach(_v BASE IMAGE_WORDS STACK_WORDS)
    if("${ARG_${_v}}" STREQUAL "")
      set(ARG_${_v} "${MERA400_CROOK_${_v}}")
    endif()
  endforeach()
  set(_default_libs OFF)
  if(NOT ARG_LIBRARIES)
    set(ARG_LIBRARIES crook c builtins)
    set(_default_libs ON)
  endif()
  set(_objs)
  foreach(_src ${ARG_SOURCES})
    _mera400_object(_obj "${_src}" "${name}")
    list(APPEND _objs "${_obj}")
  endforeach()
  set(_libs)
  set(_deps)
  foreach(_l ${ARG_LIBRARIES})
    get_target_property(_a ${_l} MERA400_ARCHIVE)
    list(APPEND _libs "${_a}")
    list(APPEND _deps ${_l})
  endforeach()
  get_target_property(_crt0 crt0_crook MERA400_OBJECT)
  set(_elf "${CMAKE_CURRENT_BINARY_DIR}/${name}.elf")
  set(_bin "${CMAKE_CURRENT_BINARY_DIR}/${name}.bin")

  if(MERA400_CROOK_DRIVER AND _default_libs)
    # The clang driver does the CROOK link: crt0-crook.o first, then the
    # objects, crook.ld, --gc-sections, libcrook.a, libc.a, libbuiltins.a, and
    # the image parameters (in words) before -T. It finds the start-up object,
    # the script and the archives in the -L directories, the same layout that
    # "cmake --install" makes in <sysroot>/lib.
    get_filename_component(_crt0_dir "${_crt0}" DIRECTORY)
    get_filename_component(_ld_dir "${MERA400_CROOK_LINKER_SCRIPT}" DIRECTORY)
    set(_wl)
    foreach(_o ${ARG_LINK_OPTIONS})
      list(APPEND _wl "-Wl,${_o}")
    endforeach()
    set(_link "${MERA400_CLANG}" --target=${MERA400_TRIPLE}
        -mcrook-base=${ARG_BASE} -mcrook-image=${ARG_IMAGE_WORDS}
        -mcrook-stack=${ARG_STACK_WORDS}
        -mcrook-version=${MERA400_CROOK_VERSION}
        -L${_crt0_dir} -L${_ld_dir} -L${MERA400_LIB_DIR} ${_wl}
        ${_objs})
    add_custom_command(
      OUTPUT "${_elf}"
      COMMAND ${_link} -o "${_elf}"
      DEPENDS ${_objs} ${_libs} ${_deps} crt0_crook "${MERA400_CROOK_LINKER_SCRIPT}"
      COMMENT "Linking ${name}.elf"
      VERBATIM)
  else()
    # No clang that knows the CROOK triple (or a custom library list): call
    # ld.lld directly. crt0-crook.o is deliberately the first input.
    math(EXPR _base_b "${ARG_BASE} * 2")
    math(EXPR _size_b "${ARG_IMAGE_WORDS} * 2")
    math(EXPR _stack_b "${ARG_STACK_WORDS} * 2")
    add_custom_command(
      OUTPUT "${_elf}"
      COMMAND "${MERA400_LD_LLD}" -m mera400elf
              --defsym=__image_base=${_base_b} --defsym=__image_size=${_size_b}
              --defsym=__stack_size=${_stack_b} ${ARG_LINK_OPTIONS}
              -T "${MERA400_CROOK_LINKER_SCRIPT}" --gc-sections
              "${_crt0}" ${_objs} ${_libs}
              -o "${_elf}"
      DEPENDS ${_objs} ${_libs} ${_deps} crt0_crook "${MERA400_CROOK_LINKER_SCRIPT}"
      COMMENT "Linking ${name}.elf"
      VERBATIM)
  endif()
  add_custom_command(
    OUTPUT "${_bin}"
    COMMAND "${MERA400_LLVM_OBJCOPY}" -O binary "${_elf}" "${_bin}"
    DEPENDS "${_elf}"
    COMMENT "Making the raw CROOK image ${name}.bin"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS "${_elf}" "${_bin}")
endfunction()
