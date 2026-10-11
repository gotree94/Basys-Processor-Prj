/* ============================================================================
 * app.h - PicoRV32 / Basys3 주변장치 데모 공통 헤더
 * ----------------------------------------------------------------------------
 * PicoRV32 는 내장 MMIO 가 없으므로, FPGA 래퍼의 네이티브 버스 디코더가
 * 아래 주소맵을 구현한다고 가정한다. (hdl/wb_interconnect.v 와 일치시킬 것)
 *
 *   0x0000_0000 ~ 0x0000_7FFF   RAM (IMEM/DMEM, 32KB)
 *   0xF000_0000                 GPIO
 *   0xF000_0100                 UART
 *   0xF000_0200                 TIMER   (irq[3])
 *   0xF000_0300                 SPI
 *   0xF000_0400                 I2C
 *   0xF000_0500                 PWM
 *   0xF000_0600                 SEG7
 *   0xF000_0700                 VGA
 *   0xF000_0800                 DMA
 *   0xF000_0900                 PS2
 * ==========================================================================*/
#ifndef APP_H
#define APP_H

#include <stdint.h>

#define REG32(addr)  (*(volatile uint32_t*)(uintptr_t)(addr))

/* --------------------------------------------------------------------------
 * #1 GPIO
 * --------------------------------------------------------------------------*/
#define GPIO_BASE   0xF0000000u
#define GPIO_I      REG32(GPIO_BASE + 0x00)   /* [15:0] SW, [20:16] BTN */
#define GPIO_O      REG32(GPIO_BASE + 0x04)   /* [15:0] LED */
#define GPIO_BTN(n) (((GPIO_I) >> (16u + (n))) & 1u)

/* --------------------------------------------------------------------------
 * #2 UART (FTDI)
 * --------------------------------------------------------------------------*/
#define UART_BASE   0xF0000100u
#define UART_DATA   REG32(UART_BASE + 0x00)   /* 쓰기=TX, 읽기=RX */
#define UART_STAT   REG32(UART_BASE + 0x04)   /* [0] TX ready, [1] RX valid */
#define UART_CTRL   REG32(UART_BASE + 0x08)   /* [0] irq_en */

/* --------------------------------------------------------------------------
 * #3 TIMER (인터럽트 -> irq[3])
 * --------------------------------------------------------------------------*/
#define TIMER_BASE  0xF0000200u
#define TIMER_LOAD  REG32(TIMER_BASE + 0x00)
#define TIMER_CNT   REG32(TIMER_BASE + 0x04)
#define TIMER_CTRL  REG32(TIMER_BASE + 0x08)  /* [0]en [1]irq_en */
#define TIMER_FLAG  REG32(TIMER_BASE + 0x0C)  /* [0] write 1 = clear */

/* --------------------------------------------------------------------------
 * #4 SPI
 * --------------------------------------------------------------------------*/
#define SPI_BASE    0xF0000300u
#define SPI_DATA    REG32(SPI_BASE + 0x00)    /* 쓰기=TX, 읽기=RX */
#define SPI_STAT    REG32(SPI_BASE + 0x04)    /* [0] busy */
#define SPI_CTRL    REG32(SPI_BASE + 0x08)    /* [0] cs_n(0=sel) [1] start */

/* --------------------------------------------------------------------------
 * #5 I2C / TWI
 * --------------------------------------------------------------------------*/
#define I2C_BASE    0xF0000400u
#define I2C_CTRL    REG32(I2C_BASE + 0x00)    /* [0]start [1]stop [2]rd [3]wr */
#define I2C_DATA    REG32(I2C_BASE + 0x04)
#define I2C_STAT    REG32(I2C_BASE + 0x08)    /* [0] busy [1] ack/nack */

/* --------------------------------------------------------------------------
 * #6 PWM
 * --------------------------------------------------------------------------*/
#define PWM_BASE    0xF0000500u
#define PWM_CH0     REG32(PWM_BASE + 0x00)    /* audio */
#define PWM_CH1     REG32(PWM_BASE + 0x04)    /* LED fade */
#define PWM_CTRL    REG32(PWM_BASE + 0x08)    /* [0] en */

/* --------------------------------------------------------------------------
 * #7 SEG7 (커스텀 HDL)
 * --------------------------------------------------------------------------*/
#define SEG7_BASE   0xF0000600u
#define SEG7_DIG0   REG32(SEG7_BASE + 0x00)
#define SEG7_DIG1   REG32(SEG7_BASE + 0x04)
#define SEG7_DIG2   REG32(SEG7_BASE + 0x08)
#define SEG7_DIG3   REG32(SEG7_BASE + 0x0C)
#define SEG7_RAW    REG32(SEG7_BASE + 0x10)

/* --------------------------------------------------------------------------
 * #8 VGA (커스텀 HDL) : 160x120 8bpp, 4px/word
 * --------------------------------------------------------------------------*/
#define VGA_BASE    0xF0000700u
#define VGA_CTRL    REG32(VGA_BASE + 0x00)    /* [0]en [1]bufsel */
#define VGA_STATUS  REG32(VGA_BASE + 0x04)    /* [0]vblank */
#define VGA_FB      ((volatile uint32_t*)(VGA_BASE + 0x400))

#define VGA_DIM_X   160
#define VGA_DIM_Y   120

/* --------------------------------------------------------------------------
 * #9 DMA
 * --------------------------------------------------------------------------*/
#define DMA_BASE    0xF0000800u
#define DMA_SRC     REG32(DMA_BASE + 0x00)
#define DMA_DST     REG32(DMA_BASE + 0x04)
#define DMA_LEN     REG32(DMA_BASE + 0x08)    /* 바이트 수 */
#define DMA_CTRL    REG32(DMA_BASE + 0x0C)    /* [0]start [1]irq_en (irq[4]) */
#define DMA_STAT    REG32(DMA_BASE + 0x10)    /* [0]busy [1]done(w1c) */

/* --------------------------------------------------------------------------
 * #10 PS2 (커스텀 HDL)
 * --------------------------------------------------------------------------*/
#define PS2_BASE    0xF0000900u
#define PS2_DATA    REG32(PS2_BASE + 0x00)    /* [8]valid [7:0]scancode */
#define PS2_CTRL    REG32(PS2_BASE + 0x04)    /* [0]irq_en (irq[5]) */

/* --------------------------------------------------------------------------
 * IRQ 번호 (HDL 의 irq[] 배선과 일치)
 * --------------------------------------------------------------------------*/
#define IRQ_TIMER   3
#define IRQ_DMA     4
#define IRQ_PS2     5

/* CPU 클럭 (Hz) - rdcycle 기반 지연/타이머 계산에 사용 */
#define CPU_HZ      100000000u

/* --------------------------------------------------------------------------
 * 로드맵 데모 함수 (#1~#10). 반환: 0 = pass
 * --------------------------------------------------------------------------*/
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

/* 인터럽트 (start.S) */
void picorv32_enable_irqs(uint32_t mask);   /* mask: 1 = 비활성 */
void picorv32_disable_irqs(void);

#endif /* APP_H */
