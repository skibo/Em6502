//
// Copyright (c) 2025 Thomas Skibo.
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

// Atari2600GtkAppWin.h

#ifndef __ATARI2600GTKAPPWIN_H__
#define __ATARI2600GTKAPPWIN_H__

#include "Atari2600GtkDisp.h"
class Atari2600;

class Atari2600GtkAppWin : public Gtk::ApplicationWindow
{
private:
	Atari2600 *atari;
	Atari2600GtkDisp *disp;
protected:
	const Glib::RefPtr<Gtk::Builder> builder;
public:
	Atari2600GtkAppWin(BaseObjectType *cobject,
			 const Glib::RefPtr<Gtk::Builder> &refBuilder);
	~Atari2600GtkAppWin();
	Glib::RefPtr<Gtk::Builder> getBuilder() { return this->builder; }

	static Atari2600GtkAppWin *create(Atari2600 *atari2600);

	Atari2600GtkDisp *getDisp(void)
	{ return disp; }
};

#endif // __ATARI2600GTKAPPWIN_H__
