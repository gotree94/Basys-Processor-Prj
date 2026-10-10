# NEORV32 on Basys 3

> **추천도 ★★★★★** — 내장 주변장치가 가장 많고, Windows 단독으로도 진행 가능하며 문서가 우수함.

RV32IMC 소프트코어. UART/SPI/TWI/PWM/GPIO/Timer/WDT/TRNG/DMA/CFU 등 **주변장치가 이미 SoC에 통합**되어 있어
"주변장치 최대 연결" 목표에 가장 부합.

---

## 0. 프로젝트 구조

```
NEORV32/
├── README.md
├── hdl/
│   ├── neorv32/           # 업스트림 RTL (git submodule/clone)
│   └── basys3_top.vhd     # 보드 래퍼 (top)
├── sw/
│   ├── main.c
│   ├── Makefile           # neorv32 sw makefile 재사용
│   └── (riscv toolchain)
├── constrs/basys3.xdc
├── sim/tb_basys3.vhd
├── scripts/
│   ├── create_project.tcl
│   ├── build.tcl
│   └── program.tcl
└── bitstream/
```

---

## 1. 환경설정

### 1.1 툴체인 (공통 README 4.3 참고)
```bash
riscv32-unknown-elf-gcc --version   # rv32imc/ilp32
```

### 1.2 NEORV32 획득
```bash
git clone https://github.com/stnolting/neorv32
cd neorv32
# RTL 라이브러리: rtl/core, rtl/system, rtl/processor_toplevel
```

### 1.3 권장 설정 (Vivado 2022.2, Artix-7)

NEORV32는 `neorv32_ProcessorTop`을 top으로 쓰거나 커스텀 래퍼를 씀.

`neorv32_top` generic 핵심값:
```vhdl
CPU_ARCH        => rv32imc_zicsr
MEM_INT_IMEM_EN => true      -- BRAM 내부 IMEM
MEM_INT_DMEM_EN => true      -- BRAM 내부 DMEM
MEM_INT_DMEM_SIZE => 16*1024 -- 16 KB
IO_GPIO_NUM     => 16
IO_PWM_NUM      => 3
IO_UART0_EN     => true
IO_SPI_EN       => true
IO_TWI_EN       => true
IO_TRNG_EN      => true      -- TRNG는 자원 소모 크니 필요시
CACHE_*         => false     -- 35T에서는 캐시 없이 단순/면적 절약
```

