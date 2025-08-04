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

// Pet2001GtkDisp.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Pet2001GtkDisp.h"

#ifdef DEBUGVID
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGVID) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern uint8_t characters_1[];
extern uint8_t characters_2[];

#define PET_NATIVE_WIDTH	320
#define PET_NATIVE_HEIGHT	200

/* VCYCLE0 is the number of 1us cycles after the SYNC signal goes low that
 * the first video RAM data is read.  Video bytes are read each of the next
 * 40 cycles.  The next line starts being read 64 cycles after the first.
 */
#define VCYCLE0	3863
#define VCYCLEEND (VCYCLE0 + (64 * 199) + 40)

// Constructor in case we need it.
Pet2001GtkDisp::Pet2001GtkDisp(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder)
	: Gtk::DrawingArea(cobject)
{
	DPRINTF(1, "Pet2001GtkDisp::constructor:\n");

	pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB,
				     false, 8, PET_NATIVE_WIDTH,
				     PET_NATIVE_HEIGHT);
	pb_data = pixbuf->get_pixels();
	pb_stride = pixbuf->get_rowstride();
	pb_n_channels = pixbuf->get_n_channels();
	update_rect = Gdk::Rectangle(0, 0, 0, 0);
	charrom = characters_1;
	version = 0;

	if (pb_n_channels > sizeof(bg_color)) {
		fprintf(stderr, "%s: pixbuf has too many channels: %d\n",
			__func__, pb_n_channels);
		::exit(1);
	}

	memset(bg_color, 0x00, pb_n_channels);
	memset(fg_color, 0xff, pb_n_channels);

	DPRINTF(1, "Pet2001GtkDisp::constructor: stride=%d n_channels=%d\n",
		pb_stride, pb_n_channels);

	syncPixbuf();
	reset();
}

void
Pet2001GtkDisp::connectSignals(void)
{
	DPRINTF(1, "Pet2001GtkDisp::%s:\n", __func__);

	signal_configure_event().connect(sigc::mem_fun(*this,
					       &Pet2001GtkDisp::onConfigure));
	signal_draw().connect(sigc::mem_fun(*this, &Pet2001GtkDisp::onDraw));
}

bool
Pet2001GtkDisp::onConfigure(GdkEventConfigure *event)
{
	int width = get_allocated_width();
	int height = get_allocated_height();

	disp_scale = MIN((float)width / PET_NATIVE_WIDTH,
		    (float)height / PET_NATIVE_HEIGHT);

	DPRINTF(1, "Pet2001GtkDisp::%s: width=%d height=%d scale=%f\n",
		__func__, width, height, disp_scale);

	disp_left = ((float)width - disp_scale * PET_NATIVE_WIDTH) / 2.0;
	disp_top = ((float)height - disp_scale * PET_NATIVE_HEIGHT) / 2.0;

	// XXX: queue_draw() ???? to initialize surface?

	return true;
}

bool
Pet2001GtkDisp::onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr)
{
#if DEBUGVID > 1
	double x1, y1, x2, y2;
	cr->get_clip_extents(x1, y1, x2, y2);
	DPRINTF(2, "Pet2001GtkDisp::%s: rect=(%.0f,%.0f,%.0f,%.0f)\n",
		__func__, x1, y1, x2, y2);
#endif

	cr->save();
	cr->translate(disp_left, disp_top);
	cr->scale(disp_scale, disp_scale);
	Gdk::Cairo::set_source_pixbuf(cr, pixbuf);
	cr->paint();
	cr->restore();

	return true;
}

void
Pet2001GtkDisp::setVersion(int n)
{
	DPRINTF(1, "Pet2001GtkDisp::%s: n=%d\n", __func__, n);

	if (version == n)
		return;

	version = n;

	charrom = (version == 1 ? characters_2 : characters_1);
	if (version > 0)
		blank = false;
}

void
Pet2001GtkDisp::setDebug(bool _m)
{
	DPRINTF(1, "Pet2001GtkDisp::%s: _m=%d\n", __func__, _m);

	debugMode = _m;
	if (_m) {
		updateAll();
		queue_draw();
	}
}

