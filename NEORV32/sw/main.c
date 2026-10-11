/* ============================================================================
 * main.c - NEORV32 / Basys3 주변장치 로드맵 데모 진입점
 * ----------------------------------------------------------------------------
 * UART 메뉴로 #1~#10 데모를 선택 실행. 'a' = 전체 스모크 테스트.
 *
 * 빌드:  make
 * 로드:  UART 부트로더(권장) 또는 BRAM init
 * ==========================================================================*/
#include "app.h"

typedef struct {
  char  key;
  const char *name;
  int (*fn)(void);
} demo_t;

static const demo_t demos[] = {
  { '1', "GPIO (SW/LED/BTN)",        app_gpio_demo  },
  { '2', "UART (echo)",              app_uart_demo  },
  { '3', "Timer + IRQ",              app_timer_demo },
  { '4', "SPI (Pmod loopback)",      app_spi_demo   },
  { '5', "TWI (I2C bus scan)",       app_twi_demo   },
  { '6', "PWM (audio/LED)",          app_pwm_demo   },
  { '7', "7-seg (counter)",          app_seg7_demo  },
  { '8', "VGA (color bars)",         app_vga_demo   },
  { '9', "DMA (mem copy)",           app_dma_demo   },
  { '0', "PS/2 (keyboard)",          app_ps2_demo   },
};

static void print_menu(void)
{
  neorv32_uart0_puts("\n=== NEORV32 Basys3 Peripheral Roadmap ===\n");
  for (unsigned i = 0; i < sizeof(demos)/sizeof(demos[0]); i++) {
    neorv32_uart0_printf("  [%c] %s\n", demos[i].key, demos[i].name);
  }
  neorv32_uart0_puts("  [a] run ALL smoke test\n");
  neorv32_uart0_puts("  [m] menu\n");
  neorv32_uart0_puts("Select> ");
}

static void run_all(void)
{
  neorv32_uart0_puts("\n--- Smoke test (non-interactive demos) ---\n");
  app_report("SPI", app_spi_demo());
  app_report("TWI", app_twi_demo());
  app_report("DMA", app_dma_demo());
  neorv32_uart0_puts("--- (interactive demos run individually) ---\n");
}

int main(void)
{
  neorv32_uart0_setup(19200, 0);
  neorv32_uart0_puts("\nNEORV32 on Basys3 boot OK\n");
  neorv32_uart0_printf("UID=0x%08x CLK=%u Hz\n",
                       (uint32_t)neorv32_cpu_get_uid(), (uint32_t)F_CPU);

  /* GPIO 방향: [15:0] = LED 출력, [19:16] = BTN 입력, [31:20] = SW 입력 */
  neorv32_gpio_port_dir_set(0x0000FFFF);

  print_menu();

  for (;;) {
    if (!neorv32_uart0_available()) {
      neorv32_cpu_sleep();
      continue;
    }
    char c = (char)neorv32_uart0_getc();
    neorv32_uart0_putc(c);
    neorv32_uart0_putc('\n');

    if (c == 'a' || c == 'A') { run_all(); print_menu(); continue; }
    if (c == 'm' || c == 'M') { print_menu(); continue; }

    int ran = 0;
    for (unsigned i = 0; i < sizeof(demos)/sizeof(demos[0]); i++) {
      if (c == demos[i].key) {
        neorv32_uart0_printf("--- run #%c: %s ---\n", demos[i].key, demos[i].name);
        int r = demos[i].fn();
        app_report(demos[i].name, r);
        ran = 1;
        break;
      }
    }
    if (!ran) neorv32_uart0_puts("invalid selection.\n");
    print_menu();
  }
  return 0;
}
