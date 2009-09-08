//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Font_Browser.cpp      v 0.0.3                        2007-04-21 
//
//         for the Fast Light Tool Kit (FLTK) 1.1.x .
//
//    David Freese, w1hkj@w1hkj.com
//    based on similar widget by Mariwan Jalal
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk_kurdi@yahoo.com".
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#include <config.h>

#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdint.h>

#include <FL/Fl.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>

#include "font_browser.h"
#include "gettext.h"

using namespace std;

// Font Color selected

void Font_Browser::ColorSelect()
{
    unsigned char r, g, b;
    Fl::get_color(fontcolor, r, g, b);
    if (fl_color_chooser(_("Font color"), r, g, b) == 0)
	    return;
    fontcolor = fl_rgb_color(r, g, b);
    btn_Color->color(fontcolor);
}

void Font_Browser::fb_callback(Fl_Widget* w, void* arg)
{
    Font_Browser* fb = reinterpret_cast<Font_Browser*>(arg);

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
	fb->fontsize = fb->txt_Size->value();
    }
    fb->box_Example->SetFont(fb->fontnbr, fb->fontsize, fb->fontcolor);
}

// Sort the font list
void Font_Browser::FontSort()
{
    int size = lst_Font->size();
     for ( int t = 1; t <= size - 1; t++ )
         for ( int r = t+1; r <= size; r++ )
             if ( strcasecmp(lst_Font->text(t), lst_Font->text(r)) > 0 ) 
                lst_Font->swap(t,r);
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
	    lst_Size->add(buf, (void*)sizes[i]);

    // scalable font with no suggested sizes
    if (!lst_Size->size()) {
	for (int i = 1; i <= 48; i++) {
	    snprintf(buf, sizeof(buf), "%d", i);
	    lst_Size->add(buf, (void*)i);
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

    txt_Size = new Fl_Value_Input(290, 15, 50, 22, _("Size:"));
    txt_Size->align(FL_ALIGN_TOP_LEFT);
    txt_Size->range(1.0, 48.0);
    txt_Size->step(1.0);
    txt_Size->callback(fb_callback, this);

    lst_Size = new Fl_Browser(290, 40, 50, 100);
    lst_Size->type(FL_HOLD_BROWSER);
    lst_Size->callback(fb_callback, this);

    btn_OK = new Fl_Return_Button(345, 40, 50, 25, _("&OK"));
    btn_OK->shortcut(0x8006f);
    btn_OK->callback(fb_callback, this);

    btn_Cancel = new Fl_Button(345, 70, 50, 25, _("Cancel"));
    btn_Cancel->labelsize(12);
    btn_Cancel->callback(fb_callback, this);

    btn_Color = new Fl_Button(345, 100, 50, 25, _("Color"));
    btn_Color->down_box(FL_BORDER_BOX);
    btn_Color->color(FL_FOREGROUND_COLOR);
    btn_Color->callback(fb_callback, this);

    box_Example = new Preview_Box(5, 145, 390, 75, _("That crazy fox jumped over the dog again!\n"
				  "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789\n"
				  "!\"#$%&'()*+,-./:;<=>?@@[\\]^_`{|}~"));
    box_Example->box(FL_DOWN_BOX);
    box_Example->align(FL_ALIGN_WRAP|FL_ALIGN_CLIP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
    resizable(box_Example);

    set_modal();
    end();

// Initializations 

    this->callback_ = 0;  // Initialize Widgets callback 
    this->data_ = 0;      // And the data

    numfonts =   Fl::set_fonts(0); // Nr of fonts available on the server

    const char* name;
    for(int i = 0; i < numfonts; i++) {
	name = Fl::get_font_name((Fl_Font)i);
	if (isalpha(*name))
	    lst_Font->add(name, (void *)i);
    }
    FontSort();

    fontnbr = FL_HELVETICA;;
    fontsize = FL_NORMAL_SIZE; // Font Size to be used
    fontcolor = FL_FOREGROUND_COLOR;

    lst_Font->value(1);
    FontNameSelect();

    Fl::focus(lst_Font);

    xclass(PACKAGE_NAME);
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

bool Font_Browser::fixed_width(Fl_Font f)
{
	fl_font(f, FL_NORMAL_SIZE);
	return fl_width('X') == fl_width('i');
}


//////////////////////////////////////////////////////////////////////

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
