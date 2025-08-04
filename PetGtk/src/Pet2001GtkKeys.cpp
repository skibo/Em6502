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

// Pet2001GtkKeys.cpp

#include <gtkmm.h>
#include <stdint.h>

#include "Pet2001GtkKeys.h"

#include "Pet2001.h"

#ifdef DEBUGKEYS
#  include <cstdio>
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGKEYS) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

// Fractional location of edges of keypads.
#define KEYPAD1_LEFT		0.01500
#define KEYPAD1_COLUMNS		11
#define KEYPAD2_LEFT		0.74125
#define KEYPAD2_COLUMNS		4
#define KEYPAD_TOP		0.04494
#define KEYPAD_ROWS		5
#define KEYPAD_COLUMNS		16	// including "ghost" column
#define KEY_WIDTH		0.06068
#define KEY_HEIGHT		0.18202

// Index of special keys in key_state[]
#define PET_KEY_HOME		6
#define PET_KEY_DOWN		14
#define PET_KEY_RIGHT		7
#define PET_KEY_DEL		15
#define PET_KEY_DIVIDE		31
#define PET_KEY_MULTIPLY	47
#define PET_KEY_ADD		63
#define PET_KEY_EQUAL		79
#define PET_KEY_SUBTRACT	71
#define PET_KEY_DECIMAL		78
#define PET_KEY_RETURN_U	37
#define PET_KEY_RETURN_L	53
#define PET_KEY_SHIFT_L		64
#define PET_KEY_RVS		72
#define PET_KEY_SPACE_L		74
#define PET_KEY_SPACE_R		67
#define PET_KEY_STOP		76
#define PET_KEY_SHIFT_R		69

// Map of key_state positions to key values.
static int keyascii[] = {
	'!', '#', '%', '&', '(', '~', GDK_KEY_Home,   GDK_KEY_Right,
	'\"','$', '\'','\\',')', 0,   GDK_KEY_Down, GDK_KEY_BackSpace,
	'q', 'e', 't', 'u', 'o', '|', '7', '9',
	'w', 'r', 'y', 'i', 'p', 0,   '8', '/',
	'a', 'd', 'g', 'j', 'l', 0,   '4', '6',
	's', 'f', 'h', 'k', ':', 0,   '5', '*',
	'z', 'c', 'b', 'm', ';', GDK_KEY_Return, '1', '3',
	'x', 'v', 'n', ',', '?', 0,   '2', '+',
	0,   '@', ']', 0,   '>', 0,   '0', '-',
	0,   '[', ' ', '<', 0,   0,   '.', '='
};

Pet2001GtkKeys::Pet2001GtkKeys(BaseObjectType *cobject,
			       const Glib::RefPtr<Gtk::Builder> &refBuilder,
			       Pet2001 *_pet)
	: Gtk::DrawingArea(cobject)
{
	this->pet = _pet;

	for (int i = 0; i < KEYSTATE_SIZE; i++)
		key_state[i] = false;
	for (int i = 0; i < KEYMAP_SIZE; i++)
		key_map[i] = 0;

	mouse_key = -1;
	key_shift = false;

	keypixbuf = Gdk::Pixbuf::create_from_resource(
		      "/net/skibo/petgtk/keyboard_800x267.png");
	if (!keypixbuf)
		throw std::runtime_error(
			 "Unable to open resource: keyboard_800x267.png");
}

void
Pet2001GtkKeys::connectSignals(void)
{
	DPRINTF(1, "Pet2001GtkKeys::%s\n", __func__);

	signal_button_press_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onButtonPress));
	signal_button_release_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onButtonRelease));

	signal_key_press_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onKeyEvent));
	signal_key_release_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onKeyEvent));

	signal_focus_out_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onFocusOut));
	signal_configure_event().connect(sigc::mem_fun(*this,
					&Pet2001GtkKeys::onConfigure));
	signal_draw().connect(sigc::mem_fun(*this, &Pet2001GtkKeys::onDraw));
}

