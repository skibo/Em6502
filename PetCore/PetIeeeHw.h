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

// PetIeeeHw.h

#ifndef __PETIEEEHW_H__
#define __PETIEEEHW_H__

#define SENDER_TIMEOUT_CYCLES	640000

class IeeeDev;

class PetIeeeHw {
private:
	uint8_t	dio;
	bool	ndac_i;
	bool	ndac_o;
	bool	nrfd_i;
	bool	nrfd_o;
	bool	atn;
	bool	dav_i;
	bool	dav_o;
	bool	srq;
	bool	eoi_i;
	bool	eoi_o;

	enum {
		IEEE_STATE_IDLE = 0,
		IEEE_STATE_LISTEN,
		IEEE_STATE_FNAME,
		IEEE_STATE_TALK,
	} state;

	char	filename[64];
	int	fnum;
	int	data_index;
	int	sender_timeout;

	void	doDataIn(uint8_t d8);

	IeeeDev	*ieeeDev;
public:
	PetIeeeHw() { reset(); }

	void	setIeeeDev(IeeeDev *_id)
		{ ieeeDev = _id; }

	void	reset(void);

	uint8_t	din(void);
	void	dout(uint8_t dout);
	bool	eoiIn(void)	{ return eoi_i && eoi_o; }
	void	eoiOut(bool eoi);
	bool	srqIn(void)	{ return srq; }
	void	atnOut(bool atn);
	bool	davIn(void)	{ return dav_i && dav_o; }
	void	davOut(bool dav);
	bool	nrfdIn(void)	{ return nrfd_i && nrfd_o; }
	void	nrfdOut(bool nrfd);
	bool	ndacIn(void)	{ return ndac_i && ndac_o; }
	void	ndacOut(bool ndac);

	void	cycle(void);
};

#define MY_ADDRESS	8

#endif // __PETIEEEHW_H__
