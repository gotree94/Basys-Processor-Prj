# PicoRV32 on Basys 3

> **추천도 ★★★★☆** — 순수 Verilog, Windows 단독 가능, 학습 친화적.
> 다만 **메모리/버스/주변장치를 직접 만들어야 함** (자동화 없음).

YosysHQ의 소형 RV32 코어. `ENABLE_IRQ`, `ENABLE_MUL/DIV`, `ENABLE_COMPRESSED` 등
파라미터로 기능 선택. **Native memory interface**(IMEM/DMEM) + (선택) `PCPI`/`IRQ`.

---

## 0. 프로젝트 구조

```
PicoRV32/
├── README.md
├── hdl/
│   ├── picorv32.v          # 업스트림 코어
│   ├── basys3_top.v        # top: CPU + BRAM + 주변장치
│   ├── wb_interconnect.v   # 주소 디코더
│   ├── gpio.v  uart.v  timer.v  spi.v  pwm.v  vga.v  ps2.v
├── sw/
│   ├── start.S  main.c  link.ld  Makefile
├── constrs/basys3.xdc
├── sim/tb_basys3.v
├── scripts/{create_project,build,program}.tcl
└── bitstream/
```

---

## 1. 환경설정

### 1.1 준비물
- RISC-V 툴체인 (공통 README 4.3)
- (선택) iverilog/Verilator: 시뮬
- Python3: BRAM init hex 생성

### 1.2 코어 획득
```bash
git clone https://github.com/YosysHQ/picorv32
# hdl/picorv32.v 복사
```

### 1.3 파라미터 (목표: RV32IMC + IRQ)
```verilog
picorv32 #(
  .ENABLE_MUL(1),         // RV32M
  .ENABLE_DIV(1),
  .ENABLE_IRQ(1),         // ✅ 인터럽트
  .ENABLE_COMPRESSED(1),  // RV32C
  .PROGADDR_RESET(32'h0000_0000),
  .STACKADDR(32'h0000_8000),
  .BARREL_SHIFTER(1)
) cpu (...);
```

### 1.4 메모리 모델 (핵심)
- **IMEM/DMEM 공유 단일 BRAM** (Harvard가 아니라 **Native 메모리 인터페이스**가
  `mem_valid/mem_ready/mem_addr/mem_wdata/mem_wstrb/mem_rdata`로 나옴)
- 주소 디코더(`wb_interconnect.v`)에서:
  - `0x0000_0000~0x0000_7FFF` → BRAM (32KB)
  - `0xF000_0000~` → MMIO 주변장치

---

## 2. 주변장치 로드맵 (직접 구현)

PicoRV32는 버스가 없으므로 **Native mem bus → 주소 디코더 → 각 장치** 구조.
IRQ는 `irq[31:0]` 벡터로 입력.

| # | 로드맵 | 구현 모듈 | IRQ |
|---|---|---|---|
| 1 | GPIO | `gpio.v` | – |
| 2 | UART | `uart.v` (simple, FIFO) | 선택 |
| 3 | Timer+IRQ | `timer.v` → `irq[0]` | ✅ |
| 4 | SPI | `spi.v` | – |
| 5 | I2C | `i2c.v` | – |
| 6 | PWM | `pwm.v` | – |
| 7 | 7-seg | GPIO + 디코더 | – |
| 8 | VGA | `vga.v` + framebuffer BRAM | vblank IRQ |
| 9 | DMA | `dma.v` (native bus master) | 완료 IRQ |
| 10 | PS/2 | `ps2.v` | 선택 |
| 11 | ExtMem | 외부 SRAM bus | – |
| 12 | USB | 외부 스택 | – |

### 주소 디코더 개념
```verilog
// mem bus -> 장치 select
localparam BASE_GPIO  = 32'hF000_0000;
localparam BASE_UART  = 32'hF000_0100;
localparam BASE_TIMER = 32'hF000_0200;
// ...
wire sel_gpio  = (mem_addr[31:8] == BASE_GPIO [31:8]);
wire sel_uart  = (mem_addr[31:8] == BASE_UART [31:8]);
// mux mem_rdata, gating mem_ready
```

