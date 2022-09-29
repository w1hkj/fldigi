// ----------------------------------------------------------------------------
//  Font_Browser.h      v 0.0.1                        2005-10-17
//
//         for the Fast Light Tool Kit (FLTK) 1.1.x .
//
//    David Freese, w1hkj@w1hkj.com
//    based on similar widget by Mariwan Jalal
//
// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef FONTBROWSER_H
#define FONTBROWSER_H

#include <string>

#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>
#include "flslider2.h"

// Preview box for showing font
class Preview_Box : public Fl_Widget
{
private:
	int		fontName;
	int		fontSize;
	Fl_Color	fontColor;

	void	draw();
public:
	Preview_Box(int x, int y, int w, int h, const char* l);
	void SetFont( int fontname, int fontsize, Fl_Color c);
};

// Font browser widget
class Font_Browser : public Fl_Window
{
friend void *find_fixed_fonts(void *);

public:
	struct font_pair {
		int  nbr;
		std::string *name;
		font_pair() {
			nbr = 0;
			name = 0;
		}
		~font_pair() {
			if (name) delete name;
		}
	};

	enum filter_t { FIXED_WIDTH, VARIABLE_WIDTH, ALL_TYPES };

// these are shared by all instances of Font_Browser
// created for instance 1 and deleted for instance 0

	static int			*fixed;
	static font_pair	*font_pairs;
	static int			instance;
	static int			numfonts;

private:

	Fl_Font	fontnbr;
	int		fontsize;
	Fl_Color	fontcolor;
	filter_t	filter;
	void	*data_;

	Fl_Browser	*lst_Font;
	Fl_Browser	*lst_Size;
	Fl_Value_Input2 *txt_Size;
	Fl_Return_Button *btn_OK;
	Fl_Button	*btn_Cancel;
	Fl_Button	*btn_Color;
	Fl_Check_Button	*btn_fixed;
	Preview_Box	*box_Example;

	Fl_Callback* callback_;

public:
	Font_Browser(int x = 100, int y = 100, int w = 430, int h = 225, const char *lbl = "Font Browser");
	~Font_Browser();

	void callback(Fl_Callback* cb, void *d = 0) { callback_ = cb; data_ = d; }
	static void fb_callback(Fl_Widget* w, void* arg);
	void	FontNameSelect();
	void	ColorSelect();

	int numFonts() { return numfonts; }
	void fontNumber(Fl_Font n);
	Fl_Font fontNumber() { return fontnbr; }
	void fontSize(int s);
	int fontSize() { return fontsize; }
	void fontColor(Fl_Color c);
	Fl_Color fontColor() { return fontcolor; };

	const char *fontName() { return lst_Font->text(lst_Font->value()); }
	void fontName(const char* n);

static	bool fixed_width(Fl_Font f);

	void fontFilter(filter_t filter);
};

extern Font_Browser* font_browser;

#endif
