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

#include <stdint.h>

#include "Apple2Disk2.h"

#ifdef DEBUGIO
extern unsigned int cycles;
#  include <cstdio>
#  define DPRINTF(l, f, arg...)					\
	do { if ((l) <= DEBUGIO) printf("[%d] " f, cycles, arg); } while (0)
#else
#  define DPRINTF(l, f, arg...)
#endif

#define	MOTOR_OFF	0x08
#define MOTOR_ON	0x09
#define DRV0EN		0x0a
#define DRV1EN		0x0b
#define Q6L		0x0c
#define Q6H		0x0d
#define Q7L		0x0e
#define Q7H		0x0f

#define TRACK_SIZE	6656

// Perform actions that are side-effects of either read or write.
void
Apple2Disk2::reference(uint16_t addr)
{
	if (addr < 0x08) {
		// Stepper motor on.
		if ((addr & 1) != 0) {
			int p = ((addr >> 1) & 3); // phase we're turning on.

			if (((phase + 1) & 3) == p) {
				// Ascending order, track arm moves inward.
				phase = p;
				if ((phase & 1) == 0) {
					if (++track >= 35)
						track = 35;
					DPRINTF(2, "Apple2Disk2::%s: trk=%d\n",
						__func__, track);
				}
			} else if (((phase - 1) & 3) == p) {
				// Descending order, track arm moves outward.
				phase = p;
				if ((phase & 1) == 0) {
					if (--track < 0)
						track = 0; // CLICK! CLICK!
					DPRINTF(2, "Apple2Disk2::%s: trk=%d\n",
						__func__, track);
				}
			}
		}
		if (disk_cb)
			disk_cb(motor, track);
	} else
		switch (addr) {
		case MOTOR_OFF:
			DPRINTF(2, "Apple2Disk2::%s: motor off.\n", __func__);
			motor = false;
			if (disk_cb)
				disk_cb(motor, track);
			break;
		case MOTOR_ON:
			DPRINTF(2, "Apple2Disk2::%s: motor on.\n", __func__);
			motor = true;
			if (disk_cb)
				disk_cb(motor, track);
			break;
		case DRV0EN:
			DPRINTF(2, "Apple2Disk2::%s: drv0 enable.\n",
				__func__);
			drv1 = false;
			break;
		case DRV1EN:
			DPRINTF(2, "Apple2Disk2::%s: drv1 enable.\n",
				__func__);
			drv1 = true;
			break;
		case Q6L:
			q6 = false;
			// Strobe data latch for I/O
			if (nibfile && motor && !drv1) {
				if (++offset == TRACK_SIZE)
					offset = 0;
				if (q7 && !writeprot) {
					// Write to disk.
					nibfile[track * TRACK_SIZE + offset] =
						data_latch;
					nibmodified = true;
				}
			}
			break;
		case Q6H:
			// Load data latch.  Also sense write-protect.
			q6 = true;
			break;
		case Q7L:
			// Prepare latch for input.
			q7 = false;
			break;
		case Q7H:
			// Prepare latch for output.
			q7 = true;
			break;
		}
}

void
Apple2Disk2::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(2, "Apple2Disk::%s: addr=0x%x d8=0x%x\n", __func__, addr, d8);

	reference(addr);

	if (addr == Q6H)
		data_latch = d8;
}

uint8_t
Apple2Disk2::read(uint16_t addr)
{
	uint8_t d8 = 0xff;

	reference(addr);

	if (addr == Q6L && nibfile && motor && !drv1 && !q7)
		// Read from disk.
		d8 = nibfile[track * TRACK_SIZE + offset];
	else if (addr == Q7L && q6)
		// Sense write protect.
		d8 = writeprot ? 0x80 : 0x00;

	DPRINTF(2, "Apple2Disk::%s: addr=0x%x d8=0x%x\n", __func__, addr, d8);

	return d8;
}

void
Apple2Disk2::reset(void)
{
	DPRINTF(1, "Apple2Disk::%s:\n", __func__);

	phase = 0;
	track = 20;
	motor = false;
	drv1 = false;
	offset = 0;
	q6 = false;
	q7 = false;

	if (disk_cb)
		disk_cb(motor, track);
}