bool
Pet2001GtkKeys::onConfigure(GdkEventConfigure *event)
{
	DPRINTF(1, "Pet2001GtkKeys::%s: width=%d height=%d\n", __func__,
		event->width, event->height);

	int keyimg_width = keypixbuf->get_width();
	int keyimg_height = keypixbuf->get_height();

	keys_scale = MIN((float)event->width / keyimg_width,
			 (float)event->height / keyimg_height);
	keys_width = keys_scale * keyimg_width;
	keys_height = keys_scale * keyimg_height;
	keys_left = ((float)event->width - keys_width) / 2.0;
	keys_top = ((float)event->height - keys_height) / 2.0;

	return true;
}

void
Pet2001GtkKeys::updatePetKeys(void)
{
	for (int row = 0; row < 10; row++) {
		uint8_t keyrow = 0xff;
		for (int column = 0; column < 8; column++) {
			if (key_state[row * 8 + column])
				keyrow &= ~(1 << column);
		}
		pet->setKeyrow(row, keyrow);
	}
}

void
Pet2001GtkKeys::setKeyState(int key, bool state)
{
	key_state[key] = state;

	int row = key / 16;
	int col = ((key << 1) & 0xe) + ((key >> 3) & 1);

	DPRINTF(3, "Pet2001GtkKeys::%s: key=%d col=%d row=%d state=%d\n",
		__func__, key, col, row, state);

	if (col < 11)
		queue_draw_area(keys_left + (KEYPAD1_LEFT + KEY_WIDTH * col) *
				keys_width, keys_top + (KEYPAD_TOP +
				KEY_HEIGHT * row) * keys_height,
				KEY_WIDTH * keys_width,
				KEY_HEIGHT * keys_height);
	else
		queue_draw_area(keys_left + (KEYPAD2_LEFT + KEY_WIDTH *
				(col - KEYPAD1_COLUMNS - 1)) * keys_width,
				keys_top + (KEYPAD_TOP + KEY_HEIGHT * row) *
				keys_height, KEY_WIDTH * keys_width,
				KEY_HEIGHT * keys_height);
}

bool
Pet2001GtkKeys::onButtonPress(GdkEventButton *event)
{
	DPRINTF(2, "Pet2001GtkKeys::%s: x=%g y=%g\n", __func__, event->x,
		event->y);

	// Sometimes button release events are missed.  Clean up.
	if (mouse_key >= 0) {
		DPRINTF(2, "Pet2001GtkKeys::%s: "
			"button press but mouse_key isn't -1\n", __func__);

		setKeyState(mouse_key, false);

		// Release other half of double keys.
		if (mouse_key == PET_KEY_SPACE_L)
			setKeyState(PET_KEY_SPACE_R, false);
		else if (mouse_key == PET_KEY_RETURN_L)
			setKeyState(PET_KEY_RETURN_U, false);

		mouse_key = -1;
	}

	static int colmap[] =
		{0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 6, 14, 7, 15};
	float x = ((float)event->x - keys_left) / keys_width;
	float y = ((float)event->y - keys_top) / keys_height;
	int row;
	int column;

	if (y >= KEYPAD_TOP && y < KEYPAD_TOP + KEY_HEIGHT * KEYPAD_ROWS) {
		row = (int)((y - KEYPAD_TOP) / KEY_HEIGHT);

		if (x >= KEYPAD1_LEFT && x < KEYPAD1_LEFT + KEY_WIDTH *
		    KEYPAD1_COLUMNS)
			column = (int)((x - KEYPAD1_LEFT) / KEY_WIDTH);
		else if (x >= KEYPAD2_LEFT && x < KEYPAD2_LEFT + KEY_WIDTH *
			 KEYPAD2_COLUMNS)
			column = KEYPAD1_COLUMNS +
				(int)((x - KEYPAD2_LEFT) / KEY_WIDTH);
		else
			return TRUE;

		mouse_key = row * KEYPAD_COLUMNS + colmap[column];

		DPRINTF(2, "Pet2001GtkKeys::%s: "
			"column=%d row=%d mouse_key=%d\n", __func__,
			column, row, mouse_key);

		// Fix up double-wide space key and double height return.
		if (mouse_key == PET_KEY_SPACE_R ||
		    mouse_key == PET_KEY_SPACE_L) {
			mouse_key = PET_KEY_SPACE_L;
			setKeyState(PET_KEY_SPACE_R, true);
		}
		else if (mouse_key == PET_KEY_RETURN_U ||
			mouse_key == PET_KEY_RETURN_L) {
			mouse_key = PET_KEY_RETURN_L;
			setKeyState(PET_KEY_RETURN_U, true);
		}

		setKeyState(mouse_key, true);
		updatePetKeys();
	}

	return true;
}

