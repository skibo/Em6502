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

#ifndef __PETDISK_H__
#define __PETDISK_H__

#define MAX_DISK_LEN		174848

enum fileType {
	       FTYPE_DEL = 0,
	       FTYPE_SEQ = 1,
	       FTYPE_PRG = 2,
	       FTYPE_USR = 3,
	       FTYPE_REL = 4
};

class PetDisk {
private:
	uint8_t		diskdata[MAX_DISK_LEN];
	int		diskdata_len;
	int		blocks_free;
	int		curr_dirent;
	int		next_dirsec;
	int		trackOff(int track);
	int		nsects(int track);
	void		firstDirEnt(void);
	char		*nextDirEnt(enum fileType &ftype, int &size);
	int		readDirectory(uint8_t *data, int maxlen);
public:
	PetDisk()
	{ blankDisk(); }
	void		blankDisk(void);
	void		loadDisk(uint8_t data[], int len);
	int		readFile(const char *name, uint8_t *data,
				 int maxlen);
	int		writeFile(const char *name, const uint8_t *data,
				  int len);
};

#endif // __PETDISK_H__
