# VexRiscv 소프트웨어 (Basys3 주변장치 로드맵 #1~#10)

VexRiscv 는 Wishbone 버스를 노출하므로 주변장치는 **Wishbone 슬레이브 MMIO**로 접근한다.
IRQ 는 PicoRV32 와 달리 **표준 RISC-V CSR/trap (mtvec/mcause/mepc)** 을 사용.

---

## 1. 디렉터리 구조

```
sw/
├── Makefile
├── link.ld                 # 0x0 부터, RAM 32KB, 스택 0x8000
├── crt0.S                  # _start + trap_entry(mtvec) + IRQ 헬퍼
├── main.c                  # UART 메뉴 디스패처
├── include/app.h           # MMIO 주소맵 + API
└── app/
    ├── app_util.c          # UART 저수준 + 미니 printf + rdcycle/지연
    ├── app_gpio.c          # #1
    ├── app_uart.c          # #2
    ├── app_timer.c         # #3  + C trap_handler() 포함
    ├── app_spi.c           # #4
    ├── app_twi.c           # #5
    ├── app_pwm.c           # #6
    ├── app_seg7.c          # #7
    ├── app_vga.c           # #8
    ├── app_dma.c           # #9
    └── app_ps2.c           # #10
```

---

## 2. 주소맵 (Wishbone 인터커넥트와 일치시킬 것)

| 영역 | 주소 |
|---|---|
| RAM (IMEM/DMEM) | `0x0000_0000`–`0x0000_7FFF` (32KB) |
| GPIO | `0xF000_0000` |
| UART | `0xF000_1000` |
| TIMER | `0xF000_2000` → external interrupt |
| SPI | `0xF000_3000` |
| I2C | `0xF000_4000` |
| PWM | `0xF000_5000` |
| SEG7 | `0xF000_6000` |
| VGA | `0xF000_7000` (FB +0x400) |
| DMA | `0xF000_8000` |
| PS2 | `0xF000_9000` |

---

## 3. 트랩 / 인터럽트

- `crt0.S` 가 `mtvec = trap_entry` 설정 (direct mode)
- `trap_entry`: caller-saved(a0-a7,t0-t6,ra) 저장 → `trap_handler(cause, epc)` 호출 →
  반환 mepc 복원 → `mret`
- 핸들러(`app_timer.c`)는 `mcause` 상위비트로 인터럽트 판별, code 11 = machine external
- 활성화: `mie.MEIE(bit11)` + `mstatus.MIE(bit3)` → `trap_irq_enable()`
- 여러 주변장치가 하나의 external 라인을 OR → 핸들러가 각 주변장치 플래그를 폴링

```c
uint32_t trap_handler(uint32_t cause, uint32_t epc) {
  if (cause & 0x80000000u && (cause & 0x7FFFFFFFu) == 11) {
    if (TIMER_FLAG & 1) { TIMER_FLAG = 1; /* ack */ ... }
    if (DMA_STAT & 2)   { DMA_STAT = 2; ... }
  }
  return epc;
}
```

> VexRiscv 구성 생성 시 **CsrPlugin**(mtvec/mcause/mepc/mie/mstatus, externalInterrupt)와
> `externalInterrupt` 입력이 Wishbone 인터커넥트의 IRQ 오라인에 연결되어 있어야 함.

---

## 4. 빌드

```bash
riscv32-unknown-elf-gcc --version     # rv32imc/ilp32
cd sw
make                                   # -> firmware.elf / .bin / .hex
make size
```
newlib 미사용(`-ffreestanding -nostdlib`) → `app_util.c` 에 `mem*`/미니 `printf` 자체 구현.

로드:
- `firmware.hex` → HDL 의 IMEM `$readmemh` (고정 펌웨어)
- `firmware.bin` → JTAG(openocd) 또는 UART 부트 업로드

---

## 5. 사용 (UART, HDL 의 baud 에 맞춤)

```
VexRiscv on Basys3 boot OK

=== VexRiscv Basys3 Peripheral Roadmap ===
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

## 6. JTAG 디버그 (선택)

VexRiscv DebugPlugin 사용 시:
```bash
openocd -f vexriscv.cfg &
riscv32-unknown-elf-gdb firmware.elf
(gdb) target remote :3333
(gdb) load
(gdb) c
```

---

## 7. 주의

- IRQ 데모는 VexRiscv 구성에 **CsrPlugin + externalInterrupt** 필요.
- `rdcycle` 은 `ENABLE_COUNTERS`/CSR 구성에 따라 동작. 미지원이면 `rdcycle` 을
  소프트웨어 루프로 대체.
- 주소맵/IRQ 배선은 **Wishbone 인터커넥트 HDL과 반드시 일치**.
