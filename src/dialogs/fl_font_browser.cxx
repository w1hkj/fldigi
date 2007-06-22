//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  fl_font_browser.cxx      v 0.0.2                              2005-10-17 
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


#include "fl_font_browser.h"
using namespace std;

void Fl_Font_Browser::callback(void (*cb)(Fl_Widget *, void *), void *d ) 
{
callback_ = cb;
data_     = d;
}


// Font preview box constructor
Fl_Font_Preview_Box::Fl_Font_Preview_Box(int x, int y, int w, int h, char*l):Fl_Widget(x, y, w, h, l)
{
fontName = 1;  // First font in the server
fontSize = 14; // Initianl font size
fontStyle=0;
box(FL_DOWN_BOX);  
color(FL_WHITE);   // Background color
}

// Draw method for the font preview box 
void Fl_Font_Preview_Box::draw()
{
  draw_box(); 
  fl_font((Fl_Font)fontName+fontStyle, fontSize); // Select font to bo used by the widget
  fl_color(FL_BLACK);
  fl_draw(label(), x()+3, y()+3, w()-6, h()-6, align()); 
}

// Set font name and size to be used by the box
void Fl_Font_Preview_Box::SetFont( int fontname,int style,int fontsize)
{
 fontName=fontname;
 fontSize=fontsize;
 fontStyle=style;
 redraw();
}
//Get font name ( in intiger)
int Fl_Font_Preview_Box::GetFontName()
{
    return fontName;
}

//Get font style ( in intiger)
int Fl_Font_Preview_Box::GetFontStyle()
{
    return fontStyle;
}
// Return font size used
int Fl_Font_Preview_Box::GetFontSize()
{
    return fontSize;
}

// Normal Bold, Italic, or Bold italic selected
void cb_SyleSelected (Fl_Widget*, void*frmWid) 
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
switch (ft->lst_Style->value())
  {
     case 1:          //Normal
          {
            ft->box_Example->SetFont(ft->box_Example->GetFontName(),0, ft->pickedsize);
            break;
          }
     case 2: //Bold
          {
            ft->box_Example->SetFont(ft->box_Example->GetFontName(),1, ft->pickedsize);
            break;
          }

     case 3: //Italic
         {
            ft->box_Example->SetFont(ft->box_Example->GetFontName(),2, ft->pickedsize);
            break;
          }
     case 4: //Bold Italic
         {
            ft->box_Example->SetFont(ft->box_Example->GetFontName(),3, ft->pickedsize);
            break;
          }

  default: 
          break;
  }   
  ft->txt_InputStyle->value(ft->lst_Style->text(ft->lst_Style->value()));
}



void cb_okBtn_Red(Fl_Button* o, void* v) 
{
((Fl_Font_Browser*)(o->parent()))->cb_okBtn(o,v);

}

void Fl_Font_Browser::cb_okBtn(Fl_Button* b, void*v)
{
    // Do any callback that is registered...
//Fl_Font_Browser *ft=(Fl_Font_Browser*) v;
if (callback_!=0)
   (*callback_)(this, data_);
}

// Cancel button callback
void cb_Cancel (Fl_Widget*, void*frmWid) 
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
ft->hide();
}

// Font size changed callback
void cb_FontSize_Selected (Fl_Widget*, void*frmWid) 
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
 ft->pickedsize=atoi(ft->lst_Size->text(ft->lst_Size->value()));
 ft->box_Example->SetFont(ft->box_Example->GetFontName(),ft->box_Example->GetFontStyle(), ft->pickedsize);
 ft->box_Example->redraw();
}

// Sorting function for Fl_browser (A -> Z)
void ForwardSort(Fl_Browser *brows) 
{
     for ( int t=1; t<=brows->size(); t++ ) 
     {
         for ( int r=t+1; r<=brows->size(); r++ ) 
         {
             if ( strcasecmp(brows->text(t), brows->text(r)) > 0 ) 
             {
                brows->swap(t,r);
             }
         }
     }
}

