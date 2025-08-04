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

// Apple2Hw.h

#ifndef __APPLE2HW_H__
#define __APPLE2HW_H__

#include "MemSpace.h"
#include "Apple2Io.h"

class Cpu6502;
class Apple2Video;
class Apple2Disk2;

#define RAM_SIZE	0xc000	// 48K
#define ROM_SIZE	0x3000	// 12K

class Apple2Hw : public MemSpace {
private:
	Apple2Io	io;
	Apple2Video	*video;
	uint8_t		ram[RAM_SIZE];
	uint8_t		rom[ROM_SIZE];

public:
	Apple2Hw(Cpu6502 *, Apple2Video *);

	uint8_t	read(uint16_t addr);
	void 	write(uint16_t addr, uint8_t d8);

	void 	reset(void);
	void 	restart(void);
	void 	cycle(void);
	void 	setVideo(Apple2Video *video)
	{
		this->video = video;
		io.setVideo(video);
	}
	void 	writeRam(uint16_t addr, const uint8_t *data, int len);
	void 	readRam(uint16_t addr, uint8_t *data, int len);
	void	setRom(uint16_t addr, const uint8_t *data, int len);
	void 	setkey(uint8_t d8)
	{ io.setkey(d8); }
	void	setPaddle(int n, float val)
	{ io.setPaddle(n, val); }
	void	setButton(int n, bool flag)
	{ io.setButton(n, flag); }
	Apple2Disk2 *getDisk(void)
	{ return io.getDisk(); }
};

#endif // __APPLE2HW_H__
