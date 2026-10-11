/* ============================================================================
 * app_spi.c - 로드맵 #4  SPI (Pmod)
 * ----------------------------------------------------------------------------
 * MOSI-MISO 를 점퍼로 연결(루프백)하여 송신=수신 검증.
 * 루프백이 없으면 임의 패턴을 전송 후 수신 값을 로그로 출력(수동 확인).
 * ==========================================================================*/
#include "app.h"

int app_spi_demo(void)
{
  neorv32_uart0_puts("SPI: loopback test (jumper MOSI<->MISO).\n");

  /* clk_div=2, CS0 사용, active-low */
  neorv32_spi_setup(2, (1u << 0), 0);

  int errs = 0;
  for (uint32_t pat = 0; pat < 256; pat++) {
    neorv32_spi_cs_en(0);
    uint8_t rx = neorv32_spi_transfer((uint8_t)pat);
    neorv32_spi_cs_dis();

    if (rx != (uint8_t)pat) {                       /* 루프백 불일치 */
      if (errs < 8) neorv32_uart0_printf("  tx=%02x rx=%02x\n", pat, rx);
      errs++;
    }
  }

  neorv32_uart0_printf("SPI: mismatch=%d\n", errs);
  /* 루프백 점퍼가 있으면 errs==0 -> pass */
  return (errs == 0) ? 0 : 1;
}
