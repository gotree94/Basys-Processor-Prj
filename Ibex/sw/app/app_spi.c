/* ============================================================================
 * app_spi.c - 로드맵 #4  SPI (Pmod 루프백)
 * ==========================================================================*/
#include "app.h"

static uint8_t spi_xfer(uint8_t tx)
{
  SPI_CTRL = 0x0;
  SPI_DATA = tx;
  SPI_CTRL = 0x2;
  while (SPI_STAT & 0x1) { /* busy */ }
  return (uint8_t)(SPI_DATA & 0xFF);
}

int app_spi_demo(void)
{
  uart_puts("SPI: loopback test (jumper MOSI<->MISO).\n");

  int errs = 0;
  for (uint32_t pat = 0; pat < 256; pat++) {
    uint8_t rx = spi_xfer((uint8_t)pat);
    if (rx != (uint8_t)pat) {
      if (errs < 8) uart_printf("  tx=%x rx=%x\n", pat, rx);
      errs++;
    }
  }
  uart_printf("SPI: mismatch=%d\n", errs);
  return (errs == 0) ? 0 : 1;
}
