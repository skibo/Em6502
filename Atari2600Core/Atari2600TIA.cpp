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

// Atari2600TIA.cpp

#include <stdint.h>

#include "Atari2600.h"
#include "Atari2600TIA.h"
#include "Atari2600Video.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

#define ATARI_SCAN_WIDTH	228
#define ATARI_SCAN_RDY		3
#define ATARI_SCAN_HBLANK	68
#define ATARI_SCAN_LHBLANK	76
#define ATARI_SCAN_CENTER	148
#define ATARI_SCAN_AWIDTH	(ATARI_SCAN_WIDTH - ATARI_SCAN_HBLANK)

// Write registers:
#define VSYNC	0x00	// vertical sync set-clear
#define    VSYNC_ON	(1 << 1)
#define VBLANK	0x01	// vertical blank set-clear
#define    VBLANK_ON	(1 << 1)
#define    VBLANK_ENL45	(1 << 6) // enable latches on I4,I5
#define    VBLANK_DI03	(1u << 7) // "dump" (pull-down) I0-I3.
#define    VBLANK_MASK	0xc2
#define WSYNC	0x02	// wait for leading edge horz blank
#define RSYNC	0x03	// reset horz sync counter
#define NUSIZ0	0x04	// number-size player-missile 0
#define NUSIZ1	0x05	// number-size player-missile 1
#define    NUSIZ_M_SHFT 4
#define    NUSIZ_M_MASK (3 << NUSIZ_M_SHFT)
#define    NUSIZ_MSZ(x)	(1 << (((x) & NUSIZ_M_MASK) >> NUSIZ_M_SHFT))
#define	   NUSIZ_P_MASK 7
#define    NUSIZ_P1	0	// player size
#define    NUSIZ_P2C	1
#define    NUSIZ_P2M	2
#define    NUSIZ_P3C	3
#define    NUSIZ_P2W	4
#define    NUSIZ_PDBL	5
#define    NUSIZ_P3M	6
#define    NUSIZ_PQD	7
#define    NUSIZ_MASK	0x37
#define COLUP0	0x06	// color-lum player 0 (see Atari2600Video.h for fields)
#define COLUP1	0x07	// color-lum player 1
#define COLUPF	0x08	// color-lum playfield
#define COLUBK	0x09	// color-lum background
#define CTRLPF	0x0a	// control playfield ball size and collision
#define    CTRLPF_REF	(1 << 0)	// reflect playfield
#define    CTRLPF_SC	(1 << 1)	// score mode
#define    CTRLPF_PFP	(1 << 2)	// playfield priority
#define    CTRLPF_BSZ_SHFT 4		// ball size field
#define    CTRLPF_BSZ_MASK (3 << CTRLPF_BSZ_SHFT)
#define    CTRLPF_BSZ(x) (1 <<(((x) & CTRLPF_BSZ_MASK) >> CTRLPF_BSZ_SHFT))
#define    CTRLPF_MASK	0x37
#define REFP0	0x0b	// reflect player 0
#define REFP1	0x0c	// reflect player 1
#define    REFP_ON	(1 << 3)
#define PF0	0x0d	// playfield reg byte 0
#define    PF0_SHFT	4
#define    PF0_MASK	(0xfu << PF0_SHFT)
#define PF1	0x0e	// playfield reg byte 1
#define PF2	0x0f	// playfield reg byte 2
#define RESP0	0x10	// reset player 0
#define RESP1	0x11	// reset player 1
#define RESM0	0x12	// reset missile 0
#define RESM1	0x13	// reset missile 1
#define RESBL	0x14	// reset ball
#define AUDC0	0x15	// audio control 0
#define AUDC1	0x16	// audio control 1
#define AUDF0	0x17	// audio frequency 0
#define AUDF1	0x18	// audio frequency 1
#define AUDV0	0x19	// audio volume 0
#define AUDV1	0x1a	// audio volume 1
#define GRP0	0x1b	// graphics player 0
#define GRP1	0x1c	// graphics player 1
#define ENAM0	0x1d	// graphics (enable) missile 0
#define ENAM1	0x1e	// graphics (enable) missile 1
#define ENABL	0x1f	// graphics (enable) ball
#define    ENA_EN	(1 << 1)
#define HMP0	0x20	// horz motion player 0
#define HMP1	0x21	// horz motion player 1
#define HMM0	0x22	// horz motion missile 0
#define HMM1	0x23	// horz motion missile 1
#define HMBL	0x24	// horz motion ball
#define	   HMOV_SHFT	4
#define    HMOV_MASK	(0xfu << HMOV_SHFT)
#define    HMOV_V(x)	(((x) >> HMOV_SHFT) ^ 8)
#define VDELP0	0x25	// vert delay player 0
#define VDELP1	0x26	// vert delay player 1
#define VDELBL	0x27	// vert delay ball
#define    VDEL_DEL	1
#define RESMP0	0x28	// reset missile 0 to player 0
#define RESMP1	0x29	// reset missile 1 to player 1
#define    RESMP_ON	(1 << 1)
#define HMOVE	0x2a	// apply horz motion
#define HMCLR	0x2b	// clear horz motion regs
#define CXCLR	0x2c	// clear collission latches

