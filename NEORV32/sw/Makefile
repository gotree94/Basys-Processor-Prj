# =============================================================================
# NEORV32 / Basys3 주변장치 로드맵 데모 - 소프트웨어 Makefile
# -----------------------------------------------------------------------------
# 사용 전 NEORV32_HOME 을 클론한 neorv32 저장소 루트로 지정:
#   make NEORV32_HOME=/path/to/neorv32
# 또는 이 파일 상단 기본값을 수정.
#
# ※ "main"(2026) 브랜치 기준:
#   - 라이브러리:  sw/lib/source/*.c,  헤더: sw/lib/include
#   - 부트/링크:  sw/common/{crt0.S, neorv32.ld}
#   - HDL 기준:   hdl/basys3_top.vhd,  IMEM 32KB / DMEM 16KB
#
# 산출물:
#   firmware.elf  - 디버그/심볼
#   firmware.bin  - UART 부트로더 업로드용
#   firmware.hex  - BRAM $readmemh 초기화용
# =============================================================================

NEORV32_HOME ?= ../../neorv32
RISCV_PREFIX ?= riscv32-unknown-elf-

CC      := $(RISCV_PREFIX)gcc
OBJCOPY := $(RISCV_PREFIX)objcopy
OBJDUMP := $(RISCV_PREFIX)objdump
SIZE    := $(RISCV_PREFIX)size

# NEORV32 라이브러리 경로 (브랜치에 따라 sw/common/neorv32.c 단일 파일인 버전도 있음)
LIB_INC  := $(NEORV32_HOME)/sw/lib/include
LIB_SRC  := $(sort $(wildcard $(NEORV32_HOME)/sw/lib/source/*.c))
COMMON   := $(NEORV32_HOME)/sw/common
CRT0     := $(COMMON)/crt0.S
LDSCRIPT := $(COMMON)/neorv32.ld

ifneq ($(strip $(LIB_SRC)),)
LIBS := $(LIB_SRC)
else
# 구버전 레이아웃 (sw/common/neorv32.c)
LIBS := $(COMMON)/neorv32.c
endif

ARCH    := -march=rv32imc -mabi=ilp32
CFLAGS  := -Os -Wall -Wextra -ffunction-sections -fdata-sections \
           -Iinclude -I$(LIB_INC) -I$(COMMON)
LDFLAGS := -nostartfiles -Wl,--gc-sections -T $(LDSCRIPT) \
           -Wl,--defsym,__neorv32_rom_size=32K \
           -Wl,--defsym,__neorv32_ram_size=16K \
           -Wl,-Map,firmware.map

APP_SRC := main.c \
           app/app_util.c  app/app_gpio.c app/app_uart.c app/app_timer.c \
           app/app_spi.c   app/app_twi.c  app/app_pwm.c \
           app/app_seg7.c  app/app_vga.c  app/app_dma.c app/app_ps2.c

SRC := $(APP_SRC) $(LIBS) $(CRT0)

.PHONY: all clean elf bin hex size disasm

all: firmware.bin firmware.hex

firmware.elf: $(SRC)
	$(CC) $(ARCH) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@
	$(SIZE) $@

elf: firmware.elf

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

firmware.hex: firmware.elf
	$(OBJCOPY) -O verilog --verilog-data-width=4 $< $@

size: firmware.elf
	$(SIZE) firmware.elf

disasm: firmware.elf
	$(OBJDUMP) -d -S $< > firmware.dis

clean:
	-$(RM) firmware.elf firmware.bin firmware.hex firmware.map firmware.dis
