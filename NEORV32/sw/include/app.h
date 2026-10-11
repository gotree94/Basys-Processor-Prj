/* ============================================================================
 * app.h - Basys3 / NEORV32 주변장치 데모 애플리케이션 공통 헤더
 * ----------------------------------------------------------------------------
 * 로드맵 #1~#10 데모 함수 프로토타입 + 커스텀 주변장치 MMIO 정의.
 *
 * 내장 주변장치(#1~#6, #9, #3)는 NEORV32 rte 라이브러리(neorv32.h)로 접근한다.
 * 커스텀 HDL 주변장치(#7 7-seg, #8 VGA, #10 PS/2)는 CFS(Custom Functions
 * Subsystem) 영역에 매핑되며 아래 매크로로 접근한다.
 *
 *  >> CFS_BASE 는 FPGA 래퍼(basys3_top.vhd)의 CFS 주소와 반드시 일치시킬 것.
 * ==========================================================================*/
#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "neorv32.h"

/* --------------------------------------------------------------------------
 * 커스텀 주변장치 MMIO (CFS: Custom Functions Subsystem)
 * NEORV32 내장 IO 는 라이브러리가 처리하므로 여기서는 커스텀만 정의.
 * --------------------------------------------------------------------------*/
#ifndef CFS_BASE
#define CFS_BASE   0xFFEB0000u   /* main(2026) 브랜치 base_io_cfs_c 와 일치 */
#endif

#define REG32(addr)  (*(volatile uint32_t*)(uintptr_t)(addr))

/* #7 7-seg : 자릿수별 값을 쓰면 HDL 디코더가 세그먼트 구동 */
#define SEG7_BASE   (CFS_BASE + 0x000u)
#define SEG7_DIG0   REG32(SEG7_BASE + 0x00)   /* 4bit hex x8(2자리씩) - 구현에 따름 */
#define SEG7_DIG1   REG32(SEG7_BASE + 0x04)
#define SEG7_DIG2   REG32(SEG7_BASE + 0x08)
#define SEG7_DIG3   REG32(SEG7_BASE + 0x0C)
#define SEG7_RAW    REG32(SEG7_BASE + 0x10)   /* 디코더 우회: a..g,dp 직접 */

/* #8 VGA : 프레임버퍼 창 + 제어 레지스터 */
#define VGA_BASE    (CFS_BASE + 0x100u)
#define VGA_CTRL    REG32(VGA_BASE + 0x00)    /* [0]EN [1]BUFSEL */
#define VGA_STATUS  REG32(VGA_BASE + 0x04)    /* [0]vblank */
#define VGA_FB      ((volatile uint32_t*)(VGA_BASE + 0x400))
/* 160x120 8bpp packed(4px/word) 예시: 160/4 * 120 = 4800 words */

/* #10 PS/2 : 키보드 스캔코드 FIFO */
#define PS2_BASE    (CFS_BASE + 0x200u)
#define PS2_DATA    REG32(PS2_BASE + 0x00)    /* [8]valid [7:0]scancode */
#define PS2_CTRL    REG32(PS2_BASE + 0x04)    /* [0]irq_en */

/* VGA 팔레트(0~15) - 3bit RGB 근사 */
#define VGA_DIM_X   160
#define VGA_DIM_Y   120

/* --------------------------------------------------------------------------
 * 로드맵 데모 함수 (#1~#10). 반환값: 0 = pass, 그 외 = fail 코드.
 * --------------------------------------------------------------------------*/
int app_gpio_demo(void);    /* #1  SW/LED/BTN/7seg */
int app_uart_demo(void);    /* #2  FTDI UART echo */
int app_timer_demo(void);   /* #3  MTIME IRQ + LED 점멸 */
int app_spi_demo(void);     /* #4  Pmod SPI 루프백 */
int app_twi_demo(void);     /* #5  Pmod I2C 버스 스캔 */
int app_pwm_demo(void);     /* #6  오디오 / LED PWM */
int app_seg7_demo(void);    /* #7  7-segment 카운터 */
int app_vga_demo(void);     /* #8  VGA 컬러바 */
int app_dma_demo(void);     /* #9  DMA 메모리 복사 + 검증 */
int app_ps2_demo(void);     /* #10 PS/2 키 입력 */

/* 공통 유틸 */
void app_delay_ms(uint32_t ms);          /* mtime 기반 지연 */
void app_report(const char *name, int r);/* 데모 결과 UART 출력 */

#endif /* APP_H */
