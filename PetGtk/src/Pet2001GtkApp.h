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

#ifndef __PET2001GTKAPP_H__
#define __PET2001GTKAPP_H__

#include "Pet2001.h"
#include "Pet2001GtkCass.h"
#include "Pet2001GtkIeee.h"
#include "Cpu6502GtkDebug.h"

class Pet2001GtkAppWin;
class Pet2001GtkDisp;

class Pet2001GtkApp : public Gtk::Application {
private:
	Pet2001GtkAppWin *appwindow;
	Pet2001GtkCass	cass;
	Pet2001GtkIeee	ieee;
	Pet2001		pet;
	Cpu6502GtkDebug	debugger;
	Pet2001GtkDisp	*disp;

	Gtk::ToggleButton *pauseButton;

	void		connectSignals(Glib::RefPtr <Gtk::Builder> builder);
	int		model;
	bool		running;
	bool		turbo;
	bool		debuggerActive;

	bool		onTimeout(void);
	bool		onIdle(void);
	void		petRun(bool flag);
	void		petTurbo(bool flag);
	void		onPauseToggle(void);
	void		onResetButton(void);
	void		debugCallback(int typ);
	void		onTurboToggle(Gtk::ToggleButton *button);
	void		onMenuDebugToggle(Gtk::CheckMenuItem *menu);
	void		onMenuDispDebugToggle(Gtk::CheckMenuItem *menu);
	void		onMenuDispGreenToggle(Gtk::CheckMenuItem *menu);
	void		onMenuReset(void);
	void		onMenuLoadPrg(void);
	void		onMenuSavePrg(void);
	void		onMenuLoadDisk(void);
	void		onMenuLoadRom(int);
	void		onMenuQuit(void);
	void		onMenuModel(Gtk::CheckMenuItem *menu, int n);
	void		onMenuRamsize(Gtk::CheckMenuItem *menu, int kbytes);
	void		onMenuAbout(Gtk::AboutDialog *about);
	void		loadRom(int model);
protected:
	Pet2001GtkApp();
public:
	static Glib::RefPtr<Pet2001GtkApp> create();
	Pet2001GtkAppWin *create_appwindow();
	std::string	doFileChooser(bool dosave, bool diskfile,
				      const char *hint = nullptr);
	void		on_startup();
	void		on_activate();
	void		on_hide_window(Gtk::Window *win);
};

#endif // __PET2001GTKAPP_H__
