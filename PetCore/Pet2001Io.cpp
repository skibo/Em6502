//
// Copyright (c) 2012, 2015, 2020 Thomas Skibo.
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

// Pet2001Io.cpp

#include <stdint.h>

#include "Pet2001Io.h"

#include "Cpu6502.h"
#include "PetVideo.h"
#include "PetCassHw.h"
#include "PetIeeeHw.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

// Chip register offsets in I/O space (0xE800).

#define PIA1_PA		0x10
#define PIA1_CRA	0x11
#define PIA1_PB		0x12
#define PIA1_CRB	0x13

#define PIA2_PA		0x20
#define PIA2_CRA	0x21
#define PIA2_PB		0x22
#define PIA2_CRB	0x23

#define VIA_DRB		0x40
#define VIA_DRA		0x41
#define VIA_DDRB	0x42
#define VIA_DDRA	0x43
#define VIA_T1CL	0x44
#define VIA_T1CH	0x45
#define VIA_T1LL	0x46
#define VIA_T1LH	0x47
#define VIA_T2CL	0x48
#define VIA_T2CH	0x49
#define VIA_SR		0x4a
#define VIA_ACR		0x4b
#define VIA_PCR		0x4c
#define VIA_IFR		0x4d
#define VIA_IER		0x4e
#define VIA_ANH		0x4f

// PET I/O in a nut-shell:
//
// PIA1.PA[3:0] =>  keyrow (0..9, 11=light LED)
//     .PA[4]   <=  cass #1 switch
//     .PA[5]   <=  cass #2 switch
//     .PA[6]   <=  _EOI in
//     .PA[7]   <=  diag jumper (0=diag, 1=basic)
//     .PB[7:0] <=  keyin
//     .CA1     <=  cass #1 read
//     .CA2     =>  blank / _EOI out
//     .CB1     <=  sync
//     .CB2     =>  cass #1 motor
//
// PIA2.PA[7:0] <=  IEEE DI
//     .PB[7:0] =>  IEEE DO
//     .CA1     <=  _ATN in
//     .CA2     =>  _NDAC out
//     .CB1     <=  _SRQ in
//     .CB2     =>  _DAV out
//
//  VIA.PA[7:0] <=> User Port
//     .PB[0]   <=  _NDAC in
//     .PB[1]   =>  _NRFD out
//     .PB[2]   =>  _ATN out
//     .PB[3]   =>  cass write
//     .PB[4]   =>  cass #2 motor
//     .PB[5]   <=  sync
//     .PB[6]   <=  _NRFD in
//     .PB[7]   <=  _DAV in
//     .CA1     <=> User Port
//     .CA2     =>  charset sel (0=graphics, 1=lower-case)
//     .CB1     <=  cass #2 read
//     .CB2     <=> User Port (usually sound output)
//

void
Pet2001Io::reset(void)
{
	for (int i = 0; i < 10; i++)
		keyrow[i] = 0xff;

	pia1_pa_in =	0xf0;
	pia1_pa_out =	0;
	pia1_ddra =	0;
	pia1_cra =	0;
	pia1_pb_in =	0xff;
	pia1_pb_out =	0;
	pia1_ddrb =	0;
	pia1_crb =	0;
	pia1_ca2 =	0;
	pia1_cb1 =	0;

	pia2_pa_in =	0xff;
	pia2_pa_out =	0;
	pia2_ddra =	0;
	pia2_cra =	0;
	pia2_pb_in =	0xff;
	pia2_pb_out =	0;
	pia2_ddrb =	0;
	pia2_crb =	0;

	via_drb_in =	0xff;
	via_drb_out =	0;
	via_dra_in =	0xff;
	via_dra_out =	0;
	via_ddrb =	0;
	via_ddra =	0;
	via_t1cl =	0xff;
	via_t1ch =	0xff;
	via_t1_1shot =	0;
	via_t1_undf =	0;
	via_t1ll =	0xff;
	via_t1lh =	0xff;
	via_t2cl =	0xff;
	via_t2ch =	0xff;
	via_t2_1shot =	0;
	via_t2_undf =	0;
	via_sr =	0;
	via_sr_cntr =	0;
	via_sr_start =  0;
	via_acr =	0;
	via_pcr =	0;
	via_ifr =	0;
	via_ier =	0x80;

	via_cb1 =	1;
	via_cb2 =	1;

	video_cycle =	0;

	if (ieee)
		ieee->reset();
	if (cass)
		cass->reset();
}

