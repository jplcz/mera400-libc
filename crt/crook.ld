/*===-- crook.ld - linker script for CROOK raw memory images -----------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===---------------------------------------------------------------------===
 *
 * CROOK (the MERA-400 operating system) has no linker or loader: a program
 * file is a raw memory image that starts at a base address, with absolute
 * addresses inside it. This script makes such an image. Use with
 *
 *   ld.lld -m mera400elf -T crook.ld crt0-crook.o program.o libc.a ...
 *   llvm-objcopy -O binary program.elf program.bin
 *
 * (cmake/crook.cmake does both). The .bin file is a sequence of 16-bit
 * words, each stored HIGH BYTE FIRST (big-endian), so its size in bytes is
 * twice its size in words.
 *
 * All numbers in a linker script are BYTE addresses (a word is two bytes).
 * Parameters (change them with --defsym=NAME=VALUE given BEFORE -T, e.g.
 * --defsym=__image_base=2048; no need to edit this file. A --defsym after -T
 * is too late: the PROVIDEd symbols that depend on it, such as __stack_top,
 * would already have been computed from the defaults. The Clang driver's
 * -mcrook-base=, -mcrook-image= and -mcrook-stack= (in words) do this:
 *   __image_base   BYTE address of the first word of the image (default 0,
 *                  the default base of CROOK's assembler). cmake/crook.cmake
 *                  passes 2 * MERA400_CROOK_BASE.
 *   __image_size   total size of the image in BYTES (default 0x4000 = 8 K
 *                  words = two 4 K-word memory blocks of CROOK). The file is
 *                  padded with zeros up to this size. See below for why.
 *   __stack_size   bytes reserved for the stack at the top (default 0x400).
 *
 * Why the image is padded: CROOK sizes a process by the highest address that
 * the file contains (or by the MEM* directive of its assembler). A flat file
 * has no room for .bss, heap or stack unless the file itself contains them,
 * so the zero padding IS the memory of the program. Layout, low to high:
 *
 *   __image_base  crt0 (_start), .text, .rodata, .data
 *                 .bss              (cleared again by crt0)
 *                 heap              (__heap_start .. __heap_end, for malloc)
 *                 stack             (grows DOWN from __stack_top)
 *   __stack_top = __image_base + __image_size
 *
 * In the small memory model a byte pointer is a 16-bit byte address, so the
 * whole image must lie below byte address 0x10000 (the ASSERT below).
 */

ENTRY(_start)

/* PROVIDE gives the defaults; a --defsym on the command line (given BEFORE
   -T) replaces them. */
PROVIDE(__image_base = 0);
PROVIDE(__image_size = 0x4000);
PROVIDE(__stack_size = 0x400);
__stack_top = __image_base + __image_size;

SECTIONS
{
  . = __image_base;

  /* crt0's section goes first: the process is assumed to start at the first
     word of its image. KEEP stops --gc-sections from dropping it. */
  .text : {
    KEEP(*(.text.crt0))
    *(.text .text.*)
  }
  .rodata : { *(.rodata .rodata.*) }

  /* Initialiser and finaliser arrays, see mera400.ld. */
  .preinit_array : {
    . = ALIGN(2);
    __preinit_array_start = .;
    KEEP(*(.preinit_array))
    __preinit_array_end = .;
  }
  .init_array : {
    . = ALIGN(2);
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
    __init_array_end = .;
  }
  .fini_array : {
    . = ALIGN(2);
    __fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) .fini_array))
    __fini_array_end = .;
  }

  .data : { *(.data .data.*) }

  .bss : {
    . = ALIGN(2);
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    __bss_end = .;
  }

  __heap_start = ALIGN(2);
  __heap_end = __stack_top - __stack_size;

  /* A real (file) byte at the very end makes the whole range, .bss, heap and
     stack included, part of the raw image. */
  .image_end : {
    . = __stack_top - 1;
    BYTE(0)
  }

  /DISCARD/ : { *(.comment) *(.note*) }
}

ASSERT(__heap_start <= __heap_end,
       "CROOK image too small: code and data do not leave room for the stack (raise __image_size)")
ASSERT(__stack_top <= 0x10000,
       "the small memory model needs the image below byte address 0x10000")
