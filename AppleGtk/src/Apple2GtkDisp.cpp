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

// Apple2GtkDisp.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Apple2GtkDisp.h"

#ifdef DEBUGVID
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGVID) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

extern uint8_t apple2CharRom[];

#define APPLE_NATIVE_WIDTH	280
#define APPLE_NATIVE_HEIGHT	192

#define TEXT0_ADDR	0x400
#define TEXT1_ADDR	0x800
#define TEXT_SIZE	0x400
#define HIRES0_ADDR	0x2000
#define HIRES1_ADDR	0x4000
#define HIRES_SIZE	0x2000

// RGB colors for LORES and HIRES graphics.  Posted on comp.emulators.apple2,
// Thanks, Robert Munafo, https://mrob.com/pub/xapple2/colors.html
//
static const uint8_t loresrgb[][3] =
	{{0, 0, 0},     {227, 30, 96},   {96, 78, 189},   {255, 68, 253},
	 {0, 163, 96},  {156, 156, 156}, {20, 207, 253},  {208, 195, 255},
	 {96, 114, 3},  {255, 106, 60},  {156, 156, 156}, {255, 160, 208},
	 {20, 245, 60}, {208, 221, 141}, {114, 255, 208}, {255, 255, 255}};
static const uint8_t hiresrgb[][3] =
	{{0, 0, 0},     {20, 245, 60},   {255, 68, 253},  {255, 255, 255},
	 {0, 0, 0},     {255, 106, 60},  {20, 207, 253},  {255, 255, 255}};

static const uint8_t rgbgrey[][3] =
	{{0, 0, 0}, {85, 85, 85}, {171, 171, 171}, {255, 255, 255}};

Apple2GtkDisp::Apple2GtkDisp(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder)
	: Gtk::DrawingArea(cobject)
{
	DPRINTF(1, "Apple2GtkDisp::constructor:\n");

	pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB,
				     false, 8, APPLE_NATIVE_WIDTH,
				     APPLE_NATIVE_HEIGHT);
	pb_data = pixbuf->get_pixels();
	pb_stride = pixbuf->get_rowstride();
	pb_n_channels = pixbuf->get_n_channels();

	DPRINTF(1, "Apple2GtkDisp::constructor: stride=%d n_channels=%d\n",
		pb_stride, pb_n_channels);

	disp_scale = 0.0;
	flashing = false;

	reset();
}

void
Apple2GtkDisp::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Apple2GtkDisp::%s:\n", __func__);

	signal_configure_event().connect(sigc::mem_fun(*this,
					       &Apple2GtkDisp::onConfigure));
	signal_draw().connect(sigc::mem_fun(*this, &Apple2GtkDisp::onDraw));
}

// Update pixbuf for one byte of character memory at col, row.
void
Apple2GtkDisp::updateChar(int col, int row, uint8_t d8)
{
	DPRINTF(3, "Apple2GtkDisp::%s: col=%d row=%d d8=0x%02x\n", __func__,
		col, row, d8);

	uint8_t *p;

	int charoffset = 8 * ((d8 & 0x3f) ^ 0x20);

	for (int y = 0; y < 8; y++) {
		uint8_t cdata = apple2CharRom[charoffset++];
		p = pb_data + (row * 8 + y) * pb_stride +
			col * 7 * pb_n_channels;
		if ((d8 & 0xc0) == 0 || ((d8 & 0xc0) == 0x40 && flash_on))
			cdata ^= 0xff;
		if (cdata == 0x00)
			memset(p, 0x00, 8 * pb_n_channels);
		else
			for (int x = 0; x < 7; x++) {
				if ((cdata & 1) != 0)
					memset(p, 0xff, pb_n_channels);
				else
					memset(p, 0x00, pb_n_channels);

				p += pb_n_channels;
				cdata >>= 1;
			}
	}
}

