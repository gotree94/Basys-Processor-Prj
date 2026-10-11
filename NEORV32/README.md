# NEORV32 on Basys 3

> **추천도 ★★★★★** — 내장 주변장치가 가장 많고, Windows 단독으로도 진행 가능하며 문서가 우수함.

RV32IMC 소프트코어. UART/SPI/TWI/PWM/GPIO/Timer/WDT/TRNG/DMA/CFU 등 **주변장치가 이미 SoC에 통합**되어 있어
"주변장치 최대 연결" 목표에 가장 부합.

---

## 0. 프로젝트 구조 (모두 파일 포함)

```
NEORV32/
├── README.md
├── hdl/                          # ★ 보드/주변장치 HDL (작성 완료)
│   ├── basys3_top.vhd            #    보드 래퍼 (top, neorv32_top 인스턴스)
│   ├── cfs_periph.vhd            #    CFS 대체: 7-seg/VGA/PS2 MMIO
│   ├── fb_ram.vhd                #    VGA 프레임버퍼 (4800 words)
│   ├── seg7_ctrl.vhd             #    7-seg 디코더+리프레시
│   ├── vga_ctrl.vhd              #    640x480@60 타이밍 + FB 읽기
│   └── ps2_rx.vhd                #    PS/2 수신기
├── sw/                    # ★ 실제 소스 포함 (sw/README.md 참고)
│   ├── Makefile
│   ├── main.c             # UART 메뉴 디스패처 (+ GPIO 방향 초기화)
│   ├── include/app.h      # MMIO 맵 + API
│   └── app/app_*.c        # 로드맵 #1~#10 드라이버/데모
├── constrs/basys3.xdc     # ★ 핀 제약 (작성 완료)
├── sim/tb_basys3.vhd      # ★ top 스모크 테스트벤치 (작성 완료)
├── scripts/
│   ├── create_project.tcl # ★ 프로젝트 생성 (NEORV32_HOME 인자)
│   ├── build.tcl          # ★ 합성/구현/비트스트림
│   └── program.tcl        # ★ 보드 프로그래밍
└── bitstream/             # .bit 산출물 (build.tcl 이 생성)
```

> NEORV32 업스트림 RTL 은 별도 경로(`NEORV32_HOME` 등)에서 참조.
> `scripts/create_project.tcl -tclargs <NEORV32_HOME>` 로 프로젝트 생성.

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

`hdl/basys3_top.vhd` 가 top 이며 `neorv32_top` generic 실제 반영값:
```vhdl
BOOT_MODE_SELECT  => 1                -- BOOT_ADDR=0x00000000 (IMEM 시작)
RISCV_ISA_C       => true
RISCV_ISA_M       => true             -- rv32imc
RISCV_ISA_Zicntr  => true             -- mtime/cycle 카운터
IMEM_SIZE         => 32*1024,         -- 32 KB (dpath 비트폭 32)
DMEM_SIZE         => 16*1024          -- 16 KB
IO_GPIO_NUM       => 24,              -- 개별 핀 (SW/LED/BTN)
IO_GPIO_DIR_EN    => true,            -- 24bit 방향 레지스터
IO_CLINT_EN       => true,            -- MTIME
IO_UART0_EN       => true,
IO_SPI_EN         => true,
IO_TWI_EN         => true,
IO_PWM_NUM        => 4,
IO_DMA_EN         => true,
IO_CFS_EN         => true,            -- 커스텀: 7-seg/VGA/PS2
```

> IMEM 32K + DMEM 16K + VGA 프레임버퍼(4800 words, ~9.6KB BRAM)를 함께 써도
> 35T BRAM(50개, 180KB) 내에서 여유 있음.

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

> **`hdl/` 의 6개 파일이 실제 구현(작성 완료)이며, 아래는 요약.**

### 3.1 FPGA 래퍼 (`hdl/basys3_top.vhd`)

포트: `clk, btnC, btnU, btnD, btnL, btnR, sw[15:0], led[15:0],
seg[6:0], dp, an[3:0], vgaRed[3:0], vgaGreen[3:0], vgaBlue[3:0],
Hsync, Vsync, RsTx, RsRx, PS2Clk, PS2Data, JA[7:0], JC[7:0]`

핵심 연결 (메모리맵/핀맵 상세 = `sw/README.md`):
- `GPIO` 24개: `gpio_i[15:0]`=SW, `gpio_i[19:16]`=BTN0..3 (전부 dir=0 입력),
  `gpio_o[15:0]` → `led` (dir=1 핀만 출력 반영; `main.c` 에서 `0x0000FFFF` 설정)
