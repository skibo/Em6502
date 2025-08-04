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

// Pet2001Io.h

#ifndef __PET2001IO_H__
#define __PET2001IO_H__

#include "MemSpace.h"

class Cpu6502;
class PetVideo;
class PetCassHw;
class PetIeeeHw;

class Pet2001Io : MemSpace {
private:
	Cpu6502		*cpu;
	PetVideo	*video;
	PetCassHw	*cass;
	PetIeeeHw	*ieee;
	uint8_t		*audiobuf;
	int		audiobuflen;
	int		audiobuftail;

	uint8_t		pia1_pa_in;
	uint8_t		pia1_pa_out;
	uint8_t		pia1_ddra;
	uint8_t		pia1_cra;
	uint8_t		pia1_pb_in;
	uint8_t		pia1_pb_out;
	uint8_t		pia1_ddrb;
	uint8_t		pia1_crb;
	uint8_t		pia1_ca1;	// CA1 input (cass1 read)
	uint8_t		pia1_ca2;	// CA2 output (screen blank)
	uint8_t		pia1_cb1;	// CB1 input

	uint8_t		pia2_pa_in;
	uint8_t		pia2_pa_out;
	uint8_t		pia2_ddra;
	uint8_t		pia2_cra;
	uint8_t		pia2_pb_in;
	uint8_t		pia2_pb_out;
	uint8_t		pia2_ddrb;
	uint8_t		pia2_crb;

	uint8_t		via_drb_in;
	uint8_t		via_drb_out;
	uint8_t		via_dra_in;
	uint8_t		via_dra_out;
	uint8_t		via_ddrb;
	uint8_t		via_ddra;
	uint8_t		via_t1cl;
	uint8_t		via_t1ch;
	uint8_t		via_t1_1shot;		// Interrupt next underflow
	uint8_t		via_t1_undf;		// Underflow.  Reload next.
	uint8_t		via_t1ll;
	uint8_t		via_t1lh;
	uint8_t		via_t2cl;
	uint8_t		via_t2ch;
	uint8_t		via_t2_1shot;		// Interrupt next underflow
	uint8_t		via_t2_undf;		// Underflow.  Reload next.
	uint8_t		via_t2ll;
	uint8_t		via_sr;
	uint8_t		via_sr_cntr;
	uint8_t		via_sr_start;
	uint8_t		via_acr;
	uint8_t		via_pcr;
	uint8_t		via_ifr;
	uint8_t		via_ier;
	uint8_t		via_cb1;
	uint8_t		via_cb2;

	uint8_t		keyrow[10];
	int		video_cycle;

	void updateIrq(void);
	void sync(int sync);
public:
	Pet2001Io(Cpu6502 *cpu, PetVideo *video,
		  PetCassHw *cass, PetIeeeHw *ieee)
		: cpu(cpu),
		  video(video),
		  cass(cass),
		  ieee(ieee),
		  audiobuf(0)
	{
		reset();
	}
	void setVideo(PetVideo *video) { this->video = video; }
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t d8);
	void setAudioBuf(uint8_t *buf, int len)
	{
		audiobuf = buf;
		audiobuflen = len;
		audiobuftail = 0;
	}
	int getAudioTail(void)
	{
		int tail = audiobuftail;
		audiobuftail = 0;
		return tail;
	}

	void setKeyrow(int row, uint8_t keyrow);
	void reset(void);
	void cycle(void);
};

#endif // __PET2001IO_H__
