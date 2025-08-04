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

// Pet2001Hw.h

#ifndef __PET2001HW_H__
#define __PET2001HW_H__

#include "MemSpace.h"
#include "Pet2001Io.h"

class Cpu6502;
class PetVideo;
class PetCassHw;
class PetIeeeHw;

#define MAX_RAM_SIZE	0x8000	// 32K
#define VIDRAM_ADDR	0x8000
#define VIDRAM_SIZE	0x1000	// 1K in 4K range
#define ROM_ADDR	0x9000
#define ROM_SIZE	0x6800	// 26K
#define IO_ADDR		0xe800
#define IO_SIZE		0x0800

class Pet2001Hw : public MemSpace {
private:
	Pet2001Io	io;
	PetVideo	*video;
	uint8_t		ram[MAX_RAM_SIZE];
	uint8_t		rom[ROM_SIZE];
	uint16_t	ramsize;

public:
	Pet2001Hw(Cpu6502 *cpu, PetVideo *video,
		  PetCassHw *cass, PetIeeeHw *ieee)
		: io(cpu, video, cass, ieee)
	{
		this->video = video;
		ramsize = MAX_RAM_SIZE;
	}

	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t d8);

	void reset(void);
	void cycle(void);
	void setVideo(PetVideo *video)
	{
		this->video = video;
		io.setVideo(video);
	}
	void setKeyrow(int row, uint8_t keyrow)
	{ io.setKeyrow(row, keyrow); }
	void setRamsize(int ramsize)
	{ this->ramsize = ramsize; }
	void writeRom(uint16_t addr, const uint8_t *data, int len);
	void setAudioBuf(uint8_t *buf, int len)
	{ io.setAudioBuf(buf, len); }
	int getAudioTail(void)
	{ return io.getAudioTail(); }
};

#endif // __PET2001HW_H__
