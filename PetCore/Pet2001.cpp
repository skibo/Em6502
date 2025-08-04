//
// Copyright (c) 2020 Thomas Skibo.
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

// Pet2001.cpp

#include <stdint.h>

#include "Pet2001.h"
#include "Pet2001Hw.h"

unsigned int cycles;

void
Pet2001::reset(void)
{
	pethw.reset();
	cpu.reset();
}

bool
Pet2001::cycle(void)
{
	bool retv;

	retv = cpu.cycle();
	if (retv) {
		pethw.cycle();
		cycles++;
	}

	return retv;
}

void
Pet2001::readRange(uint16_t addr, uint8_t *data, int length)
{
	for (int i = 0; i < length; i++)
		data[i] = pethw.read(addr + i);
}

void
Pet2001::writeRange(uint16_t addr, const uint8_t *data, int length)
{
	for (int i = 0; i < length; i++)
		pethw.write(addr + i, data[i]);
}
