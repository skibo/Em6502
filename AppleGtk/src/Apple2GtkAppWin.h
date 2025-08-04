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

// Apple2GtkAppWin.h

#ifndef __APPLE2GTKAPPWIN_H__
#define __APPLE2GTKAPPWIN_H__

#include "Apple2GtkDisp.h"
class Apple2;

class Apple2GtkAppWin : public Gtk::ApplicationWindow
{
private:
	Apple2 *apple;
	Apple2GtkDisp *disp;
protected:
	const Glib::RefPtr<Gtk::Builder> builder;
public:
	Apple2GtkAppWin(BaseObjectType *cobject,
			 const Glib::RefPtr<Gtk::Builder> &refBuilder);
	~Apple2GtkAppWin();
	Glib::RefPtr<Gtk::Builder> getBuilder() { return this->builder; }

	static Apple2GtkAppWin *create(Apple2 *apple);

	Apple2GtkDisp *getDisp(void)
	{ return disp; }
	void dispFlashing(bool flag)
	{ disp->setFlashing(flag); }
};

#endif // __APPLE2GTKAPPWIN_H__
