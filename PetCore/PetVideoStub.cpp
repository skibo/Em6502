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

// PetVideoStub.cpp

#include <stdint.h>

#include "PetVideoStub.h"

#ifdef DEBUGVID
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGVID) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

// Called on the rising edge of SYNC/VIDEO_ON.
void
PetVideoStub::sync(void)
{
	DPRINTF(2, "PetVideoStub::%s:\n", __func__);
}

uint8_t
PetVideoStub::read(uint16_t offset)
{
	uint8_t d8 = 0xaa;

	d8 = vidmem[offset & (PET_VRAM_SIZE - 1)];

	DPRINTF(1, "PetVideoStub::%s: offset=0x%x returns 0x%02x\n", __func__,
		offset, d8);

	return (d8);
}

void
PetVideoStub::write(uint16_t offset, uint8_t d8)
{
	DPRINTF(1, "PetVideoStub::%s: offset=0x%x d8=0x%02x\n", __func__,
		offset, d8);

	vidmem[offset & (PET_VRAM_SIZE - 1)] = d8;
}

void
PetVideoStub::setCharset(bool alt)
{
	DPRINTF(1, "PetVideoStub::%s: alt=%d\n", __func__, alt);
}

void
PetVideoStub::setBlank(bool blank)
{
	DPRINTF(1, "PetVideoStub::%s: blank=%d\n", __func__, blank);
}