- `PWM` ch0 → `JA[4]` (Basys3 오디오 없음), 나머지 채널은 LED LDA[1:0] 구현 가능
- `SPI` → `JA[3:0]` (MOSI=MISO 루프백으로 #4 검증)
- `TWI` → `JC[1:0]`, open-drain ('0'일 때만 drive, 그 외 'Z')
- `UART0` → FTDI (`RsTx`=A18, `RsRx`=B18)
- `CFS` → 7-seg / VGA / PS2 (아래)

### 3.2 커스텀 주변장치 (CFS) 요약

- `hdl/cfs_periph.vhd` — 엔티티명 `neorv32_cfs` 로 업스트림 `neorv32_cfs.vhd` 대체.
  프로젝트에는 **cfs_periph.vhd 만 포함**(중복 엔티티 방지, create_project.tcl 가 자동 처리).
  MMIO 맵: DIG0..3(0x00..0x0C), RAW(0x10), VGA_CTRL(0x100), VGA_STATUS(0x104),
  PS2_DATA(0x200), PS2_CTRL(0x204), VGA_FB(0x500~0x14FC). CFS IRQ = FIRQ1.
  conduit: `cfs_out[25:0]` = (cat,dp,an,R,G,B,Hsync,Vsync), `cfs_in[1:0]` = PS2(C,D).
- `hdl/seg7_ctrl.vhd` — 공통 애노드 4-digit active-low 디코더+리프레시.
- `hdl/vga_ctrl.vhd` + `hdl/fb_ram.vhd` — 640x480@60 (100MHz/4), 160x120 8bpp.
- `hdl/ps2_rx.vhd` — 11비트 PS/2 프레임 수신, FIFO 512.

### 3.3 소프트웨어 (`sw/`) — 상세 구현 포함

> **로드맵 #1~#10 실제 소스가 `sw/` 에 포함되어 있음.** 빌드/실행/검증 안내는
> [`sw/README.md`](sw/README.md) 참고. (`main.c` + `include/app.h` + `app/app_*.c`)

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
make NEORV32_HOME=/path/to/neorv32    # -> firmware.bin / firmware.hex / .elf
```

부팅 방법 (택1):
- **UART bootloader** (기본): 리셋 후 UART로 `firmware.bin` 업로드 (툴 `tools/` 등)
- **BRAM init**: `firmware.hex` 를 IMEM `$readmemh` 로 포함 후 synthesize (고정 펌웨어)

---

## 4. 제약 파일 (`constrs/basys3.xdc` — 작성 완료)

```tcl
# Clock
set_property -dict {PACKAGE_PIN W5 IOSTANDARD LVCMOS33} [get_ports clk]
create_clock -add -name sys_clk_pin -period 10.00 [get_ports clk]

# UART (FTDI)
set_property -dict {PACKAGE_PIN A18 IOSTANDARD LVCMOS33} [get_ports RsTx]
set_property -dict {PACKAGE_PIN B18 IOSTANDARD LVCMOS33} [get_ports RsRx]

# PS/2
set_property -dict {PACKAGE_PIN C17 IOSTANDARD LVCMOS33 PULLUP true} [get_ports PS2Clk]
set_property -dict {PACKAGE_PIN B17 IOSTANDARD LVCMOS33 PULLUP true} [get_ports PS2Data]

# Pmod JA: SPI(3:0)+PWM(4) / Pmod JC: TWI(1:0)
set_property -dict {PACKAGE_PIN J1 IOSTANDARD LVCMOS33} [get_ports {JA[0]}]   # sck
# ... (전체 저핀 목록은 basys3.xdc 참조)
```

> Basys3 Master XDC 와 포트명이 다르므로 **반드시 이 파일 기준**으로 사용.

---

## 5. 검증

### 5.1 시뮬레이션
```tcl
# sim/tb_basys3.vhd (top 스모크: 리셋/클럭/보드 IO 배선)
xelab tb_basys3 -debug typical
xsim tb_basys3 -runall
```
> 완전 부팅 시뮬레이션은 펌웨어를 IMEM 에 로드해야 하므로, 로직은 NEORV32 제공
> `sim/` 테스트벤치(IMEM_GENERIC_FILE=firmware.hex) 활용 권장.

### 5.2 프로젝트 생성/합성/구현/프로그램
```powershell
# 1) 프로젝트 생성 (NEORV32 업스트림 경로를 인자로)
vivado -mode batch -source scripts\create_project.tcl -tclargs C:\path\to\neorv32
# 2) 합성/구현/비트스트림
vivado -mode batch -source scripts\build.tcl           # -> bitstream\basys3.bit
# 3) 보드 프로그래밍
vivado -mode batch -source scripts\program.tcl
```
체크: WNS≥0 (100MHz), BRAM 사용량 (VGA 프레임버퍼 포함).

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

- [x] M1 툴체인 + Vivado 배치 플로우 (스크립트 준비, 보드 실측 대기)
- [x] M2 코어 부팅 + UART (SW/HDL 준비, 보드 실측 대기)
- [x] #1 GPIO  #2 UART  #3 Timer/IRQ
- [x] #4 SPI  #5 TWI  #6 PWM  #7 7-seg
- [x] #8 VGA  #9 DMA  #10 PS/2
- [ ] #11 ExtMem  #12 USB(선택)

> 위 항목은 **코드/설계 완료(작성)** 상태. Vivado 합성/보드 검증 시 개별 체크 필요.

## 참고
- NEORV32: https://github.com/stnolting/neorv32
- Datasheet/User Guide: `docs/datasheet.pdf`, `docs/userguide.pdf`
