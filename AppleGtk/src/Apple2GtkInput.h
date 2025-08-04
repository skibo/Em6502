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

#ifndef __APPLE2GTKINPUT_H__
#define __APPLE2GTKINPUT_H__

class Apple2;

class Apple2GtkInput : public Gtk::Image {
private:
	Apple2 *apple;
	Glib::RefPtr<Gtk::Adjustment> paddle[2];
	bool	onKeyPressEvent(GdkEventKey *event);
	bool	onKeyReleaseEvent(GdkEventKey *event);
	void	onPaddleChange(Glib::RefPtr<Gtk::Adjustment> &adj, int paddle);
public:
	Apple2GtkInput(BaseObjectType *cobject,
		       const Glib::RefPtr<Gtk::Builder> &refBuilder,
		       Apple2 *_apple);
	void	connectSignals(Glib::RefPtr <Gtk::Builder> builder);
};

#endif // __APPLE2GTKINPUT_H__