### IRQ 배선
```verilog
wire [31:0] irq;
assign irq[0] = timer_irq;   // timer
assign irq[1] = uart_rx_irq;
assign irq[2] = dma_done_irq;
// cpu.irq <= irq;
```

---

## 3. 코드 작성

> **로드맵 #1~#10 실제 소스가 `sw/` 에 포함되어 있음.** 빌드/실행/IRQ 설계는
> [`sw/README.md`](sw/README.md) 참고. (`start.S` + `link.ld` + `app/app_*.c`)

### 3.1 스타트업 (`sw/start.S`)
```asm
.section .text.start
.globl _start
_start:
  la sp, _stack_top
  la t0, trap_handler
  csrw mtvec, t0
  call main
  j .

trap_handler:
  # irq 원인 읽고 처리 (picorv32: q0..q3 레지스터)
  ...
  mret
```

### 3.2 `main.c`
```c
#include <stdint.h>
#define GPIO_I  (*(volatile uint32_t*)0xF0000000)
#define GPIO_O  (*(volatile uint32_t*)0xF0000004)
#define UART    (*(volatile uint32_t*)0xF0000100)

int main(void){
  const char *s="PicoRV32 Basys3 boot\n";
  while(*s) UART=*s++;
  for(;;) GPIO_O = GPIO_I;   // SW->LED
}
```

### 3.3 링커 (`link.ld`)
```
MEMORY { ram (rwx) : ORIGIN = 0x00000000, LENGTH = 32K }
SECTIONS {
  .text : { *(.text.start) *(.text*) } > ram
  .rodata : { *(.rodata*) } > ram
  .data : { *(.data*) } > ram
  .bss  : { *(.bss*)  *(.sbss*) } > ram
  _stack_top = 0x00008000;
}
```

### 3.4 BRAM 초기화
```bash
# elf -> bin -> hex (readmemh용)
riscv32-unknown-elf-objcopy -O binary main.elf main.bin
python3 scripts/bin2hex.py main.bin > hdl/firmware.hex
```
`basys3_top.v`의 BRAM:
```verilog
initial $readmemh("firmware.hex", imem);
```

---

## 4. 제약 파일

공통 Basys3 핀 매핑(`constrs/basys3.xdc`). PicoRV32는 JTAG가 없어
**UART 기반 부트로더 또는 BRAM init** 을 사용.

---

## 5. 검증

### 5.1 시뮬레이션 (iverilog)
```bash
iverilog -o sim/tb hdl/*.v sim/tb_basys3.v
vvp sim/tb
```
체크: `mem_valid/ready` 핸드셰이크 정상, 첫 UART 바이트, IRQ 진입.

### 5.2 합성/구현
```powershell
vivado -mode batch -source scripts\build.tcl
```
체크: 100MHz WNS. Native 버스는 단순해 타이밍 여유 큼.

### 5.3 보드 검증
1. UART "PicoRV32 Basys3 boot"
2. SW→LED
3. Timer IRQ LED 점멸
4. Pmod SPI/I2C 루프백
5. PWM 오디오 톤, 7-seg, VGA, DMA, PS/2

---

## 6. 체크리스트

- [ ] M1 toolchain + BRAM init 파이프라인
- [ ] M2 코어 부팅 + UART
- [ ] #1 GPIO  #2 UART  #3 Timer/IRQ
- [ ] #4 SPI  #5 I2C  #6 PWM  #7 7-seg
- [ ] #8 VGA  #9 DMA  #10 PS/2
- [ ] #11 ExtMem  #12 USB(선택)

## 참고
- PicoRV32: https://github.com/YosysHQ/picorv32
- `picosoc` (SPI 플래시 부팅 예제), `firmware/` 스타트업 코드
