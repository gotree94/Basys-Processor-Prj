/* ============================================================================
 * app_timer.c - 로드맵 #3  Timer (irq_timer, mcause 7) + IRQ
 * ----------------------------------------------------------------------------
 * Ibex 는 irq_timer_i 를 내장 -> mcause 7 로 진입. C trap_handler() 정의.
 * ==========================================================================*/
#include "app.h"

static volatile uint32_t irq_count = 0;

uint32_t trap_handler(uint32_t cause, uint32_t epc)
{
  if (cause & 0x80000000u) {                 /* interrupt */
    uint32_t code = cause & 0x7FFFFFFFu;
    if (code == IRQ_M_TIMER) {               /* machine timer */
      TIMER_FLAG = 0x1;                      /* ack */
      irq_count++;
      GPIO_O ^= 0x1;
    }
    else if (code == IRQ_M_EXTERNAL) {       /* DMA / PS2 */
      if (DMA_STAT & 0x2) { DMA_STAT = 0x2; }
      /* PS2 는 폴링으로 처리 */
    }
  }
  return epc;
}

int app_timer_demo(void)
{
  uart_puts("TIMER: irq_timer @ 2Hz on LED0. BTN1 to exit.\n");

  TIMER_FLAG = 0x1;
  TIMER_LOAD = CPU_HZ / 2;
  TIMER_CTRL = 0x3;                          /* enable + irq_en */
  trap_irq_enable();

  uint32_t last = 0;
  while (!GPIO_BTN(1)) {
    if (irq_count != last) {
      uart_printf("  IRQ count = %u\n", irq_count);
      last = irq_count;
    }
    app_delay_ms(50);
  }

  TIMER_CTRL = 0x0;
  trap_irq_disable();
  uart_puts("TIMER: exit.\n");
  return 0;
}
