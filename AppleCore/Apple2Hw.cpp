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

// Apple2Hw.cpp

#include <stdint.h>

#include "Apple2Hw.h"

#include "Apple2Video.h"

#define IO_ADDR		0xc000
#define IO_SIZE		0x0800
#define TEXT_ADDR	0x0400
#define TEXT_SIZE	0x0800	// both pages
#define HIRES_ADDR	0x2000
#define HIRES_SIZE	0x4000	// both pages
#define ROM_ADDR	0xd000

extern const uint8_t apple2Rom[];

Apple2Hw::Apple2Hw(Cpu6502 *cpu, Apple2Video *video)
	: io(cpu, video)
{
	this->video = video;

	for (int i = 0; i < ROM_SIZE; i++)
		rom[i] = apple2Rom[i];
}

void
Apple2Hw::reset(void)
{
	io.reset();
}

void
Apple2Hw::restart(void)
{
	for (int i = 0; i < RAM_SIZE; i++)
		ram[i] = 0xaa;
	reset();
}

void
Apple2Hw::cycle(void)
{
	io.cycle();
}

void
Apple2Hw::readRam(uint16_t addr, uint8_t *data, int len)
{
	for (int i = 0; i < len; i++)
		data[i] = ram[addr + i];
}

void
Apple2Hw::writeRam(uint16_t addr, const uint8_t *data, int len)
{
	for (int i = 0; i < len; i++)
		ram[addr + i] = data[i];
}

void
Apple2Hw::setRom(uint16_t addr, const uint8_t *data, int len)
{
	if (addr < ROM_ADDR || (int)addr + len > ROM_ADDR + ROM_SIZE)
		return;

	for (int i = 0; i < len; i++)
		rom[addr - ROM_ADDR + i] = data[i];
}

/////////////////////// MemSpace interface to cpu6502 //////////////////////

uint8_t
Apple2Hw::read(uint16_t addr)
{
	if (addr < RAM_SIZE)
		return ram[addr];
	else if (addr >= ROM_ADDR && addr < ROM_ADDR + ROM_SIZE)
		return rom[addr - ROM_ADDR];
	else if (addr >= IO_ADDR && addr < IO_ADDR + IO_SIZE)
		return io.read(addr - IO_ADDR);

	return 0x55;
}

void
Apple2Hw::write(uint16_t addr, uint8_t d8)
{
	if (addr < RAM_SIZE) {
		ram[addr] = d8;
		if (video)
			video->write(addr, d8);
	} else if (addr >= IO_ADDR && addr < IO_ADDR + IO_SIZE)
		io.write(addr - IO_ADDR, d8);
}
