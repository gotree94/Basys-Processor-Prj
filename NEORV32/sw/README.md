# NEORV32 소프트웨어 (Basys3 주변장치 로드맵 #1~#10)

NEORV32 내장 주변장치 드라이버(rte 라이브러리) + 커스텀 HDL 주변장치용 MMIO 드라이버.
UART 메뉴로 데모를 선택 실행하고, `a` 로 자동 스모크 테스트를 돌린다.

---

## 1. 디렉터리 구조

```
sw/
├── Makefile
├── main.c                  # 진입점 + UART 메뉴 디스패처
├── include/
│   └── app.h               # MMIO 맵 + 데모 API 선언
└── app/
    ├── app_util.c          # mtime 지연, PASS/FAIL 리포트
    ├── app_gpio.c          # #1  SW->LED, BTN
    ├── app_uart.c          # #2  FTDI 에코 + 하트비트
    ├── app_timer.c         # #3  MTIME IRQ + LED 점멸
    ├── app_spi.c           # #4  Pmod SPI 루프백
    ├── app_twi.c           # #5  I2C 버스 스캔
    ├── app_pwm.c           # #6  오디오/LED PWM
    ├── app_seg7.c          # #7  7-seg 카운터 (커스텀 HDL)
    ├── app_vga.c           # #8  VGA 컬러바 (커스텀 HDL)
    ├── app_dma.c           # #9  DMA 메모리 복사
    └── app_ps2.c           # #10 PS/2 스캔코드 (커스텀 HDL)
```

---

## 2. 접근 방식

| 구분 | 대상 | 접근 |
|---|---|---|
| 내장 주변장치 | #1 GPIO, #2 UART, #3 MTIME/IRQ, #4 SPI, #5 TWI, #6 PWM, #9 DMA | `neorv32.h` rte 라이브러리 함수 |
| 커스텀 HDL | #7 7-seg, #8 VGA, #10 PS/2 | `app.h` 의 CFS MMIO 매크로 |

> 커스텀 주변장치는 FPGA 래퍼(`hdl/basys3_top.vhd`)의 **CFS** 영역에 매핑된다.
> `app.h` 의 `CFS_BASE(0xFFEB0000)` 와 오프셋은 **`hdl/cfs_periph.vhd`** 와 일치한다.

### CFS 메모리맵 (hdl/cfs_periph.vhd)

| 오프셋 | 레지스터 | RW | 설명 |
|---|---|---|---|
| 0x000 | SEG7_DIG0 | W | 자릿수0 (4bit hex) |
| 0x004 | SEG7_DIG1 | W | 자릿수1 |
| 0x008 | SEG7_DIG2 | W | 자릿수2 |
| 0x00C | SEG7_DIG3 | W | 자릿수3 |
| 0x010 | SEG7_RAW | W | 디코더 우회 (a..g, dp) |
| 0x100 | VGA_CTRL | RW | [0]=enable |
| 0x104 | VGA_STATUS | R | [0]=vblank |
| 0x200 | PS2_DATA | R | [8]=valid [7:0]=scancode (pop) |
| 0x204 | PS2_CTRL | RW | [0]=irq_en |
| 0x500~0x14FC | VGA_FB | W | 프레임버퍼 4800 words (160x120 8bpp, 4px/word) |

### GPIO 핀맵 (hdl/basys3_top.vhd)

| GPIO 비트 | 신호 | 방향(dir) |
|---|---|---|
| 15:0 | SW (입력) &nbsp;/&nbsp; LED (출력) | 초기화: `0x0000FFFF` → 15:0 out |
| 19:16 | BTN0..BTN3 | 0 (in) |

---

## 3. 빌드

```bash
# NEORV32 저장소 클론 후
export NEORV32_HOME=/path/to/neorv32
riscv32-unknown-elf-gcc --version     # rv32imc/ilp32

cd sw
make NEORV32_HOME=$NEORV32_HOME       # -> firmware.bin / firmware.hex
make size                             # 코드/RAM 사용량
```

