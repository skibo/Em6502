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

#ifndef __MOS6532RIOT_H__
#define __MOS6532RIOT_H__

#include "MemSpace.h"

class Mos6532Riot : public MemSpace {
private:
	uint8_t	porta_in;
	uint8_t porta_out;
	uint8_t	ddra;
	uint8_t portb_in;
	uint8_t portb_out;
	uint8_t	ddrb;
	uint8_t intim;
	uint8_t instat;

	uint16_t timcnt;
	uint16_t timintvl;
	uint16_t timcmpr;

	uint8_t pa7_edge;
public:
	Mos6532Riot(void);

	void	setPortA(uint8_t _set, uint8_t _reset);
	void	setPortB(uint8_t _set, uint8_t _reset);

	// MemSpace interface
	uint8_t	read(uint16_t addr);
	void	write(uint16_t addr, uint8_t d8);

	void	reset(void);
	void	cycle(void);
};

#endif // __MOS6532RIOT_H__
