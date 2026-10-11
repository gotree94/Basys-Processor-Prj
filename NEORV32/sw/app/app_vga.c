/* ============================================================================
 * app_vga.c - 로드맵 #8  VGA Framebuffer (커스텀 HDL)
 * ----------------------------------------------------------------------------
 * 160x120x8bpp(팔레트) 프레임버퍼에 컬러바를 그린다.
 * 프레임버퍼는 4픽셀/word 로 패킹되어 VGA_BASE+0x400 에 위치.
 * ==========================================================================*/
#include "app.h"

/* 16색 팔레트(0RRGGGBB 근사) */
static const uint8_t palette[16] = {
  0x00, 0x03, 0x1C, 0x1F, 0xE0, 0xE3, 0xFC, 0xFF,
  0x92, 0x49, 0x24, 0x6D, 0xB6, 0xDB, 0x55, 0xAA
};

int app_vga_demo(void)
{
  neorv32_uart0_puts("VGA: drawing 8 color bars (160x120).\n");

  VGA_CTRL = 0x1;                                  /* enable, buffer 0 */
  app_delay_ms(20);
  /* 버퍼 클리어 */
  for (uint32_t i = 0; i < (VGA_DIM_X / 4) * VGA_DIM_Y; i++) {
    VGA_FB[i] = 0;
  }

  /* 세로 컬러바 8개 */
  const uint32_t words_x = VGA_DIM_X / 4;          /* 40 words/row */
  const uint32_t bar_w_words = words_x / 8;        /* 5 words/bar */
  for (uint32_t y = 0; y < VGA_DIM_Y; y++) {
    for (uint32_t bx = 0; bx < words_x; bx++) {
      uint8_t color = palette[(bx / (bar_w_words ? bar_w_words : 1)) % 16];
      uint32_t word = (uint32_t)color
                    | ((uint32_t)color << 8)
                    | ((uint32_t)color << 16)
                    | ((uint32_t)color << 24);
      VGA_FB[y * words_x + bx] = word;
    }
  }

  neorv32_uart0_puts("VGA: done. BTN0 (pin16) to exit.\n");
  while (!neorv32_gpio_pin_get(16)) {               /* BTN0 = pin 16 */
    app_delay_ms(50);
  }
  VGA_CTRL = 0x0;                                  /* disable */
  neorv32_uart0_puts("VGA: exit.\n");
  return 0;
}
