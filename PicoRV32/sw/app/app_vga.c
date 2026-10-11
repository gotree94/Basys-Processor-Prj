/* ============================================================================
 * app_vga.c - 로드맵 #8  VGA 컬러바 (커스텀 HDL, 160x120 8bpp)
 * ==========================================================================*/
#include "app.h"

static const uint8_t palette[16] = {
  0x00, 0x03, 0x1C, 0x1F, 0xE0, 0xE3, 0xFC, 0xFF,
  0x92, 0x49, 0x24, 0x6D, 0xB6, 0xDB, 0x55, 0xAA
};

int app_vga_demo(void)
{
  uart_puts("VGA: drawing 8 color bars (160x120).\n");

  VGA_CTRL = 0x1;
  app_delay_ms(20);

  const uint32_t words_x = VGA_DIM_X / 4;         /* 40 */
  const uint32_t bar_w   = words_x / 8;           /* 5 */
  for (uint32_t y = 0; y < VGA_DIM_Y; y++) {
    for (uint32_t bx = 0; bx < words_x; bx++) {
      uint8_t color = palette[(bx / (bar_w ? bar_w : 1)) % 16];
      VGA_FB[y * words_x + bx] = (uint32_t)color
                               | ((uint32_t)color << 8)
                               | ((uint32_t)color << 16)
                               | ((uint32_t)color << 24);
    }
  }

  uart_puts("VGA: done. BTN4 to exit.\n");
  while (!GPIO_BTN(4)) app_delay_ms(50);
  VGA_CTRL = 0x0;
  uart_puts("VGA: exit.\n");
  return 0;
}
