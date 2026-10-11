# PicoRV32 소프트웨어 (Basys3 주변장치 로드맵 #1~#10)

PicoRV32 는 내장 주변장치가 없으므로 **모든 드라이버를 MMIO 로 직접 작성**한다.
부팅/IRQ 는 PicoRV32 전용 커스텀 명령어(q-regs)로 처리.

---

## 1. 디렉터리 구조

```
sw/
├── Makefile
├── link.ld                 # 0x0 부터 배치, RAM 32KB
├── start.S                 # reset_vec(0x0) + irq_vec(0x10) + 부팅 + IRQ 헬퍼
├── custom_ops.S            # PicoRV32 IRQ 커스텀 명령어 매크로 (upstream, PD)
├── main.c                  # UART 메뉴 디스패처
├── include/app.h           # MMIO 주소맵 + API
└── app/
    ├── app_util.c          # UART 저수준 + 미니 printf + rdcycle/지연
    ├── app_gpio.c          # #1
    ├── app_uart.c          # #2
    ├── app_timer.c         # #3  C irq() 핸들러 포함
    ├── app_spi.c           # #4
    ├── app_twi.c           # #5
    ├── app_pwm.c           # #6
    ├── app_seg7.c          # #7
    ├── app_vga.c           # #8
    ├── app_dma.c           # #9
    └── app_ps2.c           # #10
```

---

## 2. 메모리 / 주소맵 (HDL 과 일치시킬 것)

| 영역 | 주소 | 비고 |
|---|---|---|
| RAM (IMEM/DMEM) | `0x0000_0000`–`0x0000_7FFF` | 32 KB, 스택 상단 `0x8000` |
| reset_vec | `0x0000_0000` | `PROGADDR_RESET` |
| irq_vec | `0x0000_0010` | `PROGADDR_IRQ` (`.balign 16`) |
| GPIO | `0xF000_0000` | SW/LED/BTN |
| UART | `0xF000_0100` | FTDI |
| TIMER | `0xF000_0200` | → `irq[3]` |
| SPI | `0xF000_0300` | Pmod |
| I2C | `0xF000_0400` | Pmod |
| PWM | `0xF000_0500` | 오디오/LED |
| SEG7 | `0xF000_0600` | 커스텀 HDL |
| VGA | `0xF000_0700` | 커스텀 HDL (FB +0x400) |
| DMA | `0xF000_0800` | |
| PS2 | `0xF000_0900` | |

> 주소 디코딩/`mem_ready` 게이팅은 FPGA 래퍼(`hdl/wb_interconnect.v`)가 담당.

---

## 3. IRQ 설계 (중요)

PicoRV32 의 IRQ 는 표준 CSR 이 아니라 **커스텀 명령어** 기반이다.

- 진입 시 `q0` = 복귀주소, `q1` = 처리할 IRQ 비트마스크
- `maskirq`: 마스크 레지스터에 기록 (**비트=1 → 비활성**)
- `retirq`: 복귀 + 인터럽트 재허용

`start.S` 가 래퍼(레지스터 저장/복원)를 제공하고, C 핸들러는:

```c
uint32_t *irq(uint32_t *regs, uint32_t irqs) {
    if (irqs & (1u << IRQ_TIMER)) { ... TIMER_FLAG = 1; }  // ack
    return regs;
}
```

IRQ 활성화(예: 타이머만):
```c
picorv32_enable_irqs(~(1u << IRQ_TIMER));
```

### 리셋 벡터 주의
- upstream `start.S` 의 reset_vec 는 `waitirq` 로 시작하지만, 이는 **테스트벤치가
  주기적으로 넣는 IRQ 펄스**(`irq[4]=&cycle[12:0]`)에 의존한다.
- 실 FPGA 에서 IRQ 소스가 없으면 리셋 직후 **블록**되므로, 이 프로젝트의
  `start.S` 는 reset_vec 를 `j _start` 로 단순화했다. (`irq_vec` 는 `0x10` 유지)

---

## 4. 빌드

```bash
riscv32-unknown-elf-gcc --version      # rv32imc/ilp32
cd sw
make                                    # -> firmware.elf / .bin / .hex
make size                               # 코드/RAM 사용량
```

- `-ffreestanding -nostdlib` 로 newlib 미사용 → `app_util.c` 에 `memcpy/memset/memmove`
  및 미니 `printf` 자체 구현.
- 산출물 로드:
  - `firmware.hex` → FPGA 래퍼 IMEM `$readmemh` (고정 펌웨어)
  - `firmware.bin` → UART 부트로더(있다면) 업로드

---

## 5. 사용 (UART 115200 8N1 기준, HDL 에 맞게 조정)

```
PicoRV32 on Basys3 boot OK
start cycle = 0

=== PicoRV32 Basys3 Peripheral Roadmap ===
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
| 2 | UART | 'q' | 에코 + 1초 하트비트 |
| 3 | Timer/IRQ | BTN1 | LED0 2Hz 점멸, 카운트 출력 |
| 4 | SPI | 자동 | 점퍼 시 `mismatch=0` |
| 5 | I2C | 자동 | 응답 주소 목록 |
| 6 | PWM | BTN2 | LED 페이드/톤 |
| 7 | 7-seg | BTN3 | 0000~FFFF |
| 8 | VGA | BTN4 | 8색 컬러바 |
| 9 | DMA | 자동 | `errors=0` |
| 0 | PS/2 | BTN0 | 스캔코드 출력 |

---

## 6. 시뮬레이션 검증

```bash
# PicoRV32 testbench 에 firmware.hex 로드 (IMEM 초기화)
# iverilog 로 hdl/*.v + testbench 컴파일 후 실행
iverilog -o sim tb.v hdl/*.v
vvp sim
```
체크 포인트: `mem_valid/mem_ready` 핸드셰이크, 첫 UART 바이트, 타이머 IRQ 진입.

---

## 7. 주의 / 버전

- `rdcycle` 은 PicoRV32 `ENABLE_COUNTERS=1` 에서만 동작(기본 1).
- `ENABLE_IRQ=1`, `ENABLE_IRQ_QREGS=1` 필요(IRQ 데모).
- `ENABLE_MUL=1`, `ENABLE_DIV=1` (gcc rv32imc 코드가 mul/div 사용).
- 주소맵/IRQ 비트는 **HDL 디코더와 반드시 일치**시킬 것.
