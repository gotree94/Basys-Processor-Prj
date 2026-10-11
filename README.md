# Basys-Processor-Prj

Basys 3 (Artix-7 **XC7A35T-1CPG236C**) 보드에 **RISC-V 소프트코어**를 올리고,
보드의 주변장치를 **최대한 연결**하는 프로젝트 모음.

- 대상 툴: **Vivado 2022.2**
- 목표: 환경설정 → 코어 빌드 → 주변장치 연결 → 소프트웨어 작성 → 시뮬레이션 → 보드 검증
- 각 코어는 **별도 디렉터리**로 분리, 동일한 주변장치 로드맵을 따름.

```
Basys-Processor-Prj/
├── README.md              # (이 파일) 공통 개요 / 환경 / 로드맵
├── NEORV32/
│   ├── README.md           # 코어별 전체 가이드 (환경~검증)
│   └── sw/                 # ★ 로드맵 #1~#10 실제 소스 + sw/README.md
├── VexRiscv/{README.md, sw/}
├── PicoRV32/{README.md, sw/}
└── Ibex/{README.md, sw/}
```

> **구현 현황:** 소프트웨어(`sw/`, 로드맵 #1~#10)는 4개 코어 모두 작성 완료.
> **HDL 래퍼 / 제약(.xdc) / 테스트벤치(sim) / Vivado Tcl(scripts)** 는 아직 미작성
> (아래 §7 참고).

---

## 1. 하드웨어 자원 (예산)

| 자원 | 수량 | 비고 |
|---|---|---|
| Logic Cells | 33,280 | |
| LUT | 20,800 | |
| Flip-Flop | 41,600 | |
| Block RAM | 1,800 Kb (50 × 36Kb) | IMEM/DMEM/프레임버퍼 공유 |
| DSP48E1 | 90 | |
| Clock | 100 MHz 오실레이터 | MMCM/PLL 사용 |
| 온보드 주변장치 | 16 SW, 16 LED, 5 BTN, 7-seg(4), VGA(12bit), Pmod×4, USB-UART(FTDI), mono audio, USB-HID host | |

> **핵심 병목은 LUT가 아니라 BRAM(1,800 Kb)과 인터커넥트 복잡도**임.

---

## 2. 코어 비교 (요약)

| 코어 | 언어 | 버스 | 생성 방식 | Windows 단독 | 리소스 | 특징 |
|---|---|---|---|---|---|---|
| **NEORV32** | VHDL | 내부(Wishbone 유사) | 바로 합성 | ◎ | 중 | 내장 주변장치 최다, 문서 우수 |
| **VexRiscv** | SpinalHDL | Wishbone / AXI4 | SBT 생성 | △(WSL) | 소~중 | 자유도 최상, LiteX 연동 |
| **PicoRV32** | Verilog | 커스텀(Wishbone/메모리) | 손 이식 | ○ | 소 | 단순, 학습용 |
| **Ibex** | SystemVerilog | AXI4-Lite | 래핑 | △ | 중 | 검증 우수, 로직 많음 |

**추천 순 (Vivado 2022.2 + 편의성):** `NEORV32 > PicoRV32 > VexRiscv > Ibex`

---

## 3. 주변장치 로드맵 (난이도 순)

각 코어 문서는 아래 로드맵을 동일하게 따릅니다. `[ ]`는 체크리스트.

| # | 주변장치 | 인터페이스 | 난이도 | 상태 |
|---|---|---|---|---|
| 1 | GPIO (SW/LED/BTN/7-seg) | Memory-mapped | ★ | [ ] |
| 2 | UART (FTDI) | MMIO / 내장 | ★ | [ ] |
| 3 | Timer + IRQ Controller | MMIO / 내장 | ★★ | [ ] |
| 4 | SPI (Pmod) | MMIO | ★★ | [ ] |
| 5 | I2C / TWI (Pmod) | MMIO | ★★ | [ ] |
| 6 | PWM (audio / 서보) | MMIO | ★★ | [ ] |
| 7 | 7-seg Display 디코더 | MMIO | ★ | [ ] |
| 8 | VGA Framebuffer | BRAM + MMIO | ★★★ | [ ] |
| 9 | DMA | 내장/MMIO | ★★★ | [ ] |
| 10 | PS/2 키보드 | MMIO | ★★★ | [ ] |
| 11 | External Memory (Pmod SRAM/PSRAM) | MMIO | ★★★★ | [ ] |
| 12 | USB-HID Host | 별도 스택 | ★★★★★ | [ ] |

> **권장 순서:** 1→2→3 을 먼저 "Hello World + 인터럽트"까지 완성한 뒤 나머지를 병렬 진행.
> USB-HID host(12)는 FTDI/PHY 제약으로 **최후순위**.
>
> **소프트웨어 구현 범위:** `#1~#10` 데모를 4개 코어 `sw/` 에 작성 완료.
> `#11 ExtMem`, `#12 USB` 는 소프트웨어 미구현(HDL 로드맵에만 계획 기재).

---

## 4. 공통 환경설정

### 4.1 디렉터리 규약 (각 코어 폴더 내부)

```
<CORE>/
├── README.md
├── hdl/            # RTL 소스 (.v/.sv/.vhd) + 래퍼
├── sw/             # C 펌웨어, 링커, Makefile
├── constrs/        # basys3.xdc
├── sim/            # 테스트벤치 (Verilator/ModelSim/xsim)
├── scripts/        # Tcl (create_project.tcl, build.tcl, program.tcl)
└── bitstream/      # .bit 산출물
```

### 4.2 툴 요구사항

| 툴 | 용도 | 비고 |
|---|---|---|
| Vivado 2022.2 | 합성/구현/비트스트림 | Artix-7 완전 지원 |
| RISC-V GNU Toolchain | `riscv32-unknown-elf-gcc` | rv32im/ rv32imc |
| (선택) WSL2/Ubuntu 22.04 | VexRiscv 생성, 오픈소스 플로우 | Windows 단독 어려운 코어용 |
| (선택) Verilator / ModelSim | 시뮬레이션 | xsim도 가능 |
| make, python3 | 빌드 자동화 | |

### 4.3 툴체인 설치 (Windows 권장: WSL2)

```bash
# Ubuntu/WSL 기준
sudo apt update
sudo apt install -y build-essential git python3 make gcc
# RISC-V 툴체인 (RV32, 멀티립)
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --with-arch=rv32imc --with-abi=ilp32
make -j$(nproc)
echo 'export PATH=/opt/riscv/bin:$PATH' >> ~/.bashrc
```

확인:
```bash
riscv32-unknown-elf-gcc --version
```

### 4.4 Vivado 2022.2 무인(헤드리스) 플로우

```tcl
# scripts/build.tcl
open_project    ./build/basys3.xpr
launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1
write_bitstream -force ./bitstream/basys3.bit
```

실행:
```powershell
vivado -mode batch -source scripts\build.tcl
```

### 4.5 공통 제약 (Pmod/VGA/오디오 핀)

각 코어의 `constrs/basys3.xdc`는 아래를 **동일하게** 사용 (보드 핀은 고정):

- 클럭: `W5` (100 MHz)
- UART: TX `A18`, RX `B18` (FTDI)
- Pmod JA(top): `J1 L2 J2 K2 J3 K1 J4 K3`
- VGA: R/G/B 4bit × 3 = 12, HS `P19`, VS `R19`
- 7-seg: 세그먼트 + 자릿수 (anode)
- PWM: Pmod `JA4` (Basys3 온보드 오디오 없음) — NEORV32/VexRiscv/PicoRV32/Ibex 공통
- SW/BTN/LED: 표준 Basys3 마스터 XDC 참조

> 정확한 핀은 Xilinx 공식 `Basys3_Master.xdc`를 복사해 사용 권장.

---

## 5. 공통 검증 전략

1. **RTL 시뮬레이션** — 코어 + 주변장치 단위 테스트벤치 (`sim/`)  *(미작성)*
2. **소프트웨어 단위 테스트** — UART로 pass/fail 출력 (`sw/` 내장, UART `a` 스모크 테스트)
3. **보드 검증** — 로드맵 #마다 LED/UART로 상태 표시
4. **회귀(regression)** — `sw/` 의 UART `a` 스모크 테스트 실행

---

## 6. 마일스톤

- [ ] M1: 환경설정 완료 (툴체인 + Vivado 배치 플로우)
- [ ] M2: 코어 부팅 + UART "Hello"
- [x] M3: 로드맵 #1–3 (GPIO/UART/IRQ) — **소프트웨어 작성 완료**
- [x] M4: 로드맵 #4–7 (SPI/I2C/PWM/7seg) — **소프트웨어 작성 완료**
- [x] M5: 로드맵 #8–10 (VGA/DMA/PS2) — **소프트웨어 작성 완료**
- [ ] M6: 로드맵 #11–12 (ExtMem/USB)

> M3~M5 는 **소프트웨어**만 완료. HDL 래퍼/제약/시뮬/합성·보드 검증은 미완.

---

## 7. 구현 현황 / 다음 단계

### 완료 (`sw/`, 4개 코어 공통)
- 로드맵 **#1~#10** 데모: `main.c` 메뉴 + `app/app_*.c` (GPIO/UART/Timer-IRQ/SPI/I2C/PWM/7seg/VGA/DMA/PS2)
- 스타트업/링커: `crt0.S`(VexRiscv·Ibex), `start.S`+`custom_ops.S`(PicoRV32), NEORV32 는 rte 라이브러리 사용
- 빌드: `Makefile`(elf/bin/hex), `link.ld`, 미니 printf/`mem*`
- 문서: 각 코어 `README.md` + `sw/README.md`

### 미작성 (다음 단계)
| 항목 | 내용 |
|---|---|
| `hdl/` | top 래퍼 + 주소 디코더/인터커넥트 + 커스텀 주변장치 RTL (7seg/VGA/PS2, PWM/DMA 등) |
| `constrs/basys3.xdc` | 클럭/UART/Pmod/VGA/7seg/오디오 핀 제약 |
| `sim/` | 코어 + 주변장치 단위 테스트벤치 |
| `scripts/` | Vivado Tcl (`create_project.tcl`, `build.tcl`, `program.tcl`) |
| `bitstream/` | 합성/구현 산출물 (.bit) |
| 소프트웨어 | 로드맵 #11(ExtMem), #12(USB) |
