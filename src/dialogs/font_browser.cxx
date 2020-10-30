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

#include <config.h>

#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <list>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>

#include "font_browser.h"
#include "flslider2.h"
#include "gettext.h"

#include "threads.h"
#include "debug.h"

using namespace std;

void find_fonts();

Font_Browser* font_browser;

int		Font_Browser::instance = 0;
int 	*Font_Browser::fixed = 0;
int 	Font_Browser::numfonts = 0;

Font_Browser::font_pair *Font_Browser::font_pairs = 0;


static int  font_compare(const void *p1, const void *p2)
{
	std::string s1 = *((const Font_Browser::font_pair *)p1)->name;
	std::string s2 = *((const Font_Browser::font_pair *)p2)->name;
	return strcasecmp( s1.c_str(), s2.c_str() );
}

// Font Color selected

void Font_Browser::ColorSelect()
{
	unsigned char r, g, b;
	Fl::get_color(fontcolor, r, g, b);
	if (fl_color_chooser(_("Font color"), r, g, b) == 0)
		return;
	fontcolor = fl_rgb_color(r, g, b);
	btn_Color->color(fontcolor);
	btn_Color->labelcolor( fl_contrast(FL_BLACK, fontcolor));
}

void Font_Browser::fb_callback(Fl_Widget* w, void* arg)
{
	Font_Browser* fb = reinterpret_cast<Font_Browser*>(arg);

	if (w == fb->btn_fixed) {
		if (fb->btn_fixed->value())
			fb->fontFilter(FIXED_WIDTH);
		else
			fb->fontFilter(ALL_TYPES);
		return;
	}

	if (w == fb->btn_Cancel)
	fb->hide();

	else if (w == fb->btn_OK) {
		if (fb->callback_)
			(*fb->callback_)(fb, fb->data_);
	}

	else if (w == fb->btn_Color)
	fb->ColorSelect();

	else if (w == fb->lst_Font)
	fb->FontNameSelect();

	else {
		if (w == fb->lst_Size)
			fb->txt_Size->value(strtol(fb->lst_Size->text(fb->lst_Size->value()), NULL, 10));
		fb->fontsize = static_cast<int>(fb->txt_Size->value());
	}
	fb->box_Example->SetFont(fb->fontnbr, fb->fontsize, fb->fontcolor);
}

// Font Name changed callback
void Font_Browser::FontNameSelect()
{
	int fn = lst_Font->value();
	if (!fn)
		return;

	fontnbr = (Fl_Font)reinterpret_cast<intptr_t>(lst_Font->data(fn));

	// get sizes and fill browser; skip first element if it is zero
	lst_Size->clear();
	int nsizes, *sizes;
	char buf[4];
	nsizes = Fl::get_font_sizes(fontnbr, sizes);
	//
	for (int i = !*sizes; i < nsizes; i++)
	if ((size_t)snprintf(buf, sizeof(buf), "%d", sizes[i]) < sizeof(buf))
		lst_Size->add(buf, reinterpret_cast<void*>(sizes[i]));

	// scalable font with no suggested sizes
	if (!lst_Size->size()) {
	for (int i = 1; i <= 48; i++) {
		snprintf(buf, sizeof(buf), "%d", i);
		lst_Size->add(buf, reinterpret_cast<void*>(i));
	}
	}
	fontSize(fontsize);
}

