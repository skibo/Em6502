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

// Pet2001GtkDisp.h

#ifndef __PET2001GTKDISP_H__
#define __PET2001GTKDISP_H__

#include "PetVideo.h"

#define PET_VIDPIXELS_SIZE 	8000

class Pet2001GtkDisp : public Gtk::DrawingArea, public PetVideo {
private:
	float		disp_scale;
	float		disp_left;
	float		disp_top;

	uint8_t		vidmem[PET_VRAM_SIZE];
	uint8_t		vidpixels[PET_VIDPIXELS_SIZE];
	uint8_t		*charrom;
	int		version;
	bool		alt_charset;
	bool		blank;
	int		video_cycle;
	bool		snowcycle;
	uint8_t		snowbyte;
	bool 		debugMode;

	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	u_char		*pb_data;
	int		pb_stride;
	int		pb_n_channels;
	u_char		bg_color[4];
	u_char		fg_color[4];

	Gdk::Rectangle	update_rect;

	bool	onConfigure(GdkEventConfigure *event);
	bool	onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr);
	void	syncPixbuf(void);
	void 	updateChar(int col, int row, uint8_t d8);
	void	updateAll(void);
public:
	Pet2001GtkDisp(BaseObjectType *cobject,
		       const Glib::RefPtr<Gtk::Builder> &refBuilder);

	void	connectSignals(void);

	void	setVersion(int);	// 0 = PET 2001, 1 = PET 2001N.
	void 	setDebug(bool _m);	// debug mode.
	void	setForeground(uint8_t []);
	void	setBackground(uint8_t []);
	int	*getCycleCounter(void)
	{ return &this->video_cycle; }

	// PetVideo interface
	void	sync(void);
	void	reset(void);
	void	cycle(void);
	void	setCharset(bool alt);
	void	setBlank(bool blank);

	void	write(uint16_t offset, uint8_t d8);
	uint8_t	read(uint16_t offset);
};

#endif // __PET2001GTKDISP_H__
