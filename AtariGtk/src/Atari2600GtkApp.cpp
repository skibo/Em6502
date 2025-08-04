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

#include <stdint.h>
#include <gtkmm.h>

#include "Atari2600GtkApp.h"

#include <cstdio>

#include <iostream>
#include <fstream>

#include "Atari2600.h"

#include "Atari2600GtkAppWin.h"

#ifdef DEBUGAPP
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern const uint8_t spaceinvaders[];
#define SPACEINVADERSLEN 4096

Atari2600GtkApp::Atari2600GtkApp()
	: Gtk::Application("net.skibo.atarigtk"),
	  atari(nullptr),
	  debugger(atari.getCpu())
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	appwindow = nullptr;
	debuggerActive = false;
	disp = nullptr;
}

Atari2600GtkApp::~Atari2600GtkApp()
{
}

Glib::RefPtr<Atari2600GtkApp>
Atari2600GtkApp::create()
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	return Glib::RefPtr<Atari2600GtkApp>(new Atari2600GtkApp());
}

std::string
Atari2600GtkApp::doFileChooser(bool dosave)
{
	DPRINTF(1, "Atari2600GtkApp::%s: dosave=%d\n", __func__, dosave);

	Gtk::FileChooserDialog chooser(*appwindow, "File", dosave ?
				       Gtk::FILE_CHOOSER_ACTION_SAVE :
				       Gtk::FILE_CHOOSER_ACTION_OPEN);

	chooser.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	chooser.add_button(dosave ? "_Save" : "_Open", Gtk::RESPONSE_OK);

	auto filter = Gtk::FileFilter::create();
	filter->set_name("Binary files");
	filter->add_pattern("*.bin");
	chooser.add_filter(filter);

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

Atari2600GtkAppWin *
Atari2600GtkApp::create_appwindow()
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	auto appwindow = Atari2600GtkAppWin::create(&atari);

	add_window(*appwindow);

	appwindow->signal_hide().connect(sigc::bind<Gtk::Window*>(mem_fun(
			*this, &Atari2600GtkApp::on_hide_window), appwindow));

	return appwindow;
}

void
Atari2600GtkApp::on_startup()
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	Gtk::Application::on_startup();
}

void
Atari2600GtkApp::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	pauseButton = nullptr;
	builder->get_widget("pause_button", pauseButton);
	pauseButton->signal_toggled().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onPauseToggle));

	Gtk::ToggleButton *toggle = nullptr;
	builder->get_widget("turbo_button", toggle);
	toggle->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onTurboToggle), toggle));

	Gtk::Button *button = nullptr;
	builder->get_widget("reset_button", button);
	button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onResetButton), button));

	button = nullptr;
	builder->get_widget("select_button", button);
	button->signal_pressed().connect(sigc::bind(sigc::mem_fun(*this,
			&Atari2600GtkApp::onSelectButton), button, true));
	button->signal_released().connect(sigc::bind(sigc::mem_fun(*this,
			&Atari2600GtkApp::onSelectButton), button, false));

	button = nullptr;
	builder->get_widget("start_button", button);
	button->signal_pressed().connect(sigc::bind(sigc::mem_fun(*this,
			&Atari2600GtkApp::onStartButton), button, true));
	button->signal_released().connect(sigc::bind(sigc::mem_fun(*this,
			&Atari2600GtkApp::onStartButton), button, false));

	toggle = nullptr;
	builder->get_widget("diff_l_toggle", toggle);
	toggle->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onDiffLToggle), toggle));

	toggle = nullptr;
	builder->get_widget("diff_r_toggle", toggle);
	toggle->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onDiffRToggle), toggle));

	Gtk::MenuItem *menu = nullptr;
	builder->get_widget("menu_reset", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onMenuReset));

	Gtk::CheckMenuItem *checkmenu = nullptr;
	builder->get_widget("menu_debug", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onMenuDebugToggle),
						       checkmenu));

	menu = nullptr;
	builder->get_widget("menu_load_rom", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onMenuLoadRom));

	menu = nullptr;
	builder->get_widget("menu_quit", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onMenuQuit));

	entryRom = nullptr;
	builder->get_widget("entry_rom", entryRom);

	Gtk::AboutDialog *about = nullptr;
	builder->get_widget("win_about", about);
	builder->get_widget("menu_about", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Atari2600GtkApp::onMenuAbout), about));

	disp = appwindow->getDisp();
}

void
Atari2600GtkApp::on_activate()
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	try {
		appwindow = create_appwindow();
		appwindow->present();
	}
	catch (const Glib::Error &ex) {
		fprintf(stderr, "Atari2600GtkApp::%s: %s\n", __func__,
			ex.what().c_str());
	}
	catch (const std::exception &ex) {
		fprintf(stderr, "Atari2600GtkApp::%s: %s\n", __func__,
			ex.what());
	}

	connectSignals(appwindow->getBuilder());
	debugger.connectSignals(appwindow->getBuilder());
	debugger.setDebugCallback([&] (int typ) {this->debugCallback(typ);});
	debugger.setCycleCounter(atari.getCycleCounter());

	// Initialize Atari hardware
	running = false;
	turbo = false;
	atari.setRom(spaceinvaders, SPACEINVADERSLEN);
	atari.reset();
	atariRun(true);

}

