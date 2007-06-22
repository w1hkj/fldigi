//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  fl_font_browser.h      v 0.0.1                              2005-10-17 
//
//         for the Fast Light Tool Kit (FLTK) 1.1.x .
//
//    by Mariwan Jalal
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


#ifndef fontbrowser2_h
#define fontbrowser2_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_show_colormap.H>
#include <iostream>
// Preview box for showing font
class Fl_Font_Preview_Box : public Fl_Widget
{
private:
  void draw();
  int fontName;
  int fontSize;
  int fontStyle;
public: 
        void SetFont( int fontname,int fontsyle,int fontsize);
        Fl_Font_Preview_Box(int x, int y , int w, int h, char *l );
        int GetFontName();
        int GetFontSize();
        int GetFontStyle();
        int GetFontStrickTrhough();
        int GetFontUnderline();
};
// Font browser widget
class Fl_Font_Browser : public Fl_Window {
private:
  void (*callback_)(Fl_Widget*, void *);
  void *data_;
  public:
  void cb_okBtn(Fl_Button*, void*);
  int numfonts;
   
public:
void callback(void (*cb)(Fl_Widget *, void *), void *d = 0);
      
public:
  Fl_Font_Browser();
  int GetFontNr(const char *fontNametoNr);
  int numFonts() { return numfonts;}

public:
  int pickedsize ;
  Fl_Browser *lst_Font;
  Fl_Input *txt_InputFont;
  Fl_Browser *lst_Style;
  Fl_Input *txt_InputStyle;
  Fl_Browser *lst_Size;
  Fl_Input *txt_InputSize;
  Fl_Button *btn_OK;
  Fl_Button *btn_Cancel;
  Fl_Font_Preview_Box *box_Example;
};
#endif