bool
Pet2001GtkKeys::onButtonRelease(GdkEventButton *event)
{

	DPRINTF(2, "Pet2001GtkKeys::%s: x=%g y=%g\n", __func__,
		event->x, event->y);

	// Should always be true...
	if (mouse_key >= 0) {
		setKeyState(mouse_key, false);

		// Release other half of double keys.
		if (mouse_key == PET_KEY_SPACE_L)
			setKeyState(PET_KEY_SPACE_R, false);
		else if (mouse_key == PET_KEY_RETURN_L)
			setKeyState(PET_KEY_RETURN_U, false);

		mouse_key = -1;
		updatePetKeys();
	}

	return true;
}

void
Pet2001GtkKeys::doKeyvalEvent(int keyval, bool release)
{
	int i;

	DPRINTF(3, "Pet2001GtkKeys::%s: keyval=0x%x release=%d\n", __func__,
		keyval, release);

	switch (keyval) {
	case GDK_KEY_Shift_L:
		setKeyState(PET_KEY_SHIFT_L, !release);
		key_shift = !release;
		break;
	case GDK_KEY_Shift_R:
		setKeyState(PET_KEY_SHIFT_R, !release);
		key_shift = !release;
		break;
	case GDK_KEY_Up:
		setKeyState(PET_KEY_DOWN, !release);
		setKeyState(PET_KEY_SHIFT_L, !release);
		key_shift = !release;
		break;
	case GDK_KEY_Left:
		setKeyState(PET_KEY_RIGHT, !release);
		setKeyState(PET_KEY_SHIFT_L, !release);
		key_shift = !release;
		break;
	case GDK_KEY_space:
	case GDK_KEY_KP_Space:
		// Double Key.
		setKeyState(PET_KEY_SPACE_L, !release);
		setKeyState(PET_KEY_SPACE_R, !release);
		break;
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
		// Double Key.
		setKeyState(PET_KEY_RETURN_L, !release);
		setKeyState(PET_KEY_RETURN_U, !release);
		break;
	case GDK_KEY_KP_Divide:
		setKeyState(PET_KEY_DIVIDE, !release);
		break;
	case GDK_KEY_KP_Multiply:
		setKeyState(PET_KEY_MULTIPLY, !release);
		break;
	case GDK_KEY_KP_Add:
		setKeyState(PET_KEY_ADD, !release);
		break;
	case GDK_KEY_KP_Equal:
		setKeyState(PET_KEY_EQUAL, !release);
		break;
	case GDK_KEY_KP_Subtract:
		setKeyState(PET_KEY_SUBTRACT, !release);
		break;
	case GDK_KEY_KP_Decimal:
		setKeyState(PET_KEY_DECIMAL, !release);
		break;
	case GDK_KEY_Delete:
		setKeyState(PET_KEY_DEL, !release);
		break;

	default:
		// g_assert(keyval != 0);
		if (keyval >= 'A' && keyval <= 'Z')
			keyval += 'a' - 'A';
		else if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9)
			keyval = '0' + (keyval - GDK_KEY_KP_0);
		for (i = 0; i < sizeof(keyascii) / sizeof(keyascii[0]); i++)
			if (keyval == keyascii[i]) {
				setKeyState(i, !release);

				// Drop the shift key because user probably
				// only pressed shift to get to the characters.
				if (key_shift) {
					setKeyState(PET_KEY_SHIFT_L, false);
					setKeyState(PET_KEY_SHIFT_R, false);
					key_shift = 0;
				}
				break;
			}
	}

	updatePetKeys();
}

