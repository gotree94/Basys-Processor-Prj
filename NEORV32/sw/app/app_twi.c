/* ============================================================================
 * app_twi.c - 로드맵 #5  I2C / TWI (Pmod)
 * ----------------------------------------------------------------------------
 * 버스를 스캔하여 응답하는 7-bit 주소 목록을 출력.
 * (SDA/SCL 에 pull-up 필요 - Pmod I2C 모듈 또는 외부 저항)
 * ==========================================================================*/
#include "app.h"

int app_twi_demo(void)
{
  neorv32_uart0_puts("TWI: scanning 7-bit address bus...\n");

  neorv32_twi_setup(1, 1, 0);                       /* prsc, cdiv, no-timeout */

  int found = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    if (neorv32_twi_start(a, 0)) {                  /* START + write */
      if (neorv32_twi_transmit(0x00)) {             /* dummy write */
        neorv32_uart0_printf("  device @ 0x%02x\n", a);
        found++;
      }
      neorv32_twi_stop();
    }
  }

  neorv32_uart0_printf("TWI: %d device(s) found.\n", found);
  return 0;                                          /* 스캔 자체는 pass */
}
