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

// Pet2001GtkAppWin.cpp

#include <gtkmm.h>

#include "Pet2001GtkAppWin.h"

#include "Pet2001GtkDisp.h"
#include "Pet2001GtkKeys.h"
#include "Pet2001.h"

#ifdef DEBUGAPP
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Pet2001GtkAppWin::Pet2001GtkAppWin(BaseObjectType *cobject,
   const Glib::RefPtr<Gtk::Builder> &refBuilder)
	: Gtk::ApplicationWindow(cobject),
	builder(refBuilder)
{
	DPRINTF(1, "Pet2001GtkAppWin::%s:\n", __func__);

	this->pet = nullptr;
}

// Class function
Pet2001GtkAppWin *
Pet2001GtkAppWin::create(Pet2001 *pet)
{
	DPRINTF(1, "Pet2001GtkAppWin::%s:\n", __func__);

	auto builder = Gtk::Builder::create_from_resource(
					 "/net/skibo/petgtk/petgtk.glade");
	Pet2001GtkAppWin *window = nullptr;
	builder->get_widget_derived("win_top", window);
	if (!window)
		throw std::runtime_error("No win_top object in petgtk.glade");

	// Add minor CSS styling to top_box
	auto css = Gtk::CssProvider::create();
	css->load_from_data("box {"
			    "   background-color: #e0f0ff;"
			    "}");
	Gtk::Box *top_box = nullptr;
	builder->get_widget("top_box", top_box);
	top_box->get_style_context()->add_provider(css,
				  GTK_STYLE_PROVIDER_PRIORITY_USER);

	Pet2001GtkDisp *disp = nullptr;
	builder->get_widget_derived("pet_display", disp);
	disp->connectSignals();
	pet->setVideo(disp);

	Pet2001GtkKeys *keys = nullptr;
	builder->get_widget_derived("pet_keys", keys, pet);
	keys->connectSignals();

	return window;
}

Pet2001GtkAppWin::~Pet2001GtkAppWin()
{
	DPRINTF(1, "Pet2001GtkAppWin::%s:\n", __func__);

	if (pet)
		pet->setVideo(nullptr);
}
