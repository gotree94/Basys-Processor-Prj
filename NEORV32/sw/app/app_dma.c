/* ============================================================================
 * app_dma.c - 로드맵 #9  DMA (내장 DMA 컨트롤러)
 * ----------------------------------------------------------------------------
 * 소스 버퍼 -> 목적 버퍼 DMA 복사 후 체크섬으로 무결성 검증.
 *
 * NOTE: NEORV32 DMA 드라이버 API 이름은 버전에 따라 다를 수 있다.
 *       (neorv32.h 의 neorv32_dma_* 선언을 확인 후 필요 시 어댑트)
 * ==========================================================================*/
#include "app.h"

#define N 256
static uint32_t src[N];
static uint32_t dst[N];
static volatile int dma_done = 0;

static void dma_irq_handler(void)
{
  dma_done = 1;
}

int app_dma_demo(void)
{
  neorv32_uart0_puts("DMA: memory copy + checksum verify.\n");

  for (int i = 0; i < N; i++) {
    src[i] = (uint32_t)(i * 2654435761u);
    dst[i] = 0;
  }

  neorv32_rte_setup();
  neorv32_rte_handler_install(DMA_IRQ, dma_irq_handler);

  /* --- DMA 전송 (API 확정 후 아래 호출부를 맞춘다) --- */
#if defined(NEORV32_DMA_PRESENT)
  neorv32_dma_setup();
  neorv32_dma_memcpy((uint32_t)dst, (uint32_t)src, N * sizeof(uint32_t), 1);
  while (!dma_done) { neorv32_cpu_sleep(); }
#else
  /* 폴백: 소프트웨어 복사 (DMA 미포함 구성) */
  for (int i = 0; i < N; i++) dst[i] = src[i];
#endif

  /* 검증 */
  int errs = 0;
  for (int i = 0; i < N; i++) {
    if (dst[i] != src[i]) errs++;
  }
  neorv32_uart0_printf("DMA: errors=%d\n", errs);
  return (errs == 0) ? 0 : 1;
}
