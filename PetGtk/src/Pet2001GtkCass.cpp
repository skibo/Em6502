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

// Pet2001GtkCass.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Pet2001GtkCass.h"

#include <iostream>
#include <fstream>

#include "PetCassHw.h"
#include "Pet2001GtkApp.h"

#ifdef DEBUGCASS
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGCASS) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

#define MAX_PROG_LEN	32768

Pet2001GtkCass::Pet2001GtkCass(Pet2001GtkApp *app)
{
	DPRINTF(1, "Pet2001GtkCass::%s: app=%p\n", __func__, app);

	this->app = app;
	progloading = false;
	progsaving = false;
	record_button = nullptr;
	play_button = nullptr;
	cass.setCassDoneCallback([&] (int len) {this->cassDoneCallback(len);});
}

void
Pet2001GtkCass::reset(void)
{
	DPRINTF(1, "Pet2001GtkCass::%s:\n", __func__);

	progloading = false;
	progsaving = false;
	play_button->set_active(false);
	record_button->set_active(false);
	cass.reset();
}

void
Pet2001GtkCass::connectSignals(Glib::RefPtr<Gtk::Builder> builder)
{
	builder->get_widget("record_button", record_button);
	record_button->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkCass::onRecordButton),
				record_button));

	builder->get_widget("play_button", play_button);
	play_button->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkCass::onPlayButton), play_button));
}

void
Pet2001GtkCass::cassDoneCallback(int len)
{
	DPRINTF(1, "Pet2001GtkCass::%s: len=%d\n", __func__, len);

	if (progsaving) {
		progsaving = false;
		record_button->set_active(false);

		std::string filename = app->doFileChooser(true, false);

		// Empty string means Cancel.
		if (filename != "") {

			DPRINTF(1, "Pet2001GtkCass::%s: filename=%s\n",
				__func__, filename.c_str());

			std::ofstream file(filename, std::ios::out |
					   std::ios::binary | std::ios::trunc);
			if (file.is_open()) {
				file.write((char *)progdata, len);
				file.close();
			} // XXX: else do error dialog
		}
	}

	if (progloading) {
		progloading = false;
		play_button->set_active(false);
	}
}

void
Pet2001GtkCass::onRecordButton(Gtk::ToggleButton *button)
{
	bool active = button->get_active();

	DPRINTF(1, "Pet2001GtkCass::%s: active=%d\n", __func__, active);

	// Make Play and Record buttons mutually exclusive.
	if (progloading) {
		if (active)
			button->set_active(false);
		return;
	}

	if (!active) {
		// Cancel save if saving.
		if (progsaving) {
			cass.reset();
			progsaving = false;
		}
		return;
	}

	cass.cassSave(proghdr, progdata, MAX_PROG_LEN);
	progsaving = true;
}

void
Pet2001GtkCass::onPlayButton(Gtk::ToggleButton *button)
{
	bool active = button->get_active();

	DPRINTF(1, "Pet2001GtkCass::%s: active=%d\n", __func__, active);

	// Make Play and Record buttons mutually exclusive.
	if (progsaving) {
		if (active)
			button->set_active(false);
		return;
	}

	if (!active) {
		// Cancel load if loading.
		if (progloading) {
			cass.reset();
			progloading = false;
		}
		return;
	}

	progloading = true;

	std::string filename = app->doFileChooser(false, false);

	// Empty string means Cancel.
	if (filename == "") {
		progloading = false;
		button->set_active(false);
		return;
	}

	DPRINTF(1, "Pet2001GtkModel::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();
		if (len > MAX_PROG_LEN)
			len = MAX_PROG_LEN;

		file.seekg(0, std::ios::beg);
		file.read((char *)progdata, len);
		file.close();

		uint16_t start_addr = progdata[0] + 256 * progdata[1];
		uint16_t end_addr = start_addr + len - 2;

		DPRINTF(1, "Pet2001GtkCass::%s: start_addr=0x%x len=%d\n",
			__func__, start_addr, len);

		memset(proghdr, ' ', sizeof(proghdr));
		proghdr[0] = 0x01;
		proghdr[1] = (start_addr & 0xff);
		proghdr[2] = (start_addr >> 8);
		proghdr[3] = (end_addr & 0xff);
		proghdr[4] = (end_addr >> 8);
		proghdr[5] = 'P';
		proghdr[6] = 'R';
		proghdr[7] = 'O';
		proghdr[8] = 'G';
		proghdr[9] = 'R';
		proghdr[10] = 'A';
		proghdr[11] = 'M';

		cass.cassLoad(proghdr, &progdata[2], len - 2);
		progloading = true;
	}
	else {
		// Couldn't open file.  XXX: need an alert!
		progloading = false;
		button->set_active(false);
		return;
	}
}
