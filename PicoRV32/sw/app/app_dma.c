/* ============================================================================
 * app_dma.c - 로드맵 #9  DMA 메모리 복사 + 검증
 * ==========================================================================*/
#include "app.h"

#define N 256
static uint32_t src[N];
static uint32_t dst[N];

int app_dma_demo(void)
{
  uart_puts("DMA: memory copy + verify.\n");

  for (int i = 0; i < N; i++) {
    src[i] = (uint32_t)(i * 2654435761u);
    dst[i] = 0;
  }

  DMA_SRC  = (uint32_t)src;
  DMA_DST  = (uint32_t)dst;
  DMA_LEN  = N * sizeof(uint32_t);
  DMA_CTRL = 0x1;                               /* start (polling) */
  while (DMA_STAT & 0x1) { /* busy */ }

  int errs = 0;
  for (int i = 0; i < N; i++) if (dst[i] != src[i]) errs++;
  uart_printf("DMA: errors=%d\n", errs);
  return (errs == 0) ? 0 : 1;
}
