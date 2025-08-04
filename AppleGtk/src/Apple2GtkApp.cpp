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

#include <stdint.h>
#include <gtkmm.h>

#include "Apple2GtkApp.h"

#include <cstdio>

#include <iostream>
#include <fstream>

#include "Apple2.h"

#include "Apple2GtkAppWin.h"

#ifdef DEBUGAPP
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern uint8_t applerom[];

Apple2GtkApp::Apple2GtkApp()
	: Gtk::Application("net.skibo.apple2"),
	  apple(nullptr),
	  debugger(apple.getCpu())
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	appwindow = nullptr;
	diskdata = nullptr;
	debuggerActive = false;
}

Apple2GtkApp::~Apple2GtkApp()
{
	delete [] diskdata;
}

Glib::RefPtr<Apple2GtkApp>
Apple2GtkApp::create()
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	return Glib::RefPtr<Apple2GtkApp>(new Apple2GtkApp());
}

std::string
Apple2GtkApp::doFileChooser(enum e_fileType type, bool dosave)
{
	DPRINTF(1, "Apple2GtkApp::%s: type=%d dosave=%d\n", __func__,
		(int)type, dosave);

	Gtk::FileChooserDialog chooser(*appwindow, "File", dosave ?
				       Gtk::FILE_CHOOSER_ACTION_SAVE :
				       Gtk::FILE_CHOOSER_ACTION_OPEN);

	chooser.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	chooser.add_button(dosave ? "_Save" : "_Open", Gtk::RESPONSE_OK);

	Glib::RefPtr<Gtk::FileFilter> filter;

	switch (type) {
	case fileTypeDsk:
		filter = Gtk::FileFilter::create();
		filter->set_name("Dsk and Nib files");
		filter->add_pattern("*.dsk");
		filter->add_pattern("*.nib");
		chooser.add_filter(filter);
		break;
	case fileTypeBinary:
		filter = Gtk::FileFilter::create();
		filter->set_name("Binary files");
		filter->add_pattern("*.bin");
		chooser.add_filter(filter);
		break;
	}

	filter = Gtk::FileFilter::create();
	filter->set_name("Any");
	filter->add_pattern("*");
	chooser.add_filter(filter);

	std::string filename = "";

	// I choo choo choose you!
	int result = chooser.run();

	if (result == Gtk::RESPONSE_OK)
		filename = chooser.get_filename();

	return filename;
}

Apple2GtkAppWin *
Apple2GtkApp::create_appwindow()
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	auto appwindow = Apple2GtkAppWin::create(&apple);

	add_window(*appwindow);

	appwindow->signal_hide().connect(sigc::bind<Gtk::Window*>(mem_fun(
			*this, &Apple2GtkApp::on_hide_window), appwindow));

	return appwindow;
}

void
Apple2GtkApp::on_startup()
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	Gtk::Application::on_startup();
}

void
Apple2GtkApp::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	builder->get_widget("pause_button", pauseButton);
	pauseButton->signal_toggled().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onPauseToggle));

	Gtk::ToggleButton *toggle = nullptr;
	builder->get_widget("turbo_button", toggle);
	toggle->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Apple2GtkApp::onTurboToggle), toggle));

	Gtk::Button *button = nullptr;
	builder->get_widget("reset_button", button);
	button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this,
				&Apple2GtkApp::onResetButton), button));

	Gtk::MenuItem *menu = nullptr;
	builder->get_widget("menu_reset", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuReset));

	builder->get_widget("menu_restart", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuRestart));

	Gtk::CheckMenuItem *checkmenu = nullptr;
	builder->get_widget("menu_debug", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuDebugToggle), checkmenu));

	builder->get_widget("menu_color", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuColorToggle), checkmenu));

	builder->get_widget("menu_load_disk", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuLoadDisk));

	builder->get_widget("menu_save_disk", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuSaveDisk));

	builder->get_widget("menu_unload_disk", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuUnloadDisk));

	builder->get_widget("menu_bload", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuBload));

	builder->get_widget("menu_load_rom_f8", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuLoadRom));

	builder->get_widget("menu_quit", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuQuit));

	builder->get_widget("entry_disk1", entryDisk1);
	builder->get_widget("spinner_disk1", spinnerDisk1);

	Gtk::AboutDialog *about = nullptr;
	builder->get_widget("win_about", about);
	builder->get_widget("menu_about", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Apple2GtkApp::onMenuAbout), about));

	disp = appwindow->getDisp();
}

