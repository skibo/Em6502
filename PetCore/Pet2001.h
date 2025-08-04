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

// Pet2001.h

#ifndef __PET2001_H__
#define __PET2001_H__

#include "Cpu6502.h"
#include "Pet2001Hw.h"

class Pet2001 {
private:
	Cpu6502		cpu;
	Pet2001Hw	pethw;

public:
	Pet2001(PetVideo *video = 0, PetCassHw *cass = 0, PetIeeeHw *ieee = 0)
		: cpu(&pethw),
		  pethw(&cpu, video, cass, ieee)
	{  }
	void		reset(void);
	bool		cycle(void);
	void		setVideo(PetVideo *video)
	{ pethw.setVideo(video); }
	void		setKeyrow(int row, uint8_t keyrow)
	{ pethw.setKeyrow(row, keyrow); }
	void		setRamsize(int ramsize)
	{ pethw.setRamsize(ramsize); }
	void		readRange(uint16_t addr, uint8_t *data, int length);
	void		writeRange(uint16_t addr, const uint8_t *data,
				   int length);
	void		writeRom(uint16_t addr, const uint8_t *data,
			int length)
	{ pethw.writeRom(addr, data, length); }
	void		setAudioBuf(uint8_t *buf, int len)
	{ pethw.setAudioBuf(buf, len); }
	int		getAudioTail(void)
	{ return pethw.getAudioTail(); }
	Cpu6502		*getCpu(void)
	{ return &cpu; }
};

#endif // __PET2001_H__
