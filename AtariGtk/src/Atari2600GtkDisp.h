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

// Atari2600GtkDisp.h

#ifndef __ATARI2600GTKDISP_H__
#define __ATARI2600GTKDISP_H__

#include "Atari2600Video.h"

class Atari2600GtkDisp : public Gtk::DrawingArea, public Atari2600Video {
private:
	float		disp_scale;
	float		disp_left;
	float		disp_top;

	int		vline;
	unsigned int	lines_since_vsync;
	int		last_vheight;
	int		lines_update_vstat;
	bool		got_scanline;
	bool 		update_disp;

	Gtk::Entry	*vstatEntry;

	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	u_char		*pb_data;
	int		pb_stride;
	int		pb_n_channels;

	void	clearLines(int line, int n);

	bool	onConfigure(GdkEventConfigure *event);
	bool	onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr);
public:
	Atari2600GtkDisp(BaseObjectType *cobject,
		       const Glib::RefPtr<Gtk::Builder> &refBuilder);

	void	connectSignals(Glib::RefPtr <Gtk::Builder> builder);

	// Atari2600Video interface
	void	setScanline(uint8_t colu[]);
	void	vsync(void);
	void	hsync(void);
	void	reset(void);
};

#endif // __ATARI2600GTKDISP_H__
