/* ============================================================================
 * app_pwm.c - 로드맵 #6  PWM (오디오 / LED)
 * ==========================================================================*/
#include "app.h"

int app_pwm_demo(void)
{
  uart_puts("PWM: LED fade + audio tone. BTN2 to exit.\n");
  PWM_CTRL = 0x1;

  uint32_t v = 0;
  int dir = 1;
  while (!GPIO_BTN(2)) {
    PWM_CH1 = v & 0xFF;                /* LED 페이드 */
    PWM_CH0 = (v & 0x80) ? 0x40 : 0x10;/* 오디오 duty */
    v += dir * 4;
    if (v >= 256) dir = -1;
    if (v == 0)   dir = 1;
    app_delay_ms(5);
  }

  PWM_CTRL = 0x0;
  uart_puts("PWM: exit.\n");
  return 0;
}
