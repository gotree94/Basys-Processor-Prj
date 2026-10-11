# VexRiscv on Basys 3

> **추천도 ★★★★★** — 자유도 최상. Wishbone 주변장치를 원하는 만큼 붙일 수 있음.
> 단, **SpinalHDL 생성(SBT) 때문에 Linux/WSL2 권장**.

SpinalHDL로 기술된 **구성 가능(configurable) RISC-V** 코어. 파이프라인/캐시/버스/디버그를
플러그인으로 조립. LiteX와 연동하면 SoC를 자동 생성할 수 있음.

---

## 0. 프로젝트 구조

```
VexRiscv/
├── README.md
├── hdl/
│   ├── VexRiscv.v          # 생성 산출물 (SpinalHDL -> Verilog)
│   └── basys3_top.v        # 보드 래퍼 (top)
├── gen/                    # SpinalHDL 생성 프로젝트 (SBT)
│   └── src/main/scala/MyVexRiscv.scala
├── sw/
│   ├── main.c
│   ├── link.ld
│   └── Makefile
├── constrs/basys3.xdc
├── sim/tb_basys3.v
├── scripts/{create_project,build,program}.tcl
└── bitstream/
```

---

## 1. 환경설정

### 1.1 WSL2/Ubuntu 권장
```bash
sudo apt update
sudo apt install -y openjdk-17-jdk scala sbt git make python3
# RISC-V toolchain (공통 README 4.3)
```

### 1.2 SpinalHDL 획득 & 생성
```bash
git clone https://github.com/SpinalHDL/VexRiscv
cd VexRiscv
# 미리 정의된 Full config 또는 Briey 예제 사용
sbt "runMain vexriscv.demo.GenFull"
# 산출물: VexRiscv.v (또는 VexRiscv.vhd)
```

### 1.3 커스텀 구성 예제 (`gen/src/main/scala/MyVexRiscv.scala`)

목표: **Wishbone 데이터 버스 + IRQ + RV32IMC**.

```scala
import spinal.core._
import spinal.lib._
import vexriscv._
import vexriscv.plugin._

class MyVexRiscv extends Component {
  val io = new Bundle {
    val jtag = slave(Jtag())                 // 디버그
    val iBus = master(Wishbone(32, 30))      // 명령: BRAM
    val dBus = master(Wishbone(32, 30))      // 데이터: BRAM + 주변장치
    val timerInterrupt = in Bool()
    val externalInterrupt = in Bool()
  }
  val config = VexRiscvConfig(
    plugins = List(
      new IBusCachedPlugin(
        memoryTranslatorPortConfig = null,
        config = PipelinedMemoryBusConfig(30, 32)
      ),
      new DBusCachedPlugin(memoryTranslatorPortConfig = null,
        config = PipelinedMemoryBusConfig(30, 32)),
      new DecoderSimplePlugin,
      new RegFilePlugin(regFileReadyTable = ...),
      new IntAluPlugin,
      new MmuPlugin,                          // 필요시
      new BarrelShifterPlugin,
      new HazardSimplePlugin,
      new MulPlugin, new DivPlugin,           // RV32M
      new CsrPlugin(CsrPluginConfig.smallest),// RV32 Zicsr
      new DebugPlugin(ClockDomain.current.clone()),
      new BranchPlugin(...),
      new YamlPlugin("cpu.yaml")
    )
  )
  val cpu = new VexRiscv(config)
  cpu.setDefinitionName("VexRiscv")
  // bus bridge: PipelinedMemoryBus -> Wishbone
  ...
}
```

> 실제로는 VexRiscv 저장소의 `PipelinedMemoryBusToWishboneBridge` /
> `Briey` 예제를 복사해 포트를 Wishbone으로 맞추는 것이 빠름.

생성 후 `hdl/VexRiscv.v` 복사 → Vivado에서 일반 Verilog로 합성.

---

## 2. 주변장치 로드맵 (Wishbone 슬레이브)

VexRiscv에는 내장 MMIO가 없으므로 **Wishbone 슬레이브를 직접 구현/재사용**.
LiteX의 `wishbone` 코어를 그대로 가져오면 빠름.

| # | 로드맵 | 구현 | 비고 |
|---|---|---|---|
| 1 | GPIO | Wishbone GPIOPeriph | 레퍼런스 많음 |
| 2 | UART | Wishbone UART (litex) | FIFO 포함 |
| 3 | Timer+IRQ | Timer + IRQ arbiter | externalInterrupt 입력 |
| 4 | SPI | Wishbone SPIMaster | |
| 5 | I2C | Wishbone I2C | |
| 6 | PWM | Wishbone PWM | audio |
| 7 | 7-seg | GPIO + 디코더 | |
| 8 | VGA | Framebuffer (Wishbone→BRAM) | 더블버퍼 |
| 9 | DMA | Wishbone DMA | litex dma |
| 10 | PS/2 | Wishbone PS2 | |
| 11 | ExtMem | Wishbone SRAM(외부 Pmod) | |
| 12 | USB | 외부 스택 | |

### 버스/디코딩 (개념)