Font_Browser::Font_Browser(int x, int y, int w, int h, const char *lbl )
	 : Fl_Window(x, y, w, h, lbl)
{
	lst_Font = new Fl_Browser(5, 15, 280, 125, _("Font:"));
	lst_Font->align(FL_ALIGN_TOP_LEFT);
	lst_Font->type(FL_HOLD_BROWSER);
	lst_Font->callback(fb_callback, this);

	txt_Size = new Fl_Value_Input2(290, 15, 50, 22, _("Size:"));
	txt_Size->align(FL_ALIGN_TOP_LEFT);
	txt_Size->range(1.0, 48.0);
	txt_Size->step(1.0);
	txt_Size->callback(fb_callback, this);

	lst_Size = new Fl_Browser(290, 40, 50, 100);
	lst_Size->type(FL_HOLD_BROWSER);
	lst_Size->callback(fb_callback, this);

	btn_fixed = new Fl_Check_Button(345, 15, 18, 18, _("Fixed"));
	btn_fixed->callback(fb_callback, this);
	btn_fixed->value(0);

	btn_OK = new Fl_Return_Button(345, 40, 80, 25, _("&OK"));
	btn_OK->shortcut(0x8006f);
	btn_OK->callback(fb_callback, this);

	btn_Cancel = new Fl_Button(345, 70, 80, 25, _("Cancel"));
	btn_Cancel->labelsize(12);
	btn_Cancel->callback(fb_callback, this);

	btn_Color = new Fl_Button(345, 100, 80, 25, _("Color"));
	btn_Color->down_box(FL_BORDER_BOX);
	btn_Color->color(FL_FOREGROUND_COLOR);
	btn_Color->labelcolor( fl_contrast(FL_BLACK, FL_FOREGROUND_COLOR));
	btn_Color->callback(fb_callback, this);

	box_Example = new Preview_Box(5, 145, 420, 75,
	_("\
abcdefghijklmnopqrstuvwxyz\n\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\n\
0123456789"
	) );
	box_Example->box(FL_DOWN_BOX);
	box_Example->align(FL_ALIGN_WRAP|FL_ALIGN_CLIP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
	resizable(box_Example);

	set_modal();
	end();

// Initializations

	this->callback_ = 0;  // Initialize Widgets callback
	this->data_ = 0;      // And the data

	std::string fntname;
	bool ok = true;

	if (instance == 0) {

		++instance;

		numfonts =   Fl::set_fonts(0); // Nr of fonts available on the server
		font_pairs = new font_pair[numfonts];
		fixed = new int[numfonts];

		int j = 0;
		for (int i = 0; i < numfonts; i++) {
			fntname = Fl::get_font_name((Fl_Font)i);
			ok = true;
			for (size_t k = 0; k < fntname.length(); k++) {
				if (fntname[k] < ' '   || fntname[k] > 'z' ||
					fntname[k] == '\\' || fntname[k] == '@') { // disallowed chars in browser widget
					ok = false;
					break;
				}
			}
			if (fntname.empty()) ok = false;
			if (ok) {
				font_pairs[j].name = new std::string;
				*(font_pairs[j].name) = fntname;
				font_pairs[j].nbr = i;
				j++;
			}
		}

		numfonts = j;

		qsort(&font_pairs[0], numfonts, sizeof(font_pair), font_compare);

		for (int i = 0; i < numfonts; i++) {
			lst_Font->add((*(font_pairs[i].name)).c_str(), reinterpret_cast<void *>(font_pairs[i].nbr));
			fixed[i] = 0;
		}
		find_fonts();
	} else {
		++instance;
		for (int i = 0; i < numfonts; i++) {
			lst_Font->add((*(font_pairs[i].name)).c_str(), reinterpret_cast<void *>(font_pairs[i].nbr));
		}
	}

	fontnbr = FL_HELVETICA;;
	fontsize = FL_NORMAL_SIZE;
	fontcolor = FL_FOREGROUND_COLOR;
	filter = ALL_TYPES;

	lst_Font->value(1);
	FontNameSelect();

	xclass(PACKAGE_NAME);
}

Font_Browser::~Font_Browser()
{
	--instance;
	if (instance == 0) {
		delete [] fixed;
		fixed = 0;
		delete [] font_pairs;
		font_pairs = 0;
	}
}

void Font_Browser::fontNumber(Fl_Font n)
{
	fontnbr = n;
	lst_Font->value(1);
	int s = lst_Font->size();
	for (int i = 1; i < s; i++ ) {
		if ((Fl_Font)reinterpret_cast<intptr_t>(lst_Font->data(i)) == n) {
			lst_Font->value(i);
			FontNameSelect();
			break;
		}
	}
}

void Font_Browser::fontSize(int s)
{
	fontsize = s;
	int n = lst_Size->size();
	for (int i = 1; i < n; i++) {
	if ((intptr_t)lst_Size->data(i) == fontsize) {
		lst_Size->value(i);
		break;
	}
	}
	txt_Size->value(s);
}

void Font_Browser::fontColor(Fl_Color c)
{
	btn_Color->color(fontcolor = c);
	box_Example->SetFont(fontnbr, fontsize, fontcolor);
	box_Example->redraw();
}

void Font_Browser::fontName(const char* n)
{
	int s = lst_Font->size();
	for (int i = 1; i < s; i++) {
		if (!strcmp(lst_Font->text(i), n)) {
			lst_Font->value(i);
			FontNameSelect();
		}
	}
}

void Font_Browser::fontFilter(filter_t filter)
{
	int s = lst_Font->size();

	switch (filter) {
	case FIXED_WIDTH:
		for (int i = 0; i < s; i++) {
			if (fixed[i])
				lst_Font->show(i + 1);
			else
				lst_Font->hide(i + 1);
		}
		btn_fixed->value(1);
		break;
	case VARIABLE_WIDTH:
		for (int i = 0; i < s; i++) {
			if (!fixed[i])
				lst_Font->show(i + 1);
			else
				lst_Font->hide(i + 1);
		}
		btn_fixed->value(0);
		break;
	case ALL_TYPES:
		for (int i = 0; i < s; i++)
			lst_Font->show(i + 1);
		btn_fixed->value(0);
		break;
	}
	lst_Font->redraw();
	lst_Font->topline(lst_Font->value());
}

// separate thread used to evaluate fixed / proportional fonts
// friend of Font_Browser
// launched by Font_Browser instance 1

static pthread_t find_font_thread;

//#define FBDEBUG 1

static int is_fixed;
Fl_Font test_font;
void font_test(void *)
{
	fl_font(test_font, FL_NORMAL_SIZE);
	is_fixed = (fl_width(".") == fl_width("W"));
}

void *find_fixed_fonts(void *)
{
#ifdef FBDEBUG
	FILE *sys_fonts = fopen("fonts.txt", "w");
	fprintf(sys_fonts, "Font #, Type, Name\n");
#endif
	for (int i = 0; i < Font_Browser::numfonts; i++) {
		test_font = Font_Browser::font_pairs[i].nbr;
		is_fixed = -1;
		Fl::awake(font_test);
		while (is_fixed == -1) MilliSleep(10);
		Font_Browser::fixed[i] = is_fixed;
#ifdef FBDEBUG
		fprintf(
			sys_fonts,
			"%d, %c, %s\n",
			Font_Browser::font_pairs[i].nbr,
			Font_Browser::fixed[i] ? 'F' : 'P',
			(*(Font_Browser::font_pairs[i].name)).c_str());
#endif
	}
#ifdef FBDEBUG
		fclose(sys_fonts);
#endif
	return NULL;
}

void find_fonts()
{
	if (pthread_create(&find_font_thread, NULL, find_fixed_fonts, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create find_fixed_fonts failed");
	}
	return;
}

bool Font_Browser::fixed_width(Fl_Font f)
{
	fl_font(f, FL_NORMAL_SIZE);
	return fl_width(".") == fl_width("W");
}

//----------------------------------------------------------------------

Preview_Box::Preview_Box(int x, int y, int w, int h, const char* l)
  : Fl_Widget(x, y, w, h, l)
{
	fontName = 1;
	fontSize = FL_NORMAL_SIZE;
	box(FL_DOWN_BOX);
	color(FL_BACKGROUND2_COLOR);
	fontColor = FL_FOREGROUND_COLOR;
}

void Preview_Box::draw()
{
	draw_box();
	fl_font((Fl_Font)fontName, fontSize);
	fl_color(fontColor);
	fl_draw(label(), x()+3, y()+3, w()-6, h()-6, align());
}

void Preview_Box::SetFont(int fontname, int fontsize, Fl_Color c)
{
	fontName = fontname;
	fontSize = fontsize;
	fontColor = c;
	redraw();
}