void
Atari2600GtkApp::on_hide_window(Gtk::Window *win)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	delete win;
	appwindow = nullptr;
}

void
Atari2600GtkApp::debugCallback(int typ)
{
	DPRINTF(1, "Pet2001GtkApp::%s: typ=%d\n", __func__, typ);

	switch (typ) {
	case CALLBACK_STOP:
		atariRun(false);
		pauseButton->set_active(true);
		debugger.setState(debuggerActive, running);
		break;

	case CALLBACK_STEP:
		atari.getCpu()->stepCpu();
		if (!running) {
			while (atari.cycle())
				;
		}
		break;

	case CALLBACK_CONT:
		atariRun(true);
		pauseButton->set_active(false);
		debugger.setState(debuggerActive, running);
		break;
	}
}

void
Atari2600GtkApp::onPauseToggle(void)
{
	bool state = pauseButton->get_active();

	DPRINTF(1, "Atari2600GtkApp::%s: state=%d\n", __func__, state);

	atariRun(!state);
	debugger.setState(debuggerActive, running);
}

void
Atari2600GtkApp::onTurboToggle(Gtk::ToggleButton *button)
{
	bool state = button->get_active();

	DPRINTF(1, "Atari2600GtkApp::%s: state=%d\n", __func__, state);

	atariTurbo(state);
}

void
Atari2600GtkApp::onResetButton(Gtk::Button *button)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	atari.reset();
}

void
Atari2600GtkApp::onMenuReset(void)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	atari.reset();
}

void
Atari2600GtkApp::onMenuDebugToggle(Gtk::CheckMenuItem *checkmenu)
{
	bool state = checkmenu->get_active();

	DPRINTF(1, "Atari2600GtkApp::%s: state=%d\n", __func__, state);

	debuggerActive = state;

	debugger.setState(debuggerActive, running);
}

void
Atari2600GtkApp::onMenuAbout(Gtk::AboutDialog *about)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	about->run();
	about->set_visible(false);
}

void
Atari2600GtkApp::onMenuLoadRom(void)
{
	DPRINTF(1, "Atari2600GtkApp::%s:\n", __func__);

	std::string filename = doFileChooser(false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Atari2600GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		if (len > ROM_MAX_SIZE)
			len = ROM_MAX_SIZE;

		uint8_t *data = new uint8_t[len];
		file.seekg(0, std::ios::beg);
		file.read((char *)data, len);
		file.close();

		atari.setRom(data, len);
		atari.reset();

		std::string romnm = filename;
		if (romnm.find_last_of("/") != std::string::npos)
			romnm = romnm.substr(romnm.find_last_of("/") + 1);
		entryRom->get_buffer()->set_text(romnm);

		delete [] data;
	}
}

void
Atari2600GtkApp::onSelectButton(Gtk::Button *button, bool flag)
{
	DPRINTF(1, "Atari2600GtkApp::%s: flag=%d\n", __func__, flag);
	atari.setSelect(flag);
}

void
Atari2600GtkApp::onStartButton(Gtk::Button *button, bool flag)
{
	DPRINTF(1, "Atari2600GtkApp::%s: flag=%d\n", __func__, flag);
	atari.setStart(flag);
}

void
Atari2600GtkApp::onDiffLToggle(Gtk::ToggleButton *toggle)
{
	bool active = toggle->get_active();
	DPRINTF(1, "Atari2600GtkApp::%s: active=%d\n", __func__, active);

	toggle->set_label(active ? "L Expert" : "L Novice");

	atari.setDiffLeft(active);
}

void
Atari2600GtkApp::onDiffRToggle(Gtk::ToggleButton *toggle)
{
	bool active = toggle->get_active();
	DPRINTF(1, "Atari2600GtkApp::%s: active=%d\n", __func__, active);

	toggle->set_label(active ? "R Expert" : "R Novice");

	atari.setDiffRight(active);
}


// Timer call-back function.
bool
Atari2600GtkApp::onTimeout(void)
{
	if (running) {
		int cycles = 35800; // 10ms

		while (cycles-- > 0) {
			if (!atari.cycle()) {
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
Atari2600GtkApp::onIdle(void)
{
	if (running && turbo) {
		int cycles = 35800; // 10ms
		while (cycles-- > 0) {
			if (!atari.cycle()) {
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

// Start or stop Atari.
void
Atari2600GtkApp::atariRun(bool flag)
{
	if (running == flag)
		return;

	if (flag) {
		Glib::signal_timeout().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onTimeout), 10);

		if (turbo)
			Glib::signal_idle().connect(sigc::mem_fun(*this,
					&Atari2600GtkApp::onIdle));
	}

	running = flag;
}

void
Atari2600GtkApp::atariTurbo(bool flag)
{
	if (turbo == flag)
		return;

	if (running && flag)
		Glib::signal_idle().connect(sigc::mem_fun(*this,
				&Atari2600GtkApp::onIdle));

	turbo = flag;
}

void
Atari2600GtkApp::onMenuQuit(void)
{
	auto windows = get_windows();
	for (auto window : windows)
		window->hide();

	quit();
}

int
main(int argc, char *argv[])
{
	auto app = Atari2600GtkApp::create();

	return app->run(argc, argv);
}
