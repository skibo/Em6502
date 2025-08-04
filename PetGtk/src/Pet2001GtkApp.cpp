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

#include <stdint.h>
#include <gtkmm.h>

#include "Pet2001GtkApp.h"

#include <cstdio>

#include <iostream>
#include <fstream>

#include "Pet2001.h"

#include "Pet2001GtkAppWin.h"
#include "Pet2001GtkDisp.h"
#include "Pet2001GtkKeys.h"
#include "Pet2001GtkCass.h"
#include "Pet2001GtkIeee.h"

#ifdef DEBUGAPP
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern uint8_t petrom1[];
extern uint8_t petrom2[];
extern uint8_t petrom4[];

Pet2001GtkApp::Pet2001GtkApp()
	: Gtk::Application("net.skibo.pet2001gtk"),
	  cass(this),
	  ieee(this),
	  pet(nullptr, cass.getCassHw(), ieee.getIeeeHw()),
	  debugger(pet.getCpu())
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	appwindow = nullptr;
	debuggerActive = false;
	disp = nullptr;
}

Glib::RefPtr<Pet2001GtkApp>
Pet2001GtkApp::create()
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	return Glib::RefPtr<Pet2001GtkApp>(new Pet2001GtkApp());
}

std::string
Pet2001GtkApp::doFileChooser(bool dosave, bool diskfile, const char *hint)
{
	DPRINTF(1, "Pet2001GtkApp::%s: dosave=%d diskfile=%d\n", __func__,
		dosave, diskfile);

	Gtk::FileChooserDialog chooser(*appwindow, diskfile ? "Disk" :
				       "Program", dosave ?
				       Gtk::FILE_CHOOSER_ACTION_SAVE :
				       Gtk::FILE_CHOOSER_ACTION_OPEN);

	chooser.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	chooser.add_button(dosave ? "_Save" : "_Open", Gtk::RESPONSE_OK);

	auto filter_prg = Gtk::FileFilter::create();
	if (diskfile) {
		filter_prg->set_name("Disk files");
		filter_prg->add_pattern("*.[Dd]64");
	} else {
		filter_prg->set_name("Prg files");
		filter_prg->add_pattern("*.[Pp][Rr][Gg]");
	}
	chooser.add_filter(filter_prg);

	auto filter_any = Gtk::FileFilter::create();
	filter_any->set_name("Any");
	filter_any->add_pattern("*");
	chooser.add_filter(filter_any);

	if (hint)
		chooser.set_current_name(hint);

	std::string filename = "";

	// I choo choo choose you!
	int result = chooser.run();

	if (result == Gtk::RESPONSE_OK)
		filename = chooser.get_filename();

	return filename;
}

Pet2001GtkAppWin *
Pet2001GtkApp::create_appwindow()
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	auto appwindow = Pet2001GtkAppWin::create(&pet);

	add_window(*appwindow);

	appwindow->signal_hide().connect(sigc::bind<Gtk::Window*>(mem_fun(
			*this, &Pet2001GtkApp::on_hide_window), appwindow));

	return appwindow;
}

void
Pet2001GtkApp::on_startup()
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	Gtk::Application::on_startup();
}

