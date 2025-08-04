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
#include <string.h>

#include "PetDisk.h"

#ifdef DEBUGIEEE
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGIEEE) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

#define SECTSIZE		256
#define DIRENTSIZE		32
#define MAX_TRACK		35
#define MAX_SECT		20

#define FNAMELEN		17

// Compute offset of sector 0 of any track.
//
//	tracks 1-17 have 21 sectors.
//	tracks 18-24 have 19 sectors.
//	tracks 25-30 have 18 sectors.
//	tracks 31-35 have 17 sectors.
int
PetDisk::trackOff(int track)
{
	int sector;

	if (track < 18)
		sector = (track - 1) * 21;
	else if (track < 25)
		sector = (track - 18) * 19 + 357;
	else if (track < 31)
		sector = (track - 25) * 18 + 490;
	else
		sector = (track - 31) * 17 + 598;

	return sector * SECTSIZE;
}

// Number of sectors in a track.  (See trackOff())
int
PetDisk::nsects(int track)
{
	if (track < 18)
		return 21;
	else if (track < 25)
		return 19;
	else if (track < 31)
		return 18;
	else
		return 17;
}

void
PetDisk::firstDirEnt(void)
{
	int hdr = trackOff(18);
	next_dirsec = trackOff(diskdata[hdr]) + diskdata[hdr + 1] * SECTSIZE;
	curr_dirent = SECTSIZE - DIRENTSIZE;
}

char *
PetDisk::nextDirEnt(enum fileType &ftype, int &fsize)
{
	static char fname[FNAMELEN];

	for (;;) {
		if (curr_dirent % SECTSIZE == (SECTSIZE - DIRENTSIZE)) {
			if (next_dirsec < 0)
				return NULL;

			curr_dirent = next_dirsec;
			if (diskdata[curr_dirent] < 1 ||
			    diskdata[curr_dirent] > MAX_TRACK ||
			    diskdata[curr_dirent + 1] > MAX_SECT)
				next_dirsec = -1;
			else
				next_dirsec = trackOff(diskdata[curr_dirent]) +
					diskdata[curr_dirent + 1] * SECTSIZE;
		} else
			curr_dirent += DIRENTSIZE;

		ftype = (enum fileType)	(diskdata[curr_dirent + 2] & 0x0f);
		if (ftype == FTYPE_DEL)
			continue;

		for (int i = 0; i < FNAMELEN - 1; i++)
			if (diskdata[curr_dirent + 5 + i] == 0xa0)
				fname[i] = '\0';
			else
				fname[i] = (char)diskdata[curr_dirent + 5 + i];
		fname[FNAMELEN - 1] = '\0';
		fsize = diskdata[curr_dirent + 30] +
			diskdata[curr_dirent + 31] * 256;

		return fname;
	}
}

void
PetDisk::blankDisk(void)
{
	DPRINTF(1, "PetDisk::%s:\n", __func__);

	diskdata_len = MAX_DISK_LEN;
	memset(diskdata, 0, diskdata_len);

	int hdr = trackOff(18);
	memset(diskdata + hdr + 0x90, 0xa0, 16);
	memcpy(diskdata + hdr + 0x90, "NEWDISK", 7);

	diskdata[hdr + 0x00] = 0x12; // First directory track
	diskdata[hdr + 0x01] = 0x01; // First directory sector
	diskdata[hdr + 0x02] = 0x41; // DOS version "A"

	blocks_free = 0;

	// Fill out BAM entries
	for(int trk = 0; trk < 36; trk++) {
		int n = nsects(trk);
		if (trk == 18)
			n = 0;

		diskdata[hdr + trk * 4] = n;
		blocks_free += n;

		uint32_t mask = 0xffffff >> (24 - n);
		diskdata[hdr + trk * 4 + 1] = mask & 0xff;
		diskdata[hdr + trk * 4 + 2] = (mask >> 8) & 0xff;
		diskdata[hdr + trk * 4 + 3] = (mask >> 16) & 0xff;
	}

	diskdata[hdr + 0xa0] = 0xa0;
	diskdata[hdr + 0xa1] = 0xa0;
	diskdata[hdr + 0xa2] = 1;	// Disk ID
	diskdata[hdr + 0xa4] = 0xa0;
	diskdata[hdr + 0xa5] = 0x32;	// DOS type "2A"
	diskdata[hdr + 0xa6] = 0x41;
	memset(diskdata + hdr + 0xa7, 0xa0, 4);

	// Interleave directory sectors
	const uint8_t intlv[] =
		{ 1, 11, 2, 12, 3, 13, 4, 14, 5, 15, 6, 16, 7, 17, 8, 18, 9 };

	// Blank directory entries
	for (int i = 0; i < 16; i++) {
		int off = intlv[i] * SECTSIZE;

		// Chain to next sector.
		diskdata[hdr + off] = 18;
		diskdata[hdr + off + 1] = intlv[i + 1];
	}
}

