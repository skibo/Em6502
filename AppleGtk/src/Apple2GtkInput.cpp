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

#include <gtkmm.h>
#include <stdint.h>

#include "Apple2GtkInput.h"

#include "Apple2.h"

#ifdef DEBUGIN
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGIN) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Apple2GtkInput::Apple2GtkInput(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder,
			       Apple2 *_apple)
	: Gtk::Image(cobject),
	apple(_apple)
{
	DPRINTF(1, "Apple2GtkInput::constructor:\n");
}

void
Apple2GtkInput::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Apple2GtkInput::%s:\n", __func__);

	signal_key_press_event().connect(sigc::mem_fun(*this,
					&Apple2GtkInput::onKeyPressEvent));

	signal_key_release_event().connect(sigc::mem_fun(*this,
					&Apple2GtkInput::onKeyReleaseEvent));


	auto p = builder->get_object("paddle0");
	paddle[0] = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(p);
	paddle[0]->signal_value_changed().connect(sigc::bind(
					sigc::mem_fun(*this,
					&Apple2GtkInput::onPaddleChange),
					paddle[0], 0));

	p = builder->get_object("paddle1");
	paddle[1] = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(p);
	paddle[1]->signal_value_changed().connect(sigc::bind(
					sigc::mem_fun(*this,
					&Apple2GtkInput::onPaddleChange),
					paddle[1], 1));
}

bool
Apple2GtkInput::onKeyPressEvent(GdkEventKey *event)
{
	int keyval = event->keyval;
	unsigned int state = event->state;

	DPRINTF(1, "Apple2GtkInput::%s: keyval=0x%x state=0x%x\n", __func__,
		keyval, state);

	uint8_t applekey = 0;

	if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) {
		// lower case letters (convert to upper case)
		applekey = 0x80 | (keyval - 0x20);
		if (state & GDK_CONTROL_MASK)
			applekey -= 0x40;
	} else if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z) {
		// upper case letters
		applekey = 0x80 | keyval;
		if (state & GDK_CONTROL_MASK)
			applekey -= 0x40;
	} else if (keyval >= GDK_KEY_space && keyval <= GDK_KEY_asciitilde)
		// all other standard ASCII characters
		applekey = 0x80 | keyval;
	else if (keyval >= GDK_KEY_KP_4 && keyval <= GDK_KEY_KP_6) {
		// Keypad 4-6 are game buttons
		apple->setButton(keyval - GDK_KEY_KP_4, true);
	} else
		// map special keys and ignore all others
		switch (keyval) {
		case GDK_KEY_Return:
		case GDK_KEY_KP_Enter:
			applekey = 0x8d;
			break;
		case GDK_KEY_Escape:
			applekey = 0x9b;
			break;
		case GDK_KEY_Delete:
		case GDK_KEY_BackSpace:
		case GDK_KEY_Left:
			applekey = 0x88;
			break;
		case GDK_KEY_Right:
			applekey = 0x95;
			break;
		}

	if (applekey)
		apple->setkey(applekey);

	return true;
}

bool
Apple2GtkInput::onKeyReleaseEvent(GdkEventKey *event)
{
	int keyval = event->keyval;

	DPRINTF(1, "Apple2GtkInput::%s: keyval=0x%x\n", __func__, keyval);

	// Keypad 4-6 are game buttons
	if (keyval >= GDK_KEY_KP_4 && keyval <= GDK_KEY_KP_6)
		apple->setButton(keyval - GDK_KEY_KP_4, false);

	return true;
}

void
Apple2GtkInput::onPaddleChange(Glib::RefPtr<Gtk::Adjustment> &adj, int paddle)
{
	float val = (float)adj->get_value();

	DPRINTF(1, "Apple2GtkInput::%s: paddle=%d adjust=%f\n", __func__,
		paddle, val);

	apple->setPaddle(paddle, val);
}