void
Apple2GtkApp::on_activate()
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	try {
		appwindow = create_appwindow();
		appwindow->present();
	}
	catch (const Glib::Error &ex) {
		fprintf(stderr, "Apple2GtkApp::%s: %s\n", __func__,
			ex.what().c_str());
	}
	catch (const std::exception &ex) {
		fprintf(stderr, "Apple2GtkApp::%s: %s\n", __func__,
			ex.what());
	}

	// Initialize Apple hardware
	running = false;
	turbo = false;
	apple.reset();
	appleRun(true);
	appwindow->dispFlashing(true);

	connectSignals(appwindow->getBuilder());
	debugger.connectSignals(appwindow->getBuilder());

	apple.getDisk()->setDiskCallback([&] (bool motor, int track)
		{ this->diskCallback(motor, track); });
	debugger.setDebugCallback([&] (int typ) {this->debugCallback(typ);});
}

void
Apple2GtkApp::on_hide_window(Gtk::Window *win)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	delete win;
	appwindow = nullptr;
}

void
Apple2GtkApp::debugCallback(int typ)
{
	DPRINTF(1, "Pet2001GtkApp::%s: typ=%d\n", __func__, typ);

	switch (typ) {
	case CALLBACK_STOP:
		appleRun(false);
		appwindow->dispFlashing(false);
		pauseButton->set_active(true);
		debugger.setState(debuggerActive, running);
		break;

	case CALLBACK_STEP:
		apple.getCpu()->stepCpu();
		if (!running) {
			while (apple.cycle())
				;
		}
		break;

	case CALLBACK_CONT:
		appleRun(true);
		appwindow->dispFlashing(true);
		pauseButton->set_active(false);
		debugger.setState(debuggerActive, running);
		break;
	}
}

void
Apple2GtkApp::onPauseToggle(void)
{
	bool state = pauseButton->get_active();

	DPRINTF(1, "Apple2GtkApp::%s: state=%d\n", __func__, state);

	appleRun(!state);
	appwindow->dispFlashing(!state);
}

void
Apple2GtkApp::onTurboToggle(Gtk::ToggleButton *button)
{
	bool state = button->get_active();

	DPRINTF(1, "Apple2GtkApp::%s: state=%d\n", __func__, state);

	appleTurbo(state);
}

void
Apple2GtkApp::onResetButton(Gtk::Button *button)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	apple.reset();
}

void
Apple2GtkApp::onMenuReset(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	apple.reset();
}

void
Apple2GtkApp::onMenuRestart(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	apple.restart();
}

void
Apple2GtkApp::onMenuColorToggle(Gtk::CheckMenuItem *checkmenu)
{
	bool state = checkmenu->get_active();

	DPRINTF(1, "Apple2GtkApp::%s: state=%d\n", __func__, state);

	disp->setColor(state);
}

void
Apple2GtkApp::onMenuDebugToggle(Gtk::CheckMenuItem *checkmenu)
{
	bool state = checkmenu->get_active();

	DPRINTF(1, "Apple2GtkApp::%s: state=%d\n", __func__, state);

	debuggerActive = state;

	debugger.setState(debuggerActive, running);
}

void
Apple2GtkApp::onMenuAbout(Gtk::AboutDialog *about)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	about->run();
	about->set_visible(false);
}

