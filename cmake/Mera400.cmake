#===-- Mera400.cmake - build helpers for MERA-400 programs ----------------===#
#
# Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
# Exceptions. See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
#
# CMake has no built-in support for this processor, so these functions run
# the LLVM tools directly:
#
#   mera400_add_library(<name> SOURCES <files...>)
#       Assembles each .s (and compiles each .c when MERA400_BUILD_C is on)
#       and archives the objects into lib<name>.a. One object per source file
#       keeps the archive granular: the linker takes only the functions a
#       program uses.
#
#   mera400_add_executable(<name> SOURCES <files...> [LIBRARIES <libs...>]
#                          [BINARY])
#       Links <name>.elf with crt0 FIRST, the library list and the standard
#       linker script. BINARY also produces <name>.bin (flat memory image).
#       The libraries are linked in the order given; list them from the most
#       dependent to the least dependent (for example c builtins).
#
# Why crt0 is always first: the processor starts executing at the first
# word of the image, which must be _start. The linker script puts crt0's
# section first, and this function additionally gives crt0.o first on the
# command line, so the layout is right even for a hand-written link command.

#   MERA400_MODEL (small or large) must be set before the functions are used.
#   Large adds -mlarge-memory to every C compilation and the large-model
#   linker defaults (stack and console at the top of the 128 KiB space).
#   Archives are written to <build>/lib/<model>/.

if(NOT MERA400_MODEL)
  set(MERA400_MODEL small)
endif()
if(NOT MERA400_TRIPLE)
  set(MERA400_TRIPLE mera400-unknown-none)
endif()
set(MERA400_LIB_DIR "${CMAKE_BINARY_DIR}/lib/${MERA400_MODEL}")
set(MERA400_CLANG_MODEL_FLAGS)
set(MERA400_ASM_MODEL_FLAGS)
set(MERA400_LINK_DEFSYMS)
if(MERA400_MODEL STREQUAL "large")
  set(MERA400_CLANG_MODEL_FLAGS -mlarge-memory)
  # Assembly sources that differ in the large model (function pointers are
  # 32-bit byte addresses there, see src/stdlib/exit.s) test this symbol.
  set(MERA400_ASM_MODEL_FLAGS --defsym=MERA400_LARGE=1)
  # Byte addresses. A word address must fit in 16 bits, so the highest
  # usable word is 0xFFFF (byte 0x1FFFE): the console port is that word and
  # the stack starts just below it (the first push goes to word 0xFFFE).
  set(MERA400_LINK_DEFSYMS --defsym=__stack_top=0x1FFFE
                           --defsym=__console_port=0x1FFFE)
endif()

set(MERA400_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "")
set(MERA400_LINKER_SCRIPT "${MERA400_ROOT}/crt/mera400.ld" CACHE INTERNAL "")

function(mera400_check_tools)
  foreach(_v MERA400_LLVM_MC MERA400_LD_LLD MERA400_LLVM_AR MERA400_LLVM_OBJDUMP)
    if(NOT ${_v})
      message(FATAL_ERROR
        "Required LLVM tool not found (${_v}). Configure with "
        "-DCMAKE_TOOLCHAIN_FILE=cmake/mera400-toolchain.cmake and put the "
        "MERA-400 enabled LLVM tools in PATH or pass -DLLVM_BIN_DIR=<dir>.")
    endif()
  endforeach()
endfunction()