// Update the IRQ level based upon PIA and VIA.
void
Pet2001Io::updateIrq(void)
{
	uint8_t irq = 0;

	if ((pia1_cra & 0x81) == 0x81 || (pia1_cra & 0x48) == 0x48 ||
		(pia1_crb & 0x81) == 0x81 || (pia1_crb & 0x48) == 0x48)
		irq = 1;
	if ((via_ifr & via_ier & 0x7f) != 0) {
		via_ifr |= 0x80;
		irq = 1;
	}
	else
		via_ifr &= ~0x80;

	DPRINTF(2, "Pet2001Io::%s: irq=%d\n", __func__, irq);

	cpu->setIrq(irq);
}

void
Pet2001Io::setKeyrow(int row, uint8_t keyrow)
{
	DPRINTF(1, "Pet2001Io::%s: row=%d keyrow=0x%02x\n", __func__, row,
		keyrow);

	this->keyrow[row] = keyrow;

	// Update pia1_pb.
	if ((pia1_pa_out & 15) < 10)
		pia1_pb_in = this->keyrow[pia1_pa_out & 15];
	else
		pia1_pb_in = 0xff;
}

void
Pet2001Io::sync(int sync)
{
	DPRINTF(2, "Pet2001Io::%s: sync=%d\n", __func__, sync);

	// SYNC is wired to PIA1.CB1 and VIA.PB[5]
	if (sync != pia1_cb1) {
		if (((pia1_crb & 0x02) != 0 && sync) ||
		    ((pia1_crb & 0x02) == 0 && !sync)) {
			pia1_crb |= 0x80;
			if ((pia1_crb & 0x01) != 0)
				updateIrq();
		}
		pia1_cb1 = sync;
	}

	// Set/clr VIA.PB[5]
	via_drb_in = sync ? (via_drb_in | 0x20) : (via_drb_in & ~0x20);
}

