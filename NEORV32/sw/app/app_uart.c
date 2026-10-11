/* ============================================================================
 * app_uart.c - 로드맵 #2  UART (FTDI)
 * ----------------------------------------------------------------------------
 * 하트비트 출력 + 입력 에코. 'q' 입력 시 종료.
 * ==========================================================================*/
#include "app.h"

int app_uart_demo(void)
{
  neorv32_uart0_puts("UART: type chars (echo). 'q' to exit.\n");

  uint32_t beats = 0;
  uint32_t next = neorv32_mtime_get_time();

  while (1) {
    /* 입력 에코 */
    while (neorv32_uart0_available()) {
      char c = (char)neorv32_uart0_getc();
      neorv32_uart0_putc(c);
      if (c == 'q' || c == 'Q') {
        neorv32_uart0_puts("\nUART: exit.\n");
        return 0;
      }
    }

    /* 1초마다 하트비트 */
    if (neorv32_mtime_get_time() >= next) {
      neorv32_uart0_printf("UART heartbeat %u (cycle=%u)\n",
                           beats++, (uint32_t)neorv32_cpu_get_cycle());
      next += F_CPU;
    }
  }
}