// Update pixbuf for one byte of lores memory (two blocks stacked vertically).
void
Apple2GtkDisp::updateLores(int col, int row, uint8_t d8)
{
	DPRINTF(3, "Apple2GtkDisp::%s: col=%d row=%d d8=0x%02x\n", __func__,
		col, row, d8);

	for (int y = 0; y < 8; y++) {
		uint8_t *p = pb_data + (row * 8 + y) * pb_stride +
			col * 7 * pb_n_channels;
		const uint8_t *rgb;
		if (color) {
			rgb = loresrgb[y < 4 ? (d8 & 0xf) : (d8 >> 4)];

			for (int x = 0; x < 7; x++)
				for (int i = 0; i < pb_n_channels; i++)
					*p++ = rgb[i];
		} else {
			uint8_t bits = y < 4 ? (d8 & 0xf) : (d8 >> 4);
			for (int x = 0; x < 7; x++) {
				rgb = rgbgrey[((x + col) & 1) ?
					      (bits & 3) : (bits >> 2)];
				for (int i = 0; i < pb_n_channels; i++)
					*p++ = rgb[i];
			}
		}
	}
}

// updateHiresPixel is used by updateHires to determine the color of a hires
// pixel based upon the state of the adjacent pixels and modify pixbuf.
// The pixel in question is bit 1 of pixels and the adjacent pixels are bits
// 2 and 0.  So, if bit 1 is set and either of the adjacent pixels is set,
// the color is white.  If bit 1 is set but no adjacent pixels are set,
// the pixel is a color depending upon if the pixel is at and odd or even
// location and if bit 7 in the pixel's byte (d8) is set.  If both adjacent
// pixels are set but this one is not set, the color of adjacent pixels is
// "bled" into this pixel.  Otherwise, the pixel is black.
//
static void
updateHiresPixel(uint8_t *p, bool odd, uint16_t pixels, uint8_t d8)
{
	const uint8_t *rgb;

	switch (pixels & 7) {
	case 2:	// 010: Color.
		rgb = hiresrgb[1 + (odd ? 0 : 1) + ((d8 & 0x80) ? 4 : 0)];
		break;
	case 5:	// 101: Other color.
		rgb = hiresrgb[1 + (odd ? 1 : 0) + ((d8 & 0x80) ? 4 : 0)];
		break;
	case 3:
	case 6:
	case 7: // 011, 110, 111: White.
		rgb = hiresrgb[3];
		break;
	default:// Black.
		rgb = hiresrgb[0];
		break;
	}
	for (int i = 0; i < 3; i++)
		*p++ = rgb[i];
}

//
// updateHires: update all pixels possiblly affected by a change in a hires
// memory byte, d8.  Single pixels to the left and right of the 7
// pixels represented by this byte are also affected so we need the
// value of the bytes to the left (d8l) and right (d8r) of the
// updating byte.
void
Apple2GtkDisp::updateHires(int col, int y, uint8_t d8l, uint8_t d8,
			   uint8_t d8r)
{
	DPRINTF(4, "Apple2GtkDisp::%s: col=%d y=%d d8l/d8/d8r="
		"0x%x,0x%x,0x%x\n", __func__, col, y, d8l, d8, d8r);

	uint8_t *p = pb_data + y * pb_stride + col * 7 * pb_n_channels;
	bool odd = (col & 1) != 0;
	if (color) {
		// Concat pixels of left, center, and right bytes.
		uint16_t pixels =
			((d8l >> 5) & 0x03) |
			(((uint16_t)d8 << 2) & 0x1fc) |
			(((uint16_t)d8r << 9) & 0x600);

		// Update pixel just to the left of updated byte.
		if (col != 0)
			updateHiresPixel(p - pb_n_channels, !odd,
					 pixels, d8l);
		pixels >>= 1;

		// Update seven pixels of updated byte.
		for (int x = 0; x < 7; x++) {
			updateHiresPixel(p, odd, pixels, d8);
			p += pb_n_channels;
			pixels >>= 1;
			odd = !odd;
		}

		// Update pixel just to the right of updated byte.
		if (col < 39)
			updateHiresPixel(p, odd, pixels, d8r);
	} else {
		// Monochrome.
		for (int x = 0; x < 7; x++) {
			if ((d8 & 1) != 0)
				memset(p, 0xff, pb_n_channels);
			else
				memset(p, 0x00, pb_n_channels);

			p += pb_n_channels;
			d8 >>= 1;
		}
	}
}

