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

// Atari2600Hw.h

#ifndef __ATARI2600HW_H__
#define __ATARI2600HW_H__

#include "MemSpace.h"

#include "Atari2600TIA.h"
#include "Mos6532Riot.h"

class Atari2600;
class Atari2600Video;

#define RAM_SIZE	128
#define ROM_MAX_SIZE	8192

#define NUMPADDLES	4
#define PADDLECTRMAX	60000	// max cycles to discharge paddle cap

class Atari2600Hw : public MemSpace {
private:
	Atari2600Video	*video;
	Atari2600TIA	tia;
	Mos6532Riot	riot;
	uint8_t		ram[RAM_SIZE];
	uint8_t		rom[ROM_MAX_SIZE];
	int		romsz;
	bool		bank;
	int 		paddle_val[NUMPADDLES];
	int 		paddle_ctr;
public:
	Atari2600Hw(Atari2600 *_atari) :
		video(0),
		tia(_atari),
		romsz(0)
	{ }

	uint8_t	read(uint16_t addr);
	void	write(uint16_t addr, uint8_t d8);

	void	reset(void);
	void	cycle(void);
	void	cycle3(void);

	void	writeRam(uint16_t addr, const uint8_t *data, int len);
	void	readRam(uint16_t addr, uint8_t *data, int len);
	void	setRom(const uint8_t *data, int len);
	void	setVideo(Atari2600Video *_video)
	{
		this->video = _video;
		tia.setVideo(_video);
	}

	void	setDiffLeft(bool _val)
	{ riot.setPortB(_val ? 0x40 : 0, 0x40); }
	void	setDiffRight(bool _val)
	{ riot.setPortB(_val ? 0x80 : 0, 0x80); }
	void	setSelect(bool _val)
	{ riot.setPortB(_val ? 0 : 0x02, 0x02); }
	void	setStart(bool _val)
	{ riot.setPortB(_val ? 0 : 0x01, 0x01); }
	void	setJoyLeft(uint8_t _set, uint8_t _reset);
	void	setJoyRight(uint8_t _set, uint8_t _reset);
	void	setPaddle(int p, int val);
	int	*getCycleCounter(void)
	{ return this->tia.getCycleCounter(); }
};

#endif // __ATARI2600HW_H__