산출물:
- `firmware.elf` — GDB 디버그용 (심볼)
- `firmware.bin` — UART 부트로더 업로드용
- `firmware.hex` — BRAM `$readmemh` 초기화용

### 로드 방법
1. **UART 부트로더 (권장, 재빌드 불필요)**
   - 보드 리셋 → NEORV32 부트로더가 UART 입력 대기
   - NEORV32 저장소의 `sw/image_gen/image_gen -i firmware.elf` 로 `.bin` 생성 후 업로드
2. **BRAM init (고정 펌웨어)**
   - `firmware.hex` 를 FPGA 래퍼의 IMEM `$readmemh` 로 포함 → synthesize

---

## 4. 사용 (UART 19200 8N1)

```
NEORV32 on Basys3 boot OK
UID=0x........ CLK=100000000 Hz

=== NEORV32 Basys3 Peripheral Roadmap ===
  [1] GPIO (SW/LED/BTN)
  [2] UART (echo)
  [3] Timer + IRQ
  [4] SPI (Pmod loopback)
  [5] TWI (I2C bus scan)
  [6] PWM (audio/LED)
  [7] 7-seg (counter)
  [8] VGA (color bars)
  [9] DMA (mem copy)
  [0] PS/2 (keyboard)
  [a] run ALL smoke test
  [m] menu
Select>
```

| 키 | 데모 | 종료 버튼 | 기대 결과 |
|---|---|---|---|
| 1 | GPIO | BTN0 | SW 상태가 LED에 반영 |
| 2 | UART | 'q' | 문자 에코 + 1초 하트비트 |
| 3 | Timer/IRQ | BTN1 | LED0 2Hz 점멸, IRQ 카운트 출력 |
| 4 | SPI | 자동 | MOSI-MISO 점퍼 시 `mismatch=0` (PASS) |
| 5 | TWI | 자동 | 응답 I2C 주소 목록 출력 |
| 6 | PWM | BTN2 | LED 페이드 + 오디오 톤 |
| 7 | 7-seg | BTN3 | 0000~FFFF 카운트 |
| 8 | VGA | BTN0 | 화면에 8색 세로 바 |
| 9 | DMA | 자동 | `errors=0` (PASS) |
| 0 | PS/2 | BTN0 | 키 스캔코드 UART 출력 |

---

## 5. 시뮬레이션 검증

NEORV32 는 자체 시뮬레이터(`sim/`)를 제공. 소프트웨어 로직 검증:
```bash
# NEORV32 저장소에서
make -C sim                       # ghdl + sim 빌드
# 이 앱의 firmware.elf 를 IMEM 에 로드(neorv32_tb 의 IMEM_GENERIC_FILE) 후 실행
```
무한루프 데모(#1~#10)는 대화형이므로, 자동 스모크(`a`) 위주로 시뮬 검증 권장.

---

## 6. 버전 의존 주의

- **빌드 레이아웃**: "main"(2026) 기준 라이브러리는 `sw/lib/source/*.c`,
  헤더는 `sw/lib/include`. 구버전은 `sw/common/neorv32.c` 단일 파일(Makefile 이 자동 인식).
- `neorv32.h` 함수명은 릴리스마다 바뀔 수 있음(예: `neorv32_gpio_port_dir_set`,
  `neorv32_mtime_*`, `neorv32_dma_*`). **반드시 사용 중인 NEORV32 버전의 헤더를 확인**할 것.
- MTIME 은 CLINT 가 제공(`IO_CLINT_EN=true`), 주소는 버전별로 이동(구버전 0xFFFFC000,
  현재 0xFFF40000)하므로 **직접 주소를 쓰지 말고 라이브러리 함수로 접근**할 것.
- DMA(#9): 컨트롤러가 없는 구성이면 `app_dma.c` 의 소프트웨어 폴백 경로가 동작.
- `F_CPU` 는 NEORV32 라이브러리가 `CLOCK_FREQUENCY` 제네릭(100 MHz)에서 정의.
