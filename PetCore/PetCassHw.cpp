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

// PetCassHw.cpp

#include <stdint.h>

#include "PetCassHw.h"

#ifdef DEBUGCASS
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGCASS) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif


// Half pulse widths in machine cycles.
#define B_SYMBOL        175
#define C_SYMBOL        260
#define D_SYMBOL        350

// State machine identifiers.  State machine relies on these enum values
// being in this sequence.
enum {
	CASS_IDLE =                 0,
	CASS_LOAD_CARRIER1 =        1,
	CASS_LOAD_HDR1 =            2,
	CASS_LOAD_CARRIER2 =        3,
	CASS_LOAD_HDR2 =            4,
	CASS_LOAD_CARRIER3 =        5,
	CASS_LOAD_PROG1 =           6,
	CASS_LOAD_CARRIER4 =        7,
	CASS_LOAD_PROG2 =           8,
	CASS_LOAD_CARRIER5 =        9,

	CASS_SAVE_CARRIER1 =        10,
	CASS_SAVE_HDR1 =            11,
	CASS_SAVE_HDR2 =            12,
	CASS_SAVE_PROG1 =           13,
	CASS_SAVE_PROG2 =           14
};

void
PetCassHw::do_load_byte(uint8_t byte)
{
	DPRINTF(2, "PetCassHw::%s: byte=0x%02x bitcount=%d\n", __func__, byte,
		bitcount);

	// XXX: I have a member called simply bit and it conflicts below.
	// should be something like bitcount or ...
	if (bitcount < 4) {
		delay_cycle = ((bitcount >> 1) & 1) ?
			C_SYMBOL: D_SYMBOL;
	} else if (bitcount >=4 && bitcount < 36) {
		int bit = ((byte >> ((bitcount - 4) >> 2)) & 1);
		delay_cycle = (bit ^ ((bitcount >> 1) & 1)) ?
			C_SYMBOL : B_SYMBOL;
	} else if (bitcount >= 36 && bitcount < 40) {
		int i;
		int parity = 1;
		for (i = 0; i < 8; i++) {
			parity ^= (byte & 1);
			byte >>= 1;
		}
		delay_cycle = (parity ^ ((bitcount >> 1) & 1)) ?
			C_SYMBOL : B_SYMBOL;
	}

	rdata = (bitcount & 1) != 0;
	if (++bitcount > 39) {
		bitcount = 0;
		offset++;
	}
}

void
PetCassHw::cassLoad(const uint8_t *proghdr, const uint8_t *data, int len)
{
	DPRINTF(1, "PetCassHw::%s: proghdr=%p data=%p len=%d\n", __func__,
		proghdr, data, len);

	state = CASS_LOAD_CARRIER1;
	bitcount = 16000; // length of carrier.
	progdata = (uint8_t *)data;
	hdr_data = (uint8_t *)proghdr;
	data_len = len;

	csense = 1;
	rdata = 1;
	delay_cycle = B_SYMBOL;
}


void
PetCassHw::cassSave(uint8_t *proghdr, uint8_t *data, int maxlen)
{
	DPRINTF(1, "PetCassHw::%s: proghdr=%p data=%p maxlen=%d\n", __func__,
		proghdr, data, maxlen);

	state = CASS_SAVE_CARRIER1;
	progdata = data;
	hdr_data = proghdr;
	data_len = maxlen;
	offset = 0;
	bitcount = 0;

	csense = 1;
	rdata = 1;
	delay_cycle = 1;
	edge_count = 0;
}

#define IS_B_SYM(w) ((w) >= B_SYMBOL * 2 - 75 && (w) <= B_SYMBOL * 2 + 75)
#define IS_C_SYM(w) ((w) >= C_SYMBOL * 2 - 75 && (w) <= C_SYMBOL * 2 + 75)
#define IS_D_SYM(w) ((w) >= D_SYMBOL * 2 - 75 && (w) <= D_SYMBOL * 2 + 75)

