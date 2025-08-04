//
// Copyright (c) 2020 Thomas Skibo.
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

// Cpu6502.h

#ifndef __CPU6502_H__
#define __CPU6502_H__

#include "MemSpace.h"

#define NBPTS	4

class Cpu6502 {
private:
	uint8_t		read_byte(uint16_t addr);
	void		write_byte(uint16_t addr, uint8_t d8);
	uint16_t	read_word(uint16_t addr);

	void		dointcycle(void);
	bool		cycle1(void);
	bool		cycle2(void);
	bool		cycle3(void);
	bool		cycle4(void);
	bool		cycle5(void);
	bool		cycle6(void);
#ifdef ILL6502
	bool		cycle7(void);
#endif
	void		push(uint8_t d8);
	uint8_t		pull(void);
	void		set_nz(uint8_t d8);
	uint8_t		asl(uint8_t d8);
	uint8_t		lsr(uint8_t d8);
	uint8_t		rol(uint8_t d8);
	uint8_t		ror(uint8_t d8);
	void		orr(uint8_t d8);
	void		andd(uint8_t d8);
	void		eor(uint8_t d8);
	void		adc(uint8_t d8);
	void		sbc(uint8_t d8);
	void		cmp(uint8_t left, uint8_t right);
	void		bit(uint8_t d8);
	void		branch(uint8_t operand);
#ifdef ILL6502
	void		arr(uint8_t operand);
	uint8_t		slo(uint8_t operand);
	uint8_t		rla(uint8_t operand);
	uint8_t		sre(uint8_t operand);
	uint8_t		rra(uint8_t operand);
	uint8_t		dcp(uint8_t operand);
	uint8_t		isc(uint8_t operand);
#endif // ILL6502
#if DEBUG6502 > 1
	void		checkCycleCount(void);
#endif
#ifdef CPU65C02
	uint16_t	ind(uint16_t operand);
	void		bbr(uint16_t operand, uint8_t bit);
	void		bbs(uint16_t operand, uint8_t bit);
	void		rmb(uint8_t operand, uint8_t bit);
	void		smb(uint8_t operand, uint8_t bit);
#endif // CPU65C02

	uint8_t		a;
	uint8_t		x;
	uint8_t		y;
	uint8_t		sp;
	uint8_t		p;
	uint16_t	pc;

	bool		irq_signal;
	bool		needs_nmi;
	bool		doing_int;
	bool		pagedelay;
	bool		step_flag;
	short		cyclenum;
	uint8_t		opcode;
	uint8_t		operand;
	uint16_t	opaddr;
	int		nbpts;
	uint16_t	bpaddrs[NBPTS];
	bool		hitbrk;

	MemSpace	*mem;

public:
	Cpu6502(MemSpace *memspace);

	void		reset(void);
	bool		cycle(void);
	void		setIrq(bool level);
	void		nmiReq(void);

	// Setter/getters for debuggers.
	uint16_t	getPc(void)
	{ return pc; }
	void		setPc(uint16_t pc)
	{ this->pc = pc; }
	uint8_t		getA(void)
	{ return a; }
	uint8_t		getX(void)
	{ return x; }
	uint8_t		getY(void)
	{ return y; }
	uint8_t		getSp(void)
	{ return sp; }
	uint8_t		getP(void)
	{ return p; }
	MemSpace	*getMemSpace(void)
	{ return mem; }
	void		stepCpu(void);
	void		setBreak(int i, uint16_t addr);
	void		setNumBreaks(int _n);
};

#endif // __CPU6502_H__
