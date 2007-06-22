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

#include <string>

#include "font_browser.h"

using namespace std;

// Font Color selected
void Font_Browser::ColorSelect()
{
    fontcolor = fl_show_colormap( fontcolor );
    btn_Color->color( fontcolor );
    box_Example->SetFont( fontnbr, fontsize, fontcolor );
}

void cb_Color_Select(Fl_Widget*, void*frmWid) 
{
    Font_Browser *ft=(Font_Browser*)frmWid;
    ft->ColorSelect();
}

// OK button pressed
void Font_Browser::okBtn()
{
    if (callback_!=0)
        (*callback_)(this, data_);
}

void cb_okBtn(Fl_Button* o, void* v) 
{
    ((Font_Browser*)(o->parent()))->okBtn();
}

// Cancel button callback
void cb_Cancel (Fl_Widget*, void*frmWid) 
{
    Font_Browser *ft=(Font_Browser*)frmWid;
    ft->hide();
}

// Font size changed callback
void Font_Browser::FontSizeSelect()
{
    txt_OutputSize->value(lst_Size->text( lst_Size->value() ) );  
    fontsize = atoi( lst_Size->text( lst_Size->value() ) );
    box_Example->SetFont( fontnbr, fontsize, fontcolor );
    box_Example->redraw();
}

void cb_FontSize_Selected (Fl_Widget*, void*frmWid) 
{
    Font_Browser *ft=(Font_Browser*)frmWid;
	ft->FontSizeSelect();
}

// Sort the font list
void Font_Browser::FontSort() 
{
     for ( int t = 1; t <= lst_Font->size() - 1; t++ )
         for ( int r = t+1; r <= lst_Font->size(); r++ )
             if ( strcasecmp(lst_Font->text(t), lst_Font->text(r)) > 0 ) 
                lst_Font->swap(t,r);
}
         
// Font Name changed callback
void Font_Browser::FontNameSelect()
{
    int fn = lst_Font->value();
    
    if (!fn) 
        return;

    fontnbr = (Fl_Font)reinterpret_cast<long>(lst_Font->data(fn));
    
// show the font name in the input
    txt_OutputFont->value(Fl::get_font_name(fontnbr));  

    box_Example->SetFont( fontnbr, fontsize, fontcolor );

}

void cb_FontName_Selected(Fl_Widget*, void*frmWid) 
{
    Font_Browser *ft=(Font_Browser*)frmWid;
    ft->FontNameSelect();
}

Font_Browser::Font_Browser(const char *lbl ) : 
	Fl_Window(100,100, 400, 225, lbl) 
{
    txt_OutputFont = new Fl_Output(5, 15, 280, 22, "Font:");
    txt_OutputFont->labelsize(12);
    txt_OutputFont->textsize(12);
    txt_OutputFont->align(FL_ALIGN_TOP_LEFT);
    txt_OutputFont->when(FL_WHEN_ENTER_KEY);
    
    txt_OutputSize = new Fl_Output(290, 15, 50, 22, "Size:");
    txt_OutputSize->labelsize(12);
    txt_OutputSize->align(FL_ALIGN_TOP_LEFT);
    txt_OutputSize->textsize(12);    
      
    lst_Font = new Fl_Browser(5, 40, 280, 100);
    lst_Font->labelsize(12);
    lst_Font->textsize(12);
    lst_Font->type(FL_HOLD_BROWSER);
    lst_Font->callback( (Fl_Callback*)cb_FontName_Selected, this );
    
    lst_Size = new Fl_Browser(290, 40, 50, 100);
    lst_Size->labelsize(12);
    lst_Size->type(FL_HOLD_BROWSER);
    lst_Size->textsize(12); 
    lst_Size->callback( (Fl_Callback*)cb_FontSize_Selected, this );
      
    btn_OK =new Fl_Button(345, 40, 50, 25, "&OK");
    btn_OK->shortcut(0x8006f);
    btn_OK->labelfont(1);
    btn_OK->labelsize(12);
    btn_OK->callback((Fl_Callback*)cb_okBtn );
         
    btn_Cancel =new Fl_Button(345, 70, 50, 25, "Cancel");
    btn_Cancel->labelsize(12);
    btn_Cancel->callback((Fl_Callback*)cb_Cancel, this );
    
    btn_Color = new Fl_Button(345, 100, 50, 25, "");
    btn_Color->down_box(FL_BORDER_BOX);
    btn_Color->labelsize(12);
    btn_Color->align(FL_ALIGN_TOP_LEFT);
    btn_Color->color(FL_BLACK);
    btn_Color->callback( (Fl_Callback*)cb_Color_Select, this );
     
    box_Example = new Preview_Box(5, 145, 390, 75, "abcdefghijk ABCDEFGHIJK\n0 1 2 3 4 5 6 7 8 9");
    box_Example->box(FL_DOWN_BOX);
    box_Example->labelsize(12);
    box_Example->align(FL_ALIGN_WRAP|FL_ALIGN_CLIP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

    set_modal();
    end();

// Initializations 

    this->callback_ = 0;  // Initialize Widgets callback 
    this->data_ = 0;      // And the data

    numfonts =   Fl::set_fonts(0); // Nr of fonts available on the server

	fontnbr = (Fl_Font)1;
	
	string name;
    for(int i= 0; i < numfonts; i++) {
        int t;
        name = Fl::get_font_name((Fl_Font)i, &t);
        if (strcmp(name.c_str(),"system") == 0) fontnbr = (Fl_Font)i;
       	if (name[0] >= 'A' && name[0] <= 'z') {
        	lst_Font->add(name.c_str(), (void *)i);
       	}
    }
    FontSort();
  
	char buf[5];
	for (int i = 1; i < 48; i+=1) {
		sprintf(buf, "%d", i);
        lst_Size->add(buf);
    }

	fontnbr = (Fl_Font)1;
    fontsize = 14; // Font Size to be used
    fontcolor = FL_BLACK;

    lst_Size->value( fontsize );
    lst_Font->value((int)fontnbr);
    
    FontNameSelect();

    Fl::focus(lst_Font);

}
