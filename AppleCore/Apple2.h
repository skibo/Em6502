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

// Apple2.h

#ifndef __APPLE2_H__
#define __APPLE2_H__

#include "Cpu6502.h"
#include "Apple2Video.h"
#include "Apple2Hw.h"

class Apple2Disk2;

class Apple2 {
private:
	Cpu6502		cpu;
	Apple2Hw	applehw;

public:
	Apple2(Apple2Video *video = 0)
		: cpu(&applehw),
		  applehw(&cpu, video)
	{  }
	void		reset(void);
	void		restart(void);
	bool		cycle(void);
	void		setVideo(Apple2Video *video)
	{ applehw.setVideo(video); }
	void		readRam(uint16_t addr, uint8_t *data, int length)
	{ applehw.readRam(addr, data, length); }
	void		writeRam(uint16_t addr, const uint8_t *data,
			int length)
	{ applehw.writeRam(addr, data, length); }
	void		setRom(uint16_t addr, const uint8_t *data,
			int length)
	{ applehw.setRom(addr, data, length); }
	void		setkey(uint8_t d8)
	{ applehw.setkey(d8); }
	void		setPaddle(int n, float val)
	{ applehw.setPaddle(n, val); }
	void		setButton(int n, bool flag)
	{ applehw.setButton(n, flag); }
	Cpu6502		*getCpu(void)
	{ return &cpu; }
	Apple2Disk2	*getDisk(void)
	{ return applehw.getDisk(); }
};

#endif // __APPLE2_H__
