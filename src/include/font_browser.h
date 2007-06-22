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

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_show_colormap.h>
#include <iostream>

// Preview box for showing font
class Preview_Box : public Fl_Widget
{
private:
	int		fontName;
	int		fontSize;
	Fl_Color fontColor;

	void	draw() {
    	draw_box(); 
    	fl_font((Fl_Font)fontName, fontSize);
    	fl_color(fontColor);
    	fl_draw(label(), x()+3, y()+3, w()-6, h()-6, align()); 
	}
public: 
	Preview_Box(int x, int y, int w, int h, char* l) : Fl_Widget(x, y, w, h, l)
	{
    	fontName = 1;
    	fontSize = 14;
    	box(FL_DOWN_BOX);  
    	color(FL_WHITE);
    	fontColor = FL_BLACK;
	}
    void SetFont( int fontname, int fontsize, Fl_Color c)
	{
    	fontName = fontname;
    	fontSize = fontsize;
    	fontColor = c;
    	redraw();
	}
};

// Font browser widget
class Font_Browser : public Fl_Window {
private:
    int		numfonts;
    Fl_Font	fontnbr;
    int		fontsize;
    Fl_Color fontcolor;
    void	*data_;

    Fl_Browser	*lst_Font;
    Fl_Output	*txt_OutputFont;
    Fl_Browser	*lst_Size;
    Fl_Output	*txt_OutputSize;
    Fl_Button	*btn_OK;
    Fl_Button	*btn_Cancel;
    Fl_Button	*btn_Color;
    Preview_Box	*box_Example;

	void	FontSort();
    void	(*callback_)(Fl_Widget*, void *);
	
public:
    Font_Browser(char *lbl = "Font Browser");
    void callback(void (*cb)(Fl_Widget *, void *), void *d = 0) {
    	callback_ = cb;
    	data_     = d;
	}
	void	FontNameSelect();
	void	FontSizeSelect();
	void	ColorSelect();
    void	okBtn();

    int numFonts() { return numfonts; }

    void fontNumber(Fl_Font n) { 
    	fontnbr = n;
    	lst_Font->value(1);
		for ( int i = 1; i <= lst_Font->size() - 1; i++ ) {
			if ((Fl_Font)reinterpret_cast<int>(lst_Font->data(i)) == n) {
				lst_Font->value(i);
				break;
			}
		}
    	FontNameSelect();
    };
    Fl_Font fontNumber() { return fontnbr; }

    void fontSize(int s) { 
    	fontsize = s;
    	lst_Size->value(fontsize);
    	FontSizeSelect(); 
    }
    int fontSize() { return fontsize; }

	void fontColor(Fl_Color c) { 
		fontcolor = c; 
	    btn_Color->color( fontcolor );
    	box_Example->SetFont( fontnbr, fontsize, fontcolor );
	}
    Fl_Color fontColor() { return fontcolor; };

    const char *fontName() { return txt_OutputFont->value(); }

};

#endif
