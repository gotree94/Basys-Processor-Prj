/* ============================================================================
 * link.ld - PicoRV32 / Basys3 (BRAM 32KB) 링커 스크립트
 * ----------------------------------------------------------------------------
 * reset_vec(0x0) 이 가장 앞에 오도록 .text.start 를 먼저 배치.
 * irq_vec 은 .balign 16 으로 0x10 에 위치 (PROGADDR_IRQ 와 일치).
 * ==========================================================================*/
OUTPUT_ARCH(riscv)
ENTRY(reset_vec)

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