// Font Name changed callback
void cb_FontName_Selected(Fl_Widget*, void*frmWid) 
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
int fn = ft->lst_Font->value();
  if (!fn) return;
  fn--;
  ft->lst_Size->clear();
  ft->txt_InputFont->value(ft->lst_Font->text(ft->lst_Font->value()));  // show the font name in the input
 int *s;
 int t;
 int i;
  for (i=0;;i++)
   {
    const char *name = Fl::get_font_name((Fl_Font)i,&t); 
    if (strcmp(name,ft->lst_Font->text(ft->lst_Font->value()))==0)
       break;
    }
 int n = Fl::get_font_sizes((Fl_Font)i, s);
 // Retrive Sizes
 if (n) 
 {
               
     if (s[0] == 0)
    {
       // many sizes;
       int j = 1;
       for (int i = 1; i<74 ; i++) 
        {
          char buf[20];
          if (j < n && i==s[j])
           {
               sprintf(buf,"%d",i); 
               j++;
           }
          else 
             sprintf(buf,"%d",i);
          ft->lst_Size->add(buf);
        }
       ft->lst_Size->value( ft->pickedsize);
    } 
    else 
   {
      // some sizes
      int w = 0;
      for (int i = 0; i < n; i++) 
       {
        if (s[i]<=ft->pickedsize) 
        w = i;
        char buf[20];
        sprintf(buf,"%d",s[i]);
        ft->lst_Size->add(buf);
        }
      ft->lst_Size->value(w+1);

   }
    int gg=ft->GetFontNr(ft->lst_Font->text(ft->lst_Font->value()));
   // Retrive Styles 
     ft->lst_Style->clear();
     ft->lst_Style->add("Normal");
     ft->lst_Style->value(1);
     for (i=gg+1; i<gg+2;i++)
     {
		 if (i >= ft->numFonts())
			 break;
       const char *name = Fl::get_font_name((Fl_Font)i,&t);
        char buffer[128];
              sprintf(buffer,"%s %s",ft->lst_Font->text(ft->lst_Font->value()), "bold");
              if(strcmp(name,buffer)==0)
             {
               ft->lst_Style->add("Bold");
              }   
            else
                sprintf(buffer,"%s %s",ft->lst_Font->text(ft->lst_Font->value()), "italic");
                if (strcmp(name,buffer)==0)
                  ft->lst_Style->add("Italic");
       
                  else 

                    sprintf(buffer,"%s %s",ft->lst_Font->text(ft->lst_Font->value()), "bold italic");
                    if (strcmp(name,buffer)==0)
                     {
                       ft->lst_Style->add("Bold Italic");
                     }
     }
      

   ft->box_Example->SetFont(ft->GetFontNr(ft->lst_Font->text(ft->lst_Font->value())),0, ft->pickedsize);
   ft->txt_InputSize->value(ft->lst_Size->text(ft->lst_Size->value()));
 }
}
// Change to Upper case 
char* ToUpperCase(const char* buf)
{
 char *buf1=new char[strlen(buf)+1];
for (unsigned int i=0; i<strlen(buf)+1;i++)
 {
  buf1[i]=toupper(buf[i]);
 }
 return buf1;
}

// Change to Lower case 
char* ToLowerCase(const char* buf)
{
 char *buf1=new char[strlen(buf)+1];
for (unsigned int i=0; i<strlen(buf)+1;i++)
 {
  buf1[i]=tolower(buf[i]);
 }
 return buf1;
}



// This function will find the font name written in the font text box..
void cb_txtInputFontName(Fl_Widget*, void* frmWid)
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
  ft->lst_Font->value(0); // deselect item selected before.
  for (int i=1; i<= ft->lst_Font->size();i++)// Search for the font Full name 
   {
     if (strcmp(ToUpperCase(ft->lst_Font->text(i)),ToUpperCase(ft->txt_InputFont->value()))==0)
     {
       ft->lst_Font->value(i);
       ft->lst_Font->show(i);
       ft->txt_InputFont->value(ft->lst_Font->text(ft->lst_Font->value()));
       ft->lst_Font->do_callback();
       break;
     }
   }
   ft->txt_InputFont->take_focus();//Return the focus to the widget
}
// This function will find the font style written in the font text box..
void cb_txtInputFontStyle(Fl_Widget*, void* frmWid)
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
  ft->lst_Style->value(0); // deselect item selected before.
  for (int i=1; i<= ft->lst_Style->size();i++)// Search for the font Full name 
   {
     if (strcmp(ToUpperCase(ft->lst_Style->text(i)),ToUpperCase(ft->txt_InputStyle->value()))==0)
     {
       ft->lst_Style->value(i);
       ft->lst_Style->show(i);
       ft->txt_InputStyle->value(ft->lst_Style->text(ft->lst_Style->value()));
       ft->lst_Style->do_callback();
       break;
     }
   }
    ft->txt_InputStyle->take_focus();//Return the focus to the widget
}

// This function will find the font size written in the font text box..
void cb_txtInputFontSize(Fl_Widget*, void* frmWid)
{
Fl_Font_Browser *ft=(Fl_Font_Browser*)frmWid;
  ft->lst_Size->value(0); // deselect item selected before.
  int i;
  for (i=1; i<= ft->lst_Size->size();i++)// Search for the font Full name 
   {
      int g1=atoi(ft->lst_Size->text(i)); // Size list
      int g2=atoi(ft->txt_InputSize->value()); // Size enterd
      if (g1==g2)
     {
       ft->lst_Size->value(i);
       ft->lst_Size->show(i);
       ft->txt_InputSize->value(ft->lst_Size->text(ft->lst_Size->value()));
       ft->lst_Size->do_callback();
       break;
     }
   }
   if (i>ft->lst_Size->size()) // This means that the size enterd is not availabel
      {

         ft->box_Example->SetFont(ft->box_Example->GetFontName(),ft->box_Example->GetFontStyle(), atoi(ft->txt_InputSize->value()));
         ft->lst_Size->deselect(0); // Deselect item previously selected
      }
   ft->txt_InputSize->take_focus(); //Return the focus to the widget
}