// Read registers:
#define CXM0P	0x00	// Collision regs
#define CXM1P	0x01
#define CXP0FB	0x02
#define CXP1FB	0x03
#define CXM0FB	0x04
#define CXM1FB	0x05
#define CXBLPF	0x06
#define CXPPMM	0x07
#define INPT0	0x08	// read pot port
#define INPT1	0x09
#define INPT2	0x0a
#define INPT3	0x0b
#define INPT4	0x0c
#define INPT5	0x0d
#define    INPT_D	0x80

#define TIA_RA_MASK 0x0f

Atari2600TIA::Atari2600TIA(Atari2600 *_atari)
{
	DPRINTF(1, "Atari2600TIA::%s:\n", __func__);

	this->atari = _atari;

	inpts = 0x3f;
}

uint8_t
Atari2600TIA::read(uint16_t addr)
{
	DPRINTF(3, "Atari2600TIA::%s: addr=0x%04x\n", __func__, addr);

	uint8_t d8 = 0xff;

	switch (addr & TIA_RA_MASK) {
	case CXM0P:
		d8 = (cx_m0p1 ? 0x80 : 0) | (cx_m0p0 ? 0x40 : 0);
		break;
	case CXM1P:
		d8 = (cx_m1p0 ? 0x80 : 0) | (cx_m1p1 ? 0x40 : 0);
		break;
	case CXP0FB:
		d8 = (cx_p0pf ? 0x80 : 0) | (cx_p0bl ? 0x40 : 0);
		break;
	case CXP1FB:
		d8 = (cx_p1pf ? 0x80 : 0) | (cx_p1bl ? 0x40 : 0);
		break;
	case CXM0FB:
		d8 = (cx_m0pf ? 0x80 : 0) | (cx_m0bl ? 0x40 : 0);
		break;
	case CXM1FB:
		d8 = (cx_m1pf ? 0x80 : 0) | (cx_m1bl ? 0x40 : 0);
		break;
	case CXBLPF:
		d8 = (cx_blpf ? 0x80 : 0);
		break;
	case CXPPMM:
		d8 = (cx_p0p1 ? 0x80 : 0) | (cx_m0m1 ? 0x40 : 0);
		break;

	case INPT0:
		d8 = (inpts & 1) != 0 && (vblank_reg & VBLANK_DI03) == 0 ?
		      INPT_D : 0;
		break;
	case INPT1:
		d8 = (inpts & 2) != 0 && (vblank_reg & VBLANK_DI03) == 0 ?
		      INPT_D : 0;
		break;
	case INPT2:
		d8 = (inpts & 4) != 0 && (vblank_reg & VBLANK_DI03) == 0 ?
		      INPT_D : 0;
		break;
	case INPT3:
		d8 = (inpts & 8) != 0 && (vblank_reg & VBLANK_DI03) == 0 ?
		      INPT_D : 0;
		break;
	case INPT4:
		d8 = (inpts & inpts_l & 16) != 0 ? INPT_D : 0;
		break;
	case INPT5:
		d8 = (inpts & inpts_l & 32) != 0 ? INPT_D : 0;
		break;
	default:
		DPRINTF(1, "Atari2600TIA::%s: unknown read: 0x%02x\n",
			__func__, addr);
	}

	return d8;
}