void
Pet2001GtkApp::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	builder->get_widget("pause_button", pauseButton);
	pauseButton->signal_toggled().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onPauseToggle));

	Gtk::Button *button = nullptr;
	builder->get_widget("reset_button", button);
	button->signal_clicked().connect(sigc::mem_fun(*this,
				      &Pet2001GtkApp::onResetButton));

	Gtk::ToggleButton *toggle_button = nullptr;
	builder->get_widget("turbo_button", toggle_button);
	toggle_button->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
			&Pet2001GtkApp::onTurboToggle), toggle_button));

	Gtk::MenuItem *menu = nullptr;
	builder->get_widget("menu_reset", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuReset));

	builder->get_widget("menu_load_prg", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuLoadPrg));

	builder->get_widget("menu_save_prg", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuSavePrg));

	builder->get_widget("menu_load_disk", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuLoadDisk));

	builder->get_widget("menu_rom_9", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuLoadRom),
				0x9));

	builder->get_widget("menu_rom_a", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuLoadRom),
				0xa));

	builder->get_widget("menu_rom_sys", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuLoadRom),
				0xc));

	builder->get_widget("menu_quit", menu);
	menu->signal_activate().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuQuit));

	Gtk::CheckMenuItem *checkmenu = nullptr;
	builder->get_widget("menu_debug", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuDebugToggle),
				checkmenu));

	builder->get_widget("menu_disp_dbg", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuDispDebugToggle),
				checkmenu));

	builder->get_widget("menu_green_disp", checkmenu);
	checkmenu->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuDispGreenToggle),
				checkmenu));

	builder->get_widget("menu_model_1", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuModel),
				checkmenu, 1));

	builder->get_widget("menu_model_2", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuModel),
				checkmenu, 2));

	builder->get_widget("menu_model_4", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuModel),
				checkmenu, 4));

	builder->get_widget("menu_ram_8k", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuRamsize),
				checkmenu, 8));

	builder->get_widget("menu_ram_16k", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuRamsize),
				checkmenu, 16));

	builder->get_widget("menu_ram_32k", checkmenu);
	checkmenu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuRamsize),
				checkmenu, 32));

	disp = nullptr;
	builder->get_widget_derived("pet_display", disp);

	Gtk::AboutDialog *about = nullptr;
	builder->get_widget("win_about", about);
	builder->get_widget("menu_about", menu);
	menu->signal_activate().connect(sigc::bind(sigc::mem_fun(*this,
				&Pet2001GtkApp::onMenuAbout), about));
}

void
Pet2001GtkApp::on_activate()
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	try {
		appwindow = create_appwindow();
		appwindow->present();
	}
	catch (const Glib::Error &ex) {
		fprintf(stderr, "Pet2001GtkApp::%s: %s\n", __func__,
			ex.what().c_str());
	}
	catch (const std::exception &ex) {
		fprintf(stderr, "Pet2001GtkApp::%s: %s\n", __func__,
			ex.what());
	}

	connectSignals(appwindow->getBuilder());
	cass.connectSignals(appwindow->getBuilder());
	ieee.connectSignals(appwindow->getBuilder());
	debugger.connectSignals(appwindow->getBuilder());
	debugger.setDebugCallback([&] (int typ) {this->debugCallback(typ);});
	debugger.setCycleCounter(disp->getCycleCounter());

	// Initialize PET hardware
	model = 2;
	running = false;
	turbo = false;
	pet.setRamsize(32768);
	disp->setVersion(0);
	loadRom(model);
	pet.reset();
	petRun(true);
}

void
Pet2001GtkApp::on_hide_window(Gtk::Window *win)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	delete win;
	appwindow = nullptr;
}

void
Pet2001GtkApp::onPauseToggle(void)
{
	bool state = pauseButton->get_active();

	DPRINTF(1, "Pet2001GtkApp::%s: state=%d\n", __func__, state);

	petRun(!state);
	debugger.setState(debuggerActive, running);
}

void
Pet2001GtkApp::onResetButton(void)
{
	DPRINTF(1, "Pet200GtkApp::%s:\n", __func__);

	pet.reset();
}

void
Pet2001GtkApp::debugCallback(int typ)
{
	DPRINTF(1, "Pet2001GtkApp::%s: typ=%d\n", __func__, typ);

	switch (typ) {
	case CALLBACK_STOP:
		petRun(false);
		pauseButton->set_active(true);
		debugger.setState(debuggerActive, running);
		break;

	case CALLBACK_STEP:
		pet.getCpu()->stepCpu();
		if (!running) {
			while (pet.cycle())
				;
		}
		break;

	case CALLBACK_CONT:
		petRun(true);
		pauseButton->set_active(false);
		debugger.setState(debuggerActive, running);
		break;
	}
}

void
Pet2001GtkApp::onTurboToggle(Gtk::ToggleButton *button)
{
	bool state = button->get_active();

	DPRINTF(1, "Pet2001GtkApp::%s: state=%d\n", __func__, state);

	petTurbo(state);
}

void
Pet2001GtkApp::onMenuDebugToggle(Gtk::CheckMenuItem *menu)
{
	bool state = menu->get_active();

	DPRINTF(1, "Pet2001GtkApp::%s: state=%d\n", __func__, state);

	debuggerActive = state;

	debugger.setState(debuggerActive, running);
}

void
Pet2001GtkApp::onMenuDispDebugToggle(Gtk::CheckMenuItem *menu)
{
	bool state = menu->get_active();

	DPRINTF(1, "Pet2001GtkApp::%s: state=%d\n", __func__, state);

	disp->setDebug(state);
}

