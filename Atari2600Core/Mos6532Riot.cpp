//
// Copyright (c) 2025 Thomas Skibo.
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

// Mos6532Riot.cpp

#include <stdint.h>

#include "Mos6532Riot.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

// An incomplete MOS 6532 (missing interrupts and flags)
#define PORTA	0x00
#define DDRA	0x01
#define PORTB	0x02
#define DDRB	0x03
#define INTIM	0x04
#define INSTAT	0x05
#define    INSTAT_TIM	(1 << 7)
#define    INSTAT_PA7	(1 << 6)
#define TIM1T	0x14
#define TIM8T	0x15
#define TIM64T	0x16
#define T1024T	0x17

Mos6532Riot::Mos6532Riot(void)
{
	DPRINTF(1, "Mos6532Riot::%s:\n", __func__);

	porta_in = 0xff;
	portb_in = 0xff;
}

uint8_t
Mos6532Riot::read(uint16_t addr)
{
	DPRINTF(3, "Mos6532Riot::%s: addr=0x%02x\n", __func__, addr);

	uint8_t d8 = 0xff;

	if ((addr & 4) == 0) {
		switch (addr & 7) {
		case PORTA:
			d8 = (porta_in & ~ddra) | (porta_out & ddra);
			break;
		case DDRA:
			d8 = ddra;
			break;
		case PORTB:
			d8 = (portb_in & ~ddrb) | (portb_out & ddrb);
			break;
		case DDRB:
			d8 = ddrb;
			break;
		}
	} else if ((addr & 5) == 4) {
		// INTIM
		d8 = intim;
		instat &= ~INSTAT_TIM;
	} else if ((addr & 5) == 5) {
		// INSTAT
		d8 = instat;
		instat &= ~INSTAT_PA7;
	} else {
		DPRINTF(1, "Mos6532Riot::%s: unknown read: 0x%02x\n",
			__func__, addr);
	}

	return d8;
}

void
Mos6532Riot::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(2, "Mos6532Riot::%s: addr=0x%02x d8=0x%02x\n", __func__,
		addr, d8);

	if ((addr & 4) == 0) {
		switch (addr & 7) {
		case PORTA:
			porta_out = d8;
			break;
		case DDRA:
			ddra = d8;
			break;
		case PORTB:
			portb_out = d8;
			break;
		case DDRB:
			ddrb = d8;
			break;
		}
	} else if ((addr & 0x14) == 0x14) {
		// TIMxT:
		intim = d8 - 1;
		timcnt = 0;
		instat &= ~INSTAT_TIM;
		switch (addr & 3) {
		case 0:
			timintvl = 1;
			break;
		case 1:
			timintvl = 8;
			break;
		case 2:
			timintvl = 64;
			break;
		case 3:
			timintvl = 1024;
			break;
		}
	} else if ((addr & 0x14) == 4) {
		// edge detect control.
		pa7_edge = (addr & 1) != 0 ? 0x80 : 0;
	} else {
		DPRINTF(1, "Mos6532Riot::%s: unknown write: "
			"addr=0x%02x d8=0x%02x\n", __func__, addr, d8);
	}
}

void
Mos6532Riot::setPortA(uint8_t _set, uint8_t _reset)
{
	DPRINTF(2, "Mos6532Riot::%s: set=0x%02x reset=0x%02x\n", __func__,
		_set, _reset);

	// PA7 edge detect.
	if ((ddra & 0x80) == 0 && ((pa7_edge ? _set : _reset) & 0x80) != 0 &&
	    (porta_in & 0x80) != pa7_edge)
		instat |= INSTAT_PA7;

	porta_in &= ~_reset;
	porta_in |= _set;
}

void
Mos6532Riot::setPortB(uint8_t _set, uint8_t _reset)
{
	DPRINTF(2, "Mos6532Riot::%s: set=0x%02x reset=0x%02x\n", __func__,
		_set, _reset);

	portb_in &= ~_reset;
	portb_in |= _set;
}

void
Mos6532Riot::reset(void)
{
	DPRINTF(1, "Mos6532Riot::%s:\n", __func__);

	ddra = 0;
	ddrb = 0;
	intim = 0;
	instat = 0;

	timcnt = 0;
	timintvl = 1024;
	pa7_edge = 0;
}

void
Mos6532Riot::cycle(void)
{
	DPRINTF(4, "Mos6532Riot::%s:\n", __func__);

	if (++timcnt == timintvl || (instat & INSTAT_TIM) != 0) {
		timcnt = 0;
		if (--intim == 0xff)
			instat |= INSTAT_TIM;
	}
}