> 35T에서 IMEM 16K + DMEM 16K 정도면 BRAM 여유 충분. VGA 프레임버퍼(로드맵 #8)를 위해 BRAM 예산 관리.

---

## 2. 주변장치 로드맵 (NEORV32 매핑)

| # | 로드맵 | NEORV32 구현 | 방식 |
|---|---|---|---|
| 1 | GPIO | `GPIO` 모듈 (입출력 16+16) | 내장 MMIO |
| 2 | UART | `UART0/1` | 내장 |
| 3 | Timer+IRQ | `MTIME` + `MEIEXT`/PLIC(내장) | 내장 |
| 4 | SPI | `SPI` | 내장 |
| 5 | I2C | `TWI` | 내장 |
| 6 | PWM | `PWM` (N채널) | 내장 |
| 7 | 7-seg | 커스텀 디코더 (GPIO 뒤에) | HDL 추가 |
| 8 | VGA | Wishbone 슬레이브 + BRAM | HDL 추가 |
| 9 | DMA | `DMA` | 내장 |
| 10 | PS/2 | 커스텀 | HDL 추가 |
| 11 | ExtMem | `XIP`/SPI Flash or Wishbone SRAM | 부분 |
| 12 | USB-HID | 외부 스택 | 최후순위 |

---

## 3. 코드 작성

### 3.1 FPGA 래퍼 (`hdl/basys3_top.vhd`, 개념)

```vhdl
entity basys3_top is
  port (
    CLK100MHZ : in  std_logic;
    -- UART
    uart_txd_o, uart_rxd_i : ...
    -- GPIO
    sw_i : in std_logic_vector(15 downto 0);
    led_o: out std_logic_vector(15 downto 0);
    btn_i: in std_logic_vector(4 downto 0);
    -- 7-seg
    seg_o: out std_logic_vector(6 downto 0);
    an_o : out std_logic_vector(3 downto 0);
    -- VGA
    vga_r, vga_g, vga_b : out std_logic_vector(3 downto 0);
    vga_hsync, vga_vsync : out std_logic;
    -- Pmod SPI/I2C
    spi_cs_o, spi_sck_o, spi_mosi_o : out std_logic;
    spi_miso_i : in std_logic;
    twi_sda_io : inout std_logic;
    twi_scl_io : inout std_logic;
    -- audio PWM
    audio_pwm_o : out std_logic
  );
end entity;
```

핵심 연결:
- `GPIO`의 일부 비트를 SW/LED/BTN에 매핑
- `PWM` 채널0 → 오디오, 채널1..N → 서보/LED
- `SPI`/`TWI` → Pmod (JA/JB) 3.3V 레벨 (Basys3 Pmod는 3.3V)
- 7-seg는 `GPIO` 상위 비트를 디코더(`sw/`가 아닌 **HDL** 낭비 최소) 또는 C 제어

### 3.2 7-seg 디코더 (HDL)

```vhdl
-- 8비트 -> 세그먼트 (a..g, dp), active-low
function seg7(b : std_logic_vector(3 downto 0)) return std_logic_vector is
begin
  case b is
    when "0000" => return "0000001"; -- 0
    when "0001" => return "1001111"; -- 1
    -- ...
    when others => return "1111111";
  end case;
end function;
```

### 3.3 소프트웨어 (`sw/main.c`)

NEORV32 내장 주변장치는 **C 헤더(`neorv32.h`)의 rte 라이브러리**로 접근:

```c
#include "neorv32.h"

int main(void) {
  neorv32_uart0_setup(19200, 0);          // #2 UART
  neorv32_uart0_printf("NEORV32 Basys3 boot\n");

  // #1 GPIO
  neorv32_gpio_port_set(neorv32_gpio_sw_get());  // SW -> LED

  // #3 Timer + IRQ
  neorv32_rte_setup();
  neorv32_mtime_set_timecmp(neorv32_mtime_get_time() + 100000);
  neorv32_rte_handler_install(MTIME_IRQ, mtime_handler);

  // #4 SPI, #5 TWI, #6 PWM, #9 DMA
  neorv32_spi_setup(...);
  neorv32_twi_setup(...);
  neorv32_pwm_setup(...);

  while (1) { neorv32_cpu_sleep(); }
}
```

빌드:
```bash
cd sw
make MARCH=rv32imc MABI=ilp32 RISCV_PREFIX=riscv32-unknown-elf- clean all
# 결과: neorv32_exe.bin / elf
```

부팅 방법 (택1):
- **UART bootloader** (기본): 리셋 후 UART로 `exe.bin` 업로드 (툴 `sw/image_gen` 사용)
- **BRAM init**: `elf` → `vhd` 변환 후 synthesize에 포함 (고정 펌웨어)

---

## 4. 제약 파일 (`constrs/basys3.xdc`, 발췌)

```tcl
# Clock
set_property -dict {PACKAGE_PIN W5 IOSTANDARD LVCMOS33} [get_ports CLK100MHZ]
create_clock -period 10.000 [get_ports CLK100MHZ]

# UART (FTDI)
set_property -dict {PACKAGE_PIN A18 IOSTANDARD LVCMOS33} [get_ports uart_txd_o]
set_property -dict {PACKAGE_PIN B18 IOSTANDARD LVCMOS33} [get_ports uart_rxd_i]

# Pmod JA (SPI)
set_property -dict {PACKAGE_PIN J1 IOSTANDARD LVCMOS33} [get_ports spi_cs_o]
set_property -dict {PACKAGE_PIN L2 IOSTANDARD LVCMOS33} [get_ports spi_sck_o]
set_property -dict {PACKAGE_PIN J2 IOSTANDARD LVCMOS33} [get_ports spi_mosi_o]
set_property -dict {PACKAGE_PIN K2 IOSTANDARD LVCMOS33} [get_ports spi_miso_i]

# VGA (RGB444 + sync)  -- 정확 핀은 Basys3 Master XDC 참조
set_property -dict {PACKAGE_PIN P19 IOSTANDARD LVCMOS33} [get_ports vga_hsync]
set_property -dict {PACKAGE_PIN R19 IOSTANDARD LVCMOS33} [get_ports vga_vsync]
```

---

## 5. 검증

### 5.1 시뮬레이션
```tcl
# sim/tb_basys3.vhd + xsim
xelab tb_basys3 -debug typical
xsim tb_basys3 -runall
```
테스트 항목: 리셋 후 첫 UART 바이트, GPIO loopback, Timer IRQ 카운트.

### 5.2 합성/구현
```powershell
vivado -mode batch -source scripts\build.tcl
```
체크: WNS≥0 (100MHz), BRAM 사용량 (VGA 프레임버퍼 추가 전/후).

### 5.3 보드 검증 순서
1. 비트스트림 program → UART 터미널(19200) "NEORV32 Basys3 boot"
2. SW를 움직여 LED 반영 (#1 pass)
3. mtime 인터럽트로 LED 점멸 (#3 pass)
4. Pmod 루프백으로 SPI/TWI 확인 (#4/#5)
5. 오디오 PWM 톤 (#6)
6. 7-seg 숫자 카운터 (#7)
7. VGA 패턴 (#8), DMA 메모리 복사 (#9), PS/2 키 (#10)

---

## 6. 체크리스트

- [ ] M1 툴체인 + Vivado 배치 플로우
- [ ] M2 코어 부팅 + UART
- [ ] #1 GPIO  #2 UART  #3 Timer/IRQ
- [ ] #4 SPI  #5 TWI  #6 PWM  #7 7-seg
- [ ] #8 VGA  #9 DMA  #10 PS/2
- [ ] #11 ExtMem  #12 USB(선택)

## 참고
- NEORV32: https://github.com/stnolting/neorv32
- Datasheet/User Guide: `docs/datasheet.pdf`, `docs/userguide.pdf`
