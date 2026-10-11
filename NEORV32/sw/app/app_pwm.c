/* ============================================================================
 * app_pwm.c - 로드맵 #6  PWM (onboard audio / LED)
 * ----------------------------------------------------------------------------
 * 채널0 = 오디오 PWM(약 1kHz 톤 원리 확인용), 채널1 = LED 페이드.
 * ==========================================================================*/
#include "app.h"

int app_pwm_demo(void)
{
  neorv32_uart0_puts("PWM: LED fade + audio tone. BTN2 to exit.\n");

  neorv32_pwm_setup(0);                             /* prescaler=0 -> 최대 분해능 */

  uint32_t v = 0;
  int dir = 1;
  while (1) {
    /* 채널1: LED 페이드 (0..255) */
    neorv32_pwm_set(1, v & 0xFF);

    /* 채널0: 오디오 영역 - 값이 클수록 duty 큼 (사각파 톤) */
    neorv32_pwm_set(0, (v & 0x80) ? 0x40 : 0x10);

    v += dir * 4;
    if (v >= 256) dir = -1;
    if (v == 0)   dir = 1;

    if (neorv32_gpio_pin_get(18)) {                 /* BTN2 = pin 18 */
      neorv32_pwm_dis();
      neorv32_uart0_puts("PWM: exit.\n");
      return 0;
    }
    app_delay_ms(5);
  }
}