void
Pet2001GtkApp::onMenuDispGreenToggle(Gtk::CheckMenuItem *menu)
{
	bool state = menu->get_active();

	DPRINTF(1, "Pet2001GtkApp::%s: state=%d\n", __func__, state);

	static uint8_t green[] = { 0x40, 0xff, 0x40, 0x00 };
	static uint8_t white[] = { 0xff, 0xff, 0xff, 0x00 };
	disp->setForeground(state ? green : white);
}

void
Pet2001GtkApp::onMenuReset(void)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	pet.reset();
}

void
Pet2001GtkApp::onMenuLoadPrg(void)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	std::string filename = doFileChooser(false, false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Pet2001GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		if (len > MAX_PROG_LEN)
			len = MAX_PROG_LEN;

		uint8_t *data = new uint8_t[len];
		file.seekg(0, std::ios::beg);
		file.read((char *)data, len);
		file.close();

		uint16_t start_addr = data[0] + ((uint16_t)data[1] << 8);

		DPRINTF(1, "Pet2001GtkApp::%s: start_addr=0x%x len=%d\n",
			__func__, start_addr, len);

		pet.writeRange(start_addr, &data[2], len - 2);
		if (start_addr == 0x0400 || start_addr == 0x0401) {
			uint16_t end_addr = start_addr + len - 2;
			uint8_t bytes[6];

			bytes[0] = end_addr & 0xff;
			bytes[1] = end_addr >> 8;
			bytes[2] = bytes[0];
			bytes[3] = bytes[1];
			bytes[4] = bytes[0];
			bytes[5] = bytes[1];

			// Tweak BASIC pointers.
			pet.writeRange(model > 1 ? 42 : 124, bytes, 6);
		}

		delete [] data;
	}
}

void
Pet2001GtkApp::onMenuLoadDisk(void)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	std::string filename = doFileChooser(false, true);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Pet2001GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		int len = file.tellg();

		if (len > MAX_DISK_LEN)
			len = MAX_DISK_LEN;

		uint8_t *data = new uint8_t[len];
		file.seekg(0, std::ios::beg);
		file.read((char *)data, len);
		file.close();

		ieee.loadDisk(data, len);

		delete [] data;
	}
}

void
Pet2001GtkApp::onMenuSavePrg(void)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	std::string filename = doFileChooser(true, false);

	// Empty string means Cancel.
	if (filename == "")
		return;

	DPRINTF(1, "Pet2001GtkApp::%s: filename=%s\n", __func__,
		filename.c_str());

	// Get end address of current program.
	uint8_t bytes[2];
	pet.readRange(model > 1 ? 42 : 124, bytes, sizeof(bytes));
	uint16_t end_addr = bytes[0] + ((uint16_t)bytes[1] << 8);

	// XXX: sanity check end_addr and create a dialog to modify these.
	uint16_t start_addr = 0x0401;

	int len = end_addr - start_addr + 2;
	uint8_t *data = new uint8_t[len];
	data[0] = start_addr & 0xff;
	data[1] = start_addr >> 8;

	DPRINTF(1, "Pet2001GtkApp::%s: start_addr=0x%x len=%d\n",
		__func__, start_addr, len);

	pet.readRange(start_addr, &data[2], len - 2);

	std::ofstream file(filename, std::ios::out | std::ios::binary |
			   std::ios::trunc);
	if (file.is_open()) {
		file.write((char *)data, len);
		file.close();
	} // XXX: else do error dialog

	delete [] data;
}

