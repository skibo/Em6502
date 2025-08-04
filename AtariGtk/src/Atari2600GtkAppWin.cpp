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

// Atari2600GtkAppWin.cpp

#include <gtkmm.h>

#include "Atari2600GtkAppWin.h"

#include "Atari2600GtkDisp.h"
#include "Atari2600GtkInput.h"
#include "Atari2600.h"

#ifdef DEBUGAPP
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Atari2600GtkAppWin::Atari2600GtkAppWin(BaseObjectType *cobject,
   const Glib::RefPtr<Gtk::Builder> &refBuilder)
	: Gtk::ApplicationWindow(cobject),
	builder(refBuilder)
{
	DPRINTF(1, "Atari2600GtkAppWin::%s:\n", __func__);

	this->atari = nullptr;
}

// Class function
Atari2600GtkAppWin *
Atari2600GtkAppWin::create(Atari2600 *atari)
{
	DPRINTF(1, "Atari2600GtkAppWin::%s:\n", __func__);

	auto builder = Gtk::Builder::create_from_resource(
			 "/net/skibo/atarigtk/atari2600.glade");
	Atari2600GtkAppWin *window = nullptr;
	builder->get_widget_derived("win_top", window);
	if (!window)
		throw std::runtime_error(
				 "No win_top object in atari2600gtk.glade");

	window->disp = nullptr;
	builder->get_widget_derived("atari_display", window->disp);
	window->disp->connectSignals(builder);
	atari->setVideo(window->disp);

	Atari2600GtkInput *inp = nullptr;
	builder->get_widget_derived("atari_input", inp, atari);
	inp->connectSignals(builder);

	return window;
}

Atari2600GtkAppWin::~Atari2600GtkAppWin()
{
	DPRINTF(1, "Atari2600GtkAppWin::%s:\n", __func__);
}
