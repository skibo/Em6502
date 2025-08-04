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

// Apple2Io.h

#ifndef __APPLE2IO_H__
#define __APPLE2IO_H__

#include "MemSpace.h"
#include "Apple2Disk2.h"

class Cpu6502;
class Apple2Video;

class Apple2Io : MemSpace {
private:
	Cpu6502		*cpu;
	Apple2Video	*video;
	Apple2Disk2	disk;
	uint8_t		keycode;
	bool		button[3];
	short		paddle[4];
	short		paddlecount[4];
	uint8_t		paddlemask;

	void reference(uint16_t addr);
public:
	Apple2Io(Cpu6502 *cpu, Apple2Video *video)
		: cpu(cpu),
		  video(video)
	{
		reset();
		disk.reset();
	}
	void setVideo(Apple2Video *_video)
	{ video = _video; }

	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t d8);
	void setkey(uint8_t d8);
	void setPaddle(int n, float val);
	void setButton(int n, bool flag);
	void reset(void);
	void cycle(void);

	Apple2Disk2 *getDisk(void)
	{ return &disk; }
};

#endif // __APPLE2IO_H__
