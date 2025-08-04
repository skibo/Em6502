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

#ifndef __PET2001GTKKEYS_H__
#define __PET2001GTKKEYS_H__

// Pet2001GtkKeys.h

#define KEYSTATE_SIZE	80
#define KEYMAP_SIZE	65536

class Pet2001;

class Pet2001GtkKeys : public Gtk::DrawingArea {
private:
	Pet2001	*pet;
	int	mouse_key;
	bool	key_state[KEYSTATE_SIZE];
	bool	key_shift;
	int	key_map[KEYMAP_SIZE];
	Glib::RefPtr<Gdk::Pixbuf> keypixbuf;
	float	keys_scale;
	float	keys_width;
	float	keys_height;
	float	keys_left;
	float	keys_top;

	void	setKeyState(int key, bool state);
	void	updatePetKeys(void);
	void	doKeyvalEvent(int keyval, bool release);
	bool	onButtonPress(GdkEventButton *event);
	bool	onButtonRelease(GdkEventButton *event);
	bool	onKeyEvent(GdkEventKey *event);
	bool	onFocusOut(GdkEventFocus *event);
	bool	onConfigure(GdkEventConfigure *event);
	bool	onDraw(const ::Cairo::RefPtr<::Cairo::Context>&cr);
public:
	Pet2001GtkKeys(BaseObjectType *cobject,
		       const Glib::RefPtr<Gtk::Builder> &refBuilder,
		       Pet2001 *_pet);
	void	connectSignals(void);
};

#endif // __PET2001GTKKEYS_H__
