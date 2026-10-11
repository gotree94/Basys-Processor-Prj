/* ============================================================================
 * app_gpio.c - 로드맵 #1  GPIO (SW / LED / BTN)
 * ==========================================================================*/
#include "app.h"

int app_gpio_demo(void)
{
  uart_puts("GPIO: SW->LED mirror. BTN0 to exit.\n");

  uint32_t prev = 0xFFFFFFFFu;
  while (1) {
    uint32_t sw = GPIO_I & 0xFFFFu;
    GPIO_O = sw;

    if (sw != prev) {
      uart_printf("  SW=0x%x\n", sw);
      prev = sw;
    }
    if (GPIO_BTN(0)) { uart_puts("GPIO: exit.\n"); break; }
    app_delay_ms(20);
  }
  return 0;
}