uint8_t
Pet2001Io::read(uint16_t addr)
{
	uint8_t d8 = 0;

	switch (addr & 0x13) {
	case PIA1_PA:
		if ((pia1_cra & 0x04) != 0) {
			// Clear IRQs in CRA as side-effect of reading PA.
			if ((pia1_cra & 0xC0) != 0) {
				pia1_cra &= 0x3F;
				updateIrq();
			}
			// Cassette sense is PIA1.PA[4]
			if(cass && (pia1_ddra & 0x10) == 0) {
				if (cass->sense())
					pia1_pa_in &= ~0x10;
				else
					pia1_pa_in |= 0x10;
			}
			if (ieee && (pia1_ddra & 0x40) == 0) {
				if (ieee->eoiIn())
					pia1_pa_in |= 0x40;
				else
					pia1_pa_in &= 0xbf;
			}
			d8 = (pia1_pa_in & ~pia1_ddra) |
				(pia1_pa_out & pia1_ddra);
		}
		else
			d8 = pia1_ddra;
		break;
	case PIA1_CRA:
		d8 = pia1_cra;
		break;
	case PIA1_PB:
		if ((pia1_crb & 0x04) != 0) {
			// Clear IRQs in CRB as side-effect of reading PB.
			if ((pia1_crb & 0xC0) != 0) {
				pia1_crb &= 0x3F;
				updateIrq();
			}
			d8 = (pia1_pb_in & ~pia1_ddrb) |
				(pia1_pb_out & pia1_ddrb);
		}
		else
			d8 = pia1_ddrb;
		break;
	case PIA1_CRB:
		d8 = pia1_crb;
		break;
	}

	switch (addr & 0x23) {
	case PIA2_PA:
		if ((pia2_cra & 0x04) != 0) {
			// Clear IRQs in CRA as side-effect of reading PA.
			if ((pia2_cra & 0xC0) != 0) {
				pia2_cra &= 0x3F;
				updateIrq();
			}
			if (ieee && pia2_ddra == 0)
				pia2_pa_in = ieee->din();
			d8 = (pia2_pa_in & ~pia2_ddra) |
				(pia2_pa_out & pia2_ddra);
		}
		else
			d8 = pia2_ddra;
		break;
	case PIA2_CRA:
		d8 = pia2_cra;
		break;
	case PIA2_PB:
		if ((pia2_crb & 0x04) != 0) {
			// Clear IRQs in CRB as side-effect of reading PB.
			if ((pia2_crb & 0x3F) != 0) {
				pia2_crb &= 0x3F;
				updateIrq();
			}
			d8 = (pia2_pb_in & ~pia2_ddrb) |
				(pia2_pb_out & pia2_ddrb);
		}
		else
			d8 = pia2_ddrb;
		break;
	case PIA2_CRB:
		if (ieee) {
			if (ieee->srqIn())
				pia2_crb |= 0x80;
			else
				pia2_crb &= 0x7f;
		}
		d8 = pia2_crb;
		break;
	}

	switch (addr & 0x4f) {
	case VIA_DRB:
		// Clear CB2 interrupt flag IFR3 (if not "independent"
		// interrupt)
		//
		if ((via_pcr & 0xa0) != 0x20) {
			if ((via_ifr & 0x08) != 0) {
				via_ifr &= ~0x08;
				if ((via_ier & 0x08) != 0)
					updateIrq();
			}
		}
		// Clear CB1 interrupt flag IFR4
		if ((via_ifr & 0x10) != 0) {
			via_ifr &= ~0x10;
			if ((via_ier & 0x10) != 0)
				updateIrq();
		}
		if (ieee) {
			if ((via_ddrb & 0x80) == 0) {
				if (ieee->davIn())
					via_drb_in |= 0x80;
				else
					via_drb_in &= 0x7f;
			}
			if ((via_ddrb & 0x40) == 0) {
				if (ieee->nrfdIn())
					via_drb_in |= 0x40;
				else
					via_drb_in &= 0xbf;
			}
			if ((via_ddrb & 0x01) == 0) {
				if (ieee->ndacIn())
					via_drb_in |= 0x01;
				else
					via_drb_in &= 0xfe;
			}
		}
		d8 = (via_drb_in & ~via_ddrb) |
			(via_drb_out & via_ddrb);
		break;
	case VIA_DRA:
		// Clear CA2 interrupt flag IFR0 (if not "independent"
		// interrupt)
		if ((via_pcr & 0x0a) != 0x02) {
			if ((via_ifr & 0x01) != 0) {
				via_ifr &= ~0x01;
				if ((via_ier & 0x01) != 0)
					updateIrq();
			}
		}

		// Clear CA1 interrupt flag IFR1
		if ((via_ifr & 0x02) != 0) {
			via_ifr &= ~0x02;
			if ((via_ier & 0x02) != 0)
				updateIrq();
		}
		d8 = (via_dra_in & ~via_ddra) |
			(via_dra_out & via_ddra);
		break;
	case VIA_DDRB:
		d8 = via_ddrb;
		break;
	case VIA_DDRA:
		d8 = via_ddra;
		break;
	case VIA_T1CL:
		// Clear T1 interrupt flag IFR6 as side-effect of read T1CL.
		if ((via_ifr & 0x40) != 0) {
			via_ifr &= ~0x40;
			if ((via_ier & 0x40) != 0)
				updateIrq();
		}
		d8 = via_t1cl;
		break;
	case VIA_T1CH:
		d8 = via_t1ch;
		break;
	case VIA_T1LL:
		d8 = via_t1ll;
		break;
	case VIA_T1LH:
		d8 = via_t1lh;
		break;
	case VIA_T2CL:
		// Clear T2 interrupt flag IFR5 as side-effect of read T2CL
		if ((via_ifr & 0x20) != 0) {
			via_ifr &= ~0x20;
			if ((via_ier & 0x20) != 0)
				updateIrq();
		}
		d8 = via_t2cl;
		break;
	case VIA_T2CH:
		d8 = via_t2ch;
		break;
	case VIA_SR:
		// Clear SR int flag IFR2
		if ((via_ifr & 0x04) != 0) {
			via_ifr &= ~0x04;
			if ((via_ier & 0x04) != 0)
				updateIrq();
		}
		// Start SR counter.
		if ((via_acr & 0x1c) != 0 && via_sr_cntr == 0)
			via_sr_start = 1;
		d8 = via_sr;
		break;
	case VIA_ACR:
		d8 = via_acr;
		break;
	case VIA_PCR:
		d8 = via_pcr;
		break;
	case VIA_IFR:
		d8 = via_ifr;
		break;
	case VIA_IER:
		d8 = via_ier;
		break;
	case VIA_ANH:
		// VIA_PA with no handshake.
		d8 = (via_dra_in & ~via_ddra) |	(via_dra_out & via_ddra);
		break;
	}

	DPRINTF(2, "Pet2001Io:read(addr=0x%04x) = 0x%02x\n", addr, d8);

	return d8;
}

