/* ============================================================================
 * app_util.c - 공통 유틸리티 (mtime 지연, 테스트 결과 리포트)
 * ==========================================================================*/
#include "app.h"

/* mtime 기반 밀리초 지연. F_CPU 는 NEORV32 라이브러리에서 제공 */
void app_delay_ms(uint32_t ms)
{
  uint64_t start = neorv32_mtime_get_time();
  while ((neorv32_mtime_get_time() - start) < ((uint64_t)ms * (F_CPU / 1000u))) {
    /* busy wait */
  }
}

void app_report(const char *name, int r)
{
  if (r == 0) {
    neorv32_uart0_printf("[PASS] %s\n", name);
  } else {
    neorv32_uart0_printf("[FAIL] %s (code=%d)\n", name, r);
  }
}