void
PetCassHw::writeData(int wdata)
{
	// We only work on the rising edges.
	if (!wdata ||
	    !(state >= CASS_SAVE_CARRIER1 && state <= CASS_SAVE_PROG2))
		return;

	// Get pulse width in machine cycles (microseconds).
	int pwidth = edge_count;
	edge_count = 0;

	DPRINTF(3, "PetCassHw::%s: pwidth=%d\n", __func__, pwidth);

	// Look for carrier, 50 or more B symbols in a row.
	if (state == CASS_SAVE_CARRIER1) {
		if (IS_B_SYM(pwidth)) {
			if (++bitcount > 50) {
				bitcount = 0;
				blocklen = 192;
				DPRINTF(1, "PetCassHw%s: received carrier.\n",
					__func__);
				state++;
			}
		}
		else
			bitcount = 0;
		return;
	}

	// A D symbol starts a byte.
	if (IS_D_SYM(pwidth)) {
		bitcount = 1;
		return;
	}

	if ((IS_B_SYM(pwidth) || IS_C_SYM(pwidth)) && bitcount > 0 &&
	    bitcount < 19) {

		// DB is end of block marker.  Skip.
		if (bitcount == 1 && !IS_C_SYM(pwidth)) {
			bitcount = 0;
			return;
		}

		if (bitcount == 18) {
			int i, parity = 1;

			// This is the parity bit. XXX: what if it's bad?
			for (i = 0; i < 8; i++)
				parity ^= ((byte >> i) & 1);
			if (IS_C_SYM(pwidth))
				parity ^= 1;

			DPRINTF(2, "PetCassHw::%s: byte: %02x parity %s "
				"offset=%d\n", __func__, byte,
				parity ? "bad" : "good", offset);

			switch (state) {
			case CASS_SAVE_HDR1:
				if (offset == 201) {
					DPRINTF(1, "PetCassHw::%s: "
						"got header1.\n", __func__);
					// check parity?
					state++;
					offset = 0;
					break;
				}
				if (offset >= 9 && offset < 201)
					hdr_data[offset - 9] = byte;
				offset++;
				break;
			case CASS_SAVE_HDR2:
				if (offset == 201) {
					state++;
					offset = 0;

					blocklen = (hdr_data[3] +
						256 * hdr_data[4]) -
						(hdr_data[1] +
						256 * hdr_data[2]);

					DPRINTF(1, "PetCassHw%s: "
						"got header2: len=%d\n",
						__func__, blocklen);

					// Copy start address into data.
					progdata[0] = hdr_data[1];
					progdata[1] = hdr_data[2];
					break;
				}
				offset++;
				break;
			case CASS_SAVE_PROG1:
				if (offset == 9 + blocklen) {
					DPRINTF(1, "PetCassHw::%s: "
						"got prog1\n", __func__);
					// XXX: parity byte. check?
					state++;
					offset = 0;
					break;
				}
				if (offset >= 9 && offset < 7 + data_len)
					progdata[offset - 7] = byte;
				offset++;
				break;
			case CASS_SAVE_PROG2:
				if (offset == 9 + blocklen) {
					DPRINTF(1, "PetCassHw::%s:"
						"save done!\n", __func__);
					// XXX: parity byte. check?
					state = CASS_IDLE;
					delay_cycle = 0;
					data_len = blocklen + 2;
					csense = 0;
					if (cass_done_cb)
						cass_done_cb(data_len);
					break;
				}
				offset++;
				break;
			} // switch

		}
		else if ((bitcount & 1) == 0) {
			byte >>= 1;
			if (IS_C_SYM(pwidth))
				byte |= 0x80;
		}

		bitcount++;
	}
}

void
PetCassHw::setMotor(bool set)
{
	DPRINTF(1, "PetCassHw::%s: set=%d\n", __func__, set);
}

// Can also be used to cancel a load or save.
void
PetCassHw::reset(void)
{
	DPRINTF(1, "PetCassHw::%s:\n", __func__);

	state = CASS_IDLE;
	delay_cycle = 0;
	csense = 0;
	rdata = 1;
}

void
PetCassHw::cycle(void)
{
	edge_count++;
	if (delay_cycle == 0 || --delay_cycle > 0)
		return;

	switch (state) {
	case CASS_IDLE:
		break;

	case CASS_LOAD_CARRIER1:
	case CASS_LOAD_CARRIER2:
	case CASS_LOAD_CARRIER3:
	case CASS_LOAD_CARRIER4:
	case CASS_LOAD_CARRIER5:
		if (--bitcount == 0) {
			offset = 0;
			bitcount = 0;
			parity = 0;

			if (state == CASS_LOAD_CARRIER5) {
				// Done!
				DPRINTF(1, "%s: load done!\n", __func__);
				csense = 0;
				rdata = 1;
				state = CASS_IDLE;
				if (cass_done_cb)
					cass_done_cb(data_len);
			} else {
				rdata = 1;
				delay_cycle = B_SYMBOL;
				state++;
			}
		}
		else {
			rdata =	((bitcount & 1) == 0);
			delay_cycle = B_SYMBOL;
		}
		break;

	case CASS_LOAD_HDR1:
	case CASS_LOAD_HDR2:
		if (offset < 202) {
			// Get byte.
			if (offset < 9)
				byte = (state == CASS_LOAD_HDR1 ?
					0x89 - offset : 0x09 - offset);
			else if (offset < 201) {
				byte = hdr_data[offset - 9];
				if (bitcount == 1)
					parity ^= byte;
			}
			else // offset == 201
				byte = parity;

			do_load_byte(byte);
		}
		else {
			delay_cycle = D_SYMBOL;
			rdata = ((bitcount & 1) != 0);
			if (++bitcount == 2) {
				bitcount = state == CASS_LOAD_HDR1 ?
					800 : 8000;
				state++;
			}
		}
		break;

	case CASS_LOAD_PROG1:
	case CASS_LOAD_PROG2:
		if (offset < 10 + data_len) {
			// Get byte.
			if (offset < 9)
				byte = (state == CASS_LOAD_PROG1 ?
					0x89 - offset : 0x09 - offset);
			else if (offset < 9+data_len) {
				byte = progdata[offset - 9];
				if (bitcount == 1)
					parity ^= byte;
			}
			else // offset == 9+data_len
				byte = parity;

			do_load_byte(byte);
		}
		else {
			delay_cycle = D_SYMBOL;
			rdata = ((bitcount & 1) != 0);
			if (++bitcount == 2) {
				bitcount = 800;
				state++;
			}
		}
		break;
	}
}