void
Pet2001GtkApp::onMenuLoadRom(int page)
{
	DPRINTF(1, "Pet2001GtkApp::%s: page=%d\n", __func__, page);

	Gtk::FileChooserDialog chooser(*appwindow, page == 0xc ?
				       "System ROM Image" :
				       "Expansion ROM Image",
				       Gtk::FILE_CHOOSER_ACTION_OPEN);

	chooser.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	chooser.add_button("_Open", Gtk::RESPONSE_OK);

	auto filter_prg = Gtk::FileFilter::create();
	filter_prg->set_name("Binary files");
	filter_prg->add_pattern("*.bin");
	chooser.add_filter(filter_prg);

	auto filter_any = Gtk::FileFilter::create();
	filter_any->set_name("Any");
	filter_any->add_pattern("*");
	chooser.add_filter(filter_any);

	std::string filename = "";

	int result = chooser.run();

	if (result != Gtk::RESPONSE_OK)
		return;

	filename = chooser.get_filename();
	DPRINTF(1, "Pet2001GtkApp::%s:  filename=%s\n", __func__,
		filename.c_str());

	std::ifstream file(filename, std::ios::in | std::ios::binary |
			   std::ios::ate);
	if (file.is_open()) {
		uint16_t addr;
		uint16_t sysaddr = (model == 4) ? 0xb000 : 0xc000;
		int maxlen;
		if (page == 0xc) {
			// System ROM image
			addr = sysaddr;
			maxlen = 0xf800 - addr;
		} else {
			// Expansion ROM image
			addr = page * 0x1000;
			maxlen = sysaddr - addr;
		}

		int len = file.tellg();

		if (len > maxlen)
			len = maxlen;

		uint8_t *data = new uint8_t[len];
		file.seekg(0, std::ios::beg);
		file.read((char *)data, len);
		file.close();

		if (page != 0xc) {
			pet.writeRom(addr, data, len);
		} else {
			int chunk1 = IO_ADDR - sysaddr;
			pet.writeRom(addr, data, std::min(chunk1, len));
			if (len > chunk1)
				pet.writeRom(IO_ADDR + IO_SIZE,
					     data + chunk1, len - chunk1);
			pet.reset();
		}

		delete [] data;
	}
}

void
Pet2001GtkApp::onMenuModel(Gtk::CheckMenuItem *menu, int n)
{
	if (!menu->get_active())
		return;

	DPRINTF(1, "Pet2001GtkApp::%s: n=%d\n", __func__, n);

	if (model == n)
		return;

	model = n;

	disp->setVersion(model < 3 ? 0 : 1);
	loadRom(model);
	pet.reset();
}

void
Pet2001GtkApp::onMenuRamsize(Gtk::CheckMenuItem *menu, int kbytes)
{
	if (!menu->get_active())
		return;

	DPRINTF(1, "Pet2001GtkApp::%s: kbytes=%d\n", __func__, kbytes);

	pet.setRamsize(1024 * kbytes);
	pet.reset();
}

void
Pet2001GtkApp::onMenuAbout(Gtk::AboutDialog *about)
{
	DPRINTF(1, "Pet2001GtkApp::%s:\n", __func__);

	about->run();
	about->set_visible(false);
}

void
Pet2001GtkApp::loadRom(int model)
{
	switch (model) {
	case 1:
		pet.writeRom(0xC000, petrom1, 0x2800);
		pet.writeRom(0xF000, petrom1 + 0x2800, 0x1000);
		break;
	case 2:
		pet.writeRom(0xC000, petrom2, 0x2800);
		pet.writeRom(0xF000, petrom2 + 0x2800, 0x1000);
		break;
	case 4:
		pet.writeRom(0xB000, petrom4, 0x3800);
		pet.writeRom(0xF000, petrom4 + 0x3800, 0x1000);
		break;
	}
}

// Timer call-back function.
bool
Pet2001GtkApp::onTimeout(void)
{
	if (running) {
		int cycles = 10000; // 10ms

		while (cycles-- > 0) {
			if (!pet.cycle()) {
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
Pet2001GtkApp::onIdle(void)
{
	if (running && turbo) {
		int cycles = 10000; // 10ms
		while (cycles-- > 0) {
			if (!pet.cycle()) {
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

// Start or stop PET.
void
Pet2001GtkApp::petRun(bool flag)
{
	if (running == flag)
		return;

	if (flag) {
		Glib::signal_timeout().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onTimeout), 10);

		if (turbo)
			Glib::signal_idle().connect(sigc::mem_fun(*this,
					&Pet2001GtkApp::onIdle));
	}

	running = flag;
}

void
Pet2001GtkApp::petTurbo(bool flag)
{
	if (turbo == flag)
		return;

	if (running && flag)
		Glib::signal_idle().connect(sigc::mem_fun(*this,
				&Pet2001GtkApp::onIdle));

	turbo = flag;
}

void
Pet2001GtkApp::onMenuQuit(void)
{
	auto windows = get_windows();
	for (auto window : windows)
		window->hide();

	quit();
}

int
main(int argc, char *argv[])
{
	auto app = Pet2001GtkApp::create();

	return app->run(argc, argv);
}
