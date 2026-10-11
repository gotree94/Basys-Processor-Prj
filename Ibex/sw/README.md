# Ibex 소프트웨어 (Basys3 주변장치 로드맵 #1~#10)

Ibex 는 AXI4-Lite(req/gnt) 시스템이므로 주변장치는 대부분 **Xilinx AXI IP**(GPIO/UART-Lite/
Timer/IIC/Quad-SPI/DMA)로 붙인다. IRQ 는 표준 RISC-V CSR — Ibex 는 `irq_timer_i` 를
내장하므로 타이머는 **mcause 7**, 나머지는 **mcause 11(external)**.

---

## 1. 디렉터리 구조

```
sw/
├── Makefile
├── link.ld                 # 0x0 부터, RAM 32KB
├── crt0.S                  # _start + trap_entry(mtvec) + IRQ 헬퍼
├── main.c                  # UART 메뉴 디스패처
├── include/app.h           # MMIO 주소맵 + API
└── app/
    ├── app_util.c          # UART 저수준 + 미니 printf + rdcycle/지연
    ├── app_gpio.c  app_uart.c  app_timer.c(트랩 핸들러)  app_spi.c
    ├── app_twi.c   app_pwm.c   app_seg7.c
    └── app_vga.c   app_dma.c   app_ps2.c
```

---

## 2. 주소맵 (인터커넥트 HDL 과 일치시킬 것)

| 영역 | 주소 | IRQ |
|---|---|---|
| RAM (IMEM/DMEM) | `0x0000_0000`–`0x0000_7FFF` | – |
| GPIO | `0x1000_0000` | – |
| UART | `0x1001_0000` | – |
| TIMER | `0x1002_0000` | `irq_timer` → mcause 7 |
| SPI | `0x1003_0000` | – |
| I2C | `0x1004_0000` | – |
| PWM | `0x1005_0000` | – |
| SEG7 | `0x1006_0000` | – |
| VGA | `0x1007_0000` | – |
| DMA | `0x1008_0000` | `irq_external` → mcause 11 |
| PS2 | `0x1009_0000` | `irq_external` → mcause 11 |

---

## 3. 트랩 / 인터럽트

- `crt0.S`: `mtvec = trap_entry` (direct). 컨텍스트 저장 → `trap_handler(cause, epc)` →
  반환 mepc 복원 → `mret`
- 활성화: `mie.MTIE(bit7)|MEIE(bit11)` + `mstatus.MIE(bit3)` → `trap_irq_enable()`
- 타이머는 Ibex 내장 `irq_timer_i` → mcause **7**
- DMA/PS2 등은 `irq_external_i` → mcause **11**

```c
uint32_t trap_handler(uint32_t cause, uint32_t epc) {
  if (cause & 0x80000000u) {
    uint32_t code = cause & 0x7FFFFFFFu;
    if (code == IRQ_M_TIMER)    { TIMER_FLAG = 1; /* ack */ ... }
    else if (code == IRQ_M_EXTERNAL) { if (DMA_STAT&2) DMA_STAT=2; }
  }
  return epc;
}
```

> Ibex 인스턴스에서 `irq_timer_i` 를 타이머 IP 에, `irq_external_i` 를 DMA/PS2 OR 에 연결.

---

## 4. 빌드

```bash
riscv32-unknown-elf-gcc --version     # rv32imc/ilp32
cd sw
make                                   # -> firmware.elf / .bin / .hex
make size
```
newlib 미사용(`-ffreestanding -nostdlib`) → `app_util.c` 에 `mem*`/미니 `printf`.

로드:
- `firmware.hex` → IMEM `$readmemh`
- `firmware.bin` → JTAG(openocd) 또는 부트 업로드

---

## 5. 사용 (UART, HDL baud 에 맞춤)

```
Ibex on Basys3 boot OK

=== Ibex Basys3 Peripheral Roadmap ===
  [1] GPIO (SW/LED/BTN)
  ...
  [0] PS/2 (keyboard)
  [a] run ALL smoke test
  [m] menu
Select>
```

| 키 | 데모 | 종료 | 기대 |
|---|---|---|---|
| 1 | GPIO | BTN0 | SW→LED |
| 2 | UART | 'q' | 에코 + 하트비트 |
| 3 | Timer/IRQ | BTN1 | LED0 2Hz, 카운트 |
| 4 | SPI | 자동 | `mismatch=0` |
| 5 | I2C | 자동 | 주소 목록 |
| 6 | PWM | BTN2 | 페이드/톤 |
| 7 | 7-seg | BTN3 | 0000~FFFF |
| 8 | VGA | BTN4 | 컬러바 |
| 9 | DMA | 자동 | `errors=0` |
| 0 | PS/2 | BTN0 | 스캔코드 |

---

## 6. 시뮬레이션 (FuseSoC / Verilator)

Ibex 공식 `simple_system` 으로 소프트웨어 동작 검증 가능:
```bash
pip install fusesoc
fusesoc --cores-root . run --target=sim lowrisc:ibex:ibex_simple_system
```
체크: `mem_valid/ready`(또는 AXI) 핸드셰이크, 첫 UART 바이트, 타이머 IRQ 진입.

---

## 7. 주의

- Ibex + AXI IP 구성은 **35T 에서 가장 무거움** → BRAM/LUT 여유 확인.
- AXI4-Lite 인터커넥트와 Ibex req/gnt→AXI 어댑터는 **별도 HDL 작업**(README §3.1).
- 주소맵/IRQ 연결은 **인터커넥트 HDL과 반드시 일치**.
