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

// Atari2600.cpp

#include <stdint.h>

#include "Atari2600.h"
#include "Atari2600Hw.h"

unsigned int cycles;

Atari2600::Atari2600(Atari2600Video *_video)
	: cpu(&atarihw),
	  atarihw(this),
	  rdy(true)
{
	if (_video)
		atarihw.setVideo(_video);

	setDiffLeft(false);
	setDiffRight(false);
	setSelect(false);
	setJoyLeft(0, 0xff);
	setJoyRight(0, 0xff);
}

void
Atari2600::reset(void)
{
	atarihw.reset();
	cpu.reset();
	cpudiv3 = 0;
	rdy = true;
}

bool
Atari2600::cycle(void)
{
	bool retv = true;

	if (cpudiv3 == 2) {
		if (rdy)
			retv = cpu.cycle();
		if (retv) {
			atarihw.cycle3();
			atarihw.cycle();
			cpudiv3 = 0;
			cycles++;
		}
	} else {
		cpudiv3++;
		atarihw.cycle3();
		cycles++;
	}

	return retv;
}