void
Pet2001GtkDisp::setForeground(uint8_t rgb[])
{
	DPRINTF(1, "Pet2001GtkDisp::%s: %02x %02x %02x\n", __func__,
		rgb[0], rgb[1], rgb[2]);

	memcpy(fg_color, rgb, pb_n_channels);
	syncPixbuf();
}

void
Pet2001GtkDisp::setBackground(uint8_t rgb[])
{
	DPRINTF(1, "Pet2001GtkDisp::%s: %02x %02x %02x\n", __func__,
		rgb[0], rgb[1], rgb[2]);

	memcpy(bg_color, rgb, pb_n_channels);
	syncPixbuf();
}

void
Pet2001GtkDisp::reset(void)
{
	DPRINTF(1, "Pet2001GtkDisp::%s:\n", __func__);

	blank = false;
	alt_charset = false;
	video_cycle = 0;
	snowcycle = false;

	// Initialize video RAM with pattern that shows all characters.
	for (int i = 0; i < PET_VRAM_SIZE; i++)
		vidmem[i] = (i & 255);
}

// Set pixbuf to background color and zero out vidpixels.
// cycle() or updateChar() will update pixbuf with current video RAM
// contents.
void
Pet2001GtkDisp::syncPixbuf(void)
{
	DPRINTF(1, "Pet2001GtkDisp::%s:\n", __func__);

	// Initialize vidpixels
	memset(vidpixels, 0x00, PET_VIDPIXELS_SIZE);

	// Paint background color in pixbuf.
	uint8_t *p;
	for (int y = 0; y < PET_NATIVE_HEIGHT; y++) {
		p = pb_data + y * pb_stride;
		for (int x = 0; x < PET_NATIVE_WIDTH; x++) {
			memcpy(p, bg_color, pb_n_channels);
			p += pb_n_channels;
		}
	}
}

// updateChar() is only used in debug mode to update the pixbuf
// when a video RAM location has changed.  When not in debug mode,
// display is updated in cycle().
void
Pet2001GtkDisp::updateChar(int col, int row, uint8_t d8)
{
	DPRINTF(3, "Pet2001GtkDisp::%s: col=%d row=%d d8=0x%02x\n", __func__,
		col, row, d8);

	uint8_t *p;

	int charoffset = (d8 & 0x7f) * 8 + (alt_charset ? 1024 : 0);

	for (int y = 0; y < 8; y++) {
		uint8_t cdata = charrom[charoffset++];
		p = pb_data + (row * 8 + y) * pb_stride +
			col * 8 * pb_n_channels;
		if ((d8 & 0x80) != 0)
			cdata ^= 0xff;
		if (blank)
			cdata = 0x00;
		if (cdata != vidpixels[col + (row * 8 + y) * 40]) {
			vidpixels[col + (row * 8 + y) * 40] = cdata;
			for (int x = 0; x < 8; x++) {
				if ((cdata & 0x80) != 0)
					memcpy(p, fg_color, pb_n_channels);
				else
					memcpy(p, bg_color, pb_n_channels);

				p += pb_n_channels;
				cdata <<= 1;
			}
		}
	}

}

// updateAll() is only used in debug mode to update the pixbuf.
// When not in debug mode, display is updated in cycle().
void
Pet2001GtkDisp::updateAll(void)
{
	DPRINTF(2, "Pet2001GtkDisp::%s:\n", __func__);

	for (int row = 0; row < 25; row++)
		for (int col = 0; col < 40; col++)
			updateChar(col, row, vidmem[row * 40 + col]);
}

void
Pet2001GtkDisp::write(uint16_t offset, uint8_t d8)
{
	DPRINTF(3, "Pet2001GtkDisp::%s offset=0x%x d8=0x%02x:\n", __func__,
		offset, d8);

	offset &= (PET_VRAM_SIZE - 1);
	vidmem[offset] = d8;

	// If in debug mode, update display immediately.  Otherwise, we
	// wouldn't see changes to display when single-stepping.
	if (debugMode && offset < 1000 && !blank) {
		int col = offset % 40;
		int row = offset / 40;
		updateChar(col, row, d8);

		// Invalidate relevant part of display plus one pixel in
		// each direction.
		queue_draw_area(disp_left + disp_scale * (col * 8 - 1),
				disp_top + disp_scale * (row * 8 - 1),
				disp_scale * 10.0, disp_scale * 10.0);
	}

	// Does this write produce snow?
	if (version == 0 && video_cycle >= VCYCLE0 && video_cycle <
	    VCYCLEEND && !blank && ((video_cycle - VCYCLE0) & 0x3f) < 40) {
		snowcycle = true;
		snowbyte = d8;
	}
}

