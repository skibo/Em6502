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

// Pet2001GtkIeee.h

#ifndef __PET2001GTKIEEE_H__
#define __PET2001GTKIEEE_H__

#include "IeeeDev.h"
#include "PetIeeeHw.h"
#include "PetDisk.h"

class Pet2001GtkApp;

#define MAXFILESIZE	32768

class Pet2001GtkIeee : public IeeeDev {
private:
	Pet2001GtkApp	*app;
	PetIeeeHw	ieee;
	PetDisk		disk;
	int		fnum;
	char		fname[64];
	uint8_t		fdata[MAXFILESIZE];
	int 		flen;
	int		findex;

	Gtk::CheckMenuItem *check_menu_save_to_disk;

	// IeeeDev interface
	void		ieeeReset(void);
	void		ieeeOpen(int f, const char *filename);
	void 		ieeeListenData(int f, uint8_t d8, bool eoi);
	uint8_t		ieeeTalkData(int f, bool *seteoi);
	void		ieeeTalkDataAck(int f);
	void		ieeeClose(int f);
public:
	Pet2001GtkIeee(Pet2001GtkApp *app);
	PetIeeeHw	*getIeeeHw(void) { return &ieee; }
	void		reset(void) { ieee.reset(); }
	void		connectSignals(Glib::RefPtr<Gtk::Builder> builder);
	void		loadDisk(uint8_t data[], int len)
		{ disk.loadDisk(data, len); }
};

#endif // __PET2001GTKIEEE_H__
