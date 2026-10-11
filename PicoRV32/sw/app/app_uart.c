/* ============================================================================
 * app_uart.c - 로드맵 #2  UART (FTDI)
 * ==========================================================================*/
#include "app.h"

int app_uart_demo(void)
{
  uart_puts("UART: type chars (echo). 'q' to exit.\n");

  uint32_t beats = 0;
  uint32_t next = rdcycle();
  while (1) {
    if (UART_STAT & 0x2) {                 /* RX valid */
      char c = (char)(UART_DATA & 0xFF);
      uart_putc(c);
      if (c == 'q' || c == 'Q') { uart_puts("\nUART: exit.\n"); return 0; }
    }
    if ((rdcycle() - next) >= CPU_HZ) {    /* 1초 */
      uart_printf("UART heartbeat %u\n", beats++);
      next += CPU_HZ;
    }
  }
}
