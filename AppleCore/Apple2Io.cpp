//
// Copyright (c) 2021 Thomas Skibo.
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


// Apple2Io.cpp

#include <stdint.h>

#include "Apple2Io.h"

#include "Cpu6502.h"
#include "Apple2Video.h"
#include "Apple2Disk2.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

#define IO_PROM_MASK		0x0700
#define IO_PROM_SLOT(a)		(((a) & IO_PROM_MASK) >> 8)
#define IO_EXTIO_MASK		0x0080
#define IO_EXTIO_SLOT(a)	(((a) & 0x70) >> 4)
#define IO_BUILTIN_MASK		0x0070

#define IO_KEYDAT_ADDR		0x0000
#define IO_KEYCLR_ADDR		0x0010
#define IO_CASSOUT_ADDR		0x0020
#define IO_SPKROUT_ADDR		0x0030
#define IO_UTIL_ADDR		0x0040
#define IO_GRFX_ADDR		0x0050
#define IO_GRFX_MASK		0x000e
#define IO_GRFX_MODE_ADDR	0x0000
#define IO_MIX_ADDR		0x0002
#define IO_PAGE_ADDR		0x0004
#define IO_HIRES_ADDR		0x0006
#define IO_GAME_ADDR		0x0060
#define IO_GAME_MASK		0x0007
#define IO_CIN_ADDR		0x0000
#define IO_PB1_ADDR		0x0001
#define IO_PB2_ADDR		0x0002
#define IO_PB3_ADDR		0x0003
#define IO_GC0_ADDR		0x0004
#define IO_GC1_ADDR		0x0005
#define IO_GC2_ADDR		0x0006
#define IO_GC3_ADDR		0x0007
#define IO_GCSTRB_ADDR		0x0070

extern const uint8_t disk2Rom[];

void
Apple2Io::reset(void)
{
	DPRINTF(1, "Apple2Io::%s:\n", __func__);

	keycode = 0;
	paddlemask = 0;
	disk.reset();
}

void
Apple2Io::setkey(uint8_t d8)
{
	DPRINTF(1, "Apple2Io::%s: key=0x%02x\n", __func__, d8);

	keycode = d8;
}

void
Apple2Io::setPaddle(int n, float val)
{
	DPRINTF(1, "Apple2Io::%s: paddle=%d val=%f\n", __func__, n, val);

	paddle[n] = (short)(val * 28.05);
}

void
Apple2Io::setButton(int n, bool flag)
{
	DPRINTF(1, "Apple2Io::%s: button=%d flag=%d\n", __func__, n, flag);

	button[n] = flag;
}

// Perform actions that are side-effects of either read or write.
void
Apple2Io::reference(uint16_t addr)
{
	switch (addr & IO_BUILTIN_MASK) {
	case IO_KEYCLR_ADDR:
		keycode &= 0x7f;
		break;
	case IO_GRFX_ADDR:
		switch (addr & IO_GRFX_MASK) {
		case IO_GRFX_MODE_ADDR:
			video->setGfx((addr & 1) == 0);
			break;
		case IO_MIX_ADDR:
			video->setMix((addr & 1) != 0);
			break;
		case IO_PAGE_ADDR:
			video->setPage((addr & 1) != 0);
			break;
		case IO_HIRES_ADDR:
			video->setHires((addr & 1) != 0);
			break;
		}
		break;
	case IO_GCSTRB_ADDR:
		// Game controller strobe.
		paddlemask = 0xf;
		for (int i = 0; i < 4; i++)
			paddlecount[i] = paddle[i];
		break;
	}
}

uint8_t
Apple2Io::read(uint16_t addr)
{
	uint8_t d8 = 0;

	if ((addr & IO_PROM_MASK) != 0)
		// Device PROMS.
		if (IO_PROM_SLOT(addr) == 6 && disk.haveNib())
			d8 = disk2Rom[addr & 0xff];
		else
			d8 = 0xff;
	else if ((addr & IO_EXTIO_MASK) != 0)
		if (IO_EXTIO_SLOT(addr) == 6)
			d8 = disk.read(addr & 0x0f);
		else
			d8 = 0xee;
	else {
		switch (addr & IO_BUILTIN_MASK) {
		case IO_KEYDAT_ADDR:
			d8 = keycode;
			break;
		case IO_GAME_ADDR:
			switch (addr & IO_GAME_MASK) {
			case IO_PB1_ADDR:
				d8 = button[0] ? 0x80 : 0x00;
				break;
			case IO_PB2_ADDR:
				d8 = button[1] ? 0x80 : 0x00;
				break;
			case IO_PB3_ADDR:
				d8 = button[2] ? 0x80 : 0x00;
				break;
			case IO_GC0_ADDR:
				d8 = (paddlemask & 1) ? 0x80 : 0x00;
				break;
			case IO_GC1_ADDR:
				d8 = (paddlemask & 2) ? 0x80 : 0x00;
				break;
			case IO_GC2_ADDR:
				d8 = (paddlemask & 4) ? 0x80 : 0x00;
				break;
			case IO_GC3_ADDR:
				d8 = (paddlemask & 8) ? 0x80 : 0x00;
				break;
			}
			break;
		}
		reference(addr);
	}

	DPRINTF(3, "Apple2Io:read(addr=0x%04x) = 0x%02x\n", addr, d8);

	return d8;
}

void
Apple2Io::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(3, "Apple2Io:write(addr=0x%04x, d8=0x%02x)\n", addr, d8);

	if ((addr & IO_PROM_MASK) != 0)
		// Device PROMS
		;
	else if ((addr & IO_EXTIO_MASK) != 0) {
		// Device I/O
		if (IO_EXTIO_SLOT(addr) == 6)
			disk.write(addr & 0x0f, d8);
	} else
		reference(addr);
}

void
Apple2Io::cycle(void)
{
	if (paddlemask != 0) {
		for (int i = 0; i < 4; i++)
			if (paddlecount[i] == 0 || --paddlecount[i] == 0)
				paddlemask &= ~(1 << i);
	}
}
