/* ============================================================================
 * app_gpio.c - 로드맵 #1  GPIO (SW / LED / BTN)
 * ----------------------------------------------------------------------------
 * SW 16개를 읽어 LED 16개에 그대로 반영. BTN0 를 누르면 동작 종료.
 * (7-seg 는 #7 에서 별도 처리)
 * ==========================================================================*/
#include "app.h"

int app_gpio_demo(void)
{
  neorv32_uart0_puts("GPIO: SW->LED mirror. BTN0 to exit.\n");

  uint16_t prev = neorv32_gpio_port_get();

  while (1) {
    uint16_t sw = (uint16_t)neorv32_gpio_port_get();
    neorv32_gpio_port_set(sw);                 /* SW -> LED */

    if (sw != prev) {                          /* 변화 시에만 로그 */
      neorv32_uart0_printf("  SW=0x%04x\n", sw);
      prev = sw;
    }

    if (neorv32_gpio_pin_get(16)) {            /* BTN0 = GPIO pin 16 */
      neorv32_uart0_puts("GPIO: BTN0 pressed, exit.\n");
      break;
    }
    app_delay_ms(20);
  }
  return 0;
}
