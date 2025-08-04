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

#ifndef __APPLE2GTKAPP_H__
#define __APPLE2GTKAPP_H__

#include "Apple2.h"
#include "Cpu6502GtkDebug.h"

class Apple2GtkAppWin;
class Apple2GtkDisp;

enum e_fileType { fileTypeDsk, fileTypeBinary };

class Apple2GtkApp : public Gtk::Application {
private:
	Apple2GtkAppWin *appwindow;
	Apple2		apple;
	Cpu6502GtkDebug	debugger;
	Apple2GtkDisp	*disp;

	Gtk::ToggleButton *pauseButton;
	Gtk::Entry	*entryDisk1;
	Gtk::Spinner	*spinnerDisk1;

	bool		debuggerActive;

	bool		running;
	bool		turbo;
	uint8_t		*diskdata;
	int		disklen;

	void		connectSignals(Glib::RefPtr <Gtk::Builder> builder);
	std::string	doFileChooser(enum e_fileType type, bool dosave);
	bool		onTimeout(void);
	bool		onIdle(void);
	void		appleRun(bool flag);
	void		appleTurbo(bool flag);
	void		onPauseToggle(void);
	void		onTurboToggle(Gtk::ToggleButton *button);
	void		onResetButton(Gtk::Button *button);
	void		onMenuReset(void);
	void		onMenuRestart(void);
	void		onMenuColorToggle(Gtk::CheckMenuItem *checkmenu);
	void		onMenuDebugToggle(Gtk::CheckMenuItem *checkmenu);
	void		onMenuAbout(Gtk::AboutDialog *about);
	void		debugCallback(int typ);
	void		convertDskToNib(void);
	void		onMenuLoadDisk(void);
	void		onMenuSaveDisk(void);
	void		onMenuUnloadDisk(void);
	void		onMenuBload(void);
	void		onMenuLoadRom(void);
	void		onMenuQuit(void);

	void		diskCallback(bool motor, int track);
protected:
	Apple2GtkApp();
	~Apple2GtkApp();
public:
	static Glib::RefPtr<Apple2GtkApp> create();
	Apple2GtkAppWin *create_appwindow();
	void		on_startup();
	void		on_activate();
	void		on_hide_window(Gtk::Window *win);
};

#endif // __APPLE2GTKAPP_H__
