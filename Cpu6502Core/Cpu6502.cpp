//
// Copyright (c) 2012, 2020 Thomas Skibo.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

// Cpu6502.cpp

#include <stdint.h>

#include "Cpu6502.h"
#include "MemSpace.h"

#if defined(CPU65C02) && defined(ILL6502)
#error "Cpu6502: CPU65C02 and ILL6502 are mutally exclusive."
#endif

#ifdef DEBUG6502
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUG6502) printf("[%d]" f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

/* 6502 constants. */
#define STACK_ADDR      0x0100
#define NMI_VECTOR      0xfffa
#define RESET_VECTOR    0xfffc
#define IRQ_VECTOR      0xfffe

/* Processor status flags. */
#define P_N     0x80
#define P_V     0x40
#define P_1     0x20    /* always set */
#define P_B     0x10
#define P_D     0x08
#define P_I     0x04
#define P_Z     0x02
#define P_C     0x01

Cpu6502::Cpu6502(MemSpace *memspace)
{
	DPRINTF(1, "Cpu6502::%s:\n", __func__);

	this->mem = memspace;
	step_flag = false;
	hitbrk = false;
	nbpts = 0;
}

#ifdef DEBUG6502
static int last_mem_cycle = -1;
#endif

uint8_t
Cpu6502::read_byte(uint16_t addr)
{
	uint8_t d8 = mem->read(addr);

	DPRINTF(3, "Cpu6502::%s: addr 0x%04x data 0x%02x\n", __func__,
		addr, d8);

#ifdef DEBUG6502
	if (cycles == last_mem_cycle)
		DPRINTF(1, "Cpu6502::%s: XXX concurrent memory cycles!\n",
			__func__);
	last_mem_cycle = cycles;
#endif

	return d8;
}

void
Cpu6502::write_byte(uint16_t addr, uint8_t d8)
{
	DPRINTF(2, "Cpu6502::%s: addr 0x%04x data 0x%02x\n", __func__,
		addr, d8);

#ifdef DEBUG6502
	if (cycles == last_mem_cycle)
		DPRINTF(1, "Cpu6502::%s: XXX concurrent memory cycles!\n",
			__func__);
	last_mem_cycle = cycles;
#endif

	mem->write(addr, d8);
}

uint16_t
Cpu6502::read_word(uint16_t addr)
{
	uint16_t d16 = mem->read(addr);
	d16 |= ((uint16_t)mem->read(addr + 1) << 8);
	DPRINTF(3, "Cpu6502::%s: addr 0x%04x data 0x%04x\n", __func__,
		addr, d16);
	return d16;
}

/* Note that ROM must be set up before calling this because
 * RESET vector must be in place.
 */
void
Cpu6502::reset(void)
{
	a = 0;
	x = 0;
	y = 0;
	sp = 0xff;
	p = P_I | P_1;
	pc = read_word(RESET_VECTOR);

	irq_signal = false;
	needs_nmi = false;
	doing_int = false;
	step_flag = false;
	cyclenum = 0;
}

void
Cpu6502::push(uint8_t d8)
{
	DPRINTF(4, "Cpu6502::%s: addr 0x%04x data 0x%02x\n", __func__,
		STACK_ADDR + sp, d8);
	write_byte(STACK_ADDR + sp, d8);
	sp--;
}

uint8_t
Cpu6502::pull(void)
{
	sp++;
	uint8_t d8 = read_byte(STACK_ADDR + sp);
	DPRINTF(4, "Cpu6502::%s: addr 0x%04x data 0x%02x\n", __func__,
		STACK_ADDR +sp, d8);
	return d8;
}

#define SET_FLAG(flag, cond)					\
	(p = ((cond) != 0) ? (p | (flag)) : (p & ~(flag)))

void
Cpu6502::set_nz(uint8_t d8)
{
	SET_FLAG(P_N, d8 & 0x80);
	SET_FLAG(P_Z, d8 == 0);
}

uint8_t
Cpu6502::asl(uint8_t d8)
{
	SET_FLAG(P_C, d8 & 0x80);
	d8 <<= 1;
	set_nz(d8);
	return d8;
}

uint8_t
Cpu6502::lsr(uint8_t d8)
{
	SET_FLAG(P_C, d8 & 0x01);
	d8 >>= 1;
	set_nz(d8);
	return d8;
}

uint8_t
Cpu6502::rol(uint8_t d8)
{
	uint8_t old_c = ((p & P_C) != 0) ? 0x01 : 0x00;

	SET_FLAG(P_C, d8 & 0x80);
	d8 = (d8 << 1) | old_c;
	set_nz(d8);
	return d8;
}

uint8_t
Cpu6502::ror(uint8_t d8)
{
	uint8_t old_c = ((p & P_C) != 0) ? 0x80 : 0x00;

	SET_FLAG(P_C, d8 & 0x01);
	d8 = (d8 >> 1) | old_c;
	set_nz(d8);
	return d8;
}

void
Cpu6502::orr(uint8_t d8)
{
	a |= d8;
	set_nz(a);
}

void
Cpu6502::andd(uint8_t d8)
{
	a &= d8;
	set_nz(a);
}

void
Cpu6502::eor(uint8_t d8)
{
	a ^= d8;
	set_nz(a);
}

void
Cpu6502::adc(uint8_t d8)
{
	uint16_t result;

	result = a + d8 + ((p & P_C) ? 1 : 0);

	if ((p & P_D) != 0) {
		/* Decimal mode.  Gack! */
		SET_FLAG(P_Z, (result & 0xff) == 0);

		result = (a & 0x0f) + (d8 & 0x0f) + ((p & P_C) ? 1 : 0);
		if (result > 0x19)
			result -= 0x0a;
		else if (result > 0x09)
			result += 0x06;
		result += (a & 0xf0) + (d8 & 0xf0);
		SET_FLAG(P_N, (result & 0x80) != 0);
		SET_FLAG(P_V, ((d8 ^ a) & 0x80) == 0 &&
			 ((result ^ a) & 0x80) != 0);
		if ((result & 0xfff0) > 0x90)
			result += 0x60;
	}
	else {
		set_nz(result & 0xff);
		SET_FLAG(P_V, ((d8 ^ a) & 0x80) == 0 &&
			 ((result ^ a) & 0x80) != 0);
	}

	SET_FLAG(P_C, result & 0xff00);

	a = (result & 0xff);
}

void
Cpu6502::sbc(uint8_t d8)
{
	uint16_t result;

	result = a - d8 - ((p & P_C) ? 0 : 1);
	SET_FLAG(P_V, ((d8 ^ a) & 0x80) != 0 && ((result ^ a) & 0x80) != 0);
	set_nz(result & 0xff);

	if ((p & P_D) != 0) {
		/* Decimal mode.  Gack! */
		result = (a & 0x0f) - (d8 & 0x0f) - ((p & P_C) ? 0 : 1);
		if (result >= 0xfff6)
			result -= 0x06;
		else if (result >= 0xff00)
			result += 0x0a;
		result += (a & 0xf0) - (d8 & 0xf0);
		if (result >= 0x100)
			result -= 0x60;
	}

	SET_FLAG(P_C, (result & 0xff00) == 0);
	a = (result & 0xff);
}

void
Cpu6502::cmp(uint8_t left, uint8_t right)
{
	uint16_t result = left - right;

	SET_FLAG(P_C, (result & 0xff00) == 0);
	set_nz(result);
}

void
Cpu6502::bit(uint8_t d8)
{
	SET_FLAG(P_N, d8 & 0x80);
	SET_FLAG(P_V, d8 & 0x40);
	SET_FLAG(P_Z, (d8 & a) == 0);
}

void
Cpu6502::branch(uint8_t operand)
{
	uint16_t page = pc >> 8;

	pc += (int8_t)operand;
	pagedelay = ((pc >> 8) != page);
}

#ifdef CPU65C02
/* 16-bit operand of bbr and bbs is concatenation of address and
 * branch relative offset.
 */
void
Cpu6502::bbr(uint16_t operand, uint8_t bit)
{
	uint8_t d8 = read_byte(operand & 0xff);

	if ((d8 & (1 << bit)) == 0)
		branch(operand >> 8);
}

void
Cpu6502::bbs(uint16_t operand, uint8_t bit)
{
	uint8_t d8 = read_byte(operand & 0xff);

	if ((d8 & (1 << bit)) != 0)
		branch(operand >> 8);
}

void
Cpu6502::rmb(uint8_t operand, uint8_t bit)
{
	uint8_t d8 = read_byte(operand);

	d8 &= ~(1 << bit);
	write_byte(operand, d8);
}

void
Cpu6502::smb(uint8_t operand, uint8_t bit)
{
	uint8_t d8 = read_byte(operand);

	d8 |= (1 << bit);
	write_byte(operand, d8);
}
#endif // CPU65C02

#ifdef ILL6502
void
Cpu6502::arr(uint8_t operand)
{
	uint8_t a_old;

	// Perform adc of (a & operand) + operand, throw out result.
	a &= operand;
	a_old = a;
	adc(operand);

	// Shift A right but exchange C with bit 8.
	a = a_old >> 1;
	if ((p & P_C) != 0)
		a |= 0x80;
	SET_FLAG(P_C, ((a_old & 0x80) != 0));

	set_nz(a);
}

uint8_t
Cpu6502::slo(uint8_t operand)
{
	operand = asl(operand);
	a |= operand;
	return operand;
}

uint8_t
Cpu6502::rla(uint8_t operand)
{
	operand = rol(operand);
	a &= operand;
	return operand;
}

uint8_t
Cpu6502::sre(uint8_t operand)
{
	operand = lsr(operand);
	a ^= operand;
	return operand;
}

uint8_t
Cpu6502::rra(uint8_t operand)
{
	operand = ror(operand);
	adc(operand); // XXX: overrides flags?
	return operand;
}

uint8_t
Cpu6502::dcp(uint8_t operand)
{
	operand--;
	cmp(a, operand);
	return operand;
}

uint8_t
Cpu6502::isc(uint8_t operand)
{
	operand++;
	sbc(operand);
	return operand;
}
#endif // ILL6502

#if !defined(CPU65C02) && !defined(ILL6502)
/* Instruction length by opcode. NMOS 6502.  0 means illegal opcode. */
static uint8_t instrlen[] = {
	2, 2, 0, 0, 0, 2, 2, 0,  1, 2, 1, 0, 0, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0,
	3, 2, 0, 0, 2, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0,

	1, 2, 0, 0, 0, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0,
	1, 2, 0, 0, 0, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0,

	0, 2, 0, 0, 2, 2, 2, 0,  1, 0, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 2, 2, 2, 0,  1, 3, 1, 0, 0, 3, 0, 0,
	2, 2, 2, 0, 2, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 2, 2, 2, 0,  1, 3, 1, 0, 3, 3, 3, 0,

	2, 2, 0, 0, 2, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0,
	2, 2, 0, 0, 2, 2, 2, 0,  1, 2, 1, 0, 3, 3, 3, 0,
	2, 2, 0, 0, 0, 2, 2, 0,  1, 3, 0, 0, 0, 3, 3, 0
};
#elif !defined(CPU65C02)
/* Instruction length by opcode: NMOS 6502 including undocumented opcodes. */
static uint8_t instrlen[] = {
	2, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	3, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	1, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	1, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,

	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3
};
#else
/* Instruction length by opcode, WDC 65C02 including NOPs. */
static uint8_t instrlen[] = {
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,
	3, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,

	1, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1,
	1, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,

	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 0, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,

	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1
};
#endif // CPU65C02 && ! ILL6502

#if DEBUG6502 > 1
#ifndef CPU65C02
/* Cycle count by opcode (not including some caveats).
 * NMOS 6502 including illegal opcodes. 9 means JAM
 */
static uint8_t cycle_count[] = {
	7, 6, 9, 8, 3, 3, 5, 5,  3, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 9, 8, 3, 3, 5, 5,  4, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7,

	6, 6, 9, 8, 3, 3, 5, 5,  3, 2, 2, 2, 3, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 9, 8, 3, 3, 5, 5,  4, 2, 2, 2, 5, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7,

	2, 6, 2, 6, 3, 3, 3, 3,  2, 2, 2, 2, 4, 4, 4, 4,
	2, 6, 9, 6, 4, 4, 4, 4,  2, 5, 2, 5, 5, 5, 5, 5,
	2, 6, 2, 6, 3, 3, 3, 3,  2, 2, 2, 2, 4, 4, 4, 4,
	2, 5, 9, 5, 4, 4, 4, 4,  2, 4, 2, 4, 4, 4, 4, 4,

	2, 6, 2, 8, 3, 3, 5, 5,  2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 8, 3, 3, 5, 5,  2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 9, 8, 4, 4, 6, 6,  2, 4, 2, 7, 4, 4, 7, 7
};
#else
/* Cycle count by opcode (not including some caveats).
 * WDC 65C02 including NOPs.
 */
static uint8_t cycle_count[] = {
	7, 6, 2, 1, 5, 3, 5, 1,  3, 2, 2, 1, 6, 4, 6, 1,
	2, 5, 5, 1, 5, 4, 6, 1,  2, 4, 2, 1, 6, 4, 7, 1,
	6, 6, 2, 1, 3, 3, 5, 1,  4, 2, 2, 1, 4, 4, 6, 1,
	2, 5, 5, 1, 3, 4, 6, 1,  2, 4, 2, 1, 4, 4, 7, 1,

	6, 6, 2, 1, 3, 3, 5, 1,  3, 2, 2, 1, 3, 4, 6, 1,
	2, 5, 5, 1, 4, 4, 6, 1,  2, 4, 3, 1, 8, 4, 7, 1,
	6, 6, 2, 1, 3, 3, 5, 1,  4, 2, 2, 1, 6, 4, 6, 1,
	2, 5, 5, 1, 4, 4, 6, 1,  2, 4, 4, 1, 6, 4, 7, 1,

	3, 6, 2, 1, 3, 3, 3, 1,  2, 3, 2, 1, 4, 4, 4, 1,
	2, 6, 5, 1, 4, 4, 4, 1,  2, 5, 2, 1, 4, 5, 5, 1,
	2, 6, 2, 1, 3, 3, 3, 1,  2, 2, 2, 1, 4, 4, 4, 1,
	2, 5, 5, 1, 4, 4, 4, 1,  2, 4, 2, 1, 4, 4, 4, 1,

	2, 6, 2, 1, 3, 3, 5, 1,  2, 2, 2, 1, 4, 4, 6, 1,
	2, 5, 5, 1, 4, 4, 6, 1,  2, 4, 3, 1, 4, 4, 7, 1,
	2, 6, 2, 1, 3, 3, 5, 1,  2, 2, 2, 1, 4, 4, 6, 1,
	2, 5, 5, 1, 4, 4, 6, 1,  2, 4, 4, 1, 4, 4, 7, 1
};
#endif // CPU65C02
#endif // DEBUG6502 > 1

void
Cpu6502::dointcycle(void)
{
	DPRINTF(2, "Cpu6502::%s: dointcycle: cyclenum = %d\n", __func__,
		cyclenum);

	switch (cyclenum) {
	case 0:
	case 1: // "internal operations"
		break;
	case 2:
		push(pc >> 8);
		break;
	case 3:
		push(pc & 255);
		break;
	case 4:
		push(p);
		p |= P_I;
#ifdef CPU65C02
		p &= ~P_D;
#endif
		break;
	case 5:
		pc = read_byte(needs_nmi ? NMI_VECTOR : IRQ_VECTOR);
		break;
	case 6:
		pc = pc | (uint16_t)read_byte(needs_nmi ? NMI_VECTOR + 1 :
					      IRQ_VECTOR + 1) << 8;
		needs_nmi = false;
		doing_int = false;
		break;
	}

	if (doing_int)
		cyclenum++;
	else
		cyclenum = 0;
}

bool
Cpu6502::cycle1(void)
{
	bool done_flag = false;

	operand = read_byte(pc);

	if (instrlen[opcode] > 1)
		pc++;

	switch (opcode) {
	case 0x09:        /* ORA imm */
		orr(operand);
		done_flag = true;
		break;
	case 0x0a:        /* ASL A */
		a = asl(a);
		done_flag = true;
		break;
	case 0x10:        /* BPL */
		done_flag = ((p & P_N) != 0);
		break;
	case 0x18:        /* CLC */
		p &= ~P_C;
		done_flag = true;
		break;
	case 0x29:        /* AND imm */
		andd(operand);
		done_flag = true;
		break;
	case 0x2a:        /* ROL A */
		a = rol(a);
		done_flag = true;
		break;
	case 0x30:        /* BMI */
		done_flag = ((p & P_N) == 0);
		break;
	case 0x38:        /* SEC */
		p |= P_C;
		done_flag = true;
		break;
	case 0x49:        /* EOR imm */
		eor(operand);
		done_flag = true;
		break;
	case 0x4a:        /* LSR A */
		a = lsr(a);
		done_flag = true;
		break;
	case 0x50:        /* BVC */
		done_flag = ((p & P_V) != 0);
		break;
	case 0x58:        /* CLI */
		p &= ~P_I;
		done_flag = true;
		break;
	case 0x69:        /* ADC imm */
		adc(operand);
		done_flag = true;
		break;
	case 0x6a:        /* ROR A */
		a = ror(a);
		done_flag = true;
		break;
	case 0x70:        /* BVS */
		done_flag = ((p & P_V) == 0);
		break;
	case 0x78:        /* SEI */
		p |= P_I;
		done_flag = true;
		break;
	case 0x88:        /* DEY */
		y--;
		set_nz(y);
		done_flag = true;
		break;
	case 0x8a:        /* TXA */
		a = x;
		set_nz(a);
		done_flag = true;
		break;
	case 0x90:        /* BCC */
		done_flag = ((p & P_C) != 0);
		break;
	case 0x98:        /* TYA */
		a = y;
		set_nz(a);
		done_flag = true;
		break;
	case 0x9a:        /* TXS */
		sp = x;
		done_flag = true;
		break;
	case 0xa0:        /* LDY imm */
		y = operand;
		set_nz(y);
		done_flag = true;
		break;
	case 0xa2:        /* LDX imm */
		x = operand;
		set_nz(x);
		done_flag = true;
		break;
	case 0xa8:        /* TAY */
		y = a;
		set_nz(y);
		done_flag = true;
		break;
	case 0xa9:        /* LDA imm */
		a = operand;
		set_nz(a);
		done_flag = true;
		break;
	case 0xaa:        /* TAX */
		x = a;
		set_nz(x);
		done_flag = true;
		break;
	case 0xb0:        /* BCS */
		done_flag = ((p & P_C) == 0);
		break;
	case 0xb8:        /* CLV */
		p &= ~P_V;
		done_flag = true;
		break;
	case 0xba:        /* TSX */
		x = sp;
		set_nz(x);
		done_flag = true;
		break;
	case 0xc0:        /* CPY imm */
		cmp(y, operand);
		done_flag = true;
		break;
	case 0xc8:        /* INY */
		y++;
		set_nz(y);
		done_flag = true;
		break;
	case 0xc9:        /* CMP imm */
		cmp(a, operand);
		done_flag = true;
		break;
	case 0xca:        /* DEX */
		x--;
		set_nz(x);
		done_flag = true;
		break;
	case 0xd0:        /* BNE */
		done_flag = ((p & P_Z) != 0);
		break;
	case 0xd8:        /* CLD */
		p &= ~P_D;
		done_flag = true;
		break;
	case 0xe0:        /* CPX imm */
		cmp(x, operand);
		done_flag = true;
		break;
	case 0xe8:        /* INX */
		x++;
		set_nz(x);
		done_flag = true;
		break;
	case 0xe9:        /* SBC imm */
		sbc(operand);
		done_flag = true;
		break;
	case 0xea:        /* NOP */
		done_flag = true;
		break;
	case 0xf0:        /* BEQ */
		done_flag = ((p & P_Z) == 0);
		break;
	case 0xf8:        /* SED */
		p |= P_D;
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x02:	  /* KIL (or JAM or HLT */
	case 0x12:
	case 0x22:
	case 0x32:
	case 0x42:
	case 0x52:
	case 0x62:
	case 0x72:
	case 0x92:
	case 0xb2:
	case 0xd2:
	case 0xf2:
		pc--;
		done_flag = true;
		DPRINTF(2, "Cpu6502::%s: Hit KIL pc=0x%x\n", __func__, pc);
		break;
	case 0x1a:	  /* NOP */
	case 0x3a:
	case 0x5a:
	case 0x7a:
	case 0xda:
	case 0xfa:
	case 0x80:	  /* NOP imm */
	case 0x82:
	case 0x89:
	case 0xc2:
	case 0xe2:
		done_flag = true;
		break;
	case 0x0b:	  /* ANC imm */
	case 0x2b:
		SET_FLAG(P_C, (a & 0x80) != 0);
		andd(operand);
		done_flag = true;
		break;
	case 0x4b:	  /* ALR imm */
		a &= operand;
		a = lsr(a);
		set_nz(a);
		done_flag = true;
		break;
	case 0x6b:	  /* ARR imm */
		arr(operand);
		done_flag = true;
		break;
	case 0xcb:	  /* AXS imm */
		SET_FLAG(P_C, (a & x) >= operand);
		x = (a & x) - operand;
		set_nz(x);
		done_flag = true;
		break;
	case 0xeb:	  /* SBC imm */
		sbc(operand);
		done_flag = true;
		break;
#endif // ILL6502
	} // switch (opcode)

	return done_flag;
}

bool
Cpu6502::cycle2(void)
{
	bool done_flag = false;

	if ((opcode & 0x1f) == 0x10) {
		(void)read_byte(pc);
		branch(operand);
		// Branches that don't cross page boundary are done.
		if (!pagedelay)
			done_flag = true;
		return done_flag;
	} else {
		// Group one instructions
		switch (opcode & 0x1f) {
		case 0x01: /* (ind,X) */
			(void)read_byte(operand);
			break;
		case 0x11: /* (ind),Y */
			opaddr = read_byte(operand);
			break;
		case 0x05:  /* ZP */
		case 0x06:
			opaddr = operand;
			if (opcode != 0x85 && opcode != 0x86) /* STA, STX */
				operand = read_byte(operand);
			break;
		case 0x15:  /* ZP,X */
			(void)read_byte(operand);
			break;
		case 0x0d: /* absolute */
		case 0x0e:
		case 0x19: /* absolute, Y */
		case 0x1d: /* absolute, X */
		case 0x1e: /* absolute,[XY] */
			opaddr = operand |
				(uint16_t)read_byte(pc++) << 8;
			break;
#ifdef ILL6502
		case 0x03: /* (ind,X) */
			(void)read_byte(operand);
			break;
		case 0x07: /* ZP */
			if (opcode != 0x87) /* SAX */
				operand = read_byte(operand);
			break;
		case 0x13: /* (ind),Y */
			opaddr = read_byte(operand);
			break;
		case 0x17: /* ZP,x */
			(void)read_byte(operand);
			break;
		case 0x0f: /* absolute */
		case 0x1f: /* absolute,X */
		case 0x1b: /* absolute,Y */
			opaddr = operand |
				(uint16_t)read_byte(pc++) << 8;
			break;
#endif // ILL6502
		}
	}

	switch (opcode) {
	case 0x00:        /* BRK */
		push(pc >> 8);
		break;
	case 0x05:        /* ORA zero page */
		orr(operand);
		done_flag = true;
		break;
	case 0x08:        /* PHP */
		push(p | P_B);
		done_flag = true;
		break;
	case 0x2c:        /* BIT absolute */
	case 0x8c:        /* STY absolute */
	case 0xac:        /* LDY absolute */
	case 0xbc:        /* LDY absolute, X */
	case 0xcc:        /* CPY absolute */
	case 0xec:        /* CPX absolute */
		opaddr = operand | (uint16_t)read_byte(pc++) << 8;
		break;
	case 0x94:	  /* STY zero,X */
	case 0xb4:        /* LDY zero,X */
		(void)read_byte(operand);
		break;
	case 0x20:        /* JSR absolute */
		(void)read_byte(STACK_ADDR + sp);
		break;
	case 0x24:        /* BIT zero page */
		operand = read_byte(operand);
		bit(operand);
		done_flag = true;
		break;
	case 0x25:        /* AND zero page */
		andd(operand);
		done_flag = true;
		break;
	case 0x28:        /* PLP */
		(void)read_byte(STACK_ADDR + sp);
		break;
	case 0x40:        /* RTI */
		(void)read_byte(STACK_ADDR + sp);
		break;
	case 0x45:        /* EOR zero page */
		eor(operand);
		done_flag = true;
		break;
	case 0x48:        /* PHA */
		push(a);
		done_flag = true;
		break;
	case 0x4c:        /* JMP absolute */
		pc = operand | (uint16_t)read_byte(pc) << 8;
		done_flag = true;
		break;
	case 0x60:        /* RTS */
		(void)read_byte(STACK_ADDR + sp);
		break;
	case 0x65:        /* ADC zero page */
		adc(operand);
		done_flag = true;
		break;
	case 0x68:        /* PLA */
		(void)read_byte(STACK_ADDR + sp);
		break;
	case 0x6c:        /* JMP (ind) */
		opaddr = operand | (uint16_t)read_byte(pc) << 8;
		break;
	case 0x84:        /* STY zero page */
		write_byte(operand, y);
		done_flag = true;
		break;
	case 0x85:        /* STA zero page */
		write_byte(operand, a);
		done_flag = true;
		break;
	case 0x86:        /* STX zero page */
		write_byte(operand, x);
		done_flag = true;
		break;
	case 0xa4:        /* LDY zero page */
		y = read_byte(operand);
		set_nz(y);
		done_flag = true;
		break;
	case 0xa5:        /* LDA zero page */
		a = operand;
		set_nz(a);
		done_flag = true;
		break;
	case 0xa6:        /* LDX zero page */
		x = operand;
		set_nz(x);
		done_flag = true;
		break;
	case 0xc4:        /* CPY zero */
		operand = read_byte(operand);
		cmp(y, operand);
		done_flag = true;
		break;
	case 0xc5:        /* CMP zero */
		cmp(a, operand);
		done_flag = true;
		break;
	case 0xe4:        /* CPX zero */
		operand = read_byte(operand);
		cmp(x, operand);
		done_flag = true;
		break;
	case 0xe5:        /* SBC zero */
		sbc(operand);
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x04:	  /* NOP zero */
	case 0x44:
	case 0x64:
		(void)read_byte(operand);
		done_flag = true;
		break;
	case 0x14:	  /* NOP zero,X */
	case 0x34:
	case 0x54:
	case 0x74:
	case 0xd4:
	case 0xf4:
		(void)read_byte(operand);
		break;
	case 0x0c:	  /* NOP absolute */
	case 0x1c:	  /* NOP absolute, X */
	case 0x3c:
	case 0x5c:
	case 0x7c:
	case 0xdc:
	case 0xfc:
		opaddr = operand |
			(uint16_t)read_byte(pc++) << 8;
		break;
	case 0x87:	  /* SAX zero */
		write_byte(operand, a & x);
		done_flag = true;
		break;
	case 0xa7:	  /* LAX zero */
		a = operand;
		set_nz(a);
		x = a;
		done_flag = true;
		break;
#endif // ILL6502
	} // switch (opcode)

	return done_flag;
}

bool
Cpu6502::cycle3(void)
{
	bool done_flag = false;

	if ((opcode & 0x1f) == 0x10) {
		/* Branch across page has completed. */
		(void)read_byte(((pc - (int8_t)operand) & 0xff00) |
				(pc & 0x00ff));
		done_flag = true;
		return done_flag;
	} else {
		// Group one instructions
		switch (opcode & 0x1f) {
		case 0x01: /* (ind,X) */
			operand += x;
			opaddr = read_byte(operand);
			break;
		case 0x06: /* zero - except LDX STX which aleady terminated */
			// writes back original operand before modifying
			write_byte(opaddr, operand);
			break;
		case 0x11: /* (ind),Y */
			operand++;
			opaddr |= (uint16_t)read_byte(operand) << 8;
			pagedelay = ((opaddr & 0xff) + y > 0xff);
			break;
		case 0x0d: /* absolute */
		case 0x0e:
			if (opcode != 0x8d && opcode != 0x8e) /* STA STX */
				operand = read_byte(opaddr);
			break;
		case 0x15:  /* ZP,X */
			operand += x;
			if (opcode != 0x95) /* STA zero,X */
				operand = read_byte(operand);
			break;
		case 0x16: /* ZP,x */
			if (opcode != 0x96 && opcode != 0xb6) {
				operand += x;
				opaddr = operand;
				operand = read_byte(opaddr);
			}
			break;
		case 0x19: /* absolute, Y */
			operand = read_byte((opaddr & 0xff00) |
					    ((opaddr + y) & 0xff));
			pagedelay = ((opaddr & 0xff) + y > 0xff);
			break;
		case 0x1d: /* absolute, X */
		case 0x1e: /* absolute, X (except LDX) */
			if (opcode != 0xbe) { /* LDX abs,X */
				operand = read_byte((opaddr & 0xff00) |
						    ((opaddr + x) & 0xff));
				pagedelay = ((opaddr & 0xff) + x > 0xff);
			}
			break;
#ifdef ILL6502
		case 0x03: /* (ind,X) */
			operand += x;
			opaddr = read_byte(operand);
			break;
		case 0x0f: /* absolute */
			if (opcode != 0x8f) /* SAX */
				operand = read_byte(opaddr);
			break;
		case 0x13: /* (ind),Y */
			operand++;
			opaddr |= (uint16_t)read_byte(operand) << 8;
			pagedelay = ((opaddr & 0xff) + y > 0xff);
			break;
		case 0x17: /* ZP,X */
			if (opcode != 0x97 && opcode != 0xb7) {
				operand += x;
				opaddr = operand;
				operand = read_byte(opaddr);
			}
			break;
		case 0x1b: /* absolute,Y */
			if (opcode != 0xbb) /* LAS */
				(void)read_byte((opaddr & 0xff00) |
						((opaddr + y) & 0xff));
			break;
		case 0x1f: /* absolute,X */
			if (opcode != 0xbf) /* LAX */
				(void)read_byte((opaddr & 0xff00) |
						((opaddr + x) & 0xff));
			break;
#endif // ILL6502
		}
	}

	switch (opcode) {
	case 0x00:        /* BRK */
		push(pc & 255);
		break;
	case 0x0d:        /* ORA absolute */
	case 0x15:        /* ORA zero,X */
		orr(operand);
		done_flag = true;
		break;
	case 0x19:        /* ORA absolute,Y */
	case 0x1d:        /* ORA absolute,X */
		if (!pagedelay) {
			orr(operand);
			done_flag = true;
		}
		break;
	case 0x20:        /* JSR absolute */
		push(pc >> 8);
		break;
	case 0x28:        /* PLP */
		p = (pull() & ~P_B) | P_1;
		done_flag = true;
		break;
	case 0x2c:        /* BIT absolute */
		operand = read_byte(opaddr);
		bit(operand);
		done_flag = true;
		break;
	case 0x2d:        /* AND absolute */
	case 0x35:        /* AND zero, X */
		andd(operand);
		done_flag = true;
		break;
	case 0x39:        /* AND absolute, Y */
	case 0x3d:        /* AND absolute, X */
		if (!pagedelay) {
			andd(operand);
			done_flag = true;
		}
		break;
	case 0x40:        /* RTI */
		p = (pull() & ~P_B) | P_1;
		break;
	case 0x4d:        /* EOR absolute */
	case 0x55:        /* EOR zero, X */
		eor(operand);
		done_flag = true;
		break;
	case 0x59:        /* EOR absolute,Y */
	case 0x5d:        /* EOR absolute,X */
		if (!pagedelay) {
			eor(operand);
			done_flag = true;
		}
		break;
	case 0x60:        /* RTS */
		pc = pull();
		break;
	case 0x68:        /* PLA */
		a = pull();
		set_nz(a);
		done_flag = true;
		break;
	case 0x6c:        /* JMP (ind) */
		pc = read_byte(opaddr);
		opaddr = (opaddr & 0xff00) | ((opaddr + 1) & 0xff);
		break;
	case 0x6d:        /* ADC absolute */
	case 0x75:        /* ADC zero, X */
		adc(operand);
		done_flag = true;
		break;
	case 0x79:        /* ADC absolute,Y */
	case 0x7d:        /* ADC absolute,X */
		if (!pagedelay) {
			adc(operand);
			done_flag = true;
		}
		break;
	case 0x8c:        /* STY absolute */
		write_byte(opaddr, y);
		done_flag = true;
		break;
	case 0x8d:        /* STA absolute */
		write_byte(opaddr, a);
		done_flag = true;
		break;
	case 0x8e:        /* STX absolute */
		write_byte(opaddr, x);
		done_flag = true;
		break;
	case 0x94:        /* STY zero, X */
		operand += x;
		write_byte(operand, y);
		done_flag = true;
		break;
	case 0x95:        /* STA zero, X */
		write_byte(operand, a);
		done_flag = true;
		break;
	case 0x96:        /* STX zero, Y */
		operand += y;
		write_byte(operand, x);
		done_flag = true;
		break;
	case 0xac:        /* LDY absolute */
		y = read_byte(opaddr);
		set_nz(y);
		done_flag = true;
		break;
	case 0xad:        /* LDA absolute */
	case 0xb5:        /* LDA zero,X */
		a = operand;
		set_nz(a);
		done_flag = true;
		break;
	case 0xae:        /* LDX absolute */
		x = operand;
		set_nz(x);
		done_flag = true;
		break;
	case 0xb4:        /* LDY zero,X */
		operand += x;
		y = read_byte(operand);
		set_nz(y);
		done_flag = true;
		break;
	case 0xb6:        /* LDX zero,Y */
		operand += y;
		x = read_byte(operand);
		set_nz(x);
		done_flag = true;
		break;
	case 0xb9:        /* LDA absolute, Y */
	case 0xbd:        /* LDA absolute, X */
		if (!pagedelay) {
			a = operand;
			set_nz(a);
			done_flag = true;
		}
		break;
	case 0xbc:        /* LDY absolute, X */
		operand = read_byte((opaddr & 0xff00) | ((opaddr + x) & 0xff));
		pagedelay = ((opaddr & 0xff) + x > 0xff);
		if (!pagedelay) {
			y = operand;
			set_nz(y);
			done_flag = true;
		}
		break;
	case 0xbe:        /* LDX absolute, Y */
		operand = read_byte((opaddr & 0xff00) | ((opaddr + y) & 0xff));
		pagedelay = ((opaddr & 0xff) + y > 0xff);
		if (!pagedelay) {
			x = operand;
			set_nz(x);
			done_flag = true;
		}
		break;
	case 0xcc:        /* CPY absolute */
		operand = read_byte(opaddr);
		cmp(y, operand);
		done_flag = true;
		break;
	case 0xcd:        /* CMP absolute */
	case 0xd5:        /* CMP zero,X */
		cmp(a, operand);
		done_flag = true;
		break;
	case 0xd9:        /* CMP absolute,Y */
	case 0xdd:        /* CMP absolute,X */
		if (!pagedelay) {
			cmp(a, operand);
			done_flag = true;
		}
		break;
	case 0xec:        /* CPX absolute */
		operand = read_byte(opaddr);
		cmp(x, operand);
		done_flag = true;
		break;
	case 0xed:        /* SBC absolute */
	case 0xf5:        /* SBC zero,X */
		sbc(operand);
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x07:	  /* SLO zero */
	case 0x27:	  /* RLA zero */
	case 0x47:	  /* SRE zero */
	case 0x67:	  /* RRA zero */
	case 0xc7:	  /* DCP zero */
	case 0xe7:	  /* ISC zero */
		// writes back original operand before modifying
		write_byte(opaddr, operand);
		break;
	case 0x0c:	  /* NOP absolute */
		(void)read_byte(opaddr);
		done_flag = true;
		break;
	case 0x14:	  /* NOP zero,X */
	case 0x34:
	case 0x54:
	case 0x74:
	case 0xd4:
	case 0xf4:
		operand += x;
		(void)read_byte(operand);
		done_flag = true;
		break;
	case 0x1c:	  /* NOP absolute,X */
	case 0x3c:
	case 0x5c:
	case 0x7c:
	case 0xdc:
	case 0xfc:
		(void)read_byte((opaddr & 0xff00) |
				((opaddr + x) & 0xff));
		pagedelay = ((opaddr & 0xff) + x > 0xff);
		if (!pagedelay)
			done_flag = true;
		break;
	case 0x8f:	  /* SAX absolute */
		write_byte(opaddr, a & x);
		done_flag = true;
		break;
	case 0x97:	  /* SAX zero,Y */
		operand += y;
		write_byte(operand, a & x);
		done_flag = true;
		break;
	case 0xaf:	  /* LAX absolute */
		a = operand;
		x = a;
		set_nz(a);
		done_flag = true;
		break;
	case 0xb7:	  /* LAX zero,Y */
		operand += y;
		a = read_byte(operand);
		set_nz(a);
		x = a;
		done_flag = true;
		break;
	case 0xbf:	  /* LAX absolute,Y */
		a = read_byte((opaddr & 0xff00) |
			      ((opaddr + y) & 0xff));
		x = a;
		pagedelay = ((opaddr & 0xff) + y > 0xff);
		if (!pagedelay)
			done_flag = true;
		break;
#endif // ILL6502
	} // switch (opcode)

	return done_flag;
}

bool
Cpu6502::cycle4(void)
{
	bool done_flag = false;

	// Group one instructions
	switch (opcode & 0x1f) {
	case 0x01: /* (ind,X) */
		operand++;
		opaddr |= (uint16_t)read_byte(operand) << 8;
		break;
	case 0x0e: /* absolute - except STX and LDX which have terminated. */
		// Write original operand back.
		write_byte(opaddr, operand);
		break;
	case 0x11: /* (ind),Y */
		operand = read_byte((opaddr & 0xff00) | ((opaddr + y) & 0xff));
		opaddr += y;
		break;
	case 0x19: /* absolute, Y */
		opaddr += y;
		if (opcode != 0x99) /* STA */
			operand = read_byte(opaddr);
		break;
	case 0x1d: /* absolute, X */
	case 0x1e:
		if (opcode != 0xbe) {
			opaddr += x;
			if (opcode != 0x9d) /* STA */
				operand = read_byte(opaddr);
		}
		break;
#ifdef ILL6502
	case 0x03: /* (ind,X) */
		operand++;
		opaddr |= (uint16_t)read_byte(operand) << 8;
		break;
	case 0x0f: /* absolute - except LAX and SAX which have terminated. */
		// Write original operand back
		write_byte(opaddr, operand);
		break;
	case 0x1b: /* absolute,Y */
		opaddr += y;
		operand = read_byte(opaddr);
		break;
	case 0x1f: /* absolute,X */
		opaddr += x;
		operand = read_byte(opaddr);
		break;
	case 0x13: /* (ind),Y */
		operand = read_byte((opaddr & 0xff00) | ((opaddr + y) & 0xff));
		opaddr += y;
		break;
#endif // ILL6502
	}

	switch (opcode) {
	case 0x00:        /* BRK */
		push(p | P_B);
		break;
	case 0x06:        /* ASL zero page */
		operand = asl(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x11:        /* ORA (ind),Y */
		if (!pagedelay) {
			orr(operand);
			done_flag = true;
		}
		break;
	case 0x19:        /* ORA absolute,Y */
	case 0x1d:        /* ORA absolute,X */
		orr(operand);
		done_flag = true;
		break;
	case 0x20:        /* JSR absolute */
		push(pc & 255);
		break;
	case 0x26:        /* ROL zero page */
		operand = rol(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x31:        /* AND (ind),Y */
		if (!pagedelay) {
			andd(operand);
			done_flag = true;
		}
		break;
	case 0x39:        /* AND absolute, Y */
	case 0x3d:        /* AND absolute, X */
		andd(operand);
		done_flag = true;
		break;
	case 0x40:        /* RTI */
		pc = pull();
		break;
	case 0x46:        /* LSR zero page */
		operand = lsr(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x51:        /* EOR (indirect), Y */
		if (!pagedelay) {
			eor(operand);
			done_flag = true;
		}
		break;
	case 0x59:        /* EOR absolute,Y */
	case 0x5d:        /* EOR absolute,X */
		eor(operand);
		done_flag = true;
		break;
	case 0x60:        /* RTS */
		pc |= (((uint16_t)pull())<<8);
		break;
	case 0x66:        /* ROR zero page */
		operand = ror(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x6c:        /* JMP (ind) */
		pc |= (uint16_t)read_byte(opaddr) << 8;
		done_flag = true;
		break;
	case 0x71:        /* ADC (indirect),Y */
		if (!pagedelay) {
			adc(operand);
			done_flag = true;
		}
		break;
	case 0x79:        /* ADC absolute,Y */
	case 0x7d:        /* ADC absolute,X */
		adc(operand);
		done_flag = true;
		break;
	case 0x99:        /* STA absolute, Y */
	case 0x9d:        /* STA absolute, X */
		write_byte(opaddr, a);
		done_flag = true;
		break;
	case 0xb1:        /* LDA (ind),Y */
		if (!pagedelay) {
			a = operand;
			set_nz(a);
			done_flag = true;
		}
		break;
	case 0xb9:        /* LDA absolute, Y */
	case 0xbd:        /* LDA absolute, X */
		a = operand;
		set_nz(a);
		done_flag = true;
		break;
	case 0xbc:        /* LDY absolute, X */
		y = read_byte(opaddr + x);
		set_nz(y);
		done_flag = true;
		break;
	case 0xbe:        /* LDX absolute, Y */
		x = read_byte(opaddr + y);
		set_nz(x);
		done_flag = true;
		break;
	case 0xc6:        /* DEC zero */
		operand--;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
	case 0xd1:        /* CMP (ind),Y */
		if (!pagedelay) {
			cmp(a, operand);
			done_flag = true;
		}
		break;
	case 0xd9:        /* CMP absolute,Y */
	case 0xdd:        /* CMP absolute,X */
		cmp(a, operand);
		done_flag = true;
		break;
	case 0xe6:        /* INC zero */
		operand++;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
	case 0xf1:        /* SBC (ind),Y */
		if (!pagedelay) {
			sbc(operand);
			done_flag = true;
		}
		break;
	case 0xf9:        /* SBC absolute,Y */
	case 0xfd:        /* SBC absolute,X */
		sbc(operand);
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x07:	  /* SLO zero */
		write_byte(opaddr, slo(operand));
		done_flag = true;
		break;
	case 0x27:	  /* RLA zero */
		write_byte(opaddr, rla(operand));
		done_flag = true;
		break;
	case 0x47:	  /* SRE zero */
		write_byte(opaddr, sre(operand));
		done_flag = true;
		break;
	case 0x67:	  /* RRA zero */
		write_byte(opaddr, rra(operand));
		done_flag = true;
		break;
	case 0xb3:	  /* LAX (ind), Y */
		if (!pagedelay) {
			a = operand;
			x = a;
			set_nz(a);
			done_flag = true;
		}
		break;
	case 0xbf:	  /* LAX absolute,Y */
		a = read_byte(opaddr + y);
		x = a;
		done_flag = true;
		break;
	case 0xc7:	  /* DCP zero */
		write_byte(opaddr, dcp(operand));
		done_flag = true;
		break;
	case 0xe7:	  /* ISC zero */
		write_byte(opaddr, isc(operand));
		done_flag = true;
		break;
	case 0x1c:	  /* NOP absolute,X */
	case 0x3c:
	case 0x5c:
	case 0x7c:
	case 0xdc:
	case 0xfc:
		read_byte(opaddr + x);
		done_flag = true;
		break;
#endif // ILL6502
	} // switch(opcode)

	return done_flag;
}

bool
Cpu6502::cycle5(void)
{
	bool done_flag = false;

	// Group one instructions
	switch (opcode & 0x1f) {
	case 0x01: /* (ind,X) */
	case 0x11: /* (ind),Y */
		if (opcode != 0x81 && opcode != 0x91)  /* STA */
			operand = read_byte(opaddr);
		break;
	case 0x1e: /* absolute,X except LDX which already terminated */
		// write back unmodified value.
		write_byte(opaddr, operand);
		break;
#ifdef ILL6502
	case 0x03: /* (ind,X) */
	case 0x13: /* (ind),Y */
		if (opcode != 0x83) /* SAX */
			operand = read_byte(opaddr);
		break;
	case 0x1b: /* absolyte,Y except LAS, already terminated */
	case 0x1f: /* absolute,X except LAX, already terminated */
		// write back unmodified value.
		write_byte(opaddr, operand);
		break;
#endif // ILL6502
	}

	switch (opcode) {
	case 0x00:        /* BRK */
		pc = read_byte(IRQ_VECTOR);
		break;
	case 0x01:        /* ORA (ind,X) */
	case 0x11:        /* ORA (ind),Y */
		orr(operand);
		done_flag = true;
		break;
	case 0x0e:        /* ASL absolute */
	case 0x16:        /* ASL zero,X */
		operand = asl(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x20:        /* JSR absolute */
		opaddr = operand | (uint16_t)read_byte(pc++) << 8;
		pc = opaddr;
		done_flag = true;
		break;
	case 0x21:        /* AND (ind,X) */
	case 0x31:        /* AND (ind),Y */
		andd(operand);
		done_flag = true;
		break;
	case 0x2e:        /* ROL absolute */
	case 0x36:        /* ROL zero, X */
		operand = rol(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x40:        /* RTI */
		pc |= (((uint16_t)pull())<<8);
		done_flag = true;
		break;
	case 0x41:        /* EOR (ind, X) */
	case 0x51:        /* EOR (indirect), Y */
		eor(operand);
		done_flag = true;
		break;
	case 0x4e:        /* LSR absolute */
	case 0x56:        /* LSR zero, X */
		operand = lsr(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x60:        /* RTS */
		(void)read_byte(pc);
		pc++;
		done_flag = true;
		break;
	case 0x61:        /* ADC (ind,X) */
	case 0x71:        /* ADC (indirect),Y */
		adc(operand);
		done_flag = true;
		break;
	case 0x6e:        /* ROR absolute */
	case 0x76:        /* ROR zero, X */
		operand = ror(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x81:        /* STA (ind, X) */
	case 0x91:        /* STA (ind),Y */
		write_byte(opaddr, a);
		done_flag = true;
		break;
	case 0xa1:        /* LDA (ind, X) */
	case 0xb1:        /* LDA (ind),Y */
		a = operand;
		set_nz(a);
		done_flag = true;
		break;
	case 0xc1:        /* CMP (ind, X) */
	case 0xd1:        /* CMP (ind),Y */
		cmp(a, operand);
		done_flag = true;
		break;
	case 0xce:        /* DEC absolute */
	case 0xd6:        /* DEC zero,X */
		operand--;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
	case 0xe1:        /* SBC (ind,X) */
	case 0xf1:        /* SBC (ind),Y */
		sbc(operand);
		done_flag = true;
		break;
	case 0xee:        /* INC absolute */
	case 0xf6:        /* INC zero,X */
		operand++;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x0f:	  /* SLO absolute */
	case 0x17:	  /* SLO zero,X */
		operand = slo(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x2f:	  /* RLA absolute */
	case 0x37:	  /* RLA zero,X */
		operand = rla(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x4f:	  /* SRE absolute */
	case 0x57:	  /* SRE zero,X */
		operand = sre(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x6f:	  /* RRA absolute */
	case 0x77:	  /* RRA zero,X */
		operand = rra(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x83:	  /* SAX (ind, X) */
		write_byte(opaddr, a & x);
		done_flag = true;
		break;
	case 0xa3:	  /* LAX (ind, X) */
	case 0xb3:	  /* LAX (ind),Y */
		a = operand;
		x = a;
		set_nz(a);
		done_flag = true;
		break;
	case 0xcf:	  /* DCP absolute */
	case 0xd7:	  /* DCP zero,X */
		operand = dcp(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xef:	  /* ISC absolute */
	case 0xf7:	  /* ISC zero,X */
		operand = isc(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
#endif // ILL6502
	} // switch (operand)

	return done_flag;
}

bool
Cpu6502::cycle6(void)
{
	bool done_flag = false;

#ifdef ILL6502
	switch (opcode & 0x1f) {
	case 0x03: /* (ind,X) */
	case 0x13: /* (ind),Y */
		/* write back original value. */
		write_byte(opaddr, operand);
		pagedelay = false;
		break;
	}
#endif // ILL6502

	switch (opcode) {
	case 0x00:        /* BRK */
		pc |= (uint16_t)read_byte(IRQ_VECTOR + 1) << 8;
		p |= P_I;
#ifdef CPU65C02
		p &= ~P_D;
#endif
		done_flag = true;
		break;
	case 0x1e:        /* ASL absolute,X */
		operand = asl(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x3e:        /* ROL absolute, X */
		operand = rol(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x5e:        /* LSR absolute,X */
		operand = lsr(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x7e:        /* ROR absolute,X */
		operand = ror(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xde:        /* DEC absolute,X */
		operand--;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
	case 0xfe:        /* INC absolute,X */
		operand++;
		write_byte(opaddr, operand);
		set_nz(operand);
		done_flag = true;
		break;
#ifdef ILL6502
	case 0x1b:	  /* SLO absolute,Y */
	case 0x1f:	  /* SLO absolute,X */
		operand = slo(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x3b:	  /* RLA absolute,Y */
	case 0x3f:	  /* RLA absolute,X */
		operand = rla(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x5b:	  /* SRE absolute,Y */
	case 0x5f:	  /* SRE absolute,X */
		operand = sre(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x7b:	  /* RRA absolute,Y */
	case 0x7f:	  /* RRA absolute,X */
		operand = rra(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xdb:	  /* DCP absolute,Y */
	case 0xdf:	  /* DCP absolute,X */
		operand = dcp(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xfb:	  /* ISC absolute,Y */
	case 0xff:	  /* ISC absolute,X */
		operand = isc(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
#endif // ILL6502
	} // switch (opcode)

	return done_flag;
}

#ifdef ILL6502
bool
Cpu6502::cycle7(void)
{
	bool done_flag = false;

	switch (opcode) {
	case 0x03:	  /* SLO (ind,X) */
	case 0x13:	  /* SLO (ind),Y */
		operand = slo(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x23:	  /* RLA (ind,X) */
	case 0x33:	  /* RLA (ind),Y */
		operand = rla(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x43:	  /* SRE (ind,X) */
	case 0x53:	  /* SRE (ind),Y */
		operand = sre(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0x63:	  /* RRA (ind,X) */
	case 0x73:	  /* RRA (ind),Y */
		operand = rra(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xc3:	  /* DCP (ind,X) */
	case 0xd3:	  /* DCP (ind),Y */
		operand = dcp(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	case 0xe3:	  /* ISC (ind,X) */
	case 0xf3:	  /* ISC (ind),Y */
		operand = isc(operand);
		write_byte(opaddr, operand);
		done_flag = true;
		break;
	}

	return done_flag;
}
#endif // ILL6502

#define BPCYCLE 99

bool
Cpu6502::cycle(void)
{
	bool done_flag = false;

	if (doing_int) {
		dointcycle();
		return true;
	}

	switch (cyclenum) {
	case 0: // cycle 0
		pagedelay = false;

		/* Handle interrupts. */
		if (needs_nmi || (irq_signal && (p & P_I) == 0)) {
			/* Interrupt! */
			doing_int = true;
			dointcycle();
			return true;
		}

		if (nbpts > 0) {
			if (!hitbrk) {
				for (int i = 0; i < nbpts; i++)
					if (pc == bpaddrs[i]) {
						hitbrk = true;
						return false;
					}
			}
			else
				hitbrk = false;
		}

		/* Fetch opcode. */
		opcode = read_byte(pc++);

		DPRINTF(5, "Cpu6502::%s: pc=0x%04x opcode=0x%02x\n", __func__,
			pc - 1, opcode);
		DPRINTF(5, "  a=0x%02x x=0x%02x y=0x%02x p=0x%02x sp=0x%02x\n",
			a, x, y, p, sp);

		/* Treat illegal opcodes as NOP. */
		if (instrlen[opcode] == 0) {
			DPRINTF(1, "Cpu6502::%s: Illegal opcode: 0x%02x "
				"pc = 0x%04x\n", __func__, opcode, pc - 1);
			done_flag = true;
			if (0) { // future break on illegal opcode
				pc--;
				hitbrk = true;
				return false;
			}
		}
		break; // end of first cycle

	case 1: // cycle 1
		done_flag = cycle1();
		break; // end of 2nd cycle

	case 2: // cycle 2
		done_flag = cycle2();
		break;

	case 3: // cycle 3
		done_flag = cycle3();
		break;

	case 4: // cycle 4
		done_flag = cycle4();
		break;

	case 5: // cycle 5
		done_flag = cycle5();
		break;

	case 6: // cycle 6
		done_flag = cycle6();
		break;
#ifdef ILL6502
	case 7: // cycle 7
		done_flag = cycle7();
		break;
#endif // ILL6502

	case BPCYCLE: // single-step pseudo cycle
		step_flag = false;
		cyclenum = 0;
		return false;
	default:
		DPRINTF(1, "CPU6502::%s: opcode %02x didn't finish.\n",
			__func__, opcode);
	}

#if DEBUG6502 > 1
	if (done_flag)
		checkCycleCount();
#endif

	if (done_flag) {
		if (step_flag)
			cyclenum = BPCYCLE;
		else
			cyclenum = 0;
	} else
		cyclenum++;

	return true;
}

void
Cpu6502::setIrq(bool level)
{
	irq_signal = level;
}

void
Cpu6502::nmiReq(void)
{
	needs_nmi = true;
}

void
Cpu6502::stepCpu(void)
{
	DPRINTF(1, "Cpu6502::%s:\n", __func__);
	step_flag = true;
}

void
Cpu6502::setBreak(int i, uint16_t addr)
{
	if (i >= 0 && i < NBPTS)
		bpaddrs[i] = addr;
}

void
Cpu6502::setNumBreaks(int _n)
{
	if (_n < 0)
		nbpts = 0;
	else if (_n >= NBPTS)
		nbpts = NBPTS;
	else
		nbpts = _n;
}

#if DEBUG6502 > 1
void
Cpu6502::checkCycleCount(void)
{
	int expected = cycle_count[opcode] + (pagedelay ? 1 : 0);

	switch (opcode) {
	case 0x10:        /* BPL */
		if ((p & P_N) == 0)
			expected++;
		break;
	case 0x30:        /* BMI */
		if ((p & P_N) != 0)
			expected++;
		break;
	case 0x50:        /* BVC */
		if ((p & P_V) == 0)
			expected++;
		break;
	case 0x70:        /* BVS */
		if ((p & P_V) != 0)
			expected++;
		break;
	case 0x90:        /* BCC */
		if ((p & P_C) == 0)
			expected++;
		break;
	case 0xb0:        /* BCS */
		if ((p & P_C) != 0)
			expected++;
		break;
	case 0xd0:        /* BNE */
		if ((p & P_Z) == 0)
			expected++;
		break;
	case 0xf0:        /* BEQ */
		if ((p & P_Z) != 0)
			expected++;
		break;
	case 0x91: /* STA (ind),Y */
	case 0x99: /* STA absolute, Y */
	case 0x9d: /* STA absoute, X */
		if (pagedelay)
			expected--;
		break;
	}
	if (cyclenum + 1 != expected)
		DPRINTF(1, "Cpu6502::%s: XXX cycle err? opcode=0x%02x "
			"got %d cycles expecting %d,\n",  __func__,
			opcode, cyclenum + 1, expected);
}
#endif // DEBUG6502 > 1