void
Pet2001Io::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(2, "Pet2001Io:write(addr=0x%04x, d8=0x%02x)\n", addr, d8);

	switch (addr & 0x13) {
	case PIA1_PA:
		if ((pia1_cra & 0x04) != 0) {
			pia1_pa_out = d8;
			// Which keyrow are we accessing?
			if ((pia1_pa_out & 15) < 10)
				pia1_pb_in = keyrow[pia1_pa_out & 15];
			else
				pia1_pb_in = 0xff;
		}
		else
			pia1_ddra = d8;
		break;
	case PIA1_CRA:
		pia1_cra = (pia1_cra & 0xc0) | (d8 & 0x3f);
		// Change in CA2? (screen blank)
		if (((pia1_cra & 0x30) == 0 || (pia1_cra & 0x38) == 0x38) &&
		    !pia1_ca2) {
			// CA2 transitioning high. (Screen On)
			pia1_ca2 = 1;
			if (video)
				video->setBlank(false);
			if (ieee)
				ieee->eoiOut(true);
		}
		else if ((pia1_cra & 0x38) == 0x30 && pia1_ca2) {
			// CA2 transitioning low. (Screen Blank)
			pia1_ca2 = 0;
			if (video)
				video->setBlank(true);
			if (ieee)
				ieee->eoiOut(false);
		}
		break;
	case PIA1_PB:
		if ((pia1_crb & 0x04) != 0)
			pia1_pb_out = d8;
		else
			pia1_ddrb = d8;
		break;
	case PIA1_CRB:
		if (cass) {
                        if ((d8 & 0x20) == 0)
                                cass->setMotor(true);
                        else if (((pia1_crb ^ d8) & 0x08) != 0)
                                cass->setMotor((d8 & 0x08) == 0);
                }
		pia1_crb = (pia1_crb & 0xc0) | (d8 & 0x3f);
		updateIrq();
		break;
	}

	switch (addr & 0x23) {
	case PIA2_PA:
		if ((pia2_cra & 0x04) != 0)
			pia2_pa_out = d8;
		else
			pia2_ddra = d8;
		break;
	case PIA2_CRA:
                if (ieee) {
                        if ((d8 & 0x20) == 0)
                                ieee->ndacOut(true);
                        else if (((pia2_cra ^ d8) & 0x08) != 0)
                                ieee->ndacOut((d8 & 0x08) != 0);
                }
		pia2_cra = (pia2_cra & 0xc0) | (d8 & 0x3f);
		break;
	case PIA2_PB:
		if ((pia2_crb & 0x04) != 0) {
			if (ieee && pia2_ddrb == 0xff && pia2_pb_out != d8)
				ieee->dout(d8);
			pia2_pb_out = d8;
		}
		else
			pia2_ddrb = d8;
		break;
	case PIA2_CRB:
                if (ieee) {
                        if ((d8 & 0x20) == 0)
                                ieee->davOut(true);
                        else if (((pia2_crb ^ d8) & 0x08) != 0)
                                ieee->davOut((d8 & 0x08) != 0);
                }
		pia2_crb = (pia2_crb & 0xc0) | (d8 & 0x3f);
		break;
	}

	switch (addr & 0x4f) {
	case VIA_DRB:
		// Clear CB2 interrupt flag IFR3 (if not "independent"
		// interrupt)
		if ((via_pcr & 0xa0) != 0x20) {
			if ((via_ifr & 0x08) != 0) {
				via_ifr &= ~0x08;
				if ((via_ier & 0x08) != 0)
					updateIrq();
			}
		}
		//Clear CB1 interrupt flag IFR4
		if ((via_ifr & 0x10) != 0) {
			via_ifr &= ~0x10;
			if ((via_ier & 0x10) != 0)
				updateIrq();
		}
		// Cass write change?
		if (cass && ((via_drb_out ^ d8) & 0x08 & via_ddrb) != 0)
			cass->writeData((d8 & 0x08) != 0);
		// IEEE output changes?
		if (ieee && ((via_drb_out ^ d8) & 0x04 & via_ddrb) != 0)
			ieee->atnOut((d8 & 0x04) != 0);
		if (ieee && ((via_drb_out ^ d8) & 0x02 & via_ddrb) != 0)
			ieee->nrfdOut((d8 & 0x02) != 0);
		via_drb_out = d8;
		break;
	case VIA_DRA:
		// Clear CA2 interrupt flag IFR0 (if not "independent"
		// interrupt)
		if ((via_pcr & 0x0a) != 0x02) {
			if ((via_ifr & 0x01) != 0) {
				via_ifr &= ~0x01;
				if ((via_ier & 0x01) != 0)
					updateIrq();
			}
		}

		// Clear CA1 interrupt flag IFR1
		if ((via_ifr & 0x02) != 0) {
			via_ifr &= ~0x02;
			if ((via_ier & 0x02) != 0)
				updateIrq();
		}
		via_dra_out = d8;
		break;
	case VIA_DDRB:
		via_ddrb = d8;
		break;
	case VIA_DDRA:
		via_ddra = d8;
		break;
	case VIA_T1CL:
		via_t1ll = d8;      // LATCH
		break;
	case VIA_T1CH:
		// Clear T1 interrupt flag IFR6 as side-effect of write T1CH
		if ((via_ifr & 0x40) != 0) {
			via_ifr &= ~0x40;
			if ((via_ier & 0x40) != 0)
				updateIrq();
		}
		// Write to T1LH and set via_t1_undf to set T1 next cycle.
		via_t1lh = d8;
		via_t1_undf = 1;
		via_t1_1shot = 1;
		break;
	case VIA_T1LL:
		via_t1ll = d8;
		break;
	case VIA_T1LH:
		// Clear T1 interrupt flag IFR6 as side-effect of write T1LH
		if ((via_ifr & 0x40) != 0) {
			via_ifr &= ~0x40;
			if ((via_ier & 0x40) != 0)
				updateIrq();
		}
		via_t1lh = d8;
		break;
	case VIA_T2CL:
		via_t2ll = d8;      // LATCH
		break;
	case VIA_T2CH:
		// Clear T2 interrupt flag IFR5 as side-effect of write T2CH
		if ((via_ifr & 0x20) != 0) {
			via_ifr &= ~0x20;
			if ((via_ier & 0x20) != 0)
				updateIrq();
		}
		if ((via_acr & 0x20) == 0)
			via_t2_1shot = 1;
		via_t2cl = via_t2ll;
		via_t2ch = d8;

		// Increment counter to take into account cycle() will
		// decrement it in the same cycle.
		if ((via_acr & 0x20) == 0 && ++via_t2cl == 0)
			++via_t2ch;
		break;
	case VIA_SR:
		// Clear SR int flag IFR2
		if ((via_ifr & 0x04) != 0) {
			via_ifr &= ~0x04;
			if ((via_ier & 0x04) != 0)
				updateIrq();
		}
		// Start the SR counter.
		if ((via_acr & 0x1c) != 0)
			via_sr_start = 1;
		via_sr = d8;
		break;
	case VIA_ACR:
		if ((d8 & 0x1c) == 0) {
			via_sr_cntr = 0;
			via_cb1 = 1;
		}
		via_acr = d8;
		break;
	case VIA_PCR:
		// Did we change CA2 output?
		if (video) {
			if ((via_pcr & 0x0e) != (d8 & 0x0e))
				video->setCharset((d8 & 0x0e) != 0x0c);
		}
		via_pcr = d8;
		break;
	case VIA_IFR:
		// Clear interrupt flags by writing 1s to the bits.
		via_ifr &= ~(d8 & 0x7f);
		updateIrq();
		break;
	case VIA_IER:
		if ((d8 & 0x80) != 0)
			via_ier |= d8;
		else
			via_ier &= ~d8;
		updateIrq();
		break;
	case VIA_ANH:
		// VIA_PA with no handshake.
		via_dra_out = d8;
		break;
	}
}

