/* ============================================================================
 * app_timer.c - 로드맵 #3  Timer + IRQ (MMIO 타이머 -> irq[3])
 * ----------------------------------------------------------------------------
 * C 인터럽트 핸들러 irq() 를 여기서 정의한다. 래퍼(start.S)가 regs/마스크를
 * 넘겨주고, 반환값으로 복원할 regs 포인터를 받는다.
 * ==========================================================================*/
#include "app.h"

static volatile uint32_t irq_count = 0;

uint32_t *irq(uint32_t *regs, uint32_t irqs)
{
  if (irqs & (1u << IRQ_TIMER)) {
    irq_count++;
    GPIO_O ^= 0x1;                     /* LED0 토글 */
    TIMER_FLAG = 0x1;                  /* 인터럽트 플래그 클리어(ack) */
  }
  return regs;
}

int app_timer_demo(void)
{
  uart_puts("TIMER: MMIO timer IRQ @ 2Hz on LED0. BTN1 to exit.\n");

  TIMER_FLAG = 0x1;                    /* 남은 플래그 클리어 */
  TIMER_LOAD = CPU_HZ / 2;             /* 0.5초 */
  TIMER_CTRL = 0x3;                    /* enable + irq_en */

  /* 타이머 IRQ(3)만 활성화: mask 비트=0 이 활성 */
  picorv32_enable_irqs(~(1u << IRQ_TIMER));

  uint32_t last = 0;
  while (!GPIO_BTN(1)) {
    if (irq_count != last) {
      uart_printf("  IRQ count = %u\n", irq_count);
      last = irq_count;
    }
    app_delay_ms(50);                  /* PicoRV32 는 WFI 미지원 -> 폴링 */
  }

  TIMER_CTRL = 0x0;
  picorv32_disable_irqs();
  uart_puts("TIMER: exit.\n");
  return 0;
}