// Update entire pixbuf in response to changes in graphics modes.
void
Apple2GtkDisp::updateAll(void)
{
	int row;
	int col;

	DPRINTF(2, "Apple2GtkDisp::%s:\n", __func__);

	// Update Text
	if (mix_en || !gfx_en)
		for (row = (mix_en && gfx_en) ? 20 : 0; row < 24; row++) {
			int offset =  ((row & 0x07) << 7) | (row & 0x18) |
				((row & 0x18) << 2);
			offset += page_en ? TEXT1_ADDR : TEXT0_ADDR;
			for (col = 0; col < 40; col++, offset++)
				updateChar(col, row, vidmem[offset]);
		}

	// Update Lores
	if (gfx_en && !hires_en)
		for (row = 0; row < (mix_en ? 20 :24); row++) {
			int offset =  ((row & 0x07) << 7) | (row & 0x18) |
				((row & 0x18) << 2);
			offset += page_en ? TEXT1_ADDR : TEXT0_ADDR;
			for (col = 0; col < 40; col++, offset++)
				updateLores(col, row, vidmem[offset]);
		}

	// Update Hires
	if (gfx_en && hires_en)
		for (int y = 0; y < (mix_en ? 160 : 192); y++) {
			row = y >> 3;
			int offset =  ((row & 0x07) << 7) | (row & 0x18) |
				((row & 0x18) << 2);
			offset += page_en ? HIRES1_ADDR : HIRES0_ADDR;
			offset += (y & 7) << 10;
			for (col = 0; col < 40; col++, offset++)
				updateHires(col, y,
				    col == 0 ? 0 : vidmem[offset - 1],
				    vidmem[offset],
				    col == 39 ? 0 : vidmem[offset + 1]);
		}
}

// Called when window resizes.
bool
Apple2GtkDisp::onConfigure(GdkEventConfigure *event)
{
	int width = get_allocated_width();
	int height = get_allocated_height();

	disp_scale = MIN((float)width / APPLE_NATIVE_WIDTH,
		    (float)height / APPLE_NATIVE_HEIGHT);

	DPRINTF(1, "Apple2GtkDisp::%s: width=%d height=%d scale=%f\n",
		__func__, width, height, disp_scale);

	disp_left = ((float)width - disp_scale * APPLE_NATIVE_WIDTH) / 2.0;
	disp_top = ((float)height - disp_scale * APPLE_NATIVE_HEIGHT) / 2.0;

	// Initialize surface.
	updateAll();

	return true;
}