void
Atari2600TIA::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(2, "Atari2600TIA::%s: addr=0x%04x d8=0x%02x\n", __func__,
		addr, d8);

	switch (addr) {
	case VSYNC:	// vertical sync set-clear
		if ((d8 & VSYNC_ON) != 0 && !vsync)
			video->vsync();
		vsync = (d8 & VSYNC_ON) != 0;
		break;
	case VBLANK:	// vertical blank set-clear
		vblank_reg = d8 & VBLANK_MASK;
		if ((vblank_reg & VBLANK_ENL45) == 0)
			inpts_l = 0x30;
		break;
	case WSYNC:	// wait for leading edge horz blank
		atari->setRdy(false);
		wsync = true;
		break;
	case RSYNC:	// reset horz sync counter
		hcounter = 0;
		hblank = true;
		break;
	case NUSIZ0:	// number-size player-missile 0
		nusiz0 = d8 & NUSIZ_MASK;
		break;
	case NUSIZ1:	// number-size player-missile 1
		nusiz1 = d8 & NUSIZ_MASK;
		break;
	case COLUP0:	// color-lum player 0
		colup0 = d8 & COLU_MASK;
		break;
	case COLUP1:	// color-lum player 1
		colup1 = d8 & COLU_MASK;
		break;
	case COLUPF:	// color-lum playfield
		colupf = d8 & COLU_MASK;
		break;
	case COLUBK:	// color-lum background
		colubk = d8 & COLU_MASK;
		break;
	case CTRLPF:	// control playfield, ball size
		ctrlpf = d8 & CTRLPF_MASK;
		break;
	case REFP0:	// reflect player 0
		refp0 = (d8 & REFP_ON) != 0;
		break;
	case REFP1:	// reflect player 1
		refp1 = (d8 & REFP_ON) != 0;
		break;
	case PF0:	// playfield reg byte 0
		pf0 = d8 & PF0_MASK;
		break;
	case PF1:	// playfield reg byte 1
		pf1 = d8;
		break;
	case PF2:	// playfield reg byte 2
		pf2 = d8;
		break;
	case RESP0:	// reset player 0
		resp0_del = 6;
		break;
	case RESP1:	// reset player 1
		resp1_del = 6;
		break;
	case RESM0:	// reset missile 0
		resm0_del = 6;
		break;
	case RESM1:	// reset missile 1
		resm1_del = 6;
		break;
	case RESBL:	// reset ball
		resbl_del = 5;
		break;
	case AUDC0:	// audio control 0
		break;
	case AUDC1:	// audio control 1
		break;
	case AUDF0:	// audio frequency 0
		break;
	case AUDF1:	// audio frequency 1
		break;
	case AUDV0:	// audio volume 0
		break;
	case AUDV1:	// audio volume 1
		break;
	case GRP0:	// graphics player 0
		grp0_new = d8;
		grp1_old = grp1_new;
		break;
	case GRP1:	// graphics player 1
		grp1_new = d8;
		grp0_old = grp0_new;
		enabl_old = enabl_new;
		break;
	case ENAM0:	// graphics (enable) missile 0
		enam0 = (d8 & ENA_EN) != 0;
		break;
	case ENAM1:	// graphics (enable) missile 1
		enam1 = (d8 & ENA_EN) != 0;
		break;
	case ENABL:	// graphics (enable) ball
		enabl_new = (d8 & ENA_EN) != 0;
		break;
	case HMP0:	// horz motion player 0
		hmp0 = HMOV_V(d8);
		break;
	case HMP1:	// horz motion player 1
		hmp1 = HMOV_V(d8);
		break;
	case HMM0:	// horz motion missile 0
		hmm0 = HMOV_V(d8);
		break;
	case HMM1:	// horz motion missile 1
		hmm1 = HMOV_V(d8);
		break;
	case HMBL:	// horz motion ball
		hmbl = HMOV_V(d8);
		break;
	case VDELP0:	// vert delay player 0
		vdelp0 = (d8 & VDEL_DEL) != 0;
		break;
	case VDELP1:	// vert delay player 1
		vdelp1 = (d8 & VDEL_DEL) != 0;
		break;
	case VDELBL:	// vert delay ball
		vdelbl = (d8 & VDEL_DEL) != 0;
		break;
	case RESMP0:	// reset missile 0 to player 0
		resmp0 = (d8 & RESMP_ON) != 0;
		break;
	case RESMP1:	// reset missile 1 to player 1
		resmp1 = (d8 & RESMP_ON) != 0;
		break;
	case HMOVE:	// apply horz motion
		hmov_ctr = 1 + ((hcounter + 1) & 3);
		break;
	case HMCLR:	// clear horz motion regs
		hmp0 = 8;
		hmp1 = 8;
		hmm0 = 8;
		hmm1 = 8;
		hmbl = 8;
		break;
	case CXCLR:	// clear collission latches
		cx_m0p1 = false;
		cx_m0p0 = false;
		cx_m1p0 = false;
		cx_m1p1 = false;
		cx_p0pf = false;
		cx_p0bl = false;
		cx_p1pf = false;
		cx_p1bl = false;
		cx_m0pf = false;
		cx_m0bl = false;
		cx_m1pf = false;
		cx_m1bl = false;
		cx_blpf = false;
		cx_p0p1 = false;
		cx_m0m1 = false;
		break;
	default:
		DPRINTF(1, "Atari2600TIA::%s: unknown write addr 0x%02x\n",
			__func__, addr);
	}
}

