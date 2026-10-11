/* ============================================================================
 * app_timer.c - 로드맵 #3  Timer + external interrupt
 * ----------------------------------------------------------------------------
 * C 트랩 핸들러를 여기서 정의한다. crt0.S 가 mcause/mepc 를 넘기고,
 * 반환값을 mepc 에 복원한다.
 * ==========================================================================*/
#include "app.h"

static volatile uint32_t irq_count = 0;
static volatile uint32_t dma_done = 0;
static volatile uint32_t ps2_pending = 0;

uint32_t trap_handler(uint32_t cause, uint32_t epc)
{
  if (cause & 0x80000000u) {                 /* interrupt */
    uint32_t code = cause & 0x7FFFFFFFu;
    if (code == 11) {                        /* machine external interrupt */
      if (TIMER_FLAG & 0x1) {                /* 타이머 소스 */
        TIMER_FLAG = 0x1;                    /* ack */
        irq_count++;
        GPIO_O ^= 0x1;
      }
      if (DMA_STAT & 0x2) {                  /* DMA 완료 */
        DMA_STAT = 0x2;
        dma_done = 1;
      }
      if (PS2_DATA & 0x100) {                /* PS/2 수신 */
        ps2_pending = 1;
      }
    }
  }
  return epc;
}

int app_timer_demo(void)
{
  uart_puts("TIMER: external IRQ @ 2Hz on LED0. BTN1 to exit.\n");

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
