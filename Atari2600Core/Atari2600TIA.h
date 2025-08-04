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

#ifndef __ATARI2600TIA_H__
#define __ATARI2600TIA_H__

#include "MemSpace.h"
#include "Atari2600Video.h"

class Atari2600Video;
class Atari2600;

class Atari2600TIA : public MemSpace {
private:
	Atari2600Video	*video;
	Atari2600	*atari;

	int	hcounter;
	bool	hblank;

	uint8_t	scanline[ATARI_NATIVE_WIDTH];

	// Registers storage
	bool	vsync;
	uint8_t	vblank_reg;
	bool	wsync;		// is waiting for sync
	uint8_t	nusiz0;		// number-size player-missile registers
	uint8_t	nusiz1;
	uint8_t	colup0;		// color-luminance registers
	uint8_t	colup1;
	uint8_t colupf;
	uint8_t colubk;
	uint8_t	ctrlpf;		// playfield control register
	bool	refp0;		// reflect player bit
	bool	refp1;
	uint8_t	pf0;		// playfield graphics regs
	uint8_t	pf1;
	uint8_t	pf2;
	uint8_t	grp0_new;	// player graphic registers
	uint8_t grp0_old;
	uint8_t	grp1_new;
	uint8_t	grp1_old;
	bool	enam0;		// enable object bits
	bool	enam1;
	bool	enabl_new;
	bool	enabl_old;
	uint8_t	hmp0;		// horz motion registers
	uint8_t	hmp1;
	uint8_t hmm0;
	uint8_t hmm1;
	uint8_t hmbl;
	bool	vdelp0;		// player, ball delay bit
	bool	vdelp1;
	bool	vdelbl;
	bool	resmp0;
	bool	resmp1;

	uint8_t	resbl_del;	// RESxx delays
	uint8_t	resm0_del;
	uint8_t	resm1_del;
	uint8_t	resp0_del;
	uint8_t resp1_del;

	bool	blec;		// ball extra clock (HMOVE)
	bool	p0ec;
	bool	p1ec;
	bool	m0ec;
	bool	m1ec;

	bool	longblank;	// long horizontal blank due to hmove
	int8_t	hmov_ctr;	// counter to walk through HMOVE sequence.

	bool	bitpf;
	uint8_t	pf0_sr;		// shift register for playfield output
	uint8_t	pf1_sr;
	uint8_t	pf2_sr;

	// ball logic
	uint8_t hzpc_bl;	// horizontal position counter
	bool	bitbl;
	uint8_t bitbl_cnt;

	// missile logic
	bool	bitm0;
	uint8_t bitm0_cnt;
	uint8_t	hzpc_m0;
	bool	bitm1;
	uint8_t bitm1_cnt;
	uint8_t	hzpc_m1;

	// payer logic
	bool	bitp0;
	uint8_t	bitp0_cnt;
	uint8_t	hzpc_p0;
	bool	bitp1;
	uint8_t bitp1_cnt;
	uint8_t	hzpc_p1;

	uint8_t	inpts;		// five inputs
	uint8_t inpts_l;	// latch for I4,I5

	// Collision latches
	bool	cx_m0p1;
	bool	cx_m0p0;
	bool	cx_m1p0;
	bool	cx_m1p1;
	bool	cx_p0pf;
	bool	cx_p0bl;
	bool	cx_p1pf;
	bool	cx_p1bl;
	bool	cx_m0pf;
	bool	cx_m0bl;
	bool	cx_m1pf;
	bool	cx_m1bl;
	bool	cx_blpf;
	bool	cx_p0p1;
	bool	cx_m0m1;

	void	doPlayfield(void);
	void	doBall(void);
	bool	startPlayer(uint8_t ctr, uint8_t nusiz);
	void	doMissiles(void);
	bool	playerBit(uint8_t &cnt, uint8_t grp, uint8_t nusiz, bool ref);
	void	doPlayers(void);
	void	doCollisions(void);
	void    doHmove(void);
	void	doPixel(int);
public:
	Atari2600TIA(Atari2600 *_atari);

	uint8_t	read(uint16_t addr);
	void	write(uint16_t addr, uint8_t d8);

	void	reset(void);
	void	cycle(void);

	void	setVideo(Atari2600Video *_video)
	{ this->video = _video; }
	void	setInput(uint8_t _set, uint8_t _reset);
	bool	dumpDI03(void);
	int	*getCycleCounter(void)
	{ return &this->hcounter; }
};

#endif // __ATARI2600TIA_H__
