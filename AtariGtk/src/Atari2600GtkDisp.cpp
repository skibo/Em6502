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

// Atari2600GtkDisp.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Atari2600GtkDisp.h"

#ifdef DEBUGVID
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGVID) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern const uint8_t colortab[];

#define ATARI_VID_HEIGHT	212	// larger than 192 native height
#define ATARI_VID_VBLANK	30	// skip lines, including sync pulse

Atari2600GtkDisp::Atari2600GtkDisp(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder)
	: Gtk::DrawingArea(cobject)
{
	DPRINTF(1, "Atari2600GtkDisp::constructor:\n");

	pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB,
				     false, 8, ATARI_NATIVE_WIDTH,
				     ATARI_VID_HEIGHT);
	pb_data = pixbuf->get_pixels();
	pb_stride = pixbuf->get_rowstride();
	pb_n_channels = pixbuf->get_n_channels();

	DPRINTF(1, "Atari2600GtkDisp::constructor: stride=%d n_channels=%d\n",
		pb_stride, pb_n_channels);

	disp_scale = 1.0;

	reset();
}

void
Atari2600GtkDisp::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Atari2600GtkDisp::%s:\n", __func__);

	signal_configure_event().connect(sigc::mem_fun(*this,
				       &Atari2600GtkDisp::onConfigure));
	signal_draw().connect(sigc::mem_fun(*this, &Atari2600GtkDisp::onDraw));

	vstatEntry = nullptr;
	builder->get_widget("vlines_entry", vstatEntry);
}

// Called when window resizes.
bool
Atari2600GtkDisp::onConfigure(GdkEventConfigure *event)
{
	int width = get_allocated_width();
	int height = get_allocated_height();

	disp_scale = MIN((float)width / ATARI_NATIVE_WIDTH / 2.0,
		    (float)height / ATARI_VID_HEIGHT);

	DPRINTF(1, "Atari2600GtkDisp::%s: width=%d height=%d scale=%f\n",
		__func__, width, height, disp_scale);

	disp_left = ((float)width - 2.0 * disp_scale * ATARI_NATIVE_WIDTH) /
		2.0;
	disp_top = ((float)height - disp_scale * ATARI_VID_HEIGHT) / 2.0;

	queue_draw();
	update_disp = false;

	return true;
}

bool
Atari2600GtkDisp::onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr)
{
#if DEBUGVID > 1
	double x1, y1, x2, y2;
	cr->get_clip_extents(x1, y1, x2, y2);
	DPRINTF(2, "Atari2600GtkDisp::%s: rect=(%.0f,%.0f,%.0f,%.0f)\n",
		__func__, x1, y1, x2, y2);
#endif
	cr->save();
	cr->translate(disp_left, disp_top);
	cr->scale(disp_scale * 2.0, disp_scale);
	Gdk::Cairo::set_source_pixbuf(cr, pixbuf);
	cr->paint();
	cr->restore();

	return true;
}

void
Atari2600GtkDisp::reset(void)
{
	DPRINTF(1, "Atari2600GtkDisp::%s:\n", __func__);

	vline = 0;
	lines_since_vsync = 0;
	lines_update_vstat = 0;
	last_vheight = 999;
	got_scanline = false;

	memset(pb_data, 0, pb_stride * ATARI_VID_HEIGHT);
	update_disp = true;
}

void
Atari2600GtkDisp::setScanline(uint8_t colu[])
{
	DPRINTF(4, "Atari2600GtkDisp::%s: y=%d\n", __func__, vline);

	if (vline < ATARI_VID_VBLANK ||
	    vline >= ATARI_VID_VBLANK + ATARI_VID_HEIGHT)
		return;

	int y = vline - ATARI_VID_VBLANK;

	u_char *p = pb_data + y * pb_stride;
	for (int i = 0; i < ATARI_NATIVE_WIDTH; i++) {
		const uint8_t *colp = &colortab[(colu[i] >> 1) * 3];
		if (memcmp(p, colp, pb_n_channels) != 0) {
			memcpy(p, colp, pb_n_channels);
			update_disp = true;
		}
		p += pb_n_channels;
	}

	got_scanline = true;
}

void
Atari2600GtkDisp::clearLines(int line, int n)
{
	DPRINTF(4, "Atari2600GtkDisp::%s: line=%d n=%d\n", __func__,
		line, n);

	u_char *p = pb_data + line * pb_stride;
	int size = n * pb_stride;

	for (int i = 0; i < size; i++) {
		if (p[i] != 0)
			update_disp = true;
		p[i] = 0;
	}
}

void
Atari2600GtkDisp::vsync(void)
{
	DPRINTF(5, "Atari2600GtkDisp::%s: vline=%d\n", __func__, vline);

	if (vline < ATARI_VID_VBLANK + ATARI_VID_HEIGHT) {
		// Blank extra lines.
		if (last_vheight > vline)
			clearLines(MAX(vline - ATARI_VID_VBLANK, 0),
				   ATARI_VID_HEIGHT -
				   MAX(vline - ATARI_VID_VBLANK, 0));
	}

	last_vheight = vline;
	vline = 0;
	lines_since_vsync = 0;

	if(update_disp) {
		queue_draw();
		update_disp = false;
	}
}

void
Atari2600GtkDisp::hsync(void)
{
	if (!got_scanline && vline >= ATARI_VID_VBLANK &&
	    vline < ATARI_VID_VBLANK + ATARI_VID_HEIGHT)
		clearLines(vline - ATARI_VID_VBLANK, 1);

	got_scanline = false;
	vline++;

	if (++lines_since_vsync == 600) {
		// No vsync in roughly 38ms
		DPRINTF(4, "Atari2600GtkDisp::%s: missing vsync.\n", __func__);
		clearLines(0, ATARI_VID_HEIGHT);
		last_vheight = 999;
	}

	if (++lines_update_vstat > 2000) {
		// Update vertical status
		static char buf[128];
		if (last_vheight >= 999)
			strncpy(buf, "Video Status: No VSync!", sizeof(buf));
		else
			snprintf(buf, sizeof(buf), "Video Status: VLines: %d", last_vheight);
		vstatEntry->get_buffer()->set_text(buf);
		lines_update_vstat = 0;
	}
}
