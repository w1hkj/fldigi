// ----------------------------------------------------------------------------
// flmisc.h
//
// Copyright (C) 2009-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef fl_misc_h_
#define fl_misc_h_

#include <config.h>

#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Return_Button.H>
#include "flinput2.h"

unsigned quick_choice_menu(const char* title, unsigned sel, const Fl_Menu_Item* menu);
unsigned quick_choice(const char* title, unsigned sel, ...);

Fl_Color adjust_color(Fl_Color fg, Fl_Color bg);
void adjust_color_inv(unsigned char& bg1r, unsigned char& bg1g, unsigned char& bg1b,
		      Fl_Color bg2, Fl_Color def);

#if !defined(__APPLE__) && !defined(__WOE32__)
#  include <FL/x.H>
void make_pixmap(Pixmap *xpm, const char **data, int argc, char** argv);
#endif

class notify_dialog : public Fl_Window
{
	Fl_Box icon;
	Fl_Input2 message;
	Fl_Dial dial;
	Fl_Return_Button button;
	Fl_Box resize_box;
	bool delete_on_hide;

public:
	notify_dialog(int X = 410, int Y = 103, const char* l = 0);
	~notify_dialog();
	int handle(int event);
	Fl_Button* make_button(int W, int H = 23);
	void notify(const char* msg, double timeout, bool delete_on_hide_ = false);
private:
	static void button_cb(Fl_Widget* w, void*);
	static void dial_timer(void* arg);
	int newx;
};


#ifdef BUILD_FLDIGI

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
// Avoid 'nitems' macro collision between FreeBSD's sys/params.h and fltk's
// FL/Fl_Check_Browser.H (http://www.fltk.org/str.php?L2984)
#undef nitems
#include <FL/Fl_Check_Browser.H>
#define FLTK_nitems nitems
#undef nitems
// ^^^ Avoid 'nitems' macro collision
#include "globals.h"

class Mode_Browser : public Fl_Double_Window
{
public:
	Mode_Browser(void);
	~Mode_Browser(void);

	void show(mode_set_t* b);
	void callback(Fl_Callback* cb, void* args = 0);
private:
	Fl_Button *close_button, *all_button, *none_button;
	Fl_Check_Browser* modes;
	mode_set_t* store;
	Fl_Callback* changed_cb;
	void* changed_args;

	static void modes_cb(Fl_Widget* w, void* arg);
	static void button_cb(Fl_Widget* w, void* arg);
};

#endif // BUILD_FLDIGI

#endif // fl_misc_h_
