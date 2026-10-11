/* ============================================================================
 * link.ld - Ibex / Basys3 (BRAM 32KB)
 * ==========================================================================*/
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  ram (rwx) : ORIGIN = 0x00000000, LENGTH = 32K
}

SECTIONS
{
  .text : {
    *(.text.start)
    *(.text*)
    *(.rodata*)
    *(.srodata*)
  } > ram

  .data : {
    *(.data*)
    *(.sdata*)
  } > ram

  .bss (NOLOAD) : {
    *(.bss*)
    *(.sbss*)
    *(COMMON)
  } > ram

  _end = .;
  PROVIDE(_stack_top = ORIGIN(ram) + LENGTH(ram));
}
