# Ibex on Basys 3

> **추천도 ★★★☆☆** — 검증(리그레션)이 매우 우수하고 구조가 깔끔.
> 단, **AXI4-Lite 인터커넥트 래핑 작업이 필요**해 이식량이 가장 많음.

lowRISC의 RV32IMC 코어. 프론트엔드가 **Instruction Fetch (IF) / ID/EX / WB** 2단 파이프라인,
**AXI4-Lite** 명령/데이터 인터페이스(또는 단순 `req/rsp`). 검증 환경(SV/UVM)이 강점.

---

## 0. 프로젝트 구조

```
Ibex/
├── README.md
├── hdl/
│   ├── ibex/               # 업스트림 RTL (lowRISC/ibex)
│   ├── basys3_top.sv       # top wrapper
│   ├── axi_lite_interconnect.sv
│   ├── axi_lite_gpio.sv  axi_lite_uart.sv  axi_lite_timer.sv ...
├── sw/{main.c, link.ld, Makefile}
├── constrs/basys3.xdc
├── sim/tb_basys3.sv
├── scripts/{create_project,build,program}.tcl
└── bitstream/
```

---

## 1. 환경설정

### 1.1 준비물
- Vivado 2022.2 (SystemVerilog 지원 충분)
- RISC-V 툴체인 (공통 README 4.3)
- (선택) FuseSoC: Ibex 공식 빌드/시뮬 시스템

### 1.2 Ibex 획득
```bash
git clone https://github.com/lowRISC/ibex
# RTL: ibex/rtl/  (ibex_core.sv, ibex_top.sv, ibex_pkg.sv, ibex_register_file_*)
# 정적 패키지: ibex/rtl/ibex_pkg.sv 먼저 컴파일
```

### 1.3 파라미터 (목표: RV32IMC)
```systemverilog
ibex_top #(
  .RV32M(ibex_pkg::RV32MSingleCycle),
  .RV32B(ibex_pkg::RV32BNone),
  .RV32ZC(ibex_pkg::RV32ZcaNone? ),        // C 확장
  .RegFile(ibex_pkg::RegFileFF),
  .DmHaltAddr(...), .DmExceptionAddr(...),
  .WritebackStage(1)                       // 3단 파이프라인
) u_ibex (
  .clk_i, .rst_ni,
  .hart_id_i(0),
  .boot_addr_i(32'h0000_0000),
  .instr_req_o, .instr_gnt_i, .instr_rvalid_i, .instr_addr_o,
  .instr_rdata_i, .instr_rdata_intg_i,
  .data_req_o,  .data_gnt_i,  .data_rvalid_i,  .data_we_o,
  .data_be_o,   .data_addr_o, .data_wdata_o,   .data_rdata_i,
  .irq_software_i, .irq_timer_i, .irq_external_i, .irq_fast_i,
  .irq_nm_i,
  .debug_req_i, .crash_dump_o, ...
);
```

> Ibex는 `instr_*`/`data_*` **요청-그랜트(req/gnt)** 프로토콜을 노출 →
> 여기서 AXI4-Lite(또는 자체 버스)로 변환하는 **어댑터를 직접 작성**해야 함.
> 공식 예제: `examples/simple_system` (SIMD bus + AXI 유틸 제공).

---

## 2. 주변장치 로드맵 (AXI4-Lite)

| # | 로드맵 | 구현 | 비고 |
|---|---|---|---|
| 1 | GPIO | `axi_lite_gpio` | |
| 2 | UART | `axi_lite_uart` (Xilinx UART-Lite IP) | Vivado IP 사용 가능 |
| 3 | Timer+IRQ | `axi_timer` (Xilinx) + IRQ | |
| 4 | SPI | `axi_quad_spi`(Xilinx) / 커스텀 | Pmod |
| 5 | I2C | `axi_iic` (Xilinx) | Pmod |
| 6 | PWM | 커스텀 `axi_lite_pwm` | |
| 7 | 7-seg | GPIO + 디코더 | |
| 8 | VGA | AXI 버스트 → framebuffer BRAM | |
| 9 | DMA | `axi_dma` (Xilinx) | |
| 10 | PS/2 | 커스텀 `axi_lite_ps2` | |
| 11 | ExtMem | `axi_bram_ctrl`/외부 SRAM | |
| 12 | USB | 외부 스택 | |

> **장점:** Xilinx AXI IP(UART-Lite, Timer, IIC, Quad-SPI, DMA, BRAM Ctrl)를
> Vivado IP Catalog에서 바로 붙일 수 있음 → IP 통합은 오히려 쉬움.
> **단점:** AXI4-Lite 크로스바/interconnect 래핑과 Ibex req/gnt → AXI 변환을 직접 구현.

