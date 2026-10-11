/* ============================================================================
 * app_seg7.c - 로드맵 #7  7-segment 카운터 (커스텀 HDL)
 * ==========================================================================*/
#include "app.h"

int app_seg7_demo(void)
{
  uart_puts("SEG7: counting 0000..FFFF. BTN3 to exit.\n");

  uint32_t n = 0;
  while (!GPIO_BTN(3)) {
    SEG7_DIG0 = (n >> 0)  & 0xF;
    SEG7_DIG1 = (n >> 4)  & 0xF;
    SEG7_DIG2 = (n >> 8)  & 0xF;
    SEG7_DIG3 = (n >> 12) & 0xF;
    uart_printf("SEG7: %x\n", n & 0xFFFF);
    n++;
    app_delay_ms(300);
  }
  uart_puts("SEG7: exit.\n");
  return 0;
}