void
Apple2GtkApp::convertDskToNib(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

#define DISK2_TRACK_SIZE	6656
#define DISK2_N_TRACKS		35
#define DISK2_N_SECS		16

	const uint8_t sixTwo[] = {
		0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
		0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
		0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
		0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
		0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
		0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
		0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
		0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
	};
	const uint8_t secSkew[] = {
		0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15
	};
	int i;
	uint8_t *nibbytes = new uint8_t[DISK2_N_TRACKS * DISK2_TRACK_SIZE];
	int offs;

	for (int track = 0; track < DISK2_N_TRACKS; track++) {
		offs = track * DISK2_TRACK_SIZE;
		for (int sec = 0; sec < DISK2_N_SECS; sec++) {

			// Sync bytes
			for (i = 0; i < 20; i++)
				nibbytes[offs++] = 0xff;

			// Addr field prologue
			nibbytes[offs++] = 0xd5;
			nibbytes[offs++] = 0xaa;
			nibbytes[offs++] = 0x96;

			// Volume, track, sec, checksum
			nibbytes[offs++] = 0xaa | (254 >> 1);
			nibbytes[offs++] = 0xaa | 254;
			nibbytes[offs++] = 0xaa | (track >> 1);
			nibbytes[offs++] = 0xaa | track;
			nibbytes[offs++] = 0xaa | (sec >> 1);
			nibbytes[offs++] = 0xaa | sec;
			nibbytes[offs++] = 0xaa | ((254 ^ track ^ sec) >> 1);
			nibbytes[offs++] = 0xaa | (254 ^ track ^ sec);

			// Addr field epilogue.
			nibbytes[offs++] = 0xde;
			nibbytes[offs++] = 0xaa;
			nibbytes[offs++] = 0xeb;

			// Sync bytes
			for (int i = 0; i < 20; i++)
				nibbytes[offs++] = 0xff;

			// Data field prologue
			nibbytes[offs++] = 0xd5;
			nibbytes[offs++] = 0xaa;
			nibbytes[offs++] = 0xad;

			// Start by prenibbilizing
			uint8_t prenib[342];
			int doffs = secSkew[sec] * 256 + track * 4096;
			for (i = 0; i < 256; i++) {
				uint8_t d8 = diskdata[doffs + i];

				prenib[i] = (d8 >> 2);

				if (i < 86)
					prenib[256 + 85 - i] =
						((d8 & 0x02) >> 1) |
						((d8 & 0x01) << 1);
				else if (i < 172)
					prenib[256 + 171 - i] |=
						(((d8 & 0x02) << 1) |
						 ((d8 & 0x01) << 3));
				else
					prenib[256 + 257 - i] |=
						(((d8 & 0x02) << 3) |
						 ((d8 & 0x01) << 5));

				if (i < 2)
					prenib[257 - i] |=
						(((d8 & 0x02) << 3) |
						 ((d8 & 0x01) << 5));
			}

			// Encode nibbilized data.
			uint8_t prev = 0;
			for (i = 0; i < 86; i++) {
				nibbytes[offs++] =
					sixTwo[prev ^ prenib[256 + 85 - i]];
				prev = prenib[256 + 85 - i];
			}
			for (i = 0; i < 256; i++) {
				nibbytes[offs++] = sixTwo[prev ^ prenib[i]];
				prev = prenib[i];
			}
			nibbytes[offs++] = sixTwo[prev];

			// Data field epilogue
			nibbytes[offs++] = 0xde;
			nibbytes[offs++] = 0xaa;
			nibbytes[offs++] = 0xeb;
		}

		while (offs < (track + 1) * DISK2_TRACK_SIZE)
			nibbytes[offs++] = 0xff;
	}

	delete [] diskdata;
	diskdata = nibbytes;
	disklen = DISK2_N_TRACKS * DISK2_TRACK_SIZE;
}

void
Apple2GtkApp::onMenuLoadDisk(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	std::string filename = doFileChooser(fileTypeDsk, false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Apple2GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		DPRINTF(1, "Apple2GtkApp::%s: loading file len %d\n", __func__,
			len);

		if (diskdata)
			delete [] diskdata;
		diskdata = new uint8_t[len];
		disklen = len;

		file.seekg(0, std::ios::beg);
		file.read((char *)diskdata, len);
		file.close();

		if (disklen == 143360)
			convertDskToNib();

		apple.getDisk()->setNib(diskdata);

		DPRINTF(1, "Apple2GtkApp::%s: NIB file len=%d\n", __func__,
			len);

		std::string basenm = filename;
		if (basenm.find_last_of("/") != std::string::npos)
			basenm = basenm.substr(basenm.find_last_of("/") + 1);
		entryDisk1->get_buffer()->set_text(basenm);
	}
}