### 버스 구조
```
Ibex (req/gnt) ── ibex_axi_adapter ──> AXI4-Lite Interconnect
        ├── 0x0000_0000  BRAM Ctrl (IMEM/DMEM)
        ├── 0x1000_0000  AXI GPIO
        ├── 0x1001_0000  AXI UART-Lite
        ├── 0x1002_0000  AXI Timer    --> irq_timer
        ├── 0x1003_0000  AXI Quad-SPI (Pmod)
        ├── 0x1004_0000  AXI IIC      (Pmod)
        ├── 0x1005_0000  AXI DMA
        └── 0x1006_0000  커스텀 PWM/7seg/VGA/PS2
```

---

## 3. 코드 작성

### 3.1 req/gnt → AXI4-Lite 어댑터 (개념)
```systemverilog
// instr 채널: 1비트 gnt -> AXI ARVALID, rvalid -> grant
always_ff @(posedge clk) begin
  if (instr_req) begin
    a_araddr  <= instr_addr;
    a_arvalid <= 1'b1;
  end else a_arvalid <= 1'b0;
  instr_gnt <= a_arvalid && a_arready;
end
assign instr_rvalid = a_rvalid;
assign instr_rdata  = a_rdata;
```
(공식 `ibex_axi` / `simple_system`의 `simd` 어댑터 참고 권장.)

### 3.2 소프트웨어 (`sw/main.c`)
```c
#include <stdint.h>
#define GPIO_I  (*(volatile uint32_t*)0x10000000)
#define GPIO_O  (*(volatile uint32_t*)0x10000004)
#define UART    (*(volatile uint32_t*)0x10010000)
#define UART_TX (*(volatile uint32_t*)(UART+0x04))

int main(void){
  const char*s="Ibex Basys3 boot\n";
  while(*s) UART_TX=*s++;
  for(;;) GPIO_O = GPIO_I;
}
```

### 3.3 링커 (`link.ld`)
```
MEMORY { ram (rwx) : ORIGIN = 0x00000000, LENGTH = 32K }
ENTRY(_start)
```
빌드:
```bash
make CROSS=riscv32-unknown-elf-  # elf -> hex (BRAM init) 또는 JTAG load
```

---

## 4. 제약 파일

공통 Basys3 핀 매핑(`constrs/basys3.xdc`).
Ibex JTAG 디버그(`debug_req_i`, dm) 사용 시 Pmod JB에 JTAG 배선 추가.
(Primary: UART 부트 또는 BRAM init 사용 권장.)

---

## 5. 검증

### 5.1 시뮬레이션 (Verilator / FuseSoC)
```bash
pip install fusesoc
fusesoc --cores-root . run --target=sim lowrisc:ibex:ibex_simple_system
```
체크: `ibex_simple_system` 바이너리로 `hello` 출력, req/gnt 프로토콜 정합.

### 5.2 합성/구현
```powershell
vivado -mode batch -source scripts\build.tcl
```
주의: Ibex + AXI IP 로직이 커서 **35T에서 LUT/BRAM 사용량이 가장 큼**.
VGA 프레임버퍼까지 넣으면 빠듯 → BRAM 배분·IP 수 감축 필요.

### 5.3 보드 검증
1. UART "Ibex Basys3 boot"
2. SW→LED (AXI GPIO)
3. AXI Timer IRQ → `irq_timer`
4. AXI Quad-SPI/IIC Pmod 루프백
5. AXI DMA 메모리 복사
6. VGA/PS2 (커스텀 IP)

---

## 6. 체크리스트

- [ ] M1 toolchain + FuseSoC(선택) + Vivado 배치
- [ ] M2 req/gnt→AXI 어댑터 + 부팅/UART
- [ ] #1 GPIO  #2 UART  #3 Timer/IRQ
- [ ] #4 SPI  #5 I2C  #6 PWM  #7 7-seg
- [ ] #8 VGA  #9 DMA  #10 PS/2
- [ ] #11 ExtMem  #12 USB(선택)
- [ ] 리소스(35T) 여유 확인

## 참고
- Ibex: https://github.com/lowRISC/ibex
- simple_system / ibex_axi 유틸: `ibex/examples/`, `ibex/vendor/lowrisc_ip`
- Xilinx AXI IP: Vivado IP Catalog (AXI GPIO/UART-Lite/Timer/IIC/Quad-SPI/DMA)