// XXX: Actually, there isn't a TIA reset.
void
Atari2600TIA::reset(void)
{
	DPRINTF(1, "Atari2600TIA::%s:\n", __func__);

	inpts_l = 0x30;

	hcounter = 0;
	hblank = true;

	vsync = false;
	vblank_reg = 0;
	wsync = false;

	pf0 = 0;
	pf1 = 0;
	pf2 = 0;
	ctrlpf = 0;
	nusiz0 = 0;
	nusiz1 = 0;
	resmp0 = false;
	resmp1 = false;
	hmp0 = 0;
	hmp1 = 0;
	hmm0 = 0;
	hmm1 = 0;
	hmbl = 0;
	enam0 = false;
	enam1 = false;
	enabl_new = false;
	enabl_old = false;
	grp0_new = 0;
	grp0_old = 0;
	grp1_new = 0;
	grp1_old = 0;
	refp0 = false;
	refp1 = false;
	vdelp0 = false;
	vdelp1 = false;
	vdelbl = false;
	colup0 = 0;
	colup1 = 0;
	colupf = 0;
	colubk = 0;

	resbl_del = 0;
	resm0_del = 0;
	resm1_del = 0;

	blec = false;
	p0ec = false;
	p1ec = false;
	m0ec = false;
	m1ec = false;

	longblank = false;
	hmov_ctr = 0;

	pf0_sr = 0;
	pf1_sr = 0;
	pf2_sr = 0;

	bitbl = false;
	bitbl_cnt = 0;
	hzpc_bl = 0;

	bitp0 = false;
	hzpc_p0 = 0;

	bitp1 = false;
	hzpc_p1 = 0;

	bitm0 = false;
	bitm0_cnt = 0;
	hzpc_m0 = 0;

	bitm1 = false;
	bitm1_cnt = 0;
	hzpc_m1 = 0;

	cx_m0p1 = false;
	cx_m0p0 = false;
	cx_m1p0 = false;
	cx_m1p1 = false;
	cx_p0pf = false;
	cx_p0bl = false;
	cx_p1pf = false;
	cx_p1bl = false;
	cx_m0pf = false;
	cx_m0bl = false;
	cx_m1pf = false;
	cx_m1bl = false;
	cx_blpf = false;
	cx_p0p1 = false;
	cx_m0m1 = false;
}

void
Atari2600TIA::setInput(uint8_t _set, uint8_t _reset)
{
	this->inpts &= ~_reset;
	this->inpts |= _set;

	// Latch 0s in I4,I5 if latches are enabled.
	if ((vblank_reg & VBLANK_ENL45) != 0)
		inpts_l &= ~_reset;
}

bool
Atari2600TIA::dumpDI03(void)
{
	return ((vblank_reg & VBLANK_DI03) != 0);
}

