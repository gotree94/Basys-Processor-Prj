/* ============================================================================
 * main.c - PicoRV32 / Basys3 주변장치 로드맵 데모 진입점
 * ----------------------------------------------------------------------------
 * UART 메뉴로 #1~#10 데모 선택 실행. 'a' = 스모크 테스트.
 * ==========================================================================*/
#include "app.h"

typedef struct {
  char  key;
  const char *name;
  int (*fn)(void);
} demo_t;

static const demo_t demos[] = {
  { '1', "GPIO (SW/LED/BTN)",   app_gpio_demo  },
  { '2', "UART (echo)",         app_uart_demo  },
  { '3', "Timer + IRQ",         app_timer_demo },
  { '4', "SPI (Pmod loopback)", app_spi_demo   },
  { '5', "TWI (I2C bus scan)",  app_twi_demo   },
  { '6', "PWM (audio/LED)",     app_pwm_demo   },
  { '7', "7-seg (counter)",     app_seg7_demo  },
  { '8', "VGA (color bars)",    app_vga_demo   },
  { '9', "DMA (mem copy)",      app_dma_demo   },
  { '0', "PS/2 (keyboard)",     app_ps2_demo   },
};

static void print_menu(void)
{
  uart_puts("\n=== PicoRV32 Basys3 Peripheral Roadmap ===\n");
  for (unsigned i = 0; i < sizeof(demos)/sizeof(demos[0]); i++) {
    uart_printf("  [%c] %s\n", demos[i].key, demos[i].name);
  }
  uart_puts("  [a] run ALL smoke test\n  [m] menu\nSelect> ");
}

static void run_all(void)
{
  uart_puts("\n--- Smoke test (non-interactive) ---\n");
  app_report("SPI", app_spi_demo());
  app_report("TWI", app_twi_demo());
  app_report("DMA", app_dma_demo());
  uart_puts("--- (interactive demos run individually) ---\n");
}

int main(void)
{
  uart_puts("\nPicoRV32 on Basys3 boot OK\n");
  uart_printf("start cycle = %u\n", rdcycle());

  print_menu();

  for (;;) {
    if (!(UART_STAT & 0x2)) continue;          /* RX valid 대기 */
    char c = (char)(UART_DATA & 0xFF);
    uart_putc(c);
    uart_putc('\n');

    if (c == 'a' || c == 'A') { run_all(); print_menu(); continue; }
    if (c == 'm' || c == 'M') { print_menu(); continue; }

    int ran = 0;
    for (unsigned i = 0; i < sizeof(demos)/sizeof(demos[0]); i++) {
      if (c == demos[i].key) {
        uart_printf("--- run #%c: %s ---\n", demos[i].key, demos[i].name);
        app_report(demos[i].name, demos[i].fn());
        ran = 1;
        break;
      }
    }
    if (!ran) uart_puts("invalid selection.\n");
    print_menu();
  }
  return 0;
}