void
Pet2001Io::cycle(void)
{
	// Synthesize the SYNC signal at 60.1hz and 76.9% duty cycle.
	if (++video_cycle == 3840)
		sync(1);
	else if (video_cycle == 16640) {
		sync(0);
		if (video)
			video->sync();
		video_cycle = 0;
	}

	if (video)
		video->cycle();


	if (ieee)
		ieee->cycle();

	// Handle VIA.TIMER1
	if (via_t1_undf) {
		// T1 underflow.  Reload.
		via_t1cl = via_t1ll;
		via_t1ch = via_t1lh;
		via_t1_undf = 0;
	} else if (via_t1cl-- == 0) {
		if (via_t1ch-- == 0) {

			via_t1_undf = 1;

			// Interrupt?
			if (via_t1_1shot) {
				via_ifr |= 0x40;
				if ((via_ier & 0x40) != 0)
					updateIrq();
				if ((via_acr & 0x40) == 0)
					via_t1_1shot = 0;
			}
		}
	}

	// Handle VIA.TIMER2
	if (via_t2_undf) {
		via_t2cl = via_t2ll;
		if ((via_acr & 0x1c) == 0x10 && via_sr_cntr > 0) {
			// Free-running shift register.
			via_cb1 = !via_cb1;
			if (via_cb1) {
				via_sr = (via_sr >> 7) | (via_sr << 1);
				via_cb2 = via_sr & 1;
				DPRINTF(3, "Pet2001Io::%s: SR=0x%02x\n",
					__func__, via_sr);
			}
		} else if ((via_acr & 0xc) == 4 && via_sr_cntr > 0) {
			// Other SR modes clocked by T2.
			via_cb1 = !via_cb1;
			if (via_cb1) {
				if ((via_acr & 0x10) != 0)
					via_sr = (via_sr >> 7) | (via_sr << 1);
				else
					via_sr = (via_sr << 1) | 1;
				via_cb2 = via_sr & 1;
				if (--via_sr_cntr == 0) {
					via_ifr |= 0x04;
					if ((via_ier & 0x04) != 0)
						updateIrq();
				}
				DPRINTF(3, "Pet2001Io::%s: SR=0x%02x ctr=%d\n",
					__func__, via_sr, via_sr_cntr);
			}
		}
		via_t2_undf = 0;
	} else if ((via_acr & 0x20) == 0 && via_t2cl-- == 0) {
		// Reload T2L on next cycle?
		if ((via_acr & 0x14) != 0)
			via_t2_undf = 1;

		if (via_t2ch-- == 0) {
			// T2 underflow.
			if (via_t2_1shot) {
				via_ifr |= 0x20;
				if ((via_ier & 0x20) != 0)
					updateIrq();
				via_t2_1shot = 0;
			}
		}
	} // else if ((via_acr & 0x20) != 0 ... count PB6 negative edges.

	// Handle VIA.SR when in system clock mode.
	if (via_sr_cntr > 0 && (via_acr & 0xc) == 8) {
		via_cb1 = !via_cb1;
		if (via_cb1) {
			if ((via_acr & 0x10) != 0)
				via_sr = (via_sr >> 7) | (via_sr << 1);
			else
				via_sr = (via_sr << 1) | 1;
			via_cb2 = via_sr & 1;
			if (--via_sr_cntr == 0) {
				via_ifr |= 0x04;
				if ((via_ier & 0x04) != 0)
					updateIrq();
			}
			DPRINTF(3, "Pet2001Io::%s: SR=0x%02x ctr=%d\n",
				__func__, via_sr, via_sr_cntr);
		}
	}
	if (via_sr_start) {
		via_sr_start = 0;
		via_sr_cntr = (via_acr & 0x10) == 0 ? 8 : 9;
	}

	// Handle CB2 sound.
	if (audiobuf && (via_acr & 0x1c) == 0x10) {
		audiobuf[audiobuftail] = via_cb2 ? 0xff : 0x00;
		if (++audiobuftail >= audiobuflen)
			audiobuftail = 0;
	}

	// Look for changes in CA1 (cassette input).
	if (cass) {
		int read = cass->readData();
		if (read != pia1_ca1) {
			if (((pia1_cra & 0x02) == 0 && !read) ||
			    ((pia1_cra & 0x02) != 0 && read)) {
				pia1_cra |= 0x80;
				if ((pia1_cra & 0x01) != 0)
					updateIrq();
			}
			pia1_ca1 = read;
		}

		// Call cass->cycle() if motor is running.
		if ((pia1_crb & 0x08) == 0)
			cass->cycle();
	}
}
