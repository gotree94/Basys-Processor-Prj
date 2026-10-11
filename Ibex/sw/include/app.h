/* ============================================================================
 * app.h - Ibex / Basys3 주변장치 데모 공통 헤더
 * ----------------------------------------------------------------------------
 * Ibex 는 AXI4-Lite(req/gnt) 기반. 아래 주소맵을 인터커넥트 HDL 과 일치시킨다.
 *
 *   0x0000_0000 ~ 0x0000_7FFF   RAM (IMEM/DMEM, 32KB)
 *   0x1000_0000                 GPIO
 *   0x1001_0000                 UART
 *   0x1002_0000                 TIMER   (irq_timer -> mcause 7)
 *   0x1003_0000                 SPI
 *   0x1004_0000                 I2C
 *   0x1005_0000                 PWM
 *   0x1006_0000                 SEG7
 *   0x1007_0000                 VGA
 *   0x1008_0000                 DMA     (-> irq_external)
 *   0x1009_0000                 PS2     (-> irq_external)
 * ==========================================================================*/
#ifndef APP_H
#define APP_H

#include <stdint.h>

#define REG32(addr)  (*(volatile uint32_t*)(uintptr_t)(addr))

/* #1 GPIO */
#define GPIO_BASE   0x10000000u
#define GPIO_I      REG32(GPIO_BASE + 0x00)   /* [15:0] SW, [20:16] BTN */
#define GPIO_O      REG32(GPIO_BASE + 0x04)   /* [15:0] LED */
#define GPIO_BTN(n) (((GPIO_I) >> (16u + (n))) & 1u)

/* #2 UART */
#define UART_BASE   0x10010000u
#define UART_DATA   REG32(UART_BASE + 0x00)
#define UART_STAT   REG32(UART_BASE + 0x04)   /* [0] TX ready, [1] RX valid */
#define UART_CTRL   REG32(UART_BASE + 0x08)

/* #3 TIMER (irq_timer) */
#define TIMER_BASE  0x10020000u
#define TIMER_LOAD  REG32(TIMER_BASE + 0x00)
#define TIMER_CNT   REG32(TIMER_BASE + 0x04)
#define TIMER_CTRL  REG32(TIMER_BASE + 0x08)  /* [0]en [1]irq_en */
#define TIMER_FLAG  REG32(TIMER_BASE + 0x0C)  /* [0] write 1 = clear */

/* #4 SPI */
#define SPI_BASE    0x10030000u
#define SPI_DATA    REG32(SPI_BASE + 0x00)
#define SPI_STAT    REG32(SPI_BASE + 0x04)    /* [0] busy */
#define SPI_CTRL    REG32(SPI_BASE + 0x08)    /* [0]cs_n [1]start */

/* #5 I2C */
#define I2C_BASE    0x10040000u
#define I2C_CTRL    REG32(I2C_BASE + 0x00)
#define I2C_DATA    REG32(I2C_BASE + 0x04)
#define I2C_STAT    REG32(I2C_BASE + 0x08)    /* [0] busy [1] ack */

/* #6 PWM */
#define PWM_BASE    0x10050000u
#define PWM_CH0     REG32(PWM_BASE + 0x00)
#define PWM_CH1     REG32(PWM_BASE + 0x04)
#define PWM_CTRL    REG32(PWM_BASE + 0x08)    /* [0] en */

/* #7 SEG7 */
#define SEG7_BASE   0x10060000u
#define SEG7_DIG0   REG32(SEG7_BASE + 0x00)
#define SEG7_DIG1   REG32(SEG7_BASE + 0x04)
#define SEG7_DIG2   REG32(SEG7_BASE + 0x08)
#define SEG7_DIG3   REG32(SEG7_BASE + 0x0C)
#define SEG7_RAW    REG32(SEG7_BASE + 0x10)

/* #8 VGA (160x120 8bpp, 4px/word) */
#define VGA_BASE    0x10070000u
#define VGA_CTRL    REG32(VGA_BASE + 0x00)    /* [0]en [1]bufsel */
#define VGA_STATUS  REG32(VGA_BASE + 0x04)    /* [0]vblank */
#define VGA_FB      ((volatile uint32_t*)(VGA_BASE + 0x400))
#define VGA_DIM_X   160
#define VGA_DIM_Y   120

/* #9 DMA (irq_external) */
#define DMA_BASE    0x10080000u
#define DMA_SRC     REG32(DMA_BASE + 0x00)
#define DMA_DST     REG32(DMA_BASE + 0x04)
#define DMA_LEN     REG32(DMA_BASE + 0x08)
#define DMA_CTRL    REG32(DMA_BASE + 0x0C)    /* [0]start [1]irq_en */
#define DMA_STAT    REG32(DMA_BASE + 0x10)    /* [0]busy [1]done(w1c) */

/* #10 PS2 (irq_external) */
#define PS2_BASE    0x10090000u
#define PS2_DATA    REG32(PS2_BASE + 0x00)    /* [8]valid [7:0]scancode */
#define PS2_CTRL    REG32(PS2_BASE + 0x04)    /* [0]irq_en */

/* mcause 인터럽트 코드 */
#define IRQ_M_TIMER     7
#define IRQ_M_EXTERNAL  11

#define CPU_HZ      100000000u

/* 로드맵 데모 (#1~#10). 반환: 0 = pass */
int app_gpio_demo(void);
int app_uart_demo(void);
int app_timer_demo(void);
int app_spi_demo(void);
int app_twi_demo(void);
int app_pwm_demo(void);
int app_seg7_demo(void);
int app_vga_demo(void);
int app_dma_demo(void);
int app_ps2_demo(void);

/* 공통 유틸 (app_util.c) */
void     uart_putc(char c);
void     uart_puts(const char *s);
void     uart_printf(const char *fmt, ...);
uint32_t rdcycle(void);
void     app_delay_ms(uint32_t ms);
void     app_report(const char *name, int r);

/* 트랩/인터럽트 (crt0.S + app_timer.c) */
void trap_irq_enable(void);      /* mie.MTIE|MEIE + mstatus.MIE */
void trap_irq_disable(void);

#endif /* APP_H */