bool
Apple2GtkDisp::onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr)
{
#if DEBUGVID > 1
	double x1, y1, x2, y2;
	cr->get_clip_extents(x1, y1, x2, y2);
	DPRINTF(2, "Apple2GtkDisp::%s: rect=(%.0f,%.0f,%.0f,%.0f)\n",
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
Apple2GtkDisp::reset(void)
{
	DPRINTF(1, "Apple2GtkDisp::%s:\n", __func__);

	gfx_en = false;
	hires_en = false;
	mix_en = false;
	page_en = false;
	color = true;
	flash_on = true;
}

// Never called but MemSpace interface requires this.
uint8_t
Apple2GtkDisp::read(uint16_t addr)
{
	DPRINTF(4, "Apple2GtkDisp::%s addr=0x%x:\n", __func__, addr);

	if (addr >= APPLE_VIDMEM_SIZE)
		return 0xaa;
	else
		return vidmem[addr];
}

// Called when any memory location that might be displayed is modified.
// Routine tries to determine as quickly as possible that write has no effect
// on display.
void
Apple2GtkDisp::write(uint16_t addr, uint8_t d8)
{
	DPRINTF(4, "Apple2GtkDisp::%s addr=0x%x d8=0x%02x:\n", __func__,
		addr, d8);

	if (addr >= APPLE_VIDMEM_SIZE || vidmem[addr] == d8)
		return;

	vidmem[addr] = d8;

	if (addr >= TEXT0_ADDR && addr < TEXT0_ADDR + 2 * TEXT_SIZE) {
		// TEXT or LORES graphics memory.
		if ((addr < TEXT1_ADDR && page_en) ||
		    (addr >= TEXT1_ADDR && !page_en))
			return;
		if (gfx_en && hires_en && !mix_en)
			return;

		int col = (addr & 0x07f);
		int row = (addr & 0x380) >> 7;

		if (col < 40)
			;
		else if (col < 80) {
			row += 8;
			col -= 40;
		} else if (col < 120) {
			row += 16;
			col -= 80;
		} else
			return;

		if (gfx_en && hires_en && row < 20)
			return; // hidden by hires
		else if (gfx_en && (!mix_en || row < 20)) {
			// LORES graphics
			updateLores(col, row, d8);
		} else {
			// TEXT mode
			updateChar(col, row, d8);
		}

		queue_draw_area(disp_left + disp_scale * (col * 7 - 1),
				disp_top + disp_scale * (row * 8 - 1),
				disp_scale * 10.0, disp_scale * 10.0);

	} else if (addr >= HIRES0_ADDR && addr < HIRES0_ADDR +
		   2 * HIRES_SIZE) {
		// HIRES memory
		if (!hires_en || !gfx_en)
			return;
		if ((addr < HIRES1_ADDR && page_en) ||
		    (addr >= HIRES1_ADDR && !page_en))
			return;

		int col = (addr & 0x07f);
		int y = ((addr & 0x380) >> 4) | ((addr & 0x1c00) >> 10);

		if (col < 40)
			;
		else if (col < 80) {
			y += 0x40;
			col -= 40;
		} else if (col < 120) {
			y += 0x80;
			col -= 80;
		} else
			return;

		// Hidden by text in mix-mode?
		if (y >= 160 && mix_en)
			return;

		updateHires(col, y, col == 0 ? 0 : vidmem[addr - 1],
			    vidmem[addr],
			    col == 39 ? 0 : vidmem[addr + 1]);

		queue_draw_area(disp_left + disp_scale * (col * 7 - 3),
				disp_top + disp_scale * (y - 2),
				disp_scale * 13.0, disp_scale * 4.0);
	}
}

void
Apple2GtkDisp::setGfx(bool gfx)
{
	DPRINTF(2, "Apple2GtkDisp::%s: gfx=%d\n", __func__, gfx);

	if (gfx != gfx_en) {
		gfx_en = gfx;
		updateAll();
		queue_draw();
	}
}

void
Apple2GtkDisp::setHires(bool hires)
{
	DPRINTF(2, "Apple2GtkDisp::%s: hires=%d\n", __func__, hires);

	if (hires != hires_en) {
		hires_en = hires;
		updateAll();
		queue_draw();
	}
}

void
Apple2GtkDisp::setMix(bool mix)
{
	DPRINTF(2, "Apple2GtkDisp::%s: mix=%d\n", __func__, mix);

	if (mix != mix_en) {
		mix_en = mix;
		updateAll();
		queue_draw();
	}
}

void
Apple2GtkDisp::setPage(bool page)
{
	DPRINTF(2, "Apple2GtkDisp::%s: page=%d\n", __func__, page);

	if (page != page_en) {
		page_en = page;
		updateAll();
		queue_draw();
	}
}

// Timeout called roughly every 200ms to flash any flashing characters.
bool
Apple2GtkDisp::onFlashTimeout(void)
{
	DPRINTF(3, "Apple2GtkDisp::%s: flash_on=%d\n", __func__, flash_on);

	flash_on = !flash_on;

	// Flashing has been disabled.  Don't schedule another timeout.
	if (!flashing)
		return false;

	// Abort if display not configured.
	if (disp_scale == 0.0)
		return true;

	// Full graphics mode, nothing to do.
	if (gfx_en && !mix_en)
		return true;

	// Find all character locations with flash bit set and update.
	for (int row = gfx_en ? 20 : 0; row < 24; row++) {
		int offset = (page_en ? TEXT1_ADDR : TEXT0_ADDR) |
			(((row & 0x07) << 7) | (row & 0x18) |
			 ((row & 0x18) << 2));
		for (int col = 0; col < 40; col++, offset++)
			if ((vidmem[offset] & 0xc0) == 0x40) {
				updateChar(col, row, vidmem[offset]);
				queue_draw_area(disp_left + disp_scale *
				    (col * 7 - 1), disp_top + disp_scale *
				    (row * 8 - 1), disp_scale * 10.0,
				    disp_scale * 10.0);
			}
	}

	return true;
}

// Enable or disable flashing (for pause mode).
void
Apple2GtkDisp::setFlashing(bool flag)
{
	DPRINTF(1, "Apple2GtkDisp::%s: flag=%d\n", __func__, flag);

	if (flag == flashing)
		return;

	if (flag) {
		Glib::signal_timeout().connect(sigc::mem_fun(*this,
				&Apple2GtkDisp::onFlashTimeout), 200);
	}

	flashing = flag;
}

void
Apple2GtkDisp::setColor(bool flag)
{
	DPRINTF(1, "Apple2GtkDisp::%s: flag=%d\n", __func__, flag);

	if (color != flag) {
		color = flag;

		updateAll();
		queue_draw();
	}
}