uint8_t
Pet2001GtkDisp::read(uint16_t offset)
{
	DPRINTF(3, "Pet2001GtkDisp::%s offset=0x%x:\n", __func__, offset);

	offset &= (PET_VRAM_SIZE - 1);
	uint8_t d8 = vidmem[offset];

	// Does this read produce snow?
	if (version == 0 && video_cycle >= VCYCLE0 && video_cycle <
	    VCYCLEEND && !blank && ((video_cycle - VCYCLE0) & 0x3f) < 40) {
		snowcycle = true;
		snowbyte = d8;
	}

	return d8;
}

void
Pet2001GtkDisp::setCharset(bool alt)
{
	DPRINTF(2, "Pet2001GtkDisp::%s: alt=%d\n", __func__, alt);

	this->alt_charset = alt;

	if (debugMode) {
		updateAll();
		queue_draw();
	}
}

void
Pet2001GtkDisp::setBlank(bool blank)
{
	DPRINTF(2, "Pet2001GtkDisp::%s: blank=%d\n", __func__, blank);

	if (version == 0) {
		this->blank = blank;
		if (debugMode) {
			updateAll();
			queue_draw();
		}
	}
}

void
Pet2001GtkDisp::cycle(void)
{
	if (video_cycle < VCYCLE0 || video_cycle >= VCYCLEEND) {
		video_cycle++;
		return;
	}

	// Which byte is being read from video RAM?
	int col = (video_cycle - VCYCLE0) & 0x3f;
	if (col > 39) {
		video_cycle++;
		return;
	}
	int row = (video_cycle - VCYCLE0) >> 6;

	DPRINTF(3, "Pet2001GtkDisp::%s: video_cycle=%d col=%d row=%d\n",
		__func__, video_cycle, col, row);

	video_cycle++;

	// Get byte from video memory.
	uint8_t vbyte = vidmem[col + (row >> 3) * 40];
	uint8_t cdata = 0;
	if (!blank) {
		if (snowcycle) {
			DPRINTF(3, "Pet2001GtkDisp::%s: snowcycle=true "
				"snowbyte=%02x\n", __func__, snowbyte);
			vbyte = snowbyte;
			snowcycle = false;
		}

		// Get 8 pixels from charrom
		int charoffset = (vbyte & 0x7f) * 8 +
			(alt_charset ? 1024 : 0) + (row & 0x07);
		cdata = charrom[charoffset];
		if ((vbyte & 0x80) != 0)
			cdata ^= 0xff;
	}

	// Update pixels?
	if (cdata != vidpixels[col + row * 40]) {
		vidpixels[col + row * 40] = cdata;

		uint8_t *p = pb_data + row * pb_stride +
			col * 8 * pb_n_channels;

		for (int x = 0; x < 8; x++) {
			if ((cdata & 0x80) != 0)
				memcpy(p, fg_color, pb_n_channels);
			else
				memcpy(p, bg_color, pb_n_channels);

			p += pb_n_channels;
			cdata <<= 1;
		}

		// Expand update rectangle.
		if (update_rect.has_zero_area())
			update_rect = Gdk::Rectangle(col * 8, row, 8, 1);
		else
			update_rect.join(Gdk::Rectangle(col * 8, row, 8, 1));
	}
}

// Called on falling edge of SYNC signal.
void
Pet2001GtkDisp::sync(void)
{
	video_cycle = 0;

	// If update rectangle non-zero...
	if (!update_rect.has_zero_area()) {
		queue_draw_area(disp_left + disp_scale *
				(update_rect.get_x() - 1),
				disp_top + disp_scale *
				(update_rect.get_y() - 1),
				disp_scale * (update_rect.get_width() + 2),
				disp_scale * (update_rect.get_height() + 2));

		update_rect = Gdk::Rectangle(0, 0, 0, 0);
	}
}
