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

#ifndef __ATARI2600GTKAPP_H__
#define __ATARI2600GTKAPP_H__

#include "Atari2600.h"
#include "Cpu6502GtkDebug.h"

class Atari2600GtkAppWin;
class Atari2600GtkDisp;

class Atari2600GtkApp : public Gtk::Application {
private:
	Atari2600GtkAppWin *appwindow;
	Atari2600	atari;
	Cpu6502GtkDebug	debugger;
	Atari2600GtkDisp *disp;

	Gtk::ToggleButton *pauseButton;
	Gtk::Entry	*entryRom;

	bool		debuggerActive;

	bool		running;
	bool		turbo;
	uint8_t		*diskdata;
	int		disklen;

	void		connectSignals(Glib::RefPtr <Gtk::Builder> builder);
	std::string	doFileChooser(bool dosave);
	bool		onTimeout(void);
	bool		onIdle(void);
	void		atariRun(bool flag);
	void		atariTurbo(bool flag);
	void		onPauseToggle(void);
	void		onTurboToggle(Gtk::ToggleButton *button);
	void		onResetButton(Gtk::Button *button);
	void		onSelectButton(Gtk::Button *button, bool flag);
	void		onStartButton(Gtk::Button *button, bool flag);
	void		onDiffLToggle(Gtk::ToggleButton *toggle);
	void		onDiffRToggle(Gtk::ToggleButton *toggle);
	void		onMenuReset(void);
	void		onMenuRestart(void);
	void		onMenuDebugToggle(Gtk::CheckMenuItem *checkmenu);
	void		onMenuAbout(Gtk::AboutDialog *about);
	void		debugCallback(int typ);
	void		onMenuLoadRom(void);
	void		onMenuQuit(void);

	void		diskCallback(bool motor, int track);
protected:
	Atari2600GtkApp();
	~Atari2600GtkApp();
public:
	static Glib::RefPtr<Atari2600GtkApp> create();
	Atari2600GtkAppWin *create_appwindow();
	void		on_startup();
	void		on_activate();
	void		on_hide_window(Gtk::Window *win);
};

#endif // __ATARI2600GTKAPP_H__