// if everything correct should return => zero .. <zero when error occure.
int Fl_Font_Browser::GetFontNr(const char *fontNametoNr)
{
  int t;
  char fontNameString[255];/// this should be fixed mariwan 2005-10-07
  if ( lst_Style->value()>1) // check if the font is not bold ,italic , bold italic
     sprintf(fontNameString,"%s %s",lst_Font->text(lst_Font->value()),ToLowerCase(lst_Style->text(lst_Style->value())));
       else
     sprintf(fontNameString,"%s",lst_Font->text(lst_Font->value()));
int i ;     
for (i=0;;i++)
 {
    const char *name = Fl::get_font_name((Fl_Font)i,&t); 
    if (strcmp(name,fontNameString)==0)
    {
       return i;   
    }
 
  }    
 return -1;
}

Fl_Font_Browser::Fl_Font_Browser():Fl_Window(100,100,440,330,"Font Browser") 
{
      lst_Font = new Fl_Browser(10, 55, 195, 160);
      lst_Font->labelsize(12);
      lst_Font->textsize(12);
      lst_Font->callback((Fl_Callback*)cb_FontName_Selected, (void*)(lst_Font->parent()));
      lst_Font->type(FL_HOLD_BROWSER);
    
      txt_InputFont = new Fl_Input(10, 30, 195, 24, "Font:");
      txt_InputFont->labelsize(12);
      txt_InputFont->textsize(12);
      txt_InputFont->align(FL_ALIGN_TOP_LEFT);
      txt_InputFont->when(FL_WHEN_ENTER_KEY);
      txt_InputFont->callback((Fl_Callback*)cb_txtInputFontName, (void*)(txt_InputFont->parent()));
    
      lst_Style = new Fl_Browser(215, 55, 70, 160);
      lst_Style->labelsize(12);
      lst_Style->type(FL_HOLD_BROWSER);
      lst_Style->textsize(12); 
      lst_Style->callback((Fl_Callback*)cb_SyleSelected, (void*)(lst_Style->parent()));   
       
      txt_InputStyle = new Fl_Input(215, 30, 70, 24, "Style:");
      txt_InputStyle->labelsize(12);
      txt_InputStyle->align(FL_ALIGN_TOP_LEFT);
      txt_InputStyle->textsize(12);    
      txt_InputStyle->callback((Fl_Callback*)cb_txtInputFontStyle, (void*)(txt_InputStyle->parent()));
      
      lst_Size = new Fl_Browser(295, 55, 75, 159);
      lst_Size->labelsize(12);
      lst_Size->type(FL_HOLD_BROWSER);
      lst_Size->textsize(12); 
      lst_Size->callback((Fl_Callback*)cb_FontSize_Selected, (void*)(lst_Size->parent()));   
      
      txt_InputSize = new Fl_Input(295, 30, 75, 24, "Size:");
      txt_InputSize->labelsize(12);
      txt_InputSize->align(FL_ALIGN_TOP_LEFT);
      txt_InputSize->textsize(12);    
      txt_InputSize->callback((Fl_Callback*)cb_txtInputFontSize, (void*)(txt_InputSize->parent()));
      
      btn_OK =new Fl_Button(375, 30, 60, 24, "&OK");
      btn_OK->shortcut(0x8006f);
      btn_OK->labelfont(1);
      btn_OK->labelsize(14);
      btn_OK->callback((Fl_Callback*)cb_okBtn_Red );
         
      btn_Cancel =new Fl_Button(375, 60, 60, 24, "Cancel");
      btn_Cancel->labelsize(14);
      btn_Cancel->callback((Fl_Callback*)cb_Cancel, (void *)(btn_Cancel->parent()));

	  box_Example = new Fl_Font_Preview_Box(5, 220, 430, 100, 
			"AaBbCcDdEeFfGgHhIk\n0123456789");
		box_Example->box(FL_DOWN_BOX);
		box_Example->labelsize(12);
		box_Example->align(FL_ALIGN_WRAP|FL_ALIGN_CLIP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
		
	set_modal();
	end();
 // Initializations 
  pickedsize = 14; // Font Size to be used
          //
  int k =   Fl::set_fonts(0); // Nr of fonts available on the server
  numfonts = k;
  for(int i= 0; i < k; i++) 
  {
    int t;
    const char *name = Fl::get_font_name((Fl_Font)i,&t);
    char buffer[128];

// Load the font list .. Ignore the bold and italic types of the font
     if(!((t & FL_BOLD) ||(t & FL_ITALIC)))
     {
        sprintf(buffer, "%s",name);
        lst_Font->add(buffer);
     }
  }
  // Sort the font Alphabetically
  ForwardSort(lst_Font);
  
  lst_Font->value(1); // Select the first font in the list
  lst_Style->value(1);  
  lst_Font->do_callback();  // Do font selected callback .. to draw the preview
  lst_Style->do_callback();
  this->callback_ = 0;  // Initialize Widgets callback 
  this->data_ = 0;      // And the data
}
