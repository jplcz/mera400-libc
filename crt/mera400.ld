/*===-- mera400.ld - linker script for flat MERA-400 programs ----------------===
 *
 * Part of the MERA-400 C library, under the Apache License v2.0 with LLVM
 * Exceptions. See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===---------------------------------------------------------------------===
 *
 * Use with:  ld.lld -m mera400elf -T mera400.ld crt0.o program.o libc.a ...
 * (add --oformat=binary for a flat binary of 16-bit words).
 *
 * All numbers in a linker script are BYTE addresses (a MERA-400 word is two
 * bytes), so 0x400 is word 0x200. The two parameters below can be changed
 * on the command line without editing this file:
 *   --defsym=__image_base=BYTE_ADDRESS   where the program is loaded
 *   --defsym=__stack_top=BYTE_ADDRESS    first address above the stack
 *
 * In the small memory model, char data must stay below byte address 0x10000
 * (32K words), because a byte pointer is a 16-bit byte address; keep the
 * stack top at or below that unless the program uses no char data on the
 * stack.
 */

ENTRY(_start)

PROVIDE(__image_base = 0x400);
PROVIDE(__stack_top = 0x10000);
PROVIDE(__stack_size = 0x400);

SECTIONS
{
  . = __image_base;

  /* crt0's section goes first and KEEP stops --gc-sections from dropping
     it: the CPU starts executing at the image base, which must be _start. */
  .text : {
    KEEP(*(.text.crt0))
    *(.text .text.*)
  }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.*) }

  /* Word aligned bounds: crt0 clears .bss with word stores. */
  .bss : {
    . = ALIGN(2);
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    __bss_end = .;
  }

  /* The heap used by malloc: from the end of .bss up to a reserved stack
     area below __stack_top (--defsym=__stack_size=BYTES to change it). */
  __heap_start = ALIGN(2);
  __heap_end = __stack_top - __stack_size;

  /DISCARD/ : { *(.comment) *(.note*) }
}
