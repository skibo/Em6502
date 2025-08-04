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

// Pet2001GtkCass.h

#ifndef __PET2001GTKCASS_H__
#define __PET2001GTKCASS_H__

#include "PetCassHw.h"
class Pet2001GtkApp;

#define MAX_PROG_LEN	32768

class Pet2001GtkCass {
private:
	Pet2001GtkApp	*app;
	PetCassHw	cass;
	uint8_t		proghdr[192];
	uint8_t		progdata[MAX_PROG_LEN];
	bool		progloading;
	bool		progsaving;
	Gtk::ToggleButton *play_button;
	Gtk::ToggleButton *record_button;
	void		onRecordButton(Gtk::ToggleButton *button);
	void		onPlayButton(Gtk::ToggleButton *button);
public:
	Pet2001GtkCass(Pet2001GtkApp *);
	void		cycle(void)	{ cass.cycle(); }
	void		reset(void);
	PetCassHw	*getCassHw(void) { return &cass; }
	void		connectSignals(Glib::RefPtr<Gtk::Builder> builder);
	void		cassDoneCallback(int len);
};

#endif // __PET2001GTKCASS_H__