// Detect collisions.
void
Atari2600TIA::doCollisions(void)
{
	if ((vblank_reg & VBLANK_ON) != 0)
		return;

	if (bitm0 && bitp1)
		cx_m0p1 = true;
	if (bitm0 && bitp0)
		cx_m0p0 = true;
	if (bitm1 && bitp0)
		cx_m1p0 = true;
	if (bitm1 && bitp1)
		cx_m1p1 = true;
	if (bitp0 && bitpf)
		cx_p0pf = true;
	if (bitp0 && bitbl)
		cx_p0bl = true;
	if (bitp1 && bitpf)
		cx_p1pf = true;
	if (bitp1 && bitbl)
		cx_p1bl = true;
	if (bitm0 && bitpf)
		cx_m0pf = true;
	if (bitm0 && bitbl)
		cx_m0bl = true;
	if (bitm1 && bitpf)
		cx_m1pf = true;
	if (bitm1 && bitbl)
		cx_m1bl = true;
	if (bitbl && bitpf)
		cx_blpf = true;
	if (bitp0 && bitp1)
		cx_p0p1 = true;
	if (bitm0 && bitm1)
		cx_m0m1 = true;
}

// Handle Playfield logic.
void
Atari2600TIA::doPlayfield()
{
	if (hcounter == ATARI_SCAN_HBLANK) {
		pf0_sr = 0x10;
		pf1_sr = 0;
		pf2_sr = 0;
	} else if (hcounter == ATARI_SCAN_CENTER) {
		if ((ctrlpf & CTRLPF_REF) != 0) {
			pf0_sr = 0;
			pf1_sr = 0;
			pf2_sr = 0x80;
		} else {
			pf0_sr = 0x10;
			pf1_sr = 0;
			pf2_sr = 0;
		}
	}

	bitpf = ((pf0 & pf0_sr) | (pf1 & pf1_sr) | (pf2 & pf2_sr)) != 0;

	if ((hcounter & 3) == 3) {
		// Shift PF registers.
		if ((ctrlpf & CTRLPF_REF) != 0 &&
		    hcounter >= ATARI_SCAN_CENTER) {
			// Reflect PF shift.
			pf0_sr = ((pf0_sr >> 1) | (pf1_sr & 0x80));
			pf1_sr = ((pf1_sr << 1) | (pf2_sr & 0x01));
			pf2_sr = (pf2_sr >> 1);
		} else {
			// Non-reflect PF shift.
			pf2_sr = ((pf2_sr << 1) | (pf1_sr & 0x01));
			pf1_sr = ((pf1_sr >> 1) | (pf0_sr & 0x80));
			pf0_sr = (pf0_sr << 1);
		}
	}
}

// Handle ball object.
void
Atari2600TIA::doBall(void)
{
	bool ball_clk = !hblank || (blec && (hmov_ctr & 3) == 0);

	if (ball_clk && bitbl_cnt > 0) {
		if (bitbl_cnt == CTRLPF_BSZ(ctrlpf)) {
			bitbl = false;
			bitbl_cnt = 0;
		} else
			bitbl_cnt++;
	}

	// Implement delay in resetting ball counter.  Some delay
	// cycles are eaten on any clock and others only on ball_clk.
	if (resbl_del > 0) {
		if (resbl_del > 3)
			resbl_del--;
		else if (ball_clk)
			resbl_del--;
		if (resbl_del == 0)
			hzpc_bl = 159;
	}

	if (ball_clk) {
		if (++hzpc_bl == 160) {
			bitbl = vdelbl ? enabl_old : enabl_new;
			bitbl_cnt = 1;
			hzpc_bl = 0;
		}
	}
}

// Return true if we should start a player or missile on this
// counter value.  This is used by missiles and players.
bool
Atari2600TIA::startPlayer(uint8_t ctr, uint8_t nusiz)
{
	nusiz &= NUSIZ_P_MASK;

	switch (ctr) {
	case 0:
		return true;
	case 16:
		return nusiz == 1 || nusiz == 3;
	case 32:
		return nusiz == 2 || nusiz == 3 || nusiz == 6;
	case 64:
		return nusiz == 4 || nusiz == 6;
	default:
		return false;
	}
}

