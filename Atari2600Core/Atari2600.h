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

// Atari2600.h

#ifndef __ATARI2600_H__
#define __ATARI2600_H__

#include "Cpu6502.h"
#include "Atari2600Hw.h"
#include "Atari2600Video.h"

// bit fields for setJoyLeft()/setJoyRight()
#define JOY_UP		1
#define JOY_DOWN	2
#define JOY_LEFT	4
#define JOY_RIGHT	8
#define JOY_TRIGGER	16

#define PADDLE_LEFT_B	0
#define PADDLE_LEFT_A	1
#define PADDLE_RIGHT_B	2
#define PADDLE_RIGHT_A	3
#define PADDLE_VAL_MAX	100

class Atari2600 {
private:
	Cpu6502		cpu;
	Atari2600Hw	atarihw;
	int		cpudiv3;
	bool		rdy;
public:
	Atari2600(Atari2600Video *_video = 0);

	void		reset(void);
	bool		cycle(void);

	void		setVideo(Atari2600Video *_video)
	{ atarihw.setVideo(_video); }

	void		readRam(uint16_t addr, uint8_t *data, int length)
	{ atarihw.readRam(addr, data, length); }
	void		writeRam(uint16_t addr, const uint8_t *data,
			int length)
	{ atarihw.writeRam(addr, data, length); }
	void		setRom(const uint8_t *data, int length)
	{ atarihw.setRom(data, length); }
	void		setRdy(bool _rdy)
	{ this->rdy = _rdy; }
	int		*getCycleCounter(void)
	{ return this->atarihw.getCycleCounter(); }

	void		setDiffLeft(bool _val)
	{ atarihw.setDiffLeft(_val); }
	void		setDiffRight(bool _val)
	{ atarihw.setDiffRight(_val); }
	void		setSelect(bool _val)
	{ atarihw.setSelect(_val); }
	void		setStart(bool _val)
	{ atarihw.setStart(_val); }
	void		setJoyLeft(uint8_t _set, uint8_t _reset)
	{ atarihw.setJoyLeft(_set, _reset); }
	void		setJoyRight(uint8_t _set, uint8_t _reset)
	{ atarihw.setJoyRight(_set, _reset); }
	void		setPaddle(int p, int val)
	{ atarihw.setPaddle(p, val); }

	Cpu6502		*getCpu(void)
	{ return &cpu; }
};

#endif // __ATARI2600_H__