# Turns one source file into an object file; sets <out_var> to its path.
function(_mera400_object out_var source target)
  get_filename_component(_name "${source}" NAME_WE)
  get_filename_component(_ext "${source}" EXT)
  get_filename_component(_abs "${source}" ABSOLUTE)
  set(_obj "${CMAKE_CURRENT_BINARY_DIR}/${target}.dir/${_name}.o")
  if(_ext STREQUAL ".s" OR _ext STREQUAL ".S")
    add_custom_command(
      OUTPUT "${_obj}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}.dir"
      COMMAND "${MERA400_LLVM_MC}" -triple=${MERA400_TRIPLE}
              ${MERA400_ASM_MODEL_FLAGS}
              -filetype=obj "${_abs}" -o "${_obj}"
      DEPENDS "${_abs}"
      COMMENT "Assembling ${source}"
      VERBATIM)
  elseif(_ext STREQUAL ".c")
    if(NOT MERA400_C_ENABLED)
      message(FATAL_ERROR "C source ${source} needs MERA400_BUILD_C")
    endif()
    add_custom_command(
      OUTPUT "${_obj}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}.dir"
      # -nostdlibinc drops the system include directories but keeps Clang's
      # own headers (stddef.h, stdarg.h, stdbool.h), which are correct for the
      # target; the library only adds its own freestanding headers.
      # Every function and every data object gets its own section, so that
      # the linker's --gc-sections (see mera400_add_executable) can drop what
      # a program does not use: code size matters on a machine with 64K words.
      COMMAND "${MERA400_CLANG}" --target=${MERA400_TRIPLE} -ffreestanding
              -nostdlibinc -O2 -ffunction-sections -fdata-sections
              ${MERA400_CLANG_MODEL_FLAGS} ${MERA400_COMPILE_OPTIONS}
              -isystem "${MERA400_ROOT}/include" -MD -MF "${_obj}.d"
              -c "${_abs}" -o "${_obj}"
      DEPENDS "${_abs}"
      DEPFILE "${_obj}.d"
      COMMENT "Compiling ${source}"
      VERBATIM)
  elseif(_ext STREQUAL ".cpp")
    if(NOT MERA400_C_ENABLED)
      message(FATAL_ERROR "C++ source ${source} needs MERA400_BUILD_C")
    endif()
    add_custom_command(
      OUTPUT "${_obj}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}.dir"
      # Freestanding C++: no exceptions, no run-time type information, no
      # standard library; the C headers of this library are available.
      COMMAND "${MERA400_CLANGXX}" --target=${MERA400_TRIPLE} -ffreestanding
              -nostdlibinc -nostdinc++ -O2 -ffunction-sections -fdata-sections
              -fno-exceptions -fno-rtti -fno-threadsafe-statics
              ${MERA400_CLANG_MODEL_FLAGS} ${MERA400_COMPILE_OPTIONS}
              -isystem "${MERA400_ROOT}/include" -MD -MF "${_obj}.d"
              -c "${_abs}" -o "${_obj}"
      DEPENDS "${_abs}"
      DEPFILE "${_obj}.d"
      COMMENT "Compiling ${source}"
      VERBATIM)
  else()
    message(FATAL_ERROR "Unsupported source type: ${source}")
  endif()
  set(${out_var} "${_obj}" PARENT_SCOPE)
endfunction()

function(mera400_add_library name)
  cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
  mera400_check_tools()
  set(_objs)
  foreach(_src ${ARG_SOURCES})
    _mera400_object(_obj "${_src}" "${name}")
    list(APPEND _objs "${_obj}")
  endforeach()
  # One directory per memory model (lib/small, lib/large), so archives of
  # different models can never be mixed up.
  set(_lib "${MERA400_LIB_DIR}/lib${name}.a")
  add_custom_command(
    OUTPUT "${_lib}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${MERA400_LIB_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E remove -f "${_lib}"
    COMMAND "${MERA400_LLVM_AR}" rcs "${_lib}" ${_objs}
    DEPENDS ${_objs}
    COMMENT "Archiving lib${name}.a"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS "${_lib}")
  set_property(TARGET ${name} PROPERTY MERA400_ARCHIVE "${_lib}")
endfunction()

function(mera400_add_executable name)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES;LINK_OPTIONS;COMPILE_OPTIONS" ${ARGN})
  mera400_check_tools()
  set(MERA400_COMPILE_OPTIONS ${ARG_COMPILE_OPTIONS})
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
  set(_elf "${CMAKE_CURRENT_BINARY_DIR}/${name}.elf")
  # crt0.o is deliberately the first input.
  add_custom_command(
    OUTPUT "${_elf}"
    COMMAND "${MERA400_LD_LLD}" -m mera400elf ${MERA400_LINK_DEFSYMS} ${ARG_LINK_OPTIONS}
            -T "${MERA400_LINKER_SCRIPT}" --gc-sections
            "$<TARGET_PROPERTY:crt0,MERA400_OBJECT>"
            ${_objs} ${_libs} -o "${_elf}"
    DEPENDS ${_objs} ${_libs} ${_deps} crt0 "${MERA400_LINKER_SCRIPT}"
    COMMENT "Linking ${name}.elf"
    VERBATIM)
  # Every program also gets a flat binary: the bytes of the memory image from
  # __image_base, with no ELF headers, which is what an emulator or a loader
  # puts into memory (the ELF keeps the symbols for debugging).
  set(_bin "${CMAKE_CURRENT_BINARY_DIR}/${name}.bin")
  add_custom_command(
    OUTPUT "${_bin}"
    COMMAND "${MERA400_LD_LLD}" -m mera400elf ${MERA400_LINK_DEFSYMS} ${ARG_LINK_OPTIONS}
            -T "${MERA400_LINKER_SCRIPT}" --gc-sections --oformat=binary
            "$<TARGET_PROPERTY:crt0,MERA400_OBJECT>" ${_objs} ${_libs}
            -o "${_bin}"
    DEPENDS ${_objs} ${_libs} ${_deps} crt0 "${MERA400_LINKER_SCRIPT}"
    COMMENT "Linking ${name}.bin"
    VERBATIM)
  set(_outputs "${_elf}" "${_bin}")
  add_custom_target(${name} ALL DEPENDS ${_outputs})
endfunction()
