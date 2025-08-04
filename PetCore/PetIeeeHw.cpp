//
// Copyright (c) 2020 - 2023 Thomas Skibo.
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

// PetIeeeHw.cpp

#include <stdint.h>

#include "PetIeeeHw.h"
#include "IeeeDev.h"

#ifdef DEBUGIEEE
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGIEEE) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif


void
PetIeeeHw::reset(void)
{
	DPRINTF(1, "PetIeeeHw::%s:\n", __func__);

	ndac_i = true;
	ndac_o = true;
	nrfd_i = true;
	nrfd_o = true;
	atn = true;
	dav_i = true;
	dav_o = true;
	srq = true;
	eoi_i = true;
	eoi_o = true;

	data_index = 0;
	sender_timeout = 0;

	state = IEEE_STATE_IDLE;

	if (ieeeDev)
		ieeeDev->ieeeReset();
}

void
PetIeeeHw::doDataIn(uint8_t d8)
{
	DPRINTF(3, "PetIeeeHw::%s: d8=0x%02x atn=%d state=%d\n",
		__func__, d8, atn, state);

	if (!atn) {
		switch (state) {
		case IEEE_STATE_IDLE:
			if (d8 == 0x20 + MY_ADDRESS) {
				DPRINTF(2, "PetIeeeHw::%s: LISTEN\n",
					__func__);
				state = IEEE_STATE_LISTEN;
			}
			else if (d8 == 0x40 + MY_ADDRESS) {
				DPRINTF(2, "PetIeeeHw::%s: TALK\n",
					__func__);
				state = IEEE_STATE_TALK;
			}
			break;
		case IEEE_STATE_LISTEN:
			if (d8 == 0x3f) {
				// UNListen
				DPRINTF(2, "PetIeeeHw::%s: UNLISTEN\n",
					__func__);
				state = IEEE_STATE_IDLE;
			} else if (d8 >= 0xe0 && d8 <= 0xef) {
				DPRINTF(2, "PetIeeeHw::%s: LISTEN: "
					"Close File.\n", __func__);
				// Close file
				if (ieeeDev)
					ieeeDev->ieeeClose(d8 - 0xe0);
			} else if (d8 >= 0xf0) {
				// Open file
				DPRINTF(2, "PetIeeeHw::%s: LISTEN: "
					"Open File.\n", __func__);
				data_index = 0;
				fnum = d8 - 0xf0;
				state = IEEE_STATE_FNAME;
			}
			break;
		case IEEE_STATE_FNAME:
			if (d8 == 0x3f) {
				// UNListen
				filename[data_index] = '\0';
				DPRINTF(1, "PetIeeeHw::%s: UNL filename=%s\n",
					__func__, filename);
				if (ieeeDev)
					ieeeDev->ieeeOpen(fnum, filename);
				state = IEEE_STATE_IDLE;
			}
			break;
		case IEEE_STATE_TALK:
			if (d8 == 0x5f) {
				// UNTalk
				DPRINTF(2, "PetIeeeHw::%s: UNTALK\n",
					__func__);
				state = IEEE_STATE_IDLE;
			} else if (d8 >= 0xe0 && d8 <= 0xef) {
				// Close file XXX
				DPRINTF(2, "PetIeeeHw::%s: TALK: "
					"Close File.\n", __func__);
				if (ieeeDev)
					ieeeDev->ieeeClose(d8 - 0xe0);
			}
			break;
		}
	}
	else {
		// Data with ATN not asserted.
		switch (state) {
		case IEEE_STATE_FNAME:
			if (data_index < sizeof(filename) - 1)
				filename[data_index++] = d8;
			break;
		case IEEE_STATE_LISTEN:
			if (ieeeDev)
				ieeeDev->ieeeListenData(fnum, d8, !eoi_i);
			break;
		default: ;
		}
	}
}

uint8_t
PetIeeeHw::din(void)
{
	DPRINTF(4, "PetIeeeHw::%s: dio=0x%02x\n", __func__, dio);
	return dio;
}

void
PetIeeeHw::dout(uint8_t dout)
{
	DPRINTF(3, "PetIeeeHw::%s: dout=0x%02x\n", __func__, dout);
	dio = dout;
}

void
PetIeeeHw::eoiOut(bool eoi)
{
	if (eoi_o == eoi)
		return;

	DPRINTF(3, "PetIeeeHw::%s: eoi=%d\n", __func__, eoi);
	eoi_o = eoi;
}

void
PetIeeeHw::atnOut(bool flag)
{
	if (flag == atn)
		return;

	DPRINTF(3, "PetIeeeHw::%s: atn=%d\n", __func__, flag);
	atn = flag;

	if (!atn) {
		// Negative transition of ATN
		ndac_i = false;
	} else {
		// Positive transition of ATN
		if (state == IEEE_STATE_TALK && nrfd_o) {
			bool seteoi = true;

			// Put data on bus.
			if (ieeeDev)
				dio = ieeeDev->ieeeTalkData(fnum, &seteoi) ^
					0xff;
			dav_i = false;
			if (seteoi)
				eoi_i = false;

			sender_timeout = SENDER_TIMEOUT_CYCLES;
		}
		if (state != IEEE_STATE_LISTEN && state != IEEE_STATE_FNAME) {
			ndac_i = true;
			nrfd_i = true;
		}
	}
}

void
PetIeeeHw::davOut(bool dav)
{
	if (dav_o == dav)
		return;

	DPRINTF(3, "PetIeeeHw::%s: dav=%d\n", __func__, dav);
	dav_o = dav;

	if (!atn || state == IEEE_STATE_LISTEN || state == IEEE_STATE_FNAME) {
		if (!dav_o) {
			// Negative transition of DAV.
			ndac_i = true;
			nrfd_i = false;
			doDataIn(dio ^ 0xff);
		}
		else {
			// Positive transition of DAV.
			ndac_i = false;
			nrfd_i = true;
		}
	}
}

void
PetIeeeHw::nrfdOut(bool nrfd)
{
	if (nrfd_o == nrfd)
		return;

	DPRINTF(3, "PetIeeeHw::%s: nrfd=%d\n", __func__, nrfd);
	nrfd_o = nrfd;

	if (nrfd_o) {
		// Positive transition of NRFD.  Put data on bus.
		if (state == IEEE_STATE_TALK) {
			bool seteoi = true;

			if (ieeeDev && atn)
				dio = ieeeDev->ieeeTalkData(fnum, &seteoi) ^
					0xff;
			dav_i = false;
			if (seteoi)
				eoi_i = false;
		}
	}
}

void
PetIeeeHw::ndacOut(bool ndac)
{
	if (ndac == ndac_o)
		return;

	DPRINTF(3, "PetIeeeHw::%s: ndac=%d\n", __func__, ndac);
	ndac_o = ndac;

	if (ndac_o) {
		// Positive transition of NDAC.  Data acknowledge.
		if (state == IEEE_STATE_TALK) {
			dav_i = true;
			eoi_i = true;
			if (!nrfd_o) {
				DPRINTF(4, "PetIeeeHw::%s: data read: "
					"0x%02x\n", __func__, dio ^ 0xff);
				if (ieeeDev)
					ieeeDev->ieeeTalkDataAck(fnum);
			}
			sender_timeout = SENDER_TIMEOUT_CYCLES;
		}
	}
}

void
PetIeeeHw::cycle(void)
{
	if (state == IEEE_STATE_TALK && sender_timeout &&
	    --sender_timeout == 0) {
		DPRINTF(1, "PetIeeeHw::%s: sender timeout!\n", __func__);
		this->reset();
	}
}
