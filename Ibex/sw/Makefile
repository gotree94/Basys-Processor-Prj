# =============================================================================
# Ibex / Basys3 주변장치 로드맵 데모 - Makefile
# -----------------------------------------------------------------------------
#   make -> firmware.elf / .bin / .hex
# =============================================================================

TOOLCHAIN_PREFIX ?= riscv32-unknown-elf-
CC      := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
SIZE    := $(TOOLCHAIN_PREFIX)size

# Ibex 구성: RV32IMC. (Ibex 빌드 시 이 아키텍처와 맞출 것)
ARCH    := -march=rv32imc -mabi=ilp32
CFLAGS  := -Os -Wall -Wextra -ffreestanding -fno-builtin \
           -ffunction-sections -fdata-sections -Iinclude
LDFLAGS := -nostartfiles -nostdlib -Wl,--gc-sections -T link.ld \
           -Wl,-Map,firmware.map -lgcc

SRC := crt0.S \
       main.c \
       app/app_util.c  app/app_gpio.c app/app_uart.c app/app_timer.c \
       app/app_spi.c   app/app_twi.c  app/app_pwm.c \
       app/app_seg7.c  app/app_vga.c  app/app_dma.c app/app_ps2.c

.PHONY: all clean elf bin hex size disasm

all: firmware.bin firmware.hex

firmware.elf: $(SRC) link.ld
	$(CC) $(ARCH) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@
	$(SIZE) $@

elf: firmware.elf

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

firmware.hex: firmware.elf
	$(OBJCOPY) -O verilog --verilog-data-width=1 $< $@

size: firmware.elf
	$(SIZE) firmware.elf

disasm: firmware.elf
	$(OBJDUMP) -d -S $< > firmware.dis

clean:
	-$(RM) firmware.elf firmware.bin firmware.hex firmware.map firmware.dis
