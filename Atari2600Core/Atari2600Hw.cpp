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

// Atari2600Hw.cpp

#include <stdint.h>

#include "Atari2600.h"
#include "Atari2600Hw.h"
#include "Atari2600Video.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

#define RAM_MASK	0x7f
#define ROM_MASK	0xfff
#define TIA_MASK	0x3f
#define RIOT_MASK	0x7f

// F8 bank switching
#define BANK_SIZE	0x1000
#define BANK0_ADDR	0xff8
#define BANK1_ADDR	0xff9

void
Atari2600Hw::reset(void)
{
	DPRINTF(1, "Atari2600Hw::%s:\n", __func__);

	video->reset();
	tia.reset();
	riot.reset();
	bank = false;

	for (int i = 0; i < NUMPADDLES; i++)
		paddle_val[i] = 0;
	paddle_ctr = 0;
}

void
Atari2600Hw::cycle(void)
{
	DPRINTF(4, "Atari2600Hw::%s:\n", __func__);

	riot.cycle();
}

void
Atari2600Hw::cycle3(void)
{
	DPRINTF(4, "Atari2600Hw::%s:\n", __func__);

	if (tia.dumpDI03() && paddle_ctr != PADDLECTRMAX) {
		paddle_ctr = PADDLECTRMAX;
		tia.setInput(0, 15);
	} else if (paddle_ctr > 0) {
		paddle_ctr--;
		for (int i = 0; i < NUMPADDLES; i++)
			if (paddle_ctr / (PADDLECTRMAX / PADDLE_VAL_MAX) <=
			    paddle_val[i])
				tia.setInput((1 << i), 0);
	}

	tia.cycle();
}

void
Atari2600Hw::readRam(uint16_t addr, uint8_t *data, int len)
{
	DPRINTF(4, "Atari2600Hw::%s:addr=0x%04x len=%d\n", __func__,
		addr, len);

	for (int i = 0; i < len; i++)
		data[i] = ram[(addr & RAM_MASK) + i];
}

void
Atari2600Hw::writeRam(uint16_t addr, const uint8_t *data, int len)
{
	DPRINTF(4, "Atari2600Hw::%s:addr=0x%04x len=%d\n", __func__,
		addr, len);

	for (int i = 0; i < len; i++)
		ram[(addr & RAM_MASK) + i] = data[i];
}

void
Atari2600Hw::setRom(const uint8_t *data, int len)
{
	DPRINTF(4, "Atari2600Hw::%s: len=%d\n", __func__, len);

	if (len > ROM_MAX_SIZE)
		len = ROM_MAX_SIZE;

	for (int i = 0; i < len; i++)
		rom[i] = data[i];

	romsz = len;
	bank = false;
}

void
Atari2600Hw::setJoyLeft(uint8_t _set, uint8_t _reset)
{
	DPRINTF(2, "Atari2600Hw::%s: _set=0x%02x _reset=0x%02x\n", __func__,
		_set, _reset);

	riot.setPortA((_reset << 4) & 0xf0, (_set << 4) & 0xf0);

	tia.setInput((_reset & JOY_TRIGGER) ? 0x10 : 0,
		     (_set & JOY_TRIGGER) ? 0x10 : 0);
}

void
Atari2600Hw::setJoyRight(uint8_t _set, uint8_t _reset)
{
	DPRINTF(2, "Atari2600Hw::%s: _set=0x%02x _reset=0x%02x\n", __func__,
		_set, _reset);

	riot.setPortA(_reset & 0x0f, _set & 0x0f);

	tia.setInput((_reset & JOY_TRIGGER) ? 0x20 : 0,
		     (_set & JOY_TRIGGER) ? 0x20 : 0);
}

void
Atari2600Hw::setPaddle(int p, int val)
{
	DPRINTF(2, "Atari2600Hw::%s: p=%d val=%d\n", __func__, p, val);

	paddle_val[p] = val;
}

/////////////////////// MemSpace interface to cpu6502 //////////////////////

uint8_t
Atari2600Hw::read(uint16_t addr)
{
	DPRINTF(4, "Atari2600Hw::%s: addr=0x%04x\n", __func__, addr);

	// A12=1: ROM
	if ((addr & 0x1000) != 0) {
		addr &= ROM_MASK;

		// Do F8 bank switching if loaded rom is >4K
		if (romsz > BANK_SIZE) {
			if (addr == BANK1_ADDR)
				bank = true;
			else if (addr == BANK0_ADDR)
				bank = false;
		}

		return rom[addr + (bank ? BANK_SIZE : 0)];
	}
	// A12=0, A7=1, A9=0: RAM
	else if ((addr & 0x280) == 0x80)
		return ram[addr & RAM_MASK];
	// A12=0, A7=1, A9=1: RIOT
	else if ((addr & 0x280) == 0x280)
		return riot.read(addr & RIOT_MASK);
	// A12=0, A7=0: TIA
	else
		return tia.read(addr & TIA_MASK);
}

void
Atari2600Hw::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(3, "Atari2600Hw::%s: addr=0x%04x d8=0x%02x\n", __func__,
		addr, d8);

	// A12=1: ROM
	if ((addr & 0x1000) != 0) {
		addr &= ROM_MASK;

		// Do F8 bank switching if loaded rom is >4K
		if (addr == BANK1_ADDR && romsz > BANK_SIZE)
			bank = true;
		else if (addr == BANK0_ADDR)
			bank = false;
	}
	// A12=0, A7=1, A9=0: RAM
	else if ((addr & 0x280) == 0x80)
		ram[addr & RAM_MASK] = d8;
	// A12=0, A7=1, A9=1: RIOT
	else if ((addr & 0x280) == 0x280)
		riot.write(addr & RIOT_MASK, d8);
	// A12=0, A7=0: TIA
	else
		tia.write(addr & TIA_MASK, d8);
}
