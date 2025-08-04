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

#ifndef __CPU6502GTKDEBUG_H__
#define __CPU6502GTKDEBUG_H__

class Cpu6502;
class MemSpace;

#define NINSTRS		32
#define MEMDISPSZ 	0x100

#define NBPENTRIES	4	// Number of BP entry widgets.

enum { CALLBACK_STOP, CALLBACK_STEP, CALLBACK_CONT, CALLBACK_CLOSE };

class Cpu6502GtkDebug {
private:
	Cpu6502		*cpu;
	MemSpace	*memspace;

	Gtk::Window	*debugWin;
	Gtk::TextView	*asmView;
	Gtk::TextView	*memView;

	Gtk::Button	*stopButton;
	Gtk::Button	*stepButton;
	Gtk::Button	*pauseButton;
	Gtk::CheckButton *illOpsButton;

	Gtk::Entry	*entryPC;
	Gtk::Entry	*entryA;
	Gtk::Entry	*entryX;
	Gtk::Entry	*entryY;
	Gtk::Entry	*entryP;
	Gtk::Entry	*entrySP;
	Gtk::Entry	*entryMemAddr;
	Gtk::Entry	*entryInstrAddr;
	Gtk::Entry	*entryBPAddr[NBPENTRIES];
	Gtk::Entry	*entryCycle;

	std::function<void (int)> debug_cb;

	int		*pCycleCounter;

	bool		running;

	bool		showIllOps;

        uint16_t	memaddr;
	uint16_t	disaddr;

	// State of debug disassembly display.
	int		numi;
	uint16_t	iaddrs[NINSTRS];
	char		*iline[NINSTRS];
	char		idispbuf[40 * NINSTRS];

	void		onDebugStep(void);
	void		onDebugStop(void);
	void		onDebugContinue(void);
	void		onIllOpsToggle(void);
	void		onUpdateMem(void);
	void		onUpdateInstr(void);
	void		onEntryBPAddrChanged(int n);
	bool 		onKeyEventMem(GdkEventKey *event);
	bool 		onKeyEventInstr(GdkEventKey *event);
	int		disinstr(uint8_t *instrp, uint16_t addr, char *line);
	void		updateDebugger(void);
	void		updateInstrDisp(uint16_t addr, uint16_t pc, bool top);
	void		updateMemDisp(void);
public:
	Cpu6502GtkDebug(Cpu6502 *_cpu);
	void		connectSignals(Glib::RefPtr<Gtk::Builder> builder);
	void		setDebugCallback(std::function<void (int)> _cb)
	{ debug_cb = _cb; }
	void		setCycleCounter(int *_pCycleCounter)
	{ pCycleCounter = _pCycleCounter; }
	void		setState(bool visible, bool _running);
};

#endif // __CPU6502GTKDEBUG_H__
