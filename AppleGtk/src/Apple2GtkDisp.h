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

// Apple2GtkDisp.h

#ifndef __APPLE2GTKDISP_H__
#define __APPLE2GTKDISP_H__

#include "Apple2Video.h"

#define APPLE_VIDMEM_SIZE	0x6000

class Apple2GtkDisp : public Gtk::DrawingArea, public Apple2Video {
private:
	float		disp_scale;
	float		disp_left;
	float		disp_top;

	bool		flashing;
	bool		color;
	bool		flash_on;

	uint8_t		vidmem[APPLE_VIDMEM_SIZE];
	bool		gfx_en;
	bool		hires_en;
	bool		mix_en;
	bool		page_en;


	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	u_char		*pb_data;
	int		pb_stride;
	int		pb_n_channels;

	void	updateChar(int col, int row, uint8_t d8);
	void	updateLores(int col, int row, uint8_t d8);
	void	updateHires(int col, int y, uint8_t d8l, uint8_t d8,
			    uint8_t d8r);
	void	updateAll(void);

	bool	onConfigure(GdkEventConfigure *event);
	bool	onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr);
	bool	onFlashTimeout(void);
public:
	Apple2GtkDisp(BaseObjectType *cobject,
		       const Glib::RefPtr<Gtk::Builder> &refBuilder);

	void	connectSignals(Glib::RefPtr <Gtk::Builder> builder);
	void	setFlashing(bool flag);
	void	setColor(bool flag);

	// Apple2Video interface
	void	reset(void);
	void	cycle(void) { }
	void	setGfx(bool gfx);
	void	setHires(bool hires);
	void	setMix(bool mix);
	void	setPage(bool page);

	void	write(uint16_t addr, uint8_t d8);
	uint8_t read(uint16_t addr);
};

#endif // __APPLE2GTKDISP_H__
