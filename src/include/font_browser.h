//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Font_Browser.h      v 0.0.1                              2005-10-17 
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


#ifndef FONTBROWSER_H
#define FONTBROWSER_H

#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_show_colormap.H>
#include <FL/Fl_Color_Chooser.H>
#include <iostream>

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
private:
    int		numfonts;
    Fl_Font	fontnbr;
    int		fontsize;
    Fl_Color fontcolor;
    void	*data_;

    Fl_Browser	*lst_Font;
    Fl_Browser	*lst_Size;
    Fl_Value_Input *txt_Size;
    Fl_Return_Button *btn_OK;
    Fl_Button	*btn_Cancel;
    Fl_Button	*btn_Color;
    Preview_Box	*box_Example;

    void	FontSort();
    Fl_Callback* callback_;

public:
    Font_Browser(int x = 100, int y = 100, int w = 400, int h = 225, const char *lbl = "Font Browser");
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

    static bool fixed_width(Fl_Font f);
};

#endif
