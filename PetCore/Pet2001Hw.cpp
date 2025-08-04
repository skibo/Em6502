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

// Pet2001Hw.cpp

#include <stdint.h>

#include "Pet2001Hw.h"

#include "MemSpace.h"
#include "PetVideo.h"

void
Pet2001Hw::reset(void)
{
	io.reset();

	if (video)
		video->reset();

	for (int i = 0; i < MAX_RAM_SIZE; i++)
		ram[i] = 0x44;
}

void
Pet2001Hw::cycle(void)
{
	io.cycle();
}

// Write ROM space.
void
Pet2001Hw::writeRom(uint16_t addr, const uint8_t *romdata, int len)
{
	for (int i = 0; i < len; i++)
		if (addr > IO_ADDR)
			rom[addr - ROM_ADDR - IO_SIZE + i] = romdata[i];
		else
			rom[addr - ROM_ADDR + i] = romdata[i];
}

// ******************* MemSpace interface to cpu6502 **********************

uint8_t
Pet2001Hw::read(uint16_t addr)
{
	if (addr < ramsize)
		return ram[addr];
	else if (addr >= ROM_ADDR && addr < IO_ADDR)
		return rom[addr - ROM_ADDR];
	else if (addr >= IO_ADDR + IO_SIZE)
		return rom[addr - ROM_ADDR - IO_SIZE];
	else if (addr >= VIDRAM_ADDR && addr < VIDRAM_ADDR + VIDRAM_SIZE) {
		if (video)
			return video->read(addr - VIDRAM_ADDR);
		else
			return 0xaa;
	}
	else if (addr >= IO_ADDR && addr < IO_ADDR + IO_SIZE)
		return io.read(addr - IO_ADDR);

	return 0x55;
}

void
Pet2001Hw::write(uint16_t addr, uint8_t d8)
{
	if (addr <ramsize)
		ram[addr] = d8;
#ifdef WRITEROM
	else if (addr >= ROM_ADDR && addr < IO_ADDR)
		rom[addr - ROM_ADDR] = d8;
	else if (addr >= IO_ADDR + IO_SIZE)
		rom[addr - ROM_ADDR - IO_SIZE] = d8;
#endif
	else if (addr >= VIDRAM_ADDR && addr < VIDRAM_ADDR + VIDRAM_SIZE) {
		if (video)
			video->write(addr - VIDRAM_ADDR, d8);
	}
	else if (addr >= IO_ADDR && addr < IO_ADDR + IO_SIZE)
		io.write(addr - IO_ADDR, d8);
}