bool
Pet2001GtkKeys::onKeyEvent(GdkEventKey *event)
{
	bool release = event->type == GDK_KEY_RELEASE;
	int keyval = event->keyval;
	uint16_t hwcode = event->hardware_keycode;

	DPRINTF(2, "Pet2001GtkKeys::%s: keyval=0x%x hwcode=0x%x release=%d\n",
		__func__, keyval, hwcode, release);

	if (release) {
		if (key_map[hwcode] != 0) {
			doKeyvalEvent(key_map[hwcode], true);
			key_map[hwcode] = 0;
		}
	} else {
		// Skip key repeats.
		if (key_map[hwcode] == keyval)
			goto done;

		// If another keyval at same hardware code is pressed,
		// release old value.
		if (key_map[hwcode] != 0)
			doKeyvalEvent(key_map[hwcode], true);
		key_map[hwcode] = keyval;
		doKeyvalEvent(keyval, false);
	}

done:
	return true;
}

bool
Pet2001GtkKeys::onFocusOut(GdkEventFocus *event)
{
	DPRINTF(1, "Pet2001GtkKeys::%s:\n", __func__);

	// Clear any keys that are pressed.
	bool anychange = false;
	for (int i = 0; i < KEYSTATE_SIZE; i++)
		if (key_state[i]) {
			setKeyState(i, false);
			anychange = true;
		}

	if (anychange) {
		for (int i = 0; i < KEYMAP_SIZE; i++)
			key_map[i] = 0;
		updatePetKeys();
	}

	return true;
}


bool
Pet2001GtkKeys::onDraw(const ::Cairo::RefPtr<::Cairo::Context> &cr)
{
#if DEBUGKEYS > 1
	double x1, y1, x2, y2;
	cr->get_clip_extents(x1, y1, x2, y2);
	DPRINTF(2, "Pet2001GtkKeys::%s: extents=(%.0f,%.0f,%.0f,%.0f)\n",
		__func__, x1, y1, x2, y2);
#endif

	cr->save();
	cr->translate(keys_left, keys_top);
	cr->scale(keys_scale, keys_scale);
	Gdk::Cairo::set_source_pixbuf(cr, keypixbuf);
	cr->paint();
	cr->restore();

	cr->set_source_rgba(0.2, 0.0, 0.8, 0.4);

	for (int row = 0; row < KEYPAD_ROWS; row++) {
		float y = keys_top + keys_height * (KEYPAD_TOP + KEY_HEIGHT *
			    row);

		for (int col = 0; col < KEYPAD1_COLUMNS; col++) {
			int key = (row << 4) + ((col << 3) & 8) +
				((col >> 1) & 7);
			if (key_state[key]) {
				float x = keys_left + keys_width *
					(KEYPAD1_LEFT + KEY_WIDTH * col);

				cr->rectangle(x, y, KEY_WIDTH * keys_width,
					      KEY_HEIGHT * keys_height);
			}
		}
		for (int col = 0; col < KEYPAD2_COLUMNS; col++) {
			int key = (row << 4) + (((col + 12) << 3) & 8) +
				(((col + 12) >> 1) & 7);
			if (key_state[key]) {
				float x = keys_left + keys_width *
					(KEYPAD2_LEFT + KEY_WIDTH * col);

				cr->rectangle(x, y, KEY_WIDTH * keys_width,
					      KEY_HEIGHT * keys_height);
			}
		}
	}
	cr->fill();

	return true;
}
