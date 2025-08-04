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

// Pet2001GtkIeee.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Pet2001GtkIeee.h"

#include <iostream>
#include <fstream>

#include "Pet2001GtkApp.h"
#include "PetIeeeHw.h"

#ifdef DEBUGIEEE
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGIEEE) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Pet2001GtkIeee::Pet2001GtkIeee(Pet2001GtkApp *app)
{
	DPRINTF(1, "Pet2001GtkIeee::%s: app=%p\n", __func__, app);

	this->app = app;

	ieee.setIeeeDev(this);

	this->ieeeReset();
}

void
Pet2001GtkIeee::connectSignals(Glib::RefPtr<Gtk::Builder> builder)
{
	DPRINTF(1, "Pet2001GtkIeee::%s:\n", __func__);

	check_menu_save_to_disk = nullptr;
	builder->get_widget("menu_save_to_disk", check_menu_save_to_disk);
}

void
Pet2001GtkIeee::ieeeReset(void)
{
	DPRINTF(1, "Pet2001GtkIeee::%s:\n", __func__);

	flen = 0;
	findex = 0;
}

void
Pet2001GtkIeee::ieeeOpen(int f, const char *filename)
{
	DPRINTF(1, "Pet2001GtkIeee::%s: f=%d filename=%s\n", __func__, f,
		filename);

	fnum = f;
	strcpy(this->fname, filename);
	findex = 0;
	flen = 0;

	switch (f) {
	case 0:
		/* LOAD */
		flen = disk.readFile(filename, fdata, sizeof(fdata));
		break;
	case 1:
		/* SAVE */
		break;
	}
}

void
Pet2001GtkIeee::ieeeListenData(int f, uint8_t d8, bool eoi)
{
	DPRINTF(3, "Pet2001GtkIeee::%s: f=%d d8=0x%02x eoi=%d\n", __func__, f,
		d8, eoi);

	if (flen < MAXFILESIZE)
		fdata[flen++] = d8;
}

uint8_t
Pet2001GtkIeee::ieeeTalkData(int f, bool *seteoi)
{
	uint8_t d8;

	if (findex < flen) {
		d8 = fdata[findex];

		*seteoi = (findex + 1 >=flen);
	} else {
		d8 = 0xaa;
		*seteoi = true;
	}

	DPRINTF(3, "Pet2001GtkIeee::%s: f=%d d8=0x%02x seteoi=%d\n",
		__func__, f, d8, *seteoi);

	return d8;
}

void
Pet2001GtkIeee::ieeeTalkDataAck(int f)
{
	DPRINTF(3, "Pet2001GtkIeee::%s: f=%d\n", __func__, f);

	if (findex < flen)
		findex++;
}


void
Pet2001GtkIeee::ieeeClose(int f)
{
	DPRINTF(1, "Pet2001GtkIeee::%s: f=%d\n", __func__, f);

	if (f == 1 && check_menu_save_to_disk->get_active()) {
		(void)disk.writeFile(fname, fdata, flen);
	} else if (f == 1) {
		char hint[256];
		memset(hint, 0, sizeof(hint));
		strncpy(hint, fname, sizeof(hint) - 4);
		strcat(hint, ".prg");

		std::string loc_filename = app->doFileChooser(true, false,
							      hint);

		// Empty string means Cancel.
		if (loc_filename != "") {
			DPRINTF(1, "Pet2001GtkIeee::%s: loc_filename=%s\n",
				__func__, loc_filename.c_str());

			std::ofstream file(loc_filename, std::ios::out |
					   std::ios::binary | std::ios::trunc);
			if (file.is_open()) {
				file.write((char *)fdata, flen);
				file.close();
			} // XXX: else do error dialog
		}
	}
}