void
PetDisk::loadDisk(uint8_t data[], int len)
{
	DPRINTF(1, "PetDisk::%s: len=%d\n", __func__, len);

	// XXX: we need to protect against short files and mal-formed disk
	// images else we get all kinds of out of range errors.
	//

	diskdata_len = len;
	if (len == 0)
		return;

	memcpy(diskdata, data, len);

#ifdef DEBUGIEEE
	char *s;
	enum fileType ftype;
	int fsize;
	firstDirEnt();
	while ((s = nextDirEnt(ftype, fsize)) != NULL) {
		DPRINTF(1, "PetDisk::%s: file=%s  ftype=%d fsize=%d\n",
			__func__, s, ftype, fsize);
	}
#endif // DEBUGIEEE

	blocks_free = 0;
	int hdr = trackOff(18);
	for (int trk = 0; trk < 36; trk++)
		blocks_free += diskdata[hdr + trk *4];
}

int
PetDisk::readDirectory(uint8_t *data, int maxlen)
{
	DPRINTF(1, "PetDisk::%s:\n", __func__);

	int i, len = 0;
	static const char *ftypesnames[] =
		{ "DEL", "SEQ", "PRG", "USR", "REL" };

	// Start address
	data[len++] = 0x01;
	data[len++] = 0x04;

	char *s;
	enum fileType ftype;
	int fsize;

	// Disk name
	if (maxlen - len < 23)
		return len;
	data[len++] = 1;	// Phony link
	data[len++] = 1;

	data[len++] = 0;	// Line number
	data[len++] = 0;

	data[len++] = 18;	// Inverse
	data[len++] = '"';
	int hdr = trackOff(18);
	for (i = 0; i < 16; i++)
		if (diskdata[hdr + 0x90 + i] == 0xa0)
			data[len++] = ' ';
		else
			data[len++] = diskdata[hdr + 0x90 + i];
	data[len++] = '"';
	data[len++] = 0;

	firstDirEnt();
	while ((s = nextDirEnt(ftype, fsize)) != NULL) {
		DPRINTF(1, "PetDisk::%s: file=%s  ftype=%d fsize=%d\n",
			__func__, s, ftype, fsize);

		if (maxlen - len < 30)
			return len;

		// Phony link
		data[len++] = 1;
		data[len++] = 1;

		// Line number is number of blocks.
		data[len++] = fsize & 255;
		data[len++] = fsize >> 8;

		if (fsize < 100)
			data[len++] = ' ';
		if (fsize < 10)
			data[len++] = ' ';

		// Name
		data[len++] = '"';
		i = 0;
		while (*s)
			data[i++,len++] = *s++;
		data[len++] = '"';
		while (++i < 17)
			data[len++] = ' ';

		// Splat file?
		data[len++] = (diskdata[curr_dirent + 2] & 0x80) ? ' ' : '*';

		// Type
		data[len++] = ftypesnames[ftype][0];
		data[len++] = ftypesnames[ftype][1];
		data[len++] = ftypesnames[ftype][2];

		// Locked file?
		data[len++] = (diskdata[curr_dirent + 2] & 0x40) ? '<' : ' ';

		// EOL
		data[len++] = 0;
	}

	// Blocks free line
	if (maxlen - len < 20)
		return len;
	data[len++] = 1;
	data[len++] = 1;
	data[len++] = blocks_free & 255;
	data[len++] = blocks_free >> 8;
	strcpy((char *)data + len, "BLOCKS FREE.");
	len += 12;
	data[len++] = 0;

	// End of program
	data[len++] = 0;
	data[len++] = 0;
	len += 2;

	return len;
}

int
PetDisk::readFile(const char *name, uint8_t *data, int maxlen)
{
	DPRINTF(1, "PetDisk::%s: name=%s\n", __func__, name);

	if (diskdata_len == 0)
		return 0;

	const char *basename = name;

	// Truncate disk number.
	if (name[0] && name[1] == ':') {
		if (name[0] != '0')
			return 0;
		basename = name + 2;
	}

	// Read directory?
	if (strcmp(basename, "$") == 0)
		return readDirectory(data, maxlen);

	// Search directory for basename.
	char *s;
	enum fileType ftype;
	int fsize;
	firstDirEnt();
	while ((s = nextDirEnt(ftype, fsize)) != NULL) {
		if (strcmp(s, basename) == 0)
			break;
	}

	if (s == NULL) {
		DPRINTF(1, "PetDisk::%s: File not found: %s\n", __func__,
			basename);
		return 0;
	}

	DPRINTF(1, "PetDisk::%s: Found file %s\n", __func__, basename);

	int len = 0;
	int offset = trackOff(diskdata[curr_dirent + 3]) +
		diskdata[curr_dirent + 4] * SECTSIZE;
	for (int i = 0; i < fsize; i++) {
		for (int j = 2; j < SECTSIZE; j++) {
			if (len >= maxlen) {
				DPRINTF(1, "PetDisk::%s: overflow file buf.\n",
					__func__);
				return len;
			}
			if (offset + j >= diskdata_len) {
				DPRINTF(1, "PetDisk::%s: out of disk data.\n",
					__func__);
				return len;
			}

			data[len++] = diskdata[offset + j];
		}

		offset = trackOff(diskdata[offset]) +
			diskdata[offset + 1] * SECTSIZE;
	}

	DPRINTF(1, "PetDisk::%s: data: len=%d %02x %02x %02x %02x\n", __func__,
		len, data[0], data[1], data[2], data[3]);

	return len;
}

int
PetDisk::writeFile(const char *name, const uint8_t *data, int len)
{
	DPRINTF(1, "PetDisk::%s: name=%s len=%d\n", __func__, name, len);

	return 0;
}
