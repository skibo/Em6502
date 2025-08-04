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

#ifndef __APPLE2DISK2_H__
#define __APPLE2DISK2_H__

#include <functional>

class Apple2Disk2 {
private:
	uint8_t *nibfile;
	bool	writeprot;
	bool	nibmodified;
	int	track;
	uint8_t	phase;
	uint8_t	data_latch;
	bool	motor;
	bool	drv1;
	bool	q6;
	bool	q7;
	int	offset;
	std::function<void (bool, int)> disk_cb;

	void	reference(uint16_t addr);
public:
	Apple2Disk2()
	{
		nibfile = nullptr;
		writeprot = false;
		nibmodified = false;
	}
	void	write(uint16_t addr, uint8_t d8);
	uint8_t	read(uint16_t addr);
	void	reset(void);
	void	setNib(uint8_t *_nibfile)
	{
		nibfile = _nibfile;
		nibmodified = false;
	}
	bool	haveNib(void)
	{ return nibfile != nullptr; }
	void	setWriteProt(bool flag)
	{ writeprot = flag; }
	void	setDiskCallback(std::function<void (bool, int)> _cb)
	{ disk_cb = _cb; }
	bool	nibModified(void)
	{ return nibmodified; }
};

#endif // __APPLE2DISK2_H__
