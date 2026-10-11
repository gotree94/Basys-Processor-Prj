/* ============================================================================
 * app_ps2.c - 로드맵 #10  PS/2 키보드 스캔코드 (커스텀 HDL)
 * ==========================================================================*/
#include "app.h"

int app_ps2_demo(void)
{
  uart_puts("PS/2: press keys (scancodes on UART). BTN0 to exit.\n");
  PS2_CTRL = 0x1;

  while (!GPIO_BTN(0)) {
    uint32_t d = PS2_DATA;
    if (d & 0x100) {
      uint8_t sc = d & 0xFF;
      if      (sc == 0xE0) uart_puts("  EXT ");
      else if (sc == 0xF0) uart_puts("  BREAK ");
      else                 uart_printf("  0x%x\n", sc);
    }
    app_delay_ms(10);
  }
  PS2_CTRL = 0x0;
  uart_puts("PS/2: exit.\n");
  return 0;
}
