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

// Atari2600video.h

#ifndef __ATARI2600VIDEO_H__
#define __ATARI2600VIDEO_H__

// Size of active area
#define ATARI_NATIVE_WIDTH	160
#define ATARI_NATIVE_HEIGHT	192

/* Color/Lum fields in data. */
#define COLU_LUM_SHFT	1
#define COLU_LUM_MASK	(7 << COLU_LUM_SHFT)
#define COLU_COL_SHFT	4
#define COLU_COL_MASK	(0xfu << COLU_COL_SHFT)
#define COLU_MASK	(COLU_COL_MASK | COLU_LUM_MASK)

class Atari2600Video {
public:
	virtual void	setScanline(uint8_t colu[]) = 0;
	virtual void	vsync(void) = 0;
	virtual void	hsync(void) = 0;
	virtual void	reset(void) = 0;
};

#endif // __ATARI2600VIDEO_H__
