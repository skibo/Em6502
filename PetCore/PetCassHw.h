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

// PetCassHw.h

#ifndef __PETCASSHW_H__
#define __PETCASSHW_H__

#include <functional>

class PetCassHw {
private:
	int	delay_cycle;
	int	edge_count;
	int	offset;
	int	bitcount;
	uint8_t byte;
	int	parity;
	int	state;
	int	rdata;
	int	csense;
	int	blocklen;
	uint8_t *hdr_data;
	uint8_t *progdata;
	int	data_len;

	std::function<void (int)> cass_done_cb;

	void	do_load_byte(uint8_t byte);
public:
	PetCassHw() { reset(); }
	void	cassLoad(const uint8_t *proghdr, const uint8_t *data,
			 int len);
	void	cassSave(uint8_t *proghdr, uint8_t *data, int maxlen);
	void	setCassDoneCallback(std::function<void (int)> _cb)
		{ cass_done_cb = _cb; }

	void	writeData(int wdata);
	int	readData(void) { return rdata; }
	int	sense(void) { return csense; }
	void	setMotor(bool set);
	void	reset(void);
	void	cycle(void);
};

#endif // __PETCASSHW_H__
