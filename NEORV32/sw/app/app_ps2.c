/* ============================================================================
 * app_ps2.c - 로드맵 #10  PS/2 키보드 (커스텀 HDL)
 * ----------------------------------------------------------------------------
 * PS/2 수신 FIFO 에서 스캔코드를 읽어 UART 로 출력.
 * make/break 코드 구분(0xE0/0xF0 프리픽스) 데모.
 * ==========================================================================*/
#include "app.h"

int app_ps2_demo(void)
{
  neorv32_uart0_puts("PS/2: press keys (scancodes on UART). BTN0 to exit.\n");

  PS2_CTRL = 0x1;                                   /* irq enable (polling 겸용) */

  while (1) {
    uint32_t d = PS2_DATA;
    if (d & 0x100u) {                               /* valid */
      uint8_t sc = d & 0xFF;
      if      (sc == 0xE0) neorv32_uart0_puts("  EXT ");
      else if (sc == 0xF0) neorv32_uart0_puts("  BREAK ");
      else                 neorv32_uart0_printf("  0x%02x\n", sc);
    }
    if (neorv32_gpio_pin_get(16)) {                 /* BTN0 = pin 16 */
      PS2_CTRL = 0x0;
      neorv32_uart0_puts("PS/2: exit.\n");
      return 0;
    }
    app_delay_ms(10);
  }
}