```
dBus(CPU) ──> Wishbone Interconnect(Decoder)
                  ├── 0x0000_0000  IMEM/DMEM (BRAM)
                  ├── 0xF000_0000  UART
                  ├── 0xF000_1000  GPIO
                  ├── 0xF000_2000  Timer
                  ├── 0xF000_3000  SPI
                  ├── 0xF000_4000  I2C
                  ├── 0xF000_5000  PWM
                  └── 0xF000_6000  VGA/7seg/DMA/PS2
```

---

## 3. 코드 작성

> **로드맵 #1~#10 실제 소스가 `sw/` 에 포함되어 있음.** 빌드/실행/트랩 설계는
> [`sw/README.md`](sw/README.md) 참고. (`crt0.S` + `link.ld` + `app/app_*.c`)

### 3.1 Wishbone GPIO (Verilog, 발췌)

```verilog
module wb_gpio #(parameter ADDR_W=30, DATA_W=32)(
  input  wire clk, rst,
  // Wishbone slave
  input  wire [ADDR_W-1:0] wb_adr,
  input  wire [DATA_W-1:0] wb_dat_w,
  output reg  [DATA_W-1:0] wb_dat_r,
  input  wire wb_we, wb_cyc, wb_stb,
  output reg  wb_ack,
  // board
  input  wire [15:0] gpio_i,     // SW
  output reg  [15:0] gpio_o      // LED
);
  always @(posedge clk) begin
    wb_ack <= 0;
    if (wb_cyc && wb_stb) begin
      if (wb_we) gpio_o <= wb_dat_w[15:0];
      else       wb_dat_r <= {16'b0, gpio_i};
      wb_ack <= 1;
    end
    if (rst) gpio_o <= 0;
  end
endmodule
```

### 3.2 소프트웨어 (`sw/main.c`)

```c
#include <stdint.h>
#define UART_TX   (*(volatile uint32_t*)0xF0000000)
#define UART_STAT (*(volatile uint32_t*)0xF0000004)
#define GPIO_O    (*(volatile uint32_t*)0xF0001000)
#define GPIO_I    (*(volatile uint32_t*)0xF0001004)

static void puts_(const char*s){ while(*s) UART_TX=*s++; }

int main(void){
  puts_("VexRiscv Basys3 boot\n");
  for(;;){ GPIO_O = GPIO_I; }   // SW -> LED
}
```

`link.ld` (메모리 맵):
```
MEMORY { ram (rwx) : ORIGIN = 0x00000000, LENGTH = 32K }
ENTRY(_start)
```
빌드:
```bash
make CROSS=riscv32-unknown-elf-
# elf -> hex/bin -> BRAM init (또는 UART/jtag load)
```

---

## 4. 제약 파일

NEORV32 문서와 동일한 Basys3 핀 매핑 사용 (`constrs/basys3.xdc`).
VexRiscv는 **JTAG 디버그 핀이 추가**됨:
```tcl
# JTAG (Pmod JB 예시)
set_property -dict {PACKAGE_PIN A14 IOSTANDARD LVCMOS33} [get_ports {jtag_tck}]
set_property -dict {PACKAGE_PIN A16 IOSTANDARD LVCMOS33} [get_ports {jtag_tdi}]
set_property -dict {PACKAGE_PIN B15 IOSTANDARD LVCMOS33} [get_ports {jtag_tdo}]
set_property -dict {PACKAGE_PIN B16 IOSTANDARD LVCMOS33} [get_ports {jtag_tms}]
```

---

## 5. 검증

### 5.1 시뮬레이션 (Verilator 권장)
```bash
verilator --cc --exe -Wno-fatal hdl/VexRiscv.v hdl/basys3_top.v sim/tb_basys3.v
make -C obj_dir -f Vtb_basys3.mk
./obj_dir/Vtb_basys3
```
체크: 리셋 해제 후 iBus fetch, UART 첫 바이트.

### 5.2 합성/구현
```powershell
vivado -mode batch -source scripts\build.tcl
```
체크: **WNS** — VexRiscv는 파이프라인/캐시 구성에 따라 100MHz 타이밍이 빡빡할 수 있음.
안 되면 `m1stage`/파이프라인 단수 조정 또는 클럭 분주.

### 5.3 보드 검증 + 디버그
- **JTAG**: `openocd -f vexriscv.cfg` + GDB로 코드 로드/브레이크포인트
  ```bash
  riscv32-unknown-elf-gdb sw/main.elf
  (gdb) target remote :3333
  (gdb) load
  (gdb) c
  ```
- SW→LED, UART echo, 각 주변장치 레지스터 read/write 확인.

---

## 6. 체크리스트

- [ ] M1 WSL + sbt + toolchain, Vivado 배치
- [ ] M2 Wishbone VexRiscv 생성 + 부팅/UART
- [ ] #1 GPIO  #2 UART  #3 Timer/IRQ
- [ ] #4 SPI  #5 I2C  #6 PWM  #7 7-seg
- [ ] #8 VGA  #9 DMA  #10 PS/2
- [ ] #11 ExtMem  #12 USB(선택)
- [ ] JTAG 디버깅 동작

## 참고
- VexRiscv: https://github.com/SpinalHDL/VexRiscv
- LiteX (Wishbone 코어 재사용): https://github.com/enjoy-digital/litex
- Briey SoC 예제: `VexRiscv/src/main/scala/vexriscv/demo/Briey.scala`
