/* ============================================================================
 * app_seg7.c - 로드맵 #7  7-segment Display (커스텀 HDL)
 * ----------------------------------------------------------------------------
 * 자릿수 레지스터에 값(0..15)을 쓰면 HDL 디코더가 세그먼트를 구동.
 * 여기서는 0000 ~ FFFF 를 약 0.3초 간격으로 카운트.
 * ==========================================================================*/
#include "app.h"

int app_seg7_demo(void)
{
  neorv32_uart0_puts("SEG7: counting 0000..FFFF. BTN3 to exit.\n");

  uint16_t n = 0;
  while (1) {
    SEG7_DIG0 = (n >> 0)  & 0xF;
    SEG7_DIG1 = (n >> 4)  & 0xF;
    SEG7_DIG2 = (n >> 8)  & 0xF;
    SEG7_DIG3 = (n >> 12) & 0xF;

    neorv32_uart0_printf("SEG7: %04x\n", n);
    n++;

    if (neorv32_gpio_pin_get(19)) {                 /* BTN3 = pin 19 */
      neorv32_uart0_puts("SEG7: exit.\n");
      return 0;
    }
    app_delay_ms(300);
  }
}
