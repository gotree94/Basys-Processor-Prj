/* ============================================================================
 * app_twi.c - 로드맵 #5  I2C / TWI (Pmod 버스 스캔)
 * ==========================================================================*/
#include "app.h"

int app_twi_demo(void)
{
  uart_puts("TWI: scanning 7-bit address bus...\n");

  int found = 0;
  for (uint32_t a = 0x08; a <= 0x77; a++) {
    I2C_DATA = (a << 1);
    I2C_CTRL = 0x1 | 0x8;
    while (I2C_STAT & 0x1) { /* busy */ }
    int ack = (I2C_STAT >> 1) & 1;
    I2C_CTRL = 0x2;
    if (ack) { uart_printf("  device @ 0x%x\n", a); found++; }
  }
  uart_printf("TWI: %d device(s) found.\n", found);
  return 0;
}
