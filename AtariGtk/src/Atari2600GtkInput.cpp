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

#include <gtkmm.h>
#include <stdint.h>

#include "Atari2600GtkInput.h"

#include "Atari2600.h"

#ifdef DEBUGIN
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGIN) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Atari2600GtkInput::Atari2600GtkInput(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder,
			       Atari2600 *_atari)
	: Gtk::Image(cobject),
	atari(_atari)
{
	DPRINTF(1, "Atari2600GtkInput::constructor:\n");
}

void
Atari2600GtkInput::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Atari2600GtkInput::%s:\n", __func__);

	signal_key_press_event().connect(sigc::mem_fun(*this,
				&Atari2600GtkInput::onKeyPressEvent));

	signal_key_release_event().connect(sigc::mem_fun(*this,
				&Atari2600GtkInput::onKeyReleaseEvent));

	Glib::RefPtr<Glib::Object> obj = builder->get_object("paddle0");
	Glib::RefPtr<Gtk::Adjustment> adjp =
		Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(obj);
	adjp->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkInput::onPaddleChange),
							PADDLE_LEFT_A, adjp));

	obj = builder->get_object("paddle1");
	adjp = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(obj);
	adjp->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkInput::onPaddleChange),
							PADDLE_LEFT_B, adjp));

	obj = builder->get_object("paddle2");
	adjp = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(obj);
	adjp->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkInput::onPaddleChange),
							PADDLE_RIGHT_A, adjp));

	obj = builder->get_object("paddle3");
	adjp = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(obj);
	adjp->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkInput::onPaddleChange),
							PADDLE_RIGHT_B, adjp));
}

bool
Atari2600GtkInput::onKeyPressEvent(GdkEventKey *event)
{
	int keyval = event->keyval;

	DPRINTF(1, "Atari2600GtkInput::%s: keyval=0x%x\n", __func__, keyval);

	switch (keyval) {
	case GDK_KEY_Left:
		atari->setJoyLeft(JOY_LEFT, 0);
		break;
	case GDK_KEY_Right:
		atari->setJoyLeft(JOY_RIGHT, 0);
		break;
	case GDK_KEY_Up:
		atari->setJoyLeft(JOY_UP, 0);
		break;
	case GDK_KEY_Down:
		atari->setJoyLeft(JOY_DOWN, 0);
		break;
	case GDK_KEY_space:
		atari->setJoyLeft(JOY_TRIGGER, 0);
		break;
	case GDK_KEY_g:
	case GDK_KEY_G:
		atari->setJoyRight(JOY_LEFT, 0);
		break;
	case GDK_KEY_j:
	case GDK_KEY_J:
		atari->setJoyRight(JOY_RIGHT, 0);
		break;
	case GDK_KEY_y:
	case GDK_KEY_Y:
		atari->setJoyRight(JOY_UP, 0);
		break;
	case GDK_KEY_h:
	case GDK_KEY_H:
		atari->setJoyRight(JOY_DOWN, 0);
		break;
	case GDK_KEY_f:
	case GDK_KEY_F:
		atari->setJoyRight(JOY_TRIGGER, 0);
		break;
	default:
		return false;
	}

	return true;
}

bool
Atari2600GtkInput::onKeyReleaseEvent(GdkEventKey *event)
{
	int keyval = event->keyval;

	DPRINTF(1, "Atari2600GtkInput::%s: keyval=0x%x\n", __func__, keyval);

	switch (keyval) {
	case GDK_KEY_Left:
		atari->setJoyLeft(0, JOY_LEFT);
		break;
	case GDK_KEY_Right:
		atari->setJoyLeft(0, JOY_RIGHT);
		break;
	case GDK_KEY_Up:
		atari->setJoyLeft(0, JOY_UP);
		break;
	case GDK_KEY_Down:
		atari->setJoyLeft(0, JOY_DOWN);
		break;
	case GDK_KEY_space:
		atari->setJoyLeft(0, JOY_TRIGGER);
		break;
	case GDK_KEY_g:
	case GDK_KEY_G:
		atari->setJoyRight(0, JOY_LEFT);
		break;
	case GDK_KEY_j:
	case GDK_KEY_J:
		atari->setJoyRight(0, JOY_RIGHT);
		break;
	case GDK_KEY_y:
	case GDK_KEY_Y:
		atari->setJoyRight(0, JOY_UP);
		break;
	case GDK_KEY_h:
	case GDK_KEY_H:
		atari->setJoyRight(0, JOY_DOWN);
		break;
	case GDK_KEY_f:
	case GDK_KEY_F:
		atari->setJoyRight(0, JOY_TRIGGER);
		break;
	default:
		return false;
	}

	return true;
}

void
Atari2600GtkInput::onPaddleChange(int p, Glib::RefPtr<Gtk::Adjustment> adj)
{
	double val = adj->get_value();

	DPRINTF(2, "Atari2600GtkInput::%s:p=%d adj=%f\n", __func__, p, val);

	atari->setPaddle(p, (int)val);
}