void
Apple2GtkApp::onMenuSaveDisk(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	// No Disk!  XXX: error dialog?
	if (!diskdata)
		return;

	std::string filename = doFileChooser(fileTypeDsk, true);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Apple2GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ofstream file(filename, std::ios::out | std::ios::binary |
			   std::ios::trunc);
	if (file.is_open()) {
		file.write((char *)diskdata, disklen);
		file.close();
	} // XXX: else do error dialog
}

void
Apple2GtkApp::onMenuUnloadDisk(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);

	if (diskdata)
		delete [] diskdata;
	diskdata = nullptr;

	apple.getDisk()->setNib(nullptr);

	entryDisk1->get_buffer()->set_text("none");
}

void
Apple2GtkApp::onMenuBload(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);
	std::string filename = doFileChooser(fileTypeBinary, false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Apple2GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		DPRINTF(1, "Apple2GtkApp::%s: loading file len %d\n", __func__,
			len);

		uint8_t *binfile = new uint8_t[len];

		file.seekg(0, std::ios::beg);
		file.read((char *)binfile, len);
		file.close();

		uint16_t addr = binfile[0] + (uint16_t)binfile[1] * 256;

		apple.writeRam(addr, &binfile[2], len - 2);

		delete [] binfile;
	}
}

void
Apple2GtkApp::onMenuLoadRom(void)
{
	DPRINTF(1, "Apple2GtkApp::%s:\n", __func__);
	std::string filename = doFileChooser(fileTypeBinary, false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Apple2GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		DPRINTF(1, "Apple2GtkApp::%s: loading file len %d\n", __func__,
			len);

		uint8_t *binfile = new uint8_t[len];

		file.seekg(0, std::ios::beg);
		file.read((char *)binfile, len);
		file.close();

		apple.setRom(0xf800, binfile, len);

		delete [] binfile;
	}
}

void
Apple2GtkApp::diskCallback(bool motor, int track)
{
	DPRINTF(1, "Apple2GtkApp::%s: motor=%d track=%d\n", __func__,
		motor, track);

	spinnerDisk1->property_active().set_value(motor);
}

// Timer call-back function.
bool
Apple2GtkApp::onTimeout(void)
{
	if (running) {
		int cycles = 10000; // 10ms

		while (cycles-- > 0) {
			if (!apple.cycle()) {
				// Stopped due to breakpoint or other
				running = false;
				pauseButton->set_active(true);
				debugger.setState(debuggerActive, running);
				return false;
			}
		}

		return true;
	}

	return false;
}

bool
Apple2GtkApp::onIdle(void)
{
	if (running && turbo) {
		int cycles = 10000; // 10ms
		while (cycles-- > 0) {
			if (!apple.cycle()) {
				// Stopped due to breakpoint or other
				running = false;
				pauseButton->set_active(true);
				debugger.setState(debuggerActive, running);
				return false;
			}
		}

		return true;
	}

	return false;
}

// Start or stop Apple.
void
Apple2GtkApp::appleRun(bool flag)
{
	if (running == flag)
		return;

	if (flag) {
		Glib::signal_timeout().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onTimeout), 10);

		if (turbo)
			Glib::signal_idle().connect(sigc::mem_fun(*this,
					&Apple2GtkApp::onIdle));
	}

	running = flag;
}

void
Apple2GtkApp::appleTurbo(bool flag)
{
	if (turbo == flag)
		return;

	if (running && flag)
		Glib::signal_idle().connect(sigc::mem_fun(*this,
				&Apple2GtkApp::onIdle));

	turbo = flag;
}

void
Apple2GtkApp::onMenuQuit(void)
{
	auto windows = get_windows();
	for (auto window : windows)
		window->hide();

	quit();
}

int
main(int argc, char *argv[])
{
	auto app = Apple2GtkApp::create();

	return app->run(argc, argv);
}