void
Atari2600TIA::doMissiles(void)
{
	bool mis0_clk = !hblank || (m0ec && (hmov_ctr & 3) == 0);
	bool mis1_clk = !hblank || (m1ec && (hmov_ctr & 3) == 0);

	if (mis0_clk && bitm0_cnt > 0) {
		if (bitm0_cnt == NUSIZ_MSZ(nusiz0)) {
			bitm0 = false;
			bitm0_cnt = 0;
		} else
			bitm0_cnt++;
	}

	if (mis1_clk && bitm1_cnt > 0) {
		if (bitm1_cnt == NUSIZ_MSZ(nusiz1)) {
			bitm1 = false;
			bitm1_cnt = 0;
		} else
			bitm1_cnt++;
	}

	// Implement delay in resetting missile ounters.  Some delay
	// cycles are eaten on any clock and others only during mis[01]_clk.
	if (resm0_del > 0) {
		if (resm0_del > 4)
			resm0_del--;
		else if (mis0_clk)
			resm0_del--;
		if (resm0_del == 0)
			hzpc_m0 = 0;
	}

	if (resm1_del > 0) {
		if (resm1_del > 4)
			resm1_del--;
		else if (mis1_clk)
			resm1_del--;
		if (resm1_del == 0)
			hzpc_m1 = 0;
	}

	if (mis0_clk) {
		if (++hzpc_m0 == 160)
			hzpc_m0 = 0;
		if (startPlayer(hzpc_m0, nusiz0)) {
			bitm0 = enam0 && !resmp0;
			bitm0_cnt = 1;
		}
	}

	if (mis1_clk) {
		if (++hzpc_m1 == 160)
			hzpc_m1 = 0;
		if (startPlayer(hzpc_m1, nusiz1)) {
			bitm1 = enam1 && !resmp1;
			bitm1_cnt = 1;
		}
	}
}

// Return pixel of player based upon counter.  Counter is set to 0
// when we're done drawing player.
bool
Atari2600TIA::playerBit(uint8_t &cnt, uint8_t grp, uint8_t nusiz, bool ref)
{
	int n;

	nusiz &= NUSIZ_P_MASK;

	switch (nusiz) {
	case 5:
		// double-wide
		if (cnt > 17) {
			cnt = 0;
			return false;
		}
		n = ref ? ((cnt - 2) >> 1) : (7 - ((cnt - 2) >> 1));
		break;
	case 7:
		// quad-wide
		if (cnt > 33) {
			cnt = 0;
			return false;
		}
		n = ref ? ((cnt - 2) >> 2) : (7 - ((cnt - 2) >> 2));
		break;
	default:
		if (cnt > 8) {
			cnt = 0;
			return false;
		}
		n = ref ? (cnt - 1) : (8 - cnt);
	}

	cnt++;

	return ((grp >> n) & 1) != 0;
}

// Handle players and player counters.
void
Atari2600TIA::doPlayers(void)
{
	bool ply0_clk = !hblank || (p0ec && (hmov_ctr & 3) == 0);
	bool ply1_clk = !hblank || (p1ec && (hmov_ctr & 3) == 0);

	if (ply0_clk && bitp0_cnt > 0)
		bitp0 = playerBit(bitp0_cnt, vdelp0 ? grp0_old : grp0_new,
				  nusiz0, refp0);

	if (ply1_clk && bitp1_cnt > 0)
		bitp1 = playerBit(bitp1_cnt, vdelp1 ? grp1_old : grp1_new,
				  nusiz1, refp1);

	// Implement delay in resetting player counters.  Some delay
	// cycles are eaten on any clock and others only during ply[01]_clk.
	if (resp0_del > 0) {
		if (resp0_del > 4)
			resp0_del--;
		else if (ply0_clk)
			resp0_del--;
		if (resp0_del == 0)
			hzpc_p0 = 0;
	}

	if (resp1_del > 0) {
		if (resp1_del > 4)
			resp1_del--;
		else if (ply1_clk)
			resp1_del--;
		if (resp1_del == 0)
			hzpc_p1 = 0;
	}

	if (ply0_clk) {
		if (++hzpc_p0 == 160)
			hzpc_p0 = 0;
		if (startPlayer(hzpc_p0, nusiz0))
			bitp0_cnt = 1;
		if (resmp0) {
			if ((nusiz0 & NUSIZ_P_MASK) == 5) {
				if (hzpc_p0 == 8)
					hzpc_m0 = 0;
			} else if ((nusiz0 & NUSIZ_P_MASK) == 7) {
				if (hzpc_p0 == 12)
					hzpc_m0 = 0;
			} else {
				if (hzpc_p0 == 5)
					hzpc_m0 = 0;
			}
		}
	}

	if (ply1_clk) {
		if (++hzpc_p1 == 160)
			hzpc_p1 = 0;
		if (startPlayer(hzpc_p1, nusiz1))
			bitp1_cnt = 1;
		if (resmp1) {
			if ((nusiz1 & NUSIZ_P_MASK) == 5) {
				if (hzpc_p1 == 8)
					hzpc_m1 = 0;
			} else if ((nusiz1 & NUSIZ_P_MASK) == 7) {
				if (hzpc_p1 == 12)
					hzpc_m1 = 0;
			} else {
				if (hzpc_p1 == 5)
					hzpc_m1 = 0;
			}
		}
	}
}

