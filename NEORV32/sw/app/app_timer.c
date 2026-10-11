/* ============================================================================
 * app_timer.c - 로드맵 #3  Timer + IRQ (MTIME)
 * ----------------------------------------------------------------------------
 * MTIME 비교 인터럽트를 설치하고, 핸들러에서 카운트 증가 + LED 토글.
 * NEORV32 는 인터럽트 벡터를 자체적으로 관리하므로 RTE 로 핸들러만 등록.
 * ==========================================================================*/
#include "app.h"

static volatile uint32_t irq_count = 0;

/* MTIME 인터럽트 핸들러: LED0 점멸 + 카운트 */
static void mtime_handler(void)
{
  irq_count++;
  neorv32_gpio_pin_toggle(0);                       /* LED0 */
  neorv32_mtime_set_timecmp(neorv32_mtime_get_time() + (F_CPU / 2)); /* 0.5s */
}

int app_timer_demo(void)
{
  neorv32_uart0_puts("TIMER: MTIME IRQ @ 2Hz on LED0. BTN1 to exit.\n");

  neorv32_rte_setup();                              /* RTE 초기화 */
  neorv32_rte_handler_install(MTIME_IRQ, mtime_handler);

  neorv32_mtime_clear();                            /* 카운터/비교값 초기화 */
  neorv32_mtime_set_timecmp(neorv32_mtime_get_time() + (F_CPU / 2));

  neorv32_cpu_eint();                               /* 전역 인터럽트 허용 */

  uint32_t last = 0;
  while (1) {
    if (irq_count != last) {
      neorv32_uart0_printf("  IRQ count = %u\n", irq_count);
      last = irq_count;
    }
    if (neorv32_gpio_pin_get(17)) {                 /* BTN1 = pin 17 */
      neorv32_cpu_dint();
      neorv32_uart0_puts("TIMER: exit.\n");
      return 0;
    }
    neorv32_cpu_sleep();                            /* 인터럽트 대기 */
  }
}
