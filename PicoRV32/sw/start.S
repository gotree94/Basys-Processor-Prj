// ============================================================================
// start.S - PicoRV32 / Basys3 부트 + IRQ 래퍼
// ----------------------------------------------------------------------------
//  * reset_vec  : PROGADDR_RESET(0x00000000). 실 HW 에서는 블록되지 않도록 j _start
//  * irq_vec    : PROGADDR_IRQ(0x00000010). 레지스터 저장 후 C irq() 호출
//  * _start     : 스택 설정 후 main() 호출
//  * q0 = 복귀주소, q1 = 처리할 IRQ 비트마스크
// ============================================================================

#define ENABLE_QREGS
#include "custom_ops.S"

	.section .text.start
	.global reset_vec
reset_vec:
	// PROGADDR_RESET = 0x0, irq 벡터(0x10) 전까지 남는 공간
	j	_start
	.balign 16

// ---------------------------------------------------------------------------
// IRQ 핸들러 래퍼
// ---------------------------------------------------------------------------
	.global irq_vec
irq_vec:
#if defined(ENABLE_QREGS)
	picorv32_setq_insn(q2, x1)
	picorv32_setq_insn(q3, x2)

	lui	x1, %hi(irq_regs)
	addi	x1, x1, %lo(irq_regs)

	picorv32_getq_insn(x2, q0)
	sw	x2,   0*4(x1)          // regs[0] = 복귀주소
	picorv32_getq_insn(x2, q2)
	sw	x2,   1*4(x1)          // regs[1] = ra
	picorv32_getq_insn(x2, q3)
	sw	x2,   2*4(x1)          // regs[2] = sp

	sw	x3,   3*4(x1)
	sw	x4,   4*4(x1)
	sw	x5,   5*4(x1)
	sw	x6,   6*4(x1)
	sw	x7,   7*4(x1)
	sw	x8,   8*4(x1)
	sw	x9,   9*4(x1)
	sw	x10, 10*4(x1)
	sw	x11, 11*4(x1)
	sw	x12, 12*4(x1)
	sw	x13, 13*4(x1)
	sw	x14, 14*4(x1)
	sw	x15, 15*4(x1)
	sw	x16, 16*4(x1)
	sw	x17, 17*4(x1)
	sw	x18, 18*4(x1)
	sw	x19, 19*4(x1)
	sw	x20, 20*4(x1)
	sw	x21, 21*4(x1)
	sw	x22, 22*4(x1)
	sw	x23, 23*4(x1)
	sw	x24, 24*4(x1)
	sw	x25, 25*4(x1)
	sw	x26, 26*4(x1)
	sw	x27, 27*4(x1)
	sw	x28, 28*4(x1)
	sw	x29, 29*4(x1)
	sw	x30, 30*4(x1)
	sw	x31, 31*4(x1)

	// IRQ 전용 스택 + C 핸들러 호출
	lui	sp, %hi(irq_stack)
	addi	sp, sp, %lo(irq_stack)

	lui	a0, %hi(irq_regs)
	addi	a0, a0, %lo(irq_regs)     // arg0 = regs 포인터
	picorv32_getq_insn(a1, q1)        // arg1 = 처리할 IRQ 마스크
	jal	ra, irq

	// C 핸들러가 반환한 regs 포인터(a0) 로 복원
	addi	x1, a0, 0

	lw	x2,   0*4(x1)
	picorv32_setq_insn(q0, x2)
	lw	x2,   1*4(x1)
	picorv32_setq_insn(q1, x2)
	lw	x2,   2*4(x1)
	picorv32_setq_insn(q2, x2)

	lw	x3,   3*4(x1)
	lw	x4,   4*4(x1)
	lw	x5,   5*4(x1)
	lw	x6,   6*4(x1)
	lw	x7,   7*4(x1)
	lw	x8,   8*4(x1)
	lw	x9,   9*4(x1)
	lw	x10, 10*4(x1)
	lw	x11, 11*4(x1)
	lw	x12, 12*4(x1)
	lw	x13, 13*4(x1)
	lw	x14, 14*4(x1)
	lw	x15, 15*4(x1)
	lw	x16, 16*4(x1)
	lw	x17, 17*4(x1)
	lw	x18, 18*4(x1)
	lw	x19, 19*4(x1)
	lw	x20, 20*4(x1)
	lw	x21, 21*4(x1)
	lw	x22, 22*4(x1)
	lw	x23, 23*4(x1)
	lw	x24, 24*4(x1)
	lw	x25, 25*4(x1)
	lw	x26, 26*4(x1)
	lw	x27, 27*4(x1)
	lw	x28, 28*4(x1)
	lw	x29, 29*4(x1)
	lw	x30, 30*4(x1)
	lw	x31, 31*4(x1)

	picorv32_retirq_insn()
#endif

	.balign 0x200
irq_regs:
	.fill 32,4
	.fill 128,4
irq_stack:

// ---------------------------------------------------------------------------
// 부팅
// ---------------------------------------------------------------------------
	.section .text
	.global _start
_start:
	// 레지스터 0 초기화
	addi	x1,  zero, 0
	addi	x2,  zero, 0
	addi	x3,  zero, 0
	addi	x4,  zero, 0
	addi	x5,  zero, 0
	addi	x6,  zero, 0
	addi	x7,  zero, 0
	addi	x8,  zero, 0
	addi	x9,  zero, 0
	addi	x10, zero, 0
	addi	x11, zero, 0
	addi	x12, zero, 0
	addi	x13, zero, 0
	addi	x14, zero, 0
	addi	x15, zero, 0
	addi	x16, zero, 0
	addi	x17, zero, 0
	addi	x18, zero, 0
	addi	x19, zero, 0
	addi	x20, zero, 0
	addi	x21, zero, 0
	addi	x22, zero, 0
	addi	x23, zero, 0
	addi	x24, zero, 0
	addi	x25, zero, 0
	addi	x26, zero, 0
	addi	x27, zero, 0
	addi	x28, zero, 0
	addi	x29, zero, 0
	addi	x30, zero, 0
	addi	x31, zero, 0

	// 스택 포인터 = RAM 상단 (32 KB -> 0x8000)
	lui	sp, 0x8

	// 인터럽트는 기본 비활성화 상태
	jal	ra, main

1:	j	1b

// ---------------------------------------------------------------------------
// C 에서 호출하는 IRQ 제어 헬퍼
// ---------------------------------------------------------------------------
	.global picorv32_enable_irqs
picorv32_enable_irqs:                  // void picorv32_enable_irqs(uint32_t mask)
	picorv32_maskirq_insn(zero, a0)    // mask: 1 = 비활성. 활성화할 비트를 0 으로
	ret

	.global picorv32_disable_irqs
picorv32_disable_irqs:                 // void picorv32_disable_irqs(void)
	li	a0, -1
	picorv32_maskirq_insn(zero, a0)
	ret