// Handle motion counters
//
// hmov_ctr counts through the 68-72 cycle HMOVE process.  hmov_ctr starts
// at 1-4 depending on which one of four phases the horizontal counter is in
// during the HMOVE write.  Cycle 6 is when the SEC signal is set and this
// also sets the latch that lengthens the horizontal blank.  Cycle 12 is the
// cycle of the first extra clock (i.e. blec, m0ec, etc.).  Subsequent extra
// clocks happen every four cycles until the last possible extra clock on
// cycle 72.
//
void
Atari2600TIA::doHmove(void)
{
	hmov_ctr++;

	if (hmov_ctr == 6)
		longblank = true;
	else if (hmov_ctr > 72)
		hmov_ctr = 0;
	else if (hmov_ctr >= 12) {

		if (hmov_ctr == 12) {
			blec = true;
			m0ec = true;
			m1ec = true;
			p0ec = true;
			p1ec = true;
		}

		if ((hmov_ctr & 3) == 0) {
			uint8_t cntr = (hmov_ctr - 12) >> 2;

			if (hmbl == cntr)
				blec = false;
			if (hmm0 == cntr)
				m0ec = false;
			if (hmm1 == cntr)
				m1ec = false;
			if (hmp0 == cntr)
				p0ec = false;
			if (hmp1 == cntr)
				p1ec = false;
		}
	}
}

void
Atari2600TIA::doPixel(int x)
{
	if (hblank || (vblank_reg & VBLANK_ON)) {
		scanline[x] = 0;
		return;
	}

	////////////////////////////////////////////////////
	// Pixel priority logic.
	////////////////////////////////////////////////////
	uint8_t pixel = colubk;
	if (bitpf || bitbl) {
		if ((ctrlpf & CTRLPF_SC) != 0)
			pixel = (x >= ATARI_NATIVE_WIDTH / 2) ?
				colup1 : colup0;
		else
			pixel = colupf;
	}
	if (bitp1 || bitm1)
		pixel = colup1;
	if (bitp0 || bitm0)
		pixel = colup0;
	if ((ctrlpf & CTRLPF_PFP) != 0 && (bitpf || bitbl))
		pixel = colupf;

	scanline[x] = pixel;
}

void
Atari2600TIA::cycle(void)
{
	DPRINTF(4, "Atari2600TIA::%s:\n", __func__);

	doPlayfield();
	doBall();
	doMissiles();
	doPlayers();

	doCollisions();

	if (hcounter >= ATARI_SCAN_HBLANK)
		doPixel(hcounter - ATARI_SCAN_HBLANK);

	// Start new horizontal line.
	hcounter++;
	if ((!longblank && hcounter == ATARI_SCAN_HBLANK) ||
	    (longblank && hcounter == ATARI_SCAN_LHBLANK))
		hblank = false;
	else if (hcounter == ATARI_SCAN_WIDTH) {
		hcounter = 0;
		hblank = true;
		longblank = false;

		if ((vblank_reg & VBLANK_ON) == 0)
			video->setScanline(scanline);
		video->hsync();
	} else if (wsync && hcounter == ATARI_SCAN_RDY) {
		atari->setRdy(true);
		wsync = false;
	}

	if (hmov_ctr > 0)
		doHmove();
}
